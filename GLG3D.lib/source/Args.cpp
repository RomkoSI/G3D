/**
 \file GLG3D/Args.cpp
  
 \maintainer Morgan McGuire, Michael Mara http://graphics.cs.williams.edu
 
 \created 2012-06-27
 \edited  2014-03-02

 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */

#include "G3D/platform.h"
#include "G3D/Matrix2.h"
#include "G3D/CoordinateFrame.h"
#include "G3D/Vector4int16.h"
#include "G3D/Vector4uint16.h"
#include "GLG3D/Args.h"
#include "GLG3D/Texture.h"
#include "GLG3D/BufferTexture.h"
#include "GLG3D/GLSamplerObject.h"

namespace G3D {

Args::Args() : m_numIndices(-1), m_rectZCoord(-1.0), m_useG3DArgs(true) {
    computeGridDim = Vector3int32(0, 0, 0);
    m_primitiveType = PrimitiveType::TRIANGLES;
    patchVertices = 3;
    m_numInstances = 1;
}

void Args::setIndexStream(const IndexStream& indexStream){
    alwaysAssertM( !hasComputeGrid() && !hasRect(), 
        "Some CPU attribute pointer(s) already set when attempting to set GPU index stream. "
        "Cannot mix standard and immediate modes.");
    m_indexStream = indexStream;
}

void Args::appendIndexStream(const IndexStream& indexStream) {
    m_indexStreamArray.append(indexStream);
}


int Args::numIndices() const {
    if ( hasComputeGrid() || hasIndirectBuffer() || hasRect()) {
        alwaysAssertM(false, "Args::numIndices called when in a mode that doesn't use indices.");
        return 0;
    } else {
        if (m_indexStream.size() > 0) {
            return m_indexStream.size();
        } else if (m_indexCounts.size() > 0) {
            int totalCount = 0;
            for (int i : m_indexCounts) {
                totalCount += i;
            }
            return totalCount;
        } else if (m_streamArgs.size() == 0) {
            // If there are no arguments
            return m_numIndices;
        } else {
            Args::GPUAttributeTable::Iterator i = m_streamArgs.begin();
            int minNumVertices = (*i).value.attributeArray.size();
            ++i;
            for (; i != m_streamArgs.end(); ++i) {
                const GPUAttribute& array = (*i).value;
                if (array.divisor == 0) {
                    minNumVertices = min(minNumVertices, array.attributeArray.size());
                }
            }
            return minNumVertices;
        }
    } 
}


String Args::toString() const {
    String s = "Args:\n";
    s += "Preamble and Macros Args:\n";
    s += preambleAndMacroString();
    s += "Uniform Args:\n";

    for (ArgTable::Iterator i = m_uniformArgs.begin(); i != m_uniformArgs.end(); ++i){
        const Arg& arg          = (*i).value;
        const String& name = (*i).key;
        s += name + ": " + arg.toString() + "\n";
    }

    s += "Stream Args:\n";
    for (GPUAttributeTable::Iterator i = m_streamArgs.begin(); i != m_streamArgs.end(); ++i){
        const String& name = (*i).key;
        s += name + "\n";
    }
    s += "\n";
    return s;
}

void Args::clearAttributeAndIndexBindings() {
    m_streamArgs.clear();
    m_indexStream = IndexStream();
    m_rect = Rect2D();
    m_texCoordRect = Rect2D();
}


void Args::setRect(const Rect2D& rect, float zCoord, const Rect2D& texCoordRect) {
    alwaysAssertM(!(hasComputeGrid() || hasIndirectBuffer() || hasComputeGrid() || hasStreamArgs() || hasGPUIndexStream()),
                  "Some CPU or GPU attributes already set when trying to setRect. Cannot mix rect mode with any other use of Args.");
    m_rect = rect;
    m_texCoordRect = texCoordRect;
    m_rectZCoord = zCoord;
}


void Args::setPrimitiveType(PrimitiveType type) {
    m_primitiveType = type;
}


void Args::enableG3DArgs(bool enable) {
    m_useG3DArgs = enable;
}

} // end namespace G3D
