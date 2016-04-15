/**
 \file GLG3D/Film.h
 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2008-07-01
 \edited  2016-04-05

 G3D Library http://g3d.cs.williams.edu
 Copyright 2000-2016, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
 Use permitted under the BSD license

 */

#ifndef G3D_Film_h
#define G3D_Film_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/Shader.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/Texture.h"
#include "GLG3D/GuiContainer.h"
#include "GLG3D/FilmSettings.h"

namespace G3D {

/** \brief Post processing: gamma correction, exposure, bloom, and
    screen-space antialiasing.

   Computer displays are not capable of representing the range of
   values that are rendered by a physically based system.  For
   example, the brightest point on a monitor rarely has the intensity
   of a light bulb.  Furthermore, for historical (and 2D GUI
   rendering) reasons, monitors apply a power ("gamma") curve to
   values.  So rendering code that directly displays radiance values
   on a monitor will neither capture the desired tonal range nor even
   present the values scaled linearly.  The Film class corrects for
   this using the simple tone mapping algorithm presented in Pharr and
   Humphreys 2004 extended with color desaturation.
      The bloom effects are most pronounced when rendering values that are 
   actually proportional to radiance.  That is, if all of the values in the
   input are on a narrow range, there will be little bloom.  But if the 
   sky, highlights, emissive surfaces, and light sources are 10x brighter 
   than most scene objects, they will produce attractive glows and halos.

   When rendering multiple viewports or off-screen images, use a separate 
   Film instance for each size of input for maximum performance.   
*/
class Film : public ReferenceCountedObject {
private:
    
    /** Filters may cache state for performance, so each Film must have its own set. */
    class Filter : public ReferenceCountedObject {
    protected:
        /** When this is not the final filter in the chain, this framebuffer is used for the output */
        mutable shared_ptr<Framebuffer> m_intermediateResultFramebuffer;

        void allocate(const String& name, const shared_ptr<Texture>& source, const shared_ptr<Framebuffer>& argTarget, int sourceDepthGuardBandThickness, const ImageFormat* fmt = ImageFormat::RGBA8()) const;

    public:
    
        /** Set by apply() */
        mutable shared_ptr<Framebuffer> target;

        /** Apply the filter to source, writing to target
        
            The CompositeFilter and EffectsDisabledBlitFilter expect a guard band (if there is one specified in \a settings) on the input and produce output without
            a guard band.  All other filters assume no guard band on input or output (and assert that). This works because exactly one of CompositeFilter or EffectsDisabledBlitFilter
            is always in the filter chain and always at the front.
            
            If \a target is null, then all filters allocate an output that is the same size as the input (except for CompositeFilter removing the
            guard band). If \a target is not null, then the output is stretched to the size of \a target using bilinear interpolation.

            \param target If null, allocate intermediateResultFramebuffer and set this->target to it. If not null, set this->target = target
         */
        virtual void apply(RenderDevice* rd, const FilmSettings& settings, const shared_ptr<Texture>& source, const shared_ptr<Framebuffer>& target, int sourceTrimBandThickness, int sourceColorBandThickness) const = 0;
    };

    /** Includes bloom, vignette, tone map */
    class CompositeFilter : public Filter {
    protected:        
        /** Used for all buffers except the output */
        const ImageFormat*                      m_intermediateFormat;

        /** Working framebuffer */
        mutable shared_ptr<Framebuffer>        m_framebuffer;
        mutable shared_ptr<Framebuffer>        m_tempFramebuffer;
        mutable shared_ptr<Framebuffer>        m_blurryFramebuffer;

        /** Captures the gamma encoded result for input to antialiasing.*/
        mutable shared_ptr<Framebuffer>        m_postGammaFramebuffer;

        /** Output of blend shader/input to the vertical blur. 16-bit float.*/
        mutable shared_ptr<Texture>            m_blended;

        /** float pre-bloom curve applied */
        mutable shared_ptr<Texture>            m_preBloom;

        /** float blurred vertical */
        mutable shared_ptr<Texture>            m_temp;

        /** float blurred vertical + horizontal */
        mutable shared_ptr<Texture>            m_blurry;

        mutable Spline<float>                  m_lastToneCurve;

        /** Maps [0, 2] to some output range */
        mutable shared_ptr<Framebuffer>        m_toneCurve;

        void maybeUpdateToneCurve(const FilmSettings& settings) const;
    public:
        CompositeFilter();
        virtual void apply(RenderDevice* rd, const FilmSettings& settings, const shared_ptr<Texture>& source, const shared_ptr<Framebuffer>& target, int sourceTrimBandThickness, int sourceColorBandThickness) const override;
    } m_compositeFilter;

    class FXAAFilter : public Filter {
    public:
        virtual void apply(RenderDevice* rd, const FilmSettings& settings, const shared_ptr<Texture>& source, const shared_ptr<Framebuffer>& target, int sourceTrimBandThickness, int sourceColorBandThickness) const override;
    } m_fxaaFilter;

    class WideAAFilter : public Filter {
    public:
        virtual void apply(RenderDevice* rd, const FilmSettings& settings, const shared_ptr<Texture>& source, const shared_ptr<Framebuffer>& target, int sourceTrimBandThickness, int sourceColorBandThickness) const override;
    } m_wideAAFilter;

    class DebugZoomFilter : public Filter {
    public:
        virtual void apply(RenderDevice* rd, const FilmSettings& settings, const shared_ptr<Texture>& source, const shared_ptr<Framebuffer>& target, int sourceTrimBandThickness, int sourceColorBandThickness) const override;
    } m_debugZoomFilter;

    class EffectsDisabledBlitFilter : public Filter {
    public:
        virtual void apply(RenderDevice* rd, const FilmSettings& settings, const shared_ptr<Texture>& source, const shared_ptr<Framebuffer>& target, int sourceTrimBandThickness, int sourceColorBandThickness) const override;
    } m_effectsDisabledBlitFilter;
        
    Film();
    
public:
    
    /** \brief Create a new Film instance. */
    static shared_ptr<Film> create();

    /** \brief Renders the input as filtered by the film settings to the currently bound framebuffer.
        \param downsample One side of the downsampling filter in pixels. 1 = no downsampling. 2 = 2x2 downsampling (antialiasing). Not implemented.

        If rendering to a bound texture, set the Texture::Visualization::documentGamma = gamma() afterwards.
        \param offset In pixels of the final framebuffer

        \image html guardBand.png
    */
    void exposeAndRender
        (RenderDevice*              rd, 
         const FilmSettings&        settings,
         const shared_ptr<Texture>& input,
         int                        sourceTrimBandThickness, 
         int                        sourceColorBandThickness);

    /**
      Render to texture helper.  You can also render to a texture by binding \a output to a FrameBuffer, 
      setting the FrameBuffer on the RenderDevice, and calling the 
      three-argument version of exposeAndRender.  That process will be faster than this
      version, which must create its FrameBuffer every time it is invoked.
      The width and height of output must be a multiple of 4.

     \param output If NULL, this will be allocated to be the same size and format as \a input.
     */
    void exposeAndRender
       (RenderDevice*               rd,
        const FilmSettings&         settings,
        const shared_ptr<Texture>&  input,
        int                         sourceTrimBandThickness, 
        int                         sourceColorBandThickness,
        shared_ptr<Texture>&        output,
        CubeFace                    outputCubeFace = CubeFace::POS_X,
        int                         outputMipLevel = 0);
};

} // namespace
#endif
