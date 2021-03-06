// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/v8.h"

#include "src/api.h"
#include "src/assembler.h"
#include "src/base/atomicops.h"
#include "src/base/once.h"
#include "src/base/platform/platform.h"
#include "src/base/utils/random-number-generator.h"
#include "src/bootstrapper.h"
#include "src/debug/debug.h"
#include "src/deoptimizer.h"
#include "src/elements.h"
#include "src/frames.h"
#include "src/isolate.h"
#include "src/libsampler/sampler.h"
#include "src/objects-inl.h"
#include "src/profiler/heap-profiler.h"
#include "src/runtime-profiler.h"
#include "src/snapshot/natives.h"
#include "src/snapshot/snapshot.h"
#include "src/tracing/tracing-category-observer.h"

namespace v8 {
namespace internal {

V8_DECLARE_ONCE(init_once);

#ifdef V8_USE_EXTERNAL_STARTUP_DATA
V8_DECLARE_ONCE(init_natives_once);
V8_DECLARE_ONCE(init_snapshot_once);
#endif

v8::Platform* V8::platform_ = NULL;


bool V8::Initialize() {
  InitializeOncePerProcess();
  return true;
}


void V8::TearDown() {
  Bootstrapper::TearDownExtensions();
  ElementsAccessor::TearDown();
  RegisteredExtension::UnregisterAll();
  Isolate::GlobalTearDown();
  base::OS::TearDown();
  sampler::Sampler::TearDown();
  FlagList::ResetAllFlags();  // Frees memory held by string arguments.
  base::Time::TearDown();
  base::RandomNumberGenerator::TearDown();
  PerThreadAssertRuntime::TearDown();
}


void V8::InitializeOncePerProcessImpl() {
  FlagList::EnforceFlagImplications();

  if (FLAG_predictable && FLAG_random_seed == 0) {
    // Avoid random seeds in predictable mode.
    FLAG_random_seed = 12347;
  }

  if (FLAG_stress_compaction) {
    FLAG_force_marking_deque_overflows = true;
    FLAG_gc_global = true;
    FLAG_max_semi_space_size = 1;
  }

  base::OS::SetUp();
  base::OS::Initialize(FLAG_random_seed, FLAG_hard_abort, FLAG_gc_fake_mmap);

  Isolate::InitializeOncePerProcess();

  sampler::Sampler::SetUp();
  CpuFeatures::Probe(false);
  ElementsAccessor::InitializeOncePerProcess();
  SetUpJSCallerSavedCodeData();
  ExternalReference::SetUp();
  Bootstrapper::InitializeOncePerProcess();
}


void V8::InitializeOncePerProcess() {
  base::CallOnce(&init_once, &InitializeOncePerProcessImpl);
}


void V8::InitializePlatform(v8::Platform* platform) {
  CHECK(!platform_);
  CHECK(platform);
  platform_ = platform;
  v8::base::SetPrintStackTrace(platform_->GetStackTracePrinter());
  v8::tracing::TracingCategoryObserver::SetUp();
}


void V8::ShutdownPlatform() {
  CHECK(platform_);
  v8::tracing::TracingCategoryObserver::TearDown();
  v8::base::SetPrintStackTrace(nullptr);
  platform_ = NULL;
}


v8::Platform* V8::GetCurrentPlatform() {
  v8::Platform* platform = reinterpret_cast<v8::Platform*>(
      base::Relaxed_Load(reinterpret_cast<base::AtomicWord*>(&platform_)));
  DCHECK(platform);
  return platform;
}

v8::Platform* V8::TryGetCurrentPlatform() {
  return reinterpret_cast<v8::Platform*>(
      base::Relaxed_Load(reinterpret_cast<base::AtomicWord*>(&platform_)));
}

void V8::SetPlatformForTesting(v8::Platform* platform) {
  base::Relaxed_Store(reinterpret_cast<base::AtomicWord*>(&platform_),
                      reinterpret_cast<base::AtomicWord>(platform));
}

void V8::SetNativesBlob(StartupData* natives_blob) {
#ifdef V8_USE_EXTERNAL_STARTUP_DATA
  base::CallOnce(&init_natives_once, &SetNativesFromFile, natives_blob);
#else
  CHECK(false);
#endif
}


void V8::SetSnapshotBlob(StartupData* snapshot_blob) {
#ifdef V8_USE_EXTERNAL_STARTUP_DATA
  base::CallOnce(&init_snapshot_once, &SetSnapshotFromFile, snapshot_blob);
#else
  CHECK(false);
#endif
}
}  // namespace internal
}  // namespace v8
