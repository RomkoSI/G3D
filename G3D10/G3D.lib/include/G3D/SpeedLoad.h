#ifndef G3D_SpeedLoad_h
#define G3D_SpeedLoad_h

#include "G3D/platform.h"
#include "G3D/Crypto.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "G3D/HashTrait.h"

namespace G3D {

/** \beta */
class SpeedLoadIdentifier {
protected:
    MD5Hash   hash;

public:

    SpeedLoadIdentifier() {}

    explicit SpeedLoadIdentifier(const MD5Hash& h) : hash(h) {}

    void deserialize(BinaryInput& b) {
        hash.deserialize(b);
    }

    explicit SpeedLoadIdentifier(BinaryInput& b) {
        deserialize(b);
    }

    void serialize(BinaryOutput& b) const {
        hash.serialize(b);
    }

    bool operator==(const SpeedLoadIdentifier& other) const {
        return hash == other.hash;
    }

    bool operator!=(const SpeedLoadIdentifier& other) const {
        return hash != other.hash;
    }

    static size_t hashCode(const SpeedLoadIdentifier& s) {
        return MD5Hash::hashCode(s.hash);
    }

};
    
}

template <> struct HashTrait<G3D::SpeedLoadIdentifier> {
    static size_t hashCode(const G3D::SpeedLoadIdentifier& key) {
        return G3D::SpeedLoadIdentifier::hashCode(key);
    }
};

namespace G3D {


/**
   ArticulatedModel and UniversalMaterial support "SpeedLoad" file formats.
   These are intended for use in reducing load times by caching
   expensive-to-load materials in an efficient binary representation.
   It is not an archival format or one for interchange between tools.
   The format is subject to change in future versions of G3D.  When it
   changes, you should be prepared to regenerate your serialized
   materials from their original sources.

   Some classes have speedCreate(), speedSerialize() and speedDeserialize()
   methods to support this functionality.  These should only be considered
   safe for caching data on a local machine.

   \sa SpeedLoadIdentifier
*/
namespace SpeedLoad {

    /** Most classes prefix their data chunk with a 20-byte string that is the
        class name. */
    enum {HEADER_LENGTH = 32};

    void readHeader(class BinaryInput& b, const String& expectedString);
    void writeHeader(class BinaryOutput& b, const String& header);

};

} // G3D

#endif 

