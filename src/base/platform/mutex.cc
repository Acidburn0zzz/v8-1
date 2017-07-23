// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/base/platform/mutex.h"

#include <errno.h>

#include "src/base/platform/threading-backend.h"

namespace v8 {
namespace base {

Mutex::Mutex() : impl_(GetThreadingBackend()->CreatePlainMutex()) {
#ifdef DEBUG
  level_ = 0;
#endif
}


Mutex::~Mutex() {
  DCHECK_EQ(0, level_);
}


void Mutex::Lock() {
  impl_->Lock();
  AssertUnheldAndMark();
}


void Mutex::Unlock() {
  AssertHeldAndUnmark();
  impl_->Unlock();
}


bool Mutex::TryLock() {
  if (!impl_->TryLock()) {
    return false;
  }
  AssertUnheldAndMark();
  return true;
}


RecursiveMutex::RecursiveMutex()
  : impl_(GetThreadingBackend()->CreateRecursiveMutex()) {
#ifdef DEBUG
  level_ = 0;
#endif
}


RecursiveMutex::~RecursiveMutex() {
  DCHECK_EQ(0, level_);
}


void RecursiveMutex::Lock() {
  impl_->Lock();
#ifdef DEBUG
  DCHECK_LE(0, level_);
  level_++;
#endif
}


void RecursiveMutex::Unlock() {
#ifdef DEBUG
  DCHECK_LT(0, level_);
  level_--;
#endif
  impl_->Unlock();
}


bool RecursiveMutex::TryLock() {
  if (!impl_->TryLock()) {
    return false;
  }
#ifdef DEBUG
  DCHECK_LE(0, level_);
  level_++;
#endif
  return true;
}


#if V8_OS_POSIX

static V8_INLINE void InitializeNativeHandle(pthread_mutex_t* mutex) {
  int result;
#if defined(DEBUG)
  // Use an error checking mutex in debug mode.
  pthread_mutexattr_t attr;
  result = pthread_mutexattr_init(&attr);
  DCHECK_EQ(0, result);
  result = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
  DCHECK_EQ(0, result);
  result = pthread_mutex_init(mutex, &attr);
  DCHECK_EQ(0, result);
  result = pthread_mutexattr_destroy(&attr);
#else
  // Use a fast mutex (default attributes).
  result = pthread_mutex_init(mutex, NULL);
#endif  // defined(DEBUG)
  DCHECK_EQ(0, result);
  USE(result);
}


static V8_INLINE void InitializeRecursiveNativeHandle(pthread_mutex_t* mutex) {
  pthread_mutexattr_t attr;
  int result = pthread_mutexattr_init(&attr);
  DCHECK_EQ(0, result);
  result = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  DCHECK_EQ(0, result);
  result = pthread_mutex_init(mutex, &attr);
  DCHECK_EQ(0, result);
  result = pthread_mutexattr_destroy(&attr);
  DCHECK_EQ(0, result);
  USE(result);
}


static V8_INLINE void DestroyNativeHandle(pthread_mutex_t* mutex) {
  int result = pthread_mutex_destroy(mutex);
  DCHECK_EQ(0, result);
  USE(result);
}


static V8_INLINE void LockNativeHandle(pthread_mutex_t* mutex) {
  int result = pthread_mutex_lock(mutex);
  DCHECK_EQ(0, result);
  USE(result);
}


static V8_INLINE void UnlockNativeHandle(pthread_mutex_t* mutex) {
  int result = pthread_mutex_unlock(mutex);
  DCHECK_EQ(0, result);
  USE(result);
}


static V8_INLINE bool TryLockNativeHandle(pthread_mutex_t* mutex) {
  int result = pthread_mutex_trylock(mutex);
  if (result == EBUSY) {
    return false;
  }
  DCHECK_EQ(0, result);
  return true;
}

#elif V8_OS_WIN

static V8_INLINE void InitializeNativeHandle(PCRITICAL_SECTION cs) {
  InitializeCriticalSection(cs);
}


static V8_INLINE void InitializeRecursiveNativeHandle(PCRITICAL_SECTION cs) {
  InitializeCriticalSection(cs);
}


static V8_INLINE void DestroyNativeHandle(PCRITICAL_SECTION cs) {
  DeleteCriticalSection(cs);
}


static V8_INLINE void LockNativeHandle(PCRITICAL_SECTION cs) {
  EnterCriticalSection(cs);
}


static V8_INLINE void UnlockNativeHandle(PCRITICAL_SECTION cs) {
  LeaveCriticalSection(cs);
}


static V8_INLINE bool TryLockNativeHandle(PCRITICAL_SECTION cs) {
  return TryEnterCriticalSection(cs) != FALSE;
}

#endif


NativeMutex::NativeMutex() {
  InitializeNativeHandle(&native_handle_);
}


NativeMutex::~NativeMutex() {
  DestroyNativeHandle(&native_handle_);
}


void NativeMutex::Lock() {
  LockNativeHandle(&native_handle_);
}


void NativeMutex::Unlock() {
  UnlockNativeHandle(&native_handle_);
}


bool NativeMutex::TryLock() {
  return TryLockNativeHandle(&native_handle_);
}


NativeRecursiveMutex::NativeRecursiveMutex() {
  InitializeRecursiveNativeHandle(&native_handle_);
}


NativeRecursiveMutex::~NativeRecursiveMutex() {
  DestroyNativeHandle(&native_handle_);
}


void NativeRecursiveMutex::Lock() {
  LockNativeHandle(&native_handle_);
}


void NativeRecursiveMutex::Unlock() {
  UnlockNativeHandle(&native_handle_);
}


bool NativeRecursiveMutex::TryLock() {
  return TryLockNativeHandle(&native_handle_);
}

}  // namespace base
}  // namespace v8
