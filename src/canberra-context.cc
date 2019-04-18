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

Context::Context(ca_proplist *props)
{
    ca_context_create(&m_ctx);
    ca_context_change_props_full(m_ctx, props);
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
    if (!m_ctx)
        return;
    ca_context_destroy(m_ctx);
    m_ctx = nullptr;
}

void Context::Init(v8::Local<v8::Object> exports)
{
    Nan::HandleScope scope;

    // Prepare constructor template
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("Context").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Prototype
    //Nan::SetPrototypeMethod(tpl, "play", Play);
    //Nan::SetPrototypeMethod(tpl, "cancel", Cancel);
    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);

    constructor.Reset(tpl->GetFunction());
    exports->Set(Nan::New("Context").ToLocalChecked(), tpl->GetFunction());
}

static char*
v8_to_string(v8::Local<v8::String> s)
{
    size_t length = s->Utf8Length();
    char *buffer = (char*)malloc(length);
    s->WriteUtf8(buffer, -1, nullptr, v8::String::REPLACE_INVALID_UTF8);

    return buffer;
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

    auto prop_names = fromjs->GetOwnPropertyNames();
    for (uint32_t i = 0; i < prop_names->Length(); i++) {
        auto name = prop_names->Get(i);
        JS_ASSERT(name->IsString(), ca_proplist_destroy(props), nullptr, "Property name must be a string");

        auto value = fromjs->Get(name);

        JS_ASSERT(!value.IsEmpty() && value->IsString(), ca_proplist_destroy(props), nullptr,
            "Property value must be a string");

        auto c_name = v8_to_string(name->ToString());
        auto c_value = v8_to_string(value->ToString());

        ca_proplist_sets(props, c_name, c_value);

        free(c_name);
        free(c_value);
    }

    return props;
}

void Context::New(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    auto isolate = info.GetIsolate();

    JS_ASSERT(info.IsConstructCall(), , , "Context() must called as constructor");
    JS_ASSERT(info.Length() == 0 || info[0]->IsObject(), , , "new Context() must be called with a single object argument");

    auto proplist = maybe_build_proplist(isolate, info.Length() > 0 ? info[0]->ToObject(isolate) : v8::Local<v8::Object>());
    if (!proplist)
        return;

    Context* ctx = new Context(proplist);
    ca_proplist_destroy(proplist);

    if (!ctx->Open(isolate))
        return;

    ctx->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
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
}

}
