/**
 \file   UniversalMaterial.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu

 \created  2009-03-19
 \edited   2011-06-27
*/
#include "GLG3D/UniversalMaterial.h"
#include "G3D/SpeedLoad.h"

namespace G3D {

void UniversalMaterial::speedSerialize(SpeedLoadIdentifier& sli, BinaryOutput& b) const {
    // Copy to CPU so that we can read directly from the components
    // without managing GPU readback here.
    setStorage(COPY_TO_CPU);

    SpeedLoad::writeHeader(b, "UniversalMaterial");

    const size_t start = b.position();
    SpeedLoadIdentifier().serialize(b);  // Reserve space for the speed load identifier
    b.writeUInt32(0);  // Reserve space for the size
    const size_t dataStart = b.position();

    b.writeBool8(notNull(m_bsdf));
    if (m_bsdf) {
        m_bsdf->speedSerialize(b);
    }
    
    m_emissive.speedSerialize(b);

    b.writeBool8(notNull(m_bump));
    if (m_bump) {
        m_bump->speedSerialize(b);
    }

    alwaysAssertM(isNull(m_customMap), 
                  "SpeedLoad UniversalMaterial format does not support custom maps");
    b.writeBool8(false);
    /*
    b.writeBool8(m_customMap);
    if (m_customMap) {
        m_customMap->image()->serialize(b);
    }
    */

    m_customConstant.serialize(b);
    b.writeString32(m_customShaderPrefix);
    m_refractionHint.serialize(b);
    m_mirrorHint.serialize(b);
    b.writeString32(m_macros);
    
    // Write the chunk size and speed load identifier
    size_t end = b.position();
    b.setPosition(start);

       
    sli = SpeedLoadIdentifier(Crypto::md5(b.getCArray() + dataStart, end - dataStart));

    sli.serialize(b);
    b.writeUInt32((uint32)(end - start));
    b.setPosition(end);
}


void UniversalMaterial::speedDeserialize(BinaryInput& b) {
    // Ignore the size
    size_t size = b.readUInt32();
    (void)size;

    const bool hasBSDF = b.readBool8();
    if (hasBSDF) {
        m_bsdf = UniversalBSDF::speedCreate(b);
    }

    m_emissive.speedDeserialize(b);

    const bool hasBump = b.readBool8();
    if (hasBump) {
        m_bump = BumpMap::speedCreate(b);
    }

    const bool hasCustomMap = b.readBool8();
    alwaysAssertM(! hasCustomMap, "SpeedLoad UniversalMaterial format does not support custom maps");
    
    m_customConstant.deserialize(b);
    m_customShaderPrefix = b.readString32();
    m_refractionHint.deserialize(b);
    m_mirrorHint.deserialize(b);
    m_macros = b.readString32();
}


static Table<SpeedLoadIdentifier, weak_ptr<UniversalMaterial> > speedLoadMaterialCache;

shared_ptr<UniversalMaterial> UniversalMaterial::speedCreate(SpeedLoadIdentifier& s, BinaryInput& b) {
    SpeedLoad::readHeader(b, "UniversalMaterial");

    // Read the identifier
    s.deserialize(b);
    
    bool created = false;
    weak_ptr<UniversalMaterial>& weakPtr = speedLoadMaterialCache.getCreate(s, created);
    shared_ptr<UniversalMaterial> material = weakPtr.lock();

    if (isNull(material)) {
        material.reset(new UniversalMaterial());
        material->speedDeserialize(b);

        // Insert into the cache
        weakPtr = material;
    } else {
        // Skip to the end of the chunk because we're reusing a cached material
        size_t skip = b.readUInt32();
        b.skip(skip);
    }

    return material;
}

} // namespace G3D
