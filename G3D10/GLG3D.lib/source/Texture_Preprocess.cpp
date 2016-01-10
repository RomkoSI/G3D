#include "GLG3D/Texture.h"
#include "G3D/Any.h"

namespace G3D {

Any Texture::Preprocess::toAny() const {
    Any a(Any::TABLE, "Texture::Preprocess");
    a["modulate"] = modulate;
    a["gammaAdjust"] = gammaAdjust;
    a["scaleFactor"] = scaleFactor;
    a["computeMinMaxMean"] = computeMinMaxMean;
    a["computeNormalMap"] = computeNormalMap;
    a["bumpMapPreprocess"] = bumpMapPreprocess;
    a["convertToPremultipliedAlpha"] = convertToPremultipliedAlpha;
    return a;
}


bool Texture::Preprocess::operator==(const Preprocess& other) const {
    return 
        (modulate == other.modulate) &&
        (gammaAdjust == other.gammaAdjust) &&
        (scaleFactor == other.scaleFactor) &&
        (computeMinMaxMean == other.computeMinMaxMean) &&
        (computeNormalMap == other.computeNormalMap) &&
        (bumpMapPreprocess == other.bumpMapPreprocess) &&
        (convertToPremultipliedAlpha == other.convertToPremultipliedAlpha);
}


Texture::Preprocess::Preprocess(const Any& any) {
    *this = Preprocess::defaults();
    any.verifyNameBeginsWith("Texture::Preprocess");
    if (any.type() == Any::TABLE) {
        for (Any::AnyTable::Iterator it = any.table().begin(); it.isValid(); ++it) {
            const String& key = it->key;
            if (key == "modulate") {
                modulate = Color4(it->value);
            } else if (key == "gammaAdjust") {
                gammaAdjust = it->value;
            } else if (key == "scaleFactor") {
                scaleFactor = it->value;
            } else if (key == "computeMinMaxMean") {
                computeMinMaxMean = it->value;
            } else if (key == "computeNormalMap") {
                computeNormalMap = it->value;
            } else if (key == "convertToPremultipliedAlpha") {
                convertToPremultipliedAlpha = it->value;
            } else if (key == "bumpMapPreprocess") {
                bumpMapPreprocess = it->value;
            } else {
                any.verify(false, "Illegal key: " + it->key);
            }
        }
    } else {
        const String& n = any.name();
        if (n == "Texture::Preprocess::defaults") {
            any.verifySize(0);
        } else if (n == "Texture::Preprocess::gamma") {
            any.verifySize(1);
            *this = Texture::Preprocess::gamma(any[0]);
        } else if (n == "Texture::preprocess::none") {
            any.verifySize(0);
            *this = Texture::Preprocess::none();
        } else if (n == "Texture::Preprocess::quake") {
            any.verifySize(0);
            *this = Texture::Preprocess::quake();
        } else if (n == "Texture::Preprocess::normalMap") {
            any.verifySize(0);
            *this = Texture::Preprocess::normalMap();
        } else {
            any.verify(false, "Unrecognized name for Texture::Preprocess constructor or factory method.");
        }
    }
}


const Texture::Preprocess& Texture::Preprocess::defaults() {
    static const Texture::Preprocess p;
    return p;
}


Texture::Preprocess Texture::Preprocess::gamma(float g) {
    Texture::Preprocess p;
    p.gammaAdjust = g;
    return p;
}


const Texture::Preprocess& Texture::Preprocess::none() {
    static Texture::Preprocess p;
    p.computeMinMaxMean = false;
    return p;
}


const Texture::Preprocess& Texture::Preprocess::quake() {
    static Texture::Preprocess p;
    p.modulate = Color4(2,2,2,1);
    p.gammaAdjust = 1.6f;
    return p;
}


const Texture::Preprocess& Texture::Preprocess::normalMap() {
    static bool initialized = false;
    static Texture::Preprocess p;
    if (! initialized) {
        p.computeNormalMap = true;
        initialized = true;
    }

    return p;
}


void Texture::Preprocess::modulateImage(ImageFormat::Code fmt, void* _byte, int n) const {
    debugAssertM(
        (fmt == ImageFormat::CODE_RGB8) ||
        (fmt == ImageFormat::CODE_RGBA8) ||
        (fmt == ImageFormat::CODE_R8) ||
        (fmt == ImageFormat::CODE_L8),
        "Texture preprocessing only implemented for 1, 3, 4 8-bit channels.");

    uint8* byte = static_cast<uint8*>(_byte);

    // Make a lookup table
    uint8 adjust[4][256];
    for (int c = 0; c < 4; ++c) {
        for (int i = 0; i < 256; ++i) {
            float s = float(pow((i * modulate[c]) / 255, gammaAdjust) * 255);
            adjust[c][i] = iClamp(iRound(s), 0, 255);
        }
    }

    switch (fmt) {
    case ImageFormat::CODE_RGBA8:
        for (int i = 0; i < n; ++i) {
            for (int c = 0; c < 3; ++c, ++i) {
                byte[i] = adjust[c][byte[i]];
            }
        }

        if (convertToPremultipliedAlpha) {
            for (int i = 0; i < n; i += 4) {
                int a = byte[i + 3];
                for (int c = 0; c < 3; ++c) {
                    byte[i + c] = (int(byte[i + c]) * a) / 255;
                }
            }
        }
        break;

    case ImageFormat::CODE_RGB8:
        for (int i = 0; i < n; ) {
            for (int c = 0; c < 3; ++c, ++i) {
                byte[i] = adjust[c][byte[i]];
            }
        }
        break;

    case ImageFormat::CODE_R8:
    case ImageFormat::CODE_L8:
        for (int i = 0; i < n; ++i) {
            byte[i] = adjust[0][byte[i]];
        }
        break;

    default:;
    }
}

}
