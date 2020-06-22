// This file is part of node-libcanberra
//
// Copyright (c) 2019 The Board of Trustees of The Leland Stanford Junior University
//
// Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//   * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   * Neither the name of Stanford University nor the
//     names of its contributors may be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Giovanni Campagna <gcampagn@cs.stanford.edu>

#include <stdio.h>

#include "canberra-context.h"

#define JS_ASSERT(condition, cleanup, ret, message) \
    do { \
        if (!(condition)) {\
            isolate->ThrowException(Nan::TypeError(Nan::NewOneByteString((const uint8_t*)(message)).ToLocalChecked())); \
            cleanup; \
            return ret; \
        } \
    } while(0)
#define CANBERRA_CHECK(err, cleanup, ret) \
    do { \
        if (err) { \
            isolate->ThrowException(Nan::Error(Nan::NewOneByteString((const uint8_t*)ca_strerror(err)).ToLocalChecked())); \
            cleanup; \
            return ret; \
        } \
    } while(0)

namespace node_libcanberra {

Nan::Persistent<v8::Function> Context::constructor;

Context::Context(ca_proplist *props, v8::Local<v8::Function> callback) : m_callback(callback)
{
    ca_context_create(&m_ctx);
    ca_context_change_props_full(m_ctx, props);

    uv_async_init(uv_default_loop(), this, AsyncCallback);
}

bool
Context::Open(v8::Isolate *isolate)
{
    int err;

    err = ca_context_open(m_ctx);
    CANBERRA_CHECK(err, , false);

    return true;
}

Context::~Context()
{
    if (m_ctx) {
        ca_context_destroy(m_ctx);
        m_ctx = nullptr;
    }
}

void Context::Init(v8::Local<v8::Object> exports)
{
    Nan::HandleScope scope;

    // Prepare constructor template
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    auto classname = Nan::New("Context").ToLocalChecked();
    tpl->SetClassName(classname);
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Prototype
    Nan::SetPrototypeMethod(tpl, "play", Play);
    Nan::SetPrototypeMethod(tpl, "cancel", Cancel);
    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "cache", Cache);
    Nan::SetPrototypeMethod(tpl, "playing", Playing);

    auto fn = Nan::GetFunction(tpl).ToLocalChecked();
    constructor.Reset(fn);
    Nan::Set(exports, classname, fn);
}

static ca_proplist*
maybe_build_proplist(v8::Isolate *isolate, v8::Local<v8::Object> fromjs)
{
    ca_proplist *props;
    int err;

    err = ca_proplist_create(&props);
    CANBERRA_CHECK(err, , nullptr);

    if (fromjs.IsEmpty())
        return props;

    auto maybe_prop_names = Nan::GetOwnPropertyNames(fromjs);
    if (maybe_prop_names.IsEmpty()) {
        ca_proplist_destroy(props);
        return nullptr;
    }

    auto prop_names = maybe_prop_names.ToLocalChecked();
    for (uint32_t i = 0; i < prop_names->Length(); i++) {
        v8::Local<v8::Value> name, value;
        if (!Nan::Get(prop_names, i).ToLocal(&name)) {
            ca_proplist_destroy(props);
            return nullptr;
        }
        JS_ASSERT(name->IsString(), ca_proplist_destroy(props), nullptr, "Property name must be a string");

        if (!Nan::Get(fromjs, name).ToLocal(&value)) {
            ca_proplist_destroy(props);
            return nullptr;
        }

        JS_ASSERT(!value.IsEmpty() && value->IsString(), ca_proplist_destroy(props), nullptr,
            "Property value must be a string");

        Nan::Utf8String c_name(name);
        Nan::Utf8String c_value(value);

        ca_proplist_sets(props, *c_name, *c_value);
    }

    return props;
}

void Context::New(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    auto isolate = info.GetIsolate();

    JS_ASSERT(info.IsConstructCall(), , , "Context() must called as constructor");
    JS_ASSERT(info.Length() == 2, , , "Expected 2 arguments");
    JS_ASSERT(info[0]->IsObject(), , , "new Context() expects an object as first argument");
    JS_ASSERT(info[1]->IsFunction(), , , "new Context() expects a function as second argument");

    auto proplist = maybe_build_proplist(isolate, Nan::To<v8::Object>(info[0]).ToLocalChecked());
    if (!proplist)
        return;

    Context* ctx = new Context(proplist, info[1].As<v8::Function>());
    ca_proplist_destroy(proplist);

    if (!ctx->Open(isolate))
        return;

    ctx->Wrap(info.This());
    ctx->Ref();
    info.GetReturnValue().Set(info.This());
}

void Context::async_close_callback(uv_handle_t* handle)
{
    static_cast<Context*>((uv_async_t*)handle)->Unref();
}

void Context::Destroy(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    Context *self = Nan::ObjectWrap::Unwrap<Context>(info.This());
    if (!self)
        return;

    if (!self->m_ctx)
        return;
    ca_context_destroy(self->m_ctx);
    self->m_ctx = nullptr;

    uv_close((uv_handle_t*)static_cast<uv_async_t*>(self), async_close_callback);

    info.GetReturnValue().Set(Nan::Undefined());
}

void
Context::play_finish_callback(ca_context *c, uint32_t id, int error_code, void *userdata)
{
    Context *self = static_cast<Context*>(userdata);

    std::lock_guard<std::mutex> locker(self->m_result_list_lock);
    self->m_result_list.emplace_back(id, error_code);
    uv_async_send(self);
}

void Context::Play(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    auto isolate = info.GetIsolate();
    Context *self = Nan::ObjectWrap::Unwrap<Context>(info.This());
    if (!self)
        return;

    JS_ASSERT(info.Length() >= 2, , , "Expected at least 2 arguments to Context.play()");
    JS_ASSERT(info[0]->IsNumber(), , , "The first argument to Context.play() must be a number");
    JS_ASSERT(info[1]->IsObject(), , , "The second argument to Context.play() must be an object with properties");

    auto proplist = maybe_build_proplist(isolate, Nan::To<v8::Object>(info[0]).ToLocalChecked());
    if (!proplist)
        return;

    uint32_t id = info[0].As<v8::Number>()->Value();
    if (!self->m_ctx) {
        std::lock_guard<std::mutex> locker(self->m_result_list_lock);
        self->m_result_list.emplace_back(id, CA_ERROR_DESTROYED);
        uv_async_send(self);

        ca_proplist_destroy(proplist);
        return;
    }

    int err = ca_context_play_full(self->m_ctx, id, proplist, play_finish_callback, self);
    if (err < 0) {
        std::lock_guard<std::mutex> locker(self->m_result_list_lock);
        self->m_result_list.emplace_back(id, err);
        uv_async_send(self);

        ca_proplist_destroy(proplist);
    }

    info.GetReturnValue().Set(Nan::Undefined());
}

void Context::Cancel(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    auto isolate = info.GetIsolate();
    Context *self = Nan::ObjectWrap::Unwrap<Context>(info.This());
    if (!self)
        return;

    JS_ASSERT(info.Length() >= 1, , , "Expected an argument to Context.cancel()");
    JS_ASSERT(info[0]->IsNumber(), , , "The first argument to Context.cancel() must be a number");

    uint32_t id = info[0].As<v8::Number>()->Value();

    if (!self->m_ctx)
        return;

    ca_context_cancel(self->m_ctx, id);

    info.GetReturnValue().Set(Nan::Undefined());
}

void Context::Playing(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    auto isolate = info.GetIsolate();
    Context *self = Nan::ObjectWrap::Unwrap<Context>(info.This());
    if (!self)
        return;

    JS_ASSERT(info.Length() >= 1, , , "Expected an argument to Context.playing()");
    JS_ASSERT(info[0]->IsNumber(), , , "The first argument to Context.playing() must be a number");

    uint32_t id = info[0].As<v8::Number>()->Value();

    if (!self->m_ctx)
        return;

    int playing;
    int err = ca_context_playing(self->m_ctx, id, &playing);
    CANBERRA_CHECK(err, , );

    info.GetReturnValue().Set(playing ? Nan::True() : Nan::False());
}

void Context::Cache(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    auto isolate = info.GetIsolate();
    Context *self = Nan::ObjectWrap::Unwrap<Context>(info.This());
    if (!self)
        return;

    JS_ASSERT(info.Length() >= 1, , , "Expected an argument to Context.cache()");
    JS_ASSERT(info[0]->IsObject(), , , "The first argument to Context.cache() must be an object");

    auto proplist = maybe_build_proplist(isolate, Nan::To<v8::Object>(info[0]).ToLocalChecked());
    if (!proplist)
        return;

    if (!self->m_ctx) {
        ca_proplist_destroy(proplist);
        return;
    }

    int err = ca_context_cache_full(self->m_ctx, proplist);
    CANBERRA_CHECK(err, , );

    info.GetReturnValue().Set(Nan::Undefined());
}

void Context::AsyncCallback(uv_async_t* async)
{
    Context *self = static_cast<Context*>(async);
    std::deque<std::pair<uint32_t, int>> result_list;

    {
        std::lock_guard<std::mutex> locker(self->m_result_list_lock);
        std::swap(result_list, self->m_result_list);
    }

    Nan::HandleScope scope;
    auto v8ctx = Nan::GetCurrentContext();
    auto isolate = v8ctx->GetIsolate();
    v8::Local<v8::Function> callback = self->m_callback.Get(isolate);

    for (const auto& result : result_list) {
        const unsigned argc = 2;

        v8::Local<v8::Value> error;
        if (result.second == CA_SUCCESS) {
            error = Nan::Null();
        } else {
            error = Nan::Error(Nan::NewOneByteString((const uint8_t*)(ca_strerror(result.second))).ToLocalChecked());
            Nan::Set(error.As<v8::Object>(), Nan::NewOneByteString((const uint8_t*)"code").ToLocalChecked(), v8::Number::New(isolate, result.second));
        }

        v8::Local<v8::Value> argv[argc] = {
            v8::Number::New(isolate, result.first),
            error
        };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), callback, argc, argv);
    }
}

}
