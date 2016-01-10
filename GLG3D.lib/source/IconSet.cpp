/**
  \file IconSet.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2010-01-04
  \edited  2011-07-02

  G3D Library http://g3d.codeplex.com
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/

#include "GLG3D/IconSet.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "G3D/fileutils.h"
#include "G3D/FileSystem.h"
#include "G3D/Image.h"
#include "G3D/WeakCache.h"
#include "G3D/CPUPixelTransferBuffer.h"

namespace G3D {

static WeakCache<String, shared_ptr<IconSet> > cache;

shared_ptr<IconSet> IconSet::fromFile(const String& filename) {
    shared_ptr<IconSet> set = cache[filename];
    if (notNull(set)) {
        return set;
    }

    BinaryInput b(filename, G3D_LITTLE_ENDIAN);
    
    const String& header = b.readString();
    alwaysAssertM(header == "ICON", "Corrupt icon file");
    
    const float version = b.readFloat32();
    alwaysAssertM(version == 1.0f, "Unsupported icon file version");

    shared_ptr<IconSet> s(new IconSet());

    s->m_icon.resize(b.readInt32());
    for (int i = 0; i < s->m_icon.size(); ++i) {
        Entry& e = s->m_icon[i];
        e.filename = b.readString32();
        float x, y, w, h;
        x = b.readFloat32();
        y = b.readFloat32();
        w = b.readFloat32();
        h = b.readFloat32();
        e.rect = Rect2D::xywh(x, y, w, h);
        s->m_index.set(e.filename, i);
    }

    shared_ptr<Image> image = Image::fromBinaryInput(b);
    bool generateMipMaps = true;
    s->m_texture = Texture::fromPixelTransferBuffer(filename, image->toPixelTransferBuffer(), ImageFormat::AUTO(), Texture::DIM_2D, generateMipMaps);

    cache.set(filename, s);
    return s;
}


void IconSet::findImages(const String& baseDir, const String& sourceDir, Array<Source>& sourceArray) {
    Array<String> filenameArray;
    FileSystem::getFiles(pathConcat(pathConcat(baseDir, sourceDir), "*"), filenameArray);
    for (int i = 0; i < filenameArray.size(); ++i) {
        if (Image::fileSupported(filenameArray[i])) {
            String f = pathConcat(sourceDir, filenameArray[i]);
            shared_ptr<Image> im = Image::fromFile(pathConcat(baseDir, f));
            Source& s = sourceArray.next();
            s.filename = f;
            s.width    = im->width();
            s.height   = im->height();
            // todo (Image upgrade): find replacement calculation for channels
            //s.channels = im.channels;
        }
    }

    Array<String> dirArray;
    FileSystem::getDirectories(pathConcat(pathConcat(baseDir, sourceDir), "*"), dirArray);
    for (int i = 0; i < dirArray.size(); ++i) {
        if (dirArray[i] != ".svn" && dirArray[i] != "CVS") {
            findImages(baseDir, pathConcat(sourceDir, dirArray[i]), sourceArray);
        }
    }
}


void IconSet::makeIconSet(const String& baseDir, const String& outFile) {
    /* todo (Image upgrade): find replacement calculation for channels
    // Find all images
    Array<Source> sourceArray;
    findImages(baseDir, "", sourceArray);
   
    // See if we can fit everything in one row
    int maxWidth = 0;
    int minHeight = 0;
    int maxChannels = 0;
    for (int i = 0; i < sourceArray.size(); ++i) {
        maxWidth += sourceArray[i].width;
        minHeight = max(minHeight, sourceArray[i].height);
        maxChannels = max(maxChannels, sourceArray[i].channels);
    }

    int width = min(maxWidth, 1024);

    // Round to the nearest multiple of 4 pixels; PNG and textures
    // like this sizing.
    width = (width / 4) * 4;
    
    // Given our width, walk through on a pretend allocation to see
    // how much space we need. 

    // Current row width
    int w = 0;

    // Current row height
    int h = 0;

    // Full image height
    int height = 0;
    for (int i = 0; i < sourceArray.size(); ++i) {
        // Walk until we hit the end of the row
        const Source& s = sourceArray[i];
        if (s.width + w > width) {
            // Start the next row
            height += h;
            w = 0;
            h = 0;
        }

        // Add this icon
        w += s.width;
        h = max(h, s.height);
    }
    height += h;

    alwaysAssertM(height < 1024, "Height must be less than 1024");

    BinaryOutput b(outFile, G3D_LITTLE_ENDIAN);

    // Write the header
    b.writeString("ICON");
    b.writeFloat32(1.0f);
    b.writeInt32(sourceArray.size());

    GImage packed(width, height, maxChannels);
    w = 0; h = 0;
    int y = 0;
    for (int i = 0; i < sourceArray.size(); ++i) {
        // Walk until we hit the end of the row
        const Source& s = sourceArray[i];
        if (s.width + w > width) {
            // Start the next row
            y += h;
            w = 0;
            h = 0;
        }

        b.writeString32(s.filename);
        b.writeFloat32(w);
        b.writeFloat32(y);
        b.writeFloat32(s.width);
        b.writeFloat32(s.height);

        GImage src;
        if (s.channels == maxChannels) {
            src.load(pathConcat(baseDir, s.filename));
        } else {
            // Need to expand the number of channels
            GImage tmp(pathConcat(baseDir, s.filename));
            src.resize(tmp.width(), tmp.height(), maxChannels);
            if (maxChannels == 4) {
                if (tmp.channels() == 1) {
                    GImage::LtoRGBA((unorm8*)tmp.byte(), (unorm8*)src.byte(), tmp.width() * tmp.height());
                } else if (tmp.channels() == 3) {
                    GImage::RGBtoRGBA((unorm8*)tmp.byte(), (unorm8*)src.byte(), tmp.width() * tmp.height());
                }
            } else {
                debugAssert(maxChannels == 3 && tmp.channels() == 1);
                GImage::LtoRGB((unorm8*)tmp.byte(), (unorm8*)src.byte(), tmp.width() * tmp.height());
            }
        }
        
        // Paste into the packed image
        GImage::copyRect(packed, src, w, y, 0, 0, src.width(), src.height());

        w += s.width;
        h = max(h, s.height);
    }

    packed.encode(GImage::PNG, b);

    b.commit();
    */
}


int IconSet::getIndex(const String& s) const {
    const int* i = m_index.getPointer(s);
        
    if (notNull(i)) {
        return *i;
    } else {
        debugAssertM(notNull(i), format("Icon \"%s\" not found.", s.c_str()));
        return 0;
    }
}


Icon IconSet::get(int index) const {
    Icon icon(m_texture, m_icon[index].rect);
    icon.m_keepAlive = dynamic_pointer_cast<IconSet>(const_cast<IconSet*>(this)->shared_from_this());
    return icon;
}

}
