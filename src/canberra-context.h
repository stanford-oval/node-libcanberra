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

#pragma once

#include <nan.h>
#include <canberra.h>
#include <algorithm>
#include <deque>
#include <mutex>

namespace node_libcanberra {

class Context : public Nan::ObjectWrap, private uv_async_t {
 public:
  static void Init(v8::Local<v8::Object> exports);

 private:
  ca_context *m_ctx;
  Nan::Persistent<v8::Function> m_callback;

  std::mutex m_result_list_lock;
  std::deque<std::pair<uint32_t, int>> m_result_list;

  explicit Context(ca_proplist *props, v8::Local<v8::Function> callback);
  ~Context();

  bool Open(v8::Isolate *isolate);

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Destroy(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Play(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Cancel(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Cache(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void Playing(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static Nan::Persistent<v8::Function> constructor;

  static void AsyncCallback(uv_async_t* self);
  static void play_finish_callback(ca_context *c, uint32_t id, int error_code, void *userdata);
  static void async_close_callback(uv_handle_t* handle);
};

}
