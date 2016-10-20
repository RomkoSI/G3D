/**
  \file G3D/typeutils.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2011-06-10
  \edited  2011-06-10

  Copyright 2000-2015, Morgan McGuire.
  All rights reserved.
 */
#ifndef G3D_typeutils_h
#define G3D_typeutils_h

#include "G3D/platform.h"
#include "G3D/HashTrait.h"
#include "G3D/Array.h"
#include "G3D/Table.h"
#include "G3D/AreaMemoryManager.h"

namespace G3D {

/**
 \brief Separates a large array into subarrays by their typeid().

 Example:
 \code
 Array<shared_ptr<Surface> > all = ...;
 Array< Array<shared_ptr<Surface> > > derivedArray;
 categorizeByDerivedType<shared_ptr<Surface> >(all, derivedArray);
 \endcode
 */
template<class PointerType>
void categorizeByDerivedType(const Array<PointerType>& all, Array< Array<PointerType> >& derivedArray) {
    derivedArray.fastClear();

    // Allocate space for the worst case, so that we don't have to copy arrays
    // all over the place during resizing.
    derivedArray.reserve(all.size());

    Table<std::type_info *const, int> typeInfoToIndex;
    // Allocate the table elements in a memory area that can be cleared all at once
    // without invoking destructors.
    typeInfoToIndex.clearAndSetMemoryManager(AreaMemoryManager::create(100 * 1024));

    for (int s = 0; s < all.size(); ++s) {
        const PointerType& instance = all[s];
        
        bool created = false;
        int& index = typeInfoToIndex.getCreate(const_cast<std::type_info*const>(&typeid(*instance)), created);
        if (created) {
            // This is the first time that we've encountered this subclass.
            // Allocate the next element of subclassArray to hold it.
            index = derivedArray.size();
            derivedArray.next();
        }
        derivedArray[index].append(instance);
    }
}

} // namespace G3D
#endif // typeutils.h
