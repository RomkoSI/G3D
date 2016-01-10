#include "GLG3D/Texture.h"
#include "G3D/Any.h"
#include "G3D/FileSystem.h"

namespace G3D {

static bool anyNameIsColor3Variant(const Any& a) {
	return a.nameBeginsWith("Color3") || a.nameBeginsWith("Power3") || a.nameBeginsWith("Radiance3") || a.nameBeginsWith("Biradiance3") || a.nameBeginsWith("Radiosity3");
}

Texture::Encoding::Encoding(const Any& a) {
    *this = Encoding();

    if (a.type() == Any::STRING) {
        format = ImageFormat::fromString(a);
    } else if (a.nameBeginsWith("Color4")) {
        readMultiplyFirst = a;
	}
	else if (anyNameIsColor3Variant(a)) {
        readMultiplyFirst = Color4(Color3(a), 1.0f);
    } else if (a.type() == Any::NUMBER) {
        readMultiplyFirst = Color4::one() * float(a.number());
    } else {
        AnyTableReader r(a);

        r.getIfPresent("frame", frame);
        r.getIfPresent("readMultiplyFirst", readMultiplyFirst);
        r.getIfPresent("readAddSecond", readAddSecond);

        String fmt;
        if (r.getIfPresent("format", fmt)) {
            format = ImageFormat::fromString(fmt);
        }
    }
}


Any Texture::Encoding::toAny() const {
    Any a(Any::TABLE, "Texture::Encoding");
    a["frame"] = frame;
    a["readMultiplyFirst"] = readMultiplyFirst;
    a["readAddSecond"] = readAddSecond;
    a["format"] = format ? format->name() : "NULL";
    return a;
}


bool Texture::Encoding::operator==(const Encoding& e) const {
    return (frame == e.frame) && 
        (readMultiplyFirst == e.readMultiplyFirst) &&
        (readAddSecond == e.readAddSecond) &&
        (format == e.format);
}



Any Texture::Specification::toAny() const {
    Any a = Any(Any::TABLE, "Texture::Specification");
    a["filename"]           = filename;
    a["alphaFilename"]      = alphaFilename;
    a["encoding"]           = encoding;
    a["dimension"]          = toString(dimension);
    a["generateMipMaps"]    = generateMipMaps;
    a["preprocess"]         = preprocess;
    a["visualization"]      = visualization;
    a["assumeSRGBSpaceForAuto"] = assumeSRGBSpaceForAuto;
    a["cachable"]           = cachable;

    return a;
}


void Texture::Specification::serialize(BinaryOutput& b) const {
    toAny().serialize(b);
}


void Texture::Specification::deserialize(BinaryInput& b) {
    Any a;
    a.deserialize(b);
    *this = a;
}


bool Texture::Specification::operator==(const Specification& other) const {
    return 
        (filename == other.filename) &&
        (alphaFilename == other.alphaFilename) &&
        (dimension == other.dimension) &&
        (generateMipMaps == other.generateMipMaps) &&
        (preprocess == other.preprocess) &&
        (visualization == other.visualization) &&
        (encoding == other.encoding) &&
        (assumeSRGBSpaceForAuto == other.assumeSRGBSpaceForAuto) &&
        (cachable == other.cachable);
}


Texture::Specification::Specification(const Any& any, bool assumesRGBForAuto, Dimension defaultDimension) {
    *this = Specification();
    assumeSRGBSpaceForAuto = assumesRGBForAuto;
    dimension              = defaultDimension;

    if (any.type() == Any::STRING) {
        filename = any.string();
        if (filename == "<whiteCube>") {
            filename = "<white>";
            dimension = Texture::DIM_CUBE_MAP;
        }

        if (! beginsWith(filename, "<")) {
            filename = any.resolveStringAsFilename();
            if (FilePath::containsWildcards(filename)) {

                // Assume this is a cube map
                dimension = Texture::DIM_CUBE_MAP;
            }
        }
    } else if ((any.type() == Any::NUMBER) ||
               any.nameBeginsWith("Color4") || 
			   anyNameIsColor3Variant(any)) {
        filename = "<white>";
        encoding.readMultiplyFirst = Color4(any);
    } else {

        any.verifyNameBeginsWith("Texture::Specification");
        AnyTableReader r(any);
        r.getFilenameIfPresent("filename", filename);
        r.getFilenameIfPresent("alphaFilename", alphaFilename);
        r.getIfPresent("encoding", encoding);
        r.getIfPresent("assumeSRGBSpaceForAuto", assumeSRGBSpaceForAuto);
        {
            Any a;
            if (r.getIfPresent("dimension", a)) {
                dimension = toDimension(a);
            }
        }
        r.getIfPresent("generateMipMaps", generateMipMaps);
        r.getIfPresent("preprocess", preprocess);
        r.getIfPresent("visualization", visualization);
        r.getIfPresent("cachable", cachable);
        r.verifyDone();

        if (! any.containsKey("dimension") && FilePath::containsWildcards(filename)) {
            // Assume this is a cube map
            dimension = Texture::DIM_CUBE_MAP;
        }
    }
}

}
