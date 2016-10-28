#pragma once
/**
\file BilateralFilter.h

\maintainer, Michael Mara

\created 2016-05-08
\edited  2016-10-28

*/
#include "G3D/Vector2.h"
#include "GLG3D/BilateralFilterSettings.h"
namespace G3D {

class Framebuffer;
class RenderDevice;
class Texture;
class GBuffer;

/**
(Separated) 2D Bilateral Filter.
Yes, bilateral filters are not generally separable. Oh well.

\sa BilateralFilterSettings
*/
class BilateralFilter {
protected:
    shared_ptr<Framebuffer> m_intermediateFramebuffer;

    void apply1D(RenderDevice* rd, const shared_ptr<Texture>& source, const shared_ptr<GBuffer>& gbuffer, const Vector2& direction, const BilateralFilterSettings& settings);
public:
    /**
        Applies a Bilateral Filter. Handles intermediate storage in
        a texture of the same format as the source texture.
    */
    void apply(RenderDevice* rd, const shared_ptr<Texture>& source, const shared_ptr<Framebuffer>& destination, const shared_ptr<GBuffer>& gbuffer, const BilateralFilterSettings& settings);

};
}


