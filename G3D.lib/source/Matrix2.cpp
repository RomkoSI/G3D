#include "G3D/platform.h"
#include "G3D/Matrix2.h"
#include "G3D/Any.h"

namespace G3D {

Matrix2::Matrix2(const Any& any) {
    any.verifyName("Matrix2");
    any.verifyType(Any::ARRAY);

    if (any.nameEquals("Matrix2::identity")) {
        *this = identity();
    } else {
        any.verifySize(4);

        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                data[r][c] = any[r * 2 + c];
            }
        }
    }
}

} // namespace
