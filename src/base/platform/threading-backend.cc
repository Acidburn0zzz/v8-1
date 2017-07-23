// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/base/platform/threading-backend.h"

#include "src/base/platform/condition-variable.h"
#include "src/base/platform/mutex.h"
#include "src/v8.h"

namespace v8 {
namespace base {

class DefaultThreadingBackend final : public ThreadingBackend {
 public:
  MutexImpl* CreatePlainMutex() override {
    return new NativeMutex();
  }

  MutexImpl* CreateRecursiveMutex() override {
    return new NativeRecursiveMutex();
  }

  ConditionVariableImpl* CreateConditionVariable() override {
    return new NativeConditionVariable();
  }
};

static DefaultThreadingBackend default_threading_backend_;


ThreadingBackend* GetThreadingBackend() {
  ThreadingBackend* backend = nullptr;

  auto platform = v8::internal::V8::TryGetCurrentPlatform();
  if (platform != nullptr) {
    backend = platform->GetThreadingBackend();
  }

  if (backend == nullptr) {
    backend = &default_threading_backend_;
  }

  return backend;
}

}  // namespace base
}  // namespace v8
