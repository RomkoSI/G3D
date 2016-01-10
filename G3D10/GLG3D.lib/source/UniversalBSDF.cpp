/**
 \file      UniversalBSDF.cpp
 \author    Morgan McGuire, http://graphics.cs.williams.edu

 \created   2008-08-10
 \edited    2015-01-03

 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/
#include "GLG3D/UniversalBSDF.h"

namespace G3D {

float UniversalBSDF::ignoreFloat;

#define INV_PI  (0.318309886f)
#define INV_8PI (0.0397887358f)


UniversalBSDF::Ref UniversalBSDF::speedCreate(BinaryInput& b) {
    UniversalBSDF::Ref s(new UniversalBSDF());
    
    SpeedLoad::readHeader(b, "UniversalBSDF");

    s->m_lambertian.speedDeserialize(b);
    s->m_glossy.speedDeserialize(b);
    s->m_transmissive.speedDeserialize(b);
    s->m_eta_t = b.readFloat32();
    s->m_extinction_t.deserialize(b);
    s->m_eta_r = b.readFloat32();
    s->m_extinction_r.deserialize(b);

    return s;
}


void UniversalBSDF::speedSerialize(BinaryOutput& b) const {
    SpeedLoad::writeHeader(b, "UniversalBSDF");

    m_lambertian.speedSerialize(b);
    m_glossy.speedSerialize(b);
    m_transmissive.speedSerialize(b);
    b.writeFloat32(m_eta_t);
    m_extinction_t.serialize(b);
    b.writeFloat32(m_eta_r);
    m_extinction_r.serialize(b);
}


shared_ptr<UniversalBSDF> UniversalBSDF::create
(const Component4& lambertian,
 const Component4& glossy,
 const Component3& transmissive,
 float             eta_t,
 const Color3&     extinction_t,
 float             eta_r,
 const Color3&     extinction_r) {

    shared_ptr<UniversalBSDF> bsdf(new UniversalBSDF());

    bsdf->m_lambertian      = lambertian;
    bsdf->m_glossy          = glossy;
    bsdf->m_transmissive    = transmissive;
    bsdf->m_eta_t           = eta_t;
    bsdf->m_extinction_t    = extinction_t;
    bsdf->m_eta_r           = eta_r;
    bsdf->m_extinction_r    = extinction_r;

    return bsdf;
}



void UniversalBSDF::setStorage(ImageStorage s) const {
    m_lambertian.setStorage(s);
    m_transmissive.setStorage(s);
    m_glossy.setStorage(s);
}


bool UniversalBSDF::hasMirror() const {
    const Color4& m = m_glossy.max();
    return (m.a == 1.0f) && ! m.rgb().isZero();
}


bool UniversalBSDF::hasGlossy() const {
    float avg = m_glossy.mean().a;
    return (avg > 0) && (avg < 1) && ! m_glossy.max().rgb().isZero();
}


bool UniversalBSDF::hasLambertian() const {
    return ! m_lambertian.max().rgb().isZero();
}

}
