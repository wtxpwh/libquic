// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/sequence_local_storage_map.h"

#include <utility>

#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"

namespace base {
namespace internal {

namespace {
LazyInstance<ThreadLocalPointer<SequenceLocalStorageMap>>::Leaky
    tls_current_sequence_local_storage = LAZY_INSTANCE_INITIALIZER;
}  // namespace

SequenceLocalStorageMap::SequenceLocalStorageMap() = default;

SequenceLocalStorageMap::~SequenceLocalStorageMap() = default;

ScopedSetSequenceLocalStorageMapForCurrentThread::
    ScopedSetSequenceLocalStorageMapForCurrentThread(
        SequenceLocalStorageMap* sequence_local_storage) {
  tls_current_sequence_local_storage.Get().Set(sequence_local_storage);
}

ScopedSetSequenceLocalStorageMapForCurrentThread::
    ~ScopedSetSequenceLocalStorageMapForCurrentThread() {
  tls_current_sequence_local_storage.Get().Set(nullptr);
}

SequenceLocalStorageMap& SequenceLocalStorageMap::GetForCurrentThread() {
  SequenceLocalStorageMap* current_sequence_local_storage =
      tls_current_sequence_local_storage.Get().Get();

  return *current_sequence_local_storage;
}

void* SequenceLocalStorageMap::Get(int slot_id) {
  const auto it = sls_map_.find(slot_id);
  if (it == sls_map_.end())
    return nullptr;
  return it->second.value();
}

void SequenceLocalStorageMap::Set(
    int slot_id,
    SequenceLocalStorageMap::ValueDestructorPair value_destructor_pair) {
  auto it = sls_map_.find(slot_id);

  if (it == sls_map_.end())
    sls_map_.emplace(slot_id, std::move(value_destructor_pair));
  else
    it->second = std::move(value_destructor_pair);
}

SequenceLocalStorageMap::ValueDestructorPair::ValueDestructorPair(
    void* value,
    DestructorFunc* destructor)
    : value_(value), destructor_(destructor) {}

SequenceLocalStorageMap::ValueDestructorPair::~ValueDestructorPair() {
  if (value_)
    destructor_(value_);
}

SequenceLocalStorageMap::ValueDestructorPair::ValueDestructorPair(
    ValueDestructorPair&& value_destructor_pair)
    : value_(value_destructor_pair.value_),
      destructor_(value_destructor_pair.destructor_) {
  value_destructor_pair.value_ = nullptr;
}

SequenceLocalStorageMap::ValueDestructorPair&
SequenceLocalStorageMap::ValueDestructorPair::operator=(
    ValueDestructorPair&& value_destructor_pair) {
  // Destroy |value_| before overwriting it with a new value.
  if (value_)
    destructor_(value_);

  value_ = value_destructor_pair.value_;
  destructor_ = value_destructor_pair.destructor_;

  value_destructor_pair.value_ = nullptr;

  return *this;
}

}  // namespace internal
}  // namespace base
