/**
  \file G3D/ReferenceCount.h

  Reference Counting Garbage Collector for C++

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2001-10-23
  \edited  2016-07-09

  G3D Innovation Engine
  Copyright 2000-2016, Morgan McGuire.
  All rights reserved.
*/
#pragma once

#include "G3D/platform.h"

#define USE_SHARED_PTR

#define ReferenceCountedPointer shared_ptr
#define WeakReferenceCountedPointer weak_ptr
namespace G3D {

class ReferenceCountedObject : public enable_shared_from_this<ReferenceCountedObject> {
public:
    virtual ~ReferenceCountedObject() {};
};

template<class T>
bool isNull(const ReferenceCountedPointer<T>& ptr) {
    return ! ptr;
}

template<class T>
bool notNull(const ReferenceCountedPointer<T>& ptr) {
    return (bool)ptr;
}

template<class T>
bool isNull(const T* ptr) {
    return ptr == nullptr;
}

template<class T>
bool notNull(const T* ptr) {
    return ptr != nullptr;
}

} // namespace

