// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brick/memory/discardable_memory_allocator.h"

#include "brick/logging.h"

namespace base {
namespace {

DiscardableMemoryAllocator* g_discardable_allocator = nullptr;

}  // namespace

// static
void DiscardableMemoryAllocator::SetInstance(
    DiscardableMemoryAllocator* allocator) {
  DCHECK(!allocator || !g_discardable_allocator);
  g_discardable_allocator = allocator;
}

// static
DiscardableMemoryAllocator* DiscardableMemoryAllocator::GetInstance() {
  DCHECK(g_discardable_allocator);
  return g_discardable_allocator;
}

}  // namespace base
