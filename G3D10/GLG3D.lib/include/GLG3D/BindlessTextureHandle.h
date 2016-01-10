/**
\file GLG3D/BindlessHandle.h

\maintainer Michael Mara, http://www.illuminationcodified.com

\created 2015-07-13
\edited  2015-07-13

Copyright 2000-2015, Morgan McGuire.
All rights reserved.
*/


#ifndef G3D_BindlessHandle_h
#define G3D_BindlessHandle_h

#include "G3D/ReferenceCount.h"

namespace G3D {
    class Texture;
    class GLSamplerObject;
    class Sampler;
    /** 
        A wrapper for bindless texture handles, as introduced by https://www.opengl.org/registry/specs/ARB/bindless_texture.txt 
        Only use this class if your OpenGL context supports ARB_bindless_texture.


        We keep a shared_ptr to the texture and sampler objects that were used to create the underlying handle, so that it is valid until
        the destructor is run or set() is called again. 
        
        Common use:

        Array<uint64> rawHandles;
        for (int i = 0; i < arrayOfRelevantTextures; ++i) {
            bindlessHandles.append(BindlessTextureHandle(arrayOfRelevantTextures[i], Sampler::defaults()));
            rawHandles.append(bindlessHandles.last().glHandle());
        }
        shared_ptr<CPUPixelTransferBuffer> ptb = CPUPixelTransferBuffer::fromData(rawHandles.size(), 1, ImageFormat::RG32UI(), rawHandles.getCArray());
        m_indirectionTexture = Texture::fromPixelTransferBuffer("Indirection Texture", ptb);

        ... later, in a shader with the indirection texture bound ...

        sampler2D myTexture = sampler2D(texelFetch(indirectionTexture, ivec2(index, 0), 0).rg);
        // Use myTexture as you would any other sampler2D
        ...


        It is up to the user to make sure this object does not go out of scope until the last use of the underlying GL handle.
        
        As of 7/12/2015, supported on most recent AMD and NVIDIA cards, but none of Intel's hardware.

        \beta We expect to soon add support for bindless image handles; perhaps by augmenting this class, perhaps by a separate BindlessImageHandle.

    */
    class BindlessTextureHandle {
    private:
        /** 0 if before initialization */
        uint64                      m_GLHandle;
        shared_ptr<Texture>         m_texture;
        shared_ptr<GLSamplerObject> m_samplerObject;
    public:
        BindlessTextureHandle();
        ~BindlessTextureHandle();

        /** Construct and call set() */
        BindlessTextureHandle(shared_ptr<Texture> tex, const Sampler& sampler);

        /** 
            If this object is already valid, make sure the handle is non-resident and then create a new handle
            from the specified texture and sampler. Since this object holds on to a copy of the Texture and GLSamplerObjects,
            the underlying handle should be valid until the destructor is run or set() is called again.
            */
        void set(shared_ptr<Texture> tex, const Sampler& sampler);

        /** 
            The underlying bindless texture handle. Pass this into shaders to access textures without needing to bind them. Currently the
            easiest method for doing this is to use an RG32UI texture, texelFetch from it, and use the sampler2D(uvec) constructor.
            
            We will hopefully soon add support for passing texture handles as uniforms and vertex attributes.
            */
        uint64 glHandle() const {
            return m_GLHandle;
        }

        /** Is the underlying texture handle valid? */
        bool isValid() const {
            return m_GLHandle != 0;
        }

        shared_ptr<Texture> texture() const {
            return m_texture;
        }

        bool isResident() const;

        /** Raises an assertion if the texture handle is invalid, and does nothing if the handle is already resident. */
        void makeResident();

        /** Does nothing if the handle is invalid or already non resident. */
        void makeNonResident();

    };
}

#endif