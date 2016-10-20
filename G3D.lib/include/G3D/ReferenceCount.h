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
protected:

	/** Like std::make_shared, but works for protected constructors. Call as createShared<myclass>. */
	template<class T, class ... ArgTypes>
	static std::shared_ptr<T> createShared(ArgTypes&& ... args) {
		// Work around C++ protection problem with std::make_shared
		// http://stackoverflow.com/questions/3530771/passing-variable-arguments-to-another-function-that-accepts-a-variable-argument
		struct make_shared_enabler : public T {
			make_shared_enabler(ArgTypes&& ... args) : T(std::forward<ArgTypes>(args) ...) {}
		};
		return std::make_shared<make_shared_enabler>(std::forward<ArgTypes>(args) ...);
	};

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

