#include "G3D/ParseError.h"
#include "G3D/format.h"

namespace G3D {

String ParseError::formatFileInfo() const {
    String s;

    if (line != UNKNOWN) {
        if (character != UNKNOWN) {
            return format("%s:%d(%d): ", filename.c_str(), line, character);
        } else {
            return format("%s:%d: ", filename.c_str(), line);
        }
    } else if (byte != UNKNOWN) {
        return format("%s:(%lld): ", filename.c_str(), byte);
    } else if (filename.empty()) {
        return "";
    } else {
        return filename + ": ";
    }
}

}
