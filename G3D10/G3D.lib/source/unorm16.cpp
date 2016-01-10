#include "G3D/platform.h"
#include "G3D/Any.h"
#include "G3D/unorm16.h"

namespace G3D {
unorm16::unorm16(const class Any& a) {
    *this = unorm16(a.number());
}

Any unorm16::toAny() const {
    return Any((float)*this);
}

}
