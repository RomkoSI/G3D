/**
 \file   Material_Specification.cpp
 \author Morgan McGuire, http://graphics.cs.williams.edu
 \date   2009-03-10
 \edited 2016-02-06
*/
#include "GLG3D/UniversalMaterial.h"
#include "G3D/Any.h"
#include "G3D/CPUPixelTransferBuffer.h"
#include "GLG3D/glcalls.h"

namespace G3D {

UniversalMaterial::Specification::Specification() : 
  m_lambertian(Texture::Specification(Color4(0.85f, 0.85f, 0.85f, 1.0f))),
  m_glossy(Texture::Specification(Color4::zero())),
  m_transmissive(Texture::Specification(Color3::zero())),
  m_etaTransmit(1.0f),
  m_extinctionTransmit(1.0f),
  m_etaReflect(1.0f),
  m_extinctionReflect(1.0f),
  m_emissive(Texture::Specification(Color3::zero())),
  m_refractionHint(RefractionHint::DYNAMIC_FLAT), 
  m_mirrorHint(MirrorQuality::STATIC_PROBE),
  m_numLightMapDirections(0),
  m_alphaHint(AlphaHint::DETECT),
  m_inferAmbientOcclusionAtTransparentPixels(true) {
}



UniversalMaterial::Specification::Specification(const Color4& color) {
    *this = Specification();
    m_lambertian = Texture::Specification(color);
}


void UniversalMaterial::Specification::setLightMaps(const shared_ptr<Texture>& lightMap) {
    for (int i = 0; i < 3; ++i) {
        m_lightMap[i].reset();
    }

    if (isNull(lightMap)) {
        m_numLightMapDirections = 0;
    } else {
        m_numLightMapDirections = 1;
        m_lightMap[0] = lightMap;
    }
}


void UniversalMaterial::Specification::setLightMaps(const shared_ptr<UniversalMaterial>& material) {
    debugAssert(notNull(material));
    m_numLightMapDirections = material->m_numLightMapDirections;

    for (int i = 0; i < 3; ++i) {
        m_lightMap[i].reset();
    }

    if (m_numLightMapDirections == 1) {
        m_lightMap[0] = material->m_lightMap[0].texture();
    } else if (m_numLightMapDirections == 3) {
        for (int i = 0; i < 3; ++i) {
            m_lightMap[i] = material->m_lightMap[i].texture();
        }
    }
}


void UniversalMaterial::Specification::setLightMaps(const shared_ptr<Texture> lightMap[3]) {
    m_numLightMapDirections = 3;
    for (int i = 0; i < 3; ++i) {
        m_lightMap[i] = lightMap[i];
        debugAssert(notNull(lightMap));
    }
}


UniversalMaterial::Specification::Specification(const Any& any) {
    *this = Specification();

    if ((any.type() == Any::STRING) || (any.type() == Any::NUMBER) || any.nameBeginsWith("Color3") || any.nameBeginsWith("Color4")) {
        // Single filename; treat as a diffuse-only texture map
        setLambertian(Texture::Specification(any));
        return;
    }
    
    any.verifyName("UniversalMaterial::Specification");

    Any texture;
    Color4 constant;

    for (Any::AnyTable::Iterator it = any.table().begin(); it.isValid(); ++it) {
        const String& key = toLower(it->key);
        if (key == "lambertian") {
            setLambertian(Texture::Specification(it->value, true, Texture::DIM_2D));
        } else if ((key == "glossy") || (key == "specular")) {
            if (key == "specular") {
                debugPrintf("%s(%d): Warning: 'specular' is deprecated in UniversalMaterial::Specification...use 'glossy'\n", it->value.source().filename.c_str(), it->value.source().line);
            }
            setGlossy(Texture::Specification(it->value, true, Texture::DIM_2D));
        } else if (key == "shininess") {
            it->value.verify(false, "shininess is no longer accepted by UniversalMaterial. Use the alpha (smoothness) channel of the glossy value.");
        } else if (key == "transmissive") {
            setTransmissive(Texture::Specification(it->value, true, Texture::DIM_2D));
        } else if (key == "emissive") {
            setEmissive(Texture::Specification(it->value, true, Texture::DIM_2D));
        } else if (key == "bump") {
            setBump(BumpMap::Specification(it->value));
        } else if (key == "refractionhint") {
            m_refractionHint = it->value;
        } else if (key == "mirrorhint") {
            m_mirrorHint = it->value;
        } else if (key == "etatransmit") {
            m_etaTransmit = it->value;
        } else if (key == "extinctiontransmit") {
            m_extinctionTransmit = it->value;
        } else if (key == "etareflect") {
            m_etaReflect = it->value;
        } else if (key == "extinctionreflect") {
            m_extinctionReflect = it->value;
        } else if (key == "customshaderprefix") {
            m_customShaderPrefix = it->value.string();
        } else if (key == "custom") {
            m_customTex = Texture::create(it->value);
        } else if (key == "alphahint") {
            m_alphaHint = it->value;
        } else if (key == "sampler") {
            m_sampler = it->value;
        } else if (key == "inferambientocclusionattransparentpixels") {
            m_inferAmbientOcclusionAtTransparentPixels = it->value;
        } else if (key == "constanttable") {
            for (Any::AnyTable::Iterator it2 = it->value.table().begin(); it2.isValid(); ++it2) {
                const Any& v = it2->value;
                if (v.type() == Any::BOOLEAN) {
                    m_constantTable.set(it2->key, bool(v));
                } else {
                    m_constantTable.set(it2->key, double(v));
                }
            }
        } else {
            any.verify(false, "Illegal key: " + it->key);
        }
    }
}


Any UniversalMaterial::Specification::toAny() const {
    Any a(Any::TABLE, "UniversalMaterial::Specification");
    // TODO
    debugAssertM(false, "Unimplemented");
    return a;
}


void UniversalMaterial::Specification::setLambertian(const shared_ptr<Texture>& tex) {
    m_lambertianTex = tex;
}


void UniversalMaterial::Specification::setLambertian(const Texture::Specification& spec) {
    m_lambertian = spec;        
    m_lambertianTex = shared_ptr<Texture>();
}


void UniversalMaterial::Specification::removeLambertian() {
    setLambertian(Texture::Specification(Color4(0,0,0,1)));
}


void UniversalMaterial::Specification::setEmissive(const Texture::Specification& spec) {
    m_emissive = spec;
    m_emissiveTex = shared_ptr<Texture>();
}

void UniversalMaterial::Specification::setEmissive(const shared_ptr<Texture>& tex) {
    m_emissiveTex = tex;
}


void UniversalMaterial::Specification::removeEmissive() {
    setEmissive(Texture::Specification(Color3::zero()));
}


void UniversalMaterial::Specification::setGlossy(const Texture::Specification& spec) {
    m_glossy = spec;        
}


void UniversalMaterial::Specification::setGlossy(const shared_ptr<Texture>& tex) {
    m_glossyTex = tex;
}


void UniversalMaterial::Specification::removeGlossy() {
    setGlossy(Texture::Specification(Color4::zero()));
}


void UniversalMaterial::Specification::setTransmissive(const Texture::Specification& spec) {
    m_transmissive = spec;
    m_transmissiveTex.reset();
}


void UniversalMaterial::Specification::setTransmissive(const shared_ptr<Texture>& tex) {
    m_transmissiveTex = tex;
}


void UniversalMaterial::Specification::removeTransmissive() {
    setTransmissive(Texture::Specification(Color3::zero()));
}


void UniversalMaterial::Specification::setEta(float etaTransmit, float etaReflect) {
    m_etaTransmit = etaTransmit;
    m_etaReflect = etaReflect;
    debugAssert(m_etaTransmit > 0);
    debugAssert(m_etaTransmit < 10);
    debugAssert(m_etaReflect > 0);
    debugAssert(m_etaReflect < 10);
}


void UniversalMaterial::Specification::setBump
(const String&              filename, 
 const BumpMap::Settings&   settings,
 float                      normalMapWhiteHeightInPixels) {
    
     m_bump = BumpMap::Specification();
     m_bump.texture.filename = filename;
     m_bump.texture.preprocess = Texture::Preprocess::normalMap();
     m_bump.texture.preprocess.bumpMapPreprocess.zExtentPixels = 
         normalMapWhiteHeightInPixels;
     m_bump.settings = settings;
}


void UniversalMaterial::Specification::removeBump() {
    m_bump.texture.filename = "";
}


bool UniversalMaterial::Specification::operator==(const Specification& s) const {
    return 
        (m_lambertian == s.m_lambertian) &&
        (m_lambertianTex == s.m_lambertianTex) &&

        (m_glossy == s.m_glossy) &&
        (m_glossyTex == s.m_glossyTex) &&

        (m_transmissive == s.m_transmissive) &&
        (m_transmissiveTex == s.m_transmissiveTex) &&

        (m_emissive == s.m_emissive) &&
        (m_emissiveTex == s.m_emissiveTex) &&

        (m_bump == s.m_bump) &&

        (m_etaTransmit == s.m_etaTransmit) &&
        (m_extinctionTransmit == s.m_extinctionTransmit) &&
        (m_etaReflect == s.m_etaReflect) &&
        (m_extinctionReflect == s.m_extinctionReflect) &&

        (m_customTex == s.m_customTex) &&

        (m_refractionHint == s.m_refractionHint) &&
        (m_mirrorHint == s.m_mirrorHint) &&
        
        (m_customShaderPrefix == s.m_customShaderPrefix) &&

        (m_numLightMapDirections == s.m_numLightMapDirections) &&

        (m_lightMap[0] == s.m_lightMap[0]) &&
        (m_lightMap[1] == s.m_lightMap[1]) &&
        (m_lightMap[2] == s.m_lightMap[2]) &&

        (m_inferAmbientOcclusionAtTransparentPixels == s.m_inferAmbientOcclusionAtTransparentPixels) && 

        (m_alphaHint == s.m_alphaHint) &&
        
        (m_constantTable == s.m_constantTable);
}


size_t UniversalMaterial::Specification::hashCode() const {
    return 
        HashTrait<String>::hashCode(m_lambertian.filename) ^
        HashTrait<shared_ptr<Texture> >::hashCode(m_lambertianTex) ^

        HashTrait<String>::hashCode(m_glossy.filename) ^
        HashTrait<shared_ptr<Texture> >::hashCode(m_glossyTex) ^

        HashTrait<String>::hashCode(m_transmissive.filename) ^
        HashTrait<shared_ptr<Texture> >::hashCode(m_transmissiveTex) ^

        HashTrait<String>::hashCode(m_emissive.filename) ^
        HashTrait<shared_ptr<Texture> >::hashCode(m_emissiveTex) ^

        HashTrait<String>::hashCode(m_bump.texture.filename) ^
        
        HashTrait<String>::hashCode(m_customShaderPrefix) 

        ^ m_alphaHint.hashCode()

        ^ HashTrait<shared_ptr<Texture> >::hashCode(m_lightMap[0])
        ^ HashTrait<shared_ptr<Texture> >::hashCode(m_lightMap[1])
        ^ HashTrait<shared_ptr<Texture> >::hashCode(m_lightMap[2])
        
        ^ m_constantTable.size();
}


Component4 UniversalMaterial::Specification::loadLambertian() const {
    if (notNull(m_lambertianTex)) {
        return Component4(m_lambertianTex);
    } else {
        return Component4(Texture::create(m_lambertian));
    }
}


Component3 UniversalMaterial::Specification::loadTransmissive() const {
    if (notNull(m_transmissiveTex)) {
        return Component3(m_transmissiveTex);
    } else {
        return Component3(Texture::create(m_transmissive));
    }
}


Component4 UniversalMaterial::Specification::loadGlossy() const {
    if (notNull(m_glossyTex)) {
        return Component4(m_glossyTex);
    } else {
        debugAssertGLOk();
        return Component4(Texture::create(m_glossy));
    }
}


Component3 UniversalMaterial::Specification::loadEmissive() const {
    if (isNull(m_emissiveTex) && ! m_emissive.filename.empty()) {
        return Component3(Texture::create(m_emissive));
    }

    return Component3(m_emissiveTex);
}

} // namespace G3D

