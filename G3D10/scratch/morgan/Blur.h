

class Blur {
private:
    // Intentionally unimplemented
    Blur();

public:

    class Kernel {
    public:

        /** GAUSSIAN uses \f$ \sigma = \f$ pixelRadius / 2.5, to capture about 99.6% of the non-zero coefficients. */
        DECLARE_ENUM_CLASS(Shape, GAUSSIAN, BOX);

        Shape  shape;
        bool   bilateralNormalKey;
        bool   bilateralDepthKey;
        bool   bilateralPositionKey;

        /** If true, search outwards from the center tap and stop considering coefficients when any one yields a sample weight less than ??? */
        bool   bilateralEdgeEnforcement;

        // TODO: some bilateral coefficients (maybe eliminate the booleans above if that can be expressed by weights = 0 here)

        /** If fractional, an integer number of taps are obviously still used but the taps are weighted to produce the correct fractional-radius result. */
        float  pixelRadius;

        static Kernel gaussian(???);
        static Kernel box(???);
    };


    /**   
     Performs a 2D blur as two passes.

     \param channels Number of channels to blur (other channels are copied directly).  If 0, uses to source->channels().
     \param intermediateFormat If ImageFormat::AUTO, uses source->format
     \param intermediateBuffer If specified, color buffer zero of this framebuffer is used for the intermediate result.  It will be resized as needed.  In this case, intermediateFormat is ignored.

     Uses the current blending mode, colorWrite, alphaTest, and alphaWrite mode when outputting to the current framebuffer.
     Uses BLEND_ONE, BLEND_ZERO when writing to the intermediate buffer, and enables alphaWrite only if channels = 4

     Automatically scales up or down to the output size.
    */
    static void apply
       (const shared_ptr<Texture>& source,
        const Vector2int32 sourceGuardBand,
        const Vector2int32 destinationGuardBand,
        const Kernel& kernel,
        int channels = 0,
        const ImageFormat* intermediateFormat = ImageFormat::AUTO(), 
        const shared_ptr<Framebuffer>& intermediateBuffer = shared_ptr<Framebuffer>(),
        const shared_ptr<ScaledBiasedTexture>& depth = shared_ptr<ScaledBiasedTexture>(),
        const shared_ptr<ScaledBiasedTexture>& normal = shared_ptr<ScaledBiasedTexture>(),
        const shared_ptr<ScaledBiasedTexture>& position = shared_ptr<ScaledBiasedTexture>(),
        );
};
