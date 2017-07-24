// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/base/platform/memory-backend.h"

#include "src/v8.h"

namespace v8 {
namespace base {

MemoryBackend* GetMemoryBackend() {
  return v8::internal::V8::GetCurrentPlatform()->GetMemoryBackend();
}

}  // namespace base
}  // namespace v8
