/**
  \file GLG3D/GBuffer.h
  \author Morgan McGuire, http://graphics.cs.williams.edu
 */
#ifndef GLG3D_GBuffer_h
#define GLG3D_GBuffer_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "G3D/ImageFormat.h"
#include "G3D/enumclass.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/Texture.h"
#include "GLG3D/UniversalMaterial.h"
#include "GLG3D/Args.h"
#include "G3D/Access.h"
#include "G3D/Table.h"

namespace G3D {

class RenderDevice;
class Args;
class Camera;

/** Encoding of the depth buffer (not the GBuffer::Field::CS_Z buffer) */
class DepthEncoding {
public:

    enum Value {
        /** "Z-buffer" Traditional (n)/(f-n) * (1 - f/z) encoding.  Easy to
            produce from a projection matrix, few good numerical
            properties.*/
        HYPERBOLIC,

        /** "W-buffer" (z-n)/(f-n), provides uniform precision for fixed
            point formats like DEPTH24, and easy to reconstruct
            csZ directly from the depth buffer. Poor precision
            under floating-point formats.

            Accomplished using a custom vertex shader at the expense of complicated
            interpolation in the fragment shader and losing hierarchical and early-z culling;
            not possible with a projection matrix.

            This is provided for future compatibility with potential OpenGL extensions
            and for software renderers. There is no way to make G3D render a LINEAR
            depth buffer currently.
        */
        LINEAR,
            
        /** (n)/(f-n) * (f/z-1) encoding, good for floating-point
            formats like DEPTH32F.
             
            Accomplished using glDepthRange(1.0f, 0.0f) with a traditionl
            projection matrix. 

            \cite http://portal.acm.org/citation.cfm?id=311579&dl=ACM&coll=DL&CFID=28183902&CFTOKEN=63987370*/
        COMPLEMENTARY
    } value;

    static const char* toString(int i, Value& v);

    G3D_DECLARE_ENUM_CLASS_METHODS(DepthEncoding);

}; // class DepthEncoding

} // namespace G3D
G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::DepthEncoding);
namespace G3D {

/** \brief Saito and Takahashi's Geometry Buffers, typically 
    used today for deferred and forward+ shading. 

    Optionally contains position, normal, depth, velocity, and BSDF parameters
    as well as depth and stencil.
    
    Example:
    \code
    GBuffer::Specification specification;
    specification.format[GBuffer::Field::WS_NORMAL]   = ImageFormat::RGB8();
    specification.encoding[GBuffer::Field::WS_NORMAL].readMultiplyFirst =  2.0f;
    specification.encoding[GBuffer::Field::WS_NORMAL].readAddSecond     = -1.0f;
    specification.format[GBuffer::Field::WS_POSITION] = ImageFormat::RGB16F();
    specification.format[GBuffer::Field::LAMBERTIAN]  = ImageFormat::RGB8();
    specification.format[GBuffer::Field::GLOSSY]      = ImageFormat::RGBA8();
    specification.format[GBuffer::Field::DEPTH_AND_STENCIL] = ImageFormat::DEPTH32();
    specification.depthEncoding = GBuffer::DepthEncoding::HYPERBOLIC;
    
    gbuffer = GBuffer::create(specification);
    gbuffer->setSize(w, h);

    ...

    gbuffer->prepare(rd, m_debugCamera, 0, -1.0f / desiredFrameRate());
    Surface::renderIntoGBuffer(rd, surface3D, gbuffer);
    \endcode

    \sa Surface, Texture, Framebuffer
*/
class GBuffer : public ReferenceCountedObject {
public:

    /**
     \brief Names of fields that may be present in a GBuffer.

     These use the abbreviations CS = camera space, WS = world space, SS = screen space, TS = tangent space.

     Normals are always encoded as n' = (n+1)/2, even if they are in
     floating point format, to simplify the implementation of routines
     that read and write GBuffers.
     */
    class Field {
    public:

        enum Value {

            /** Shading normal, after interpolation and bump-mapping.*/
            WS_NORMAL,

            /** \copydoc WS_NORMAL */
            CS_NORMAL,

            /** Geometric normal of the face, independent of the
                vertex normals. */
            WS_FACE_NORMAL,

            /** \copydoc WS_FACE_NORMAL */
            CS_FACE_NORMAL,

            /** Must be a floating-point format */
            WS_POSITION,

            /** Must be a floating-point format */
            CS_POSITION,

            /** Must be a floating-point or normalized fixed-point
                format. \sa UniversalBSDF, UniversalMaterial.*/
            LAMBERTIAN,

            /** RGB = magnitude; A = exponent. Fresnel has not been
                applied. Must be a floating-point or normalized
                fixed-point format. \sa UniversalBSDF, UniversalMaterial. */
            GLOSSY,

            /** Must be a RGBA floating-point or normalized
                fixed-point format. Index of refraction is in the A
                channel. \sa UniversalBSDF, UniversalMaterial. */
            TRANSMISSIVE,

            /** Must be a floating-point or normalized fixed-point
                format. \sa UniversalBSDF, UniversalMaterial. */
            EMISSIVE,

            /** World-space position change since the previous frame,
                according to a Surface.  Must be RGB floating-point.
            
                The name "velocity" is reserved for future use as
                instantaneous velocity. 

                There is no "WS_POSITION_CHANGE" because there is no
                application (for a screen-space buffer of position
                changes that don't take the camera's own movement into
                account) to justify the added implementation
                complexity required for that.
            */
            CS_POSITION_CHANGE,

            /** Texture storing the screen-space pixel displacement
                since the previous frame. As a result, floating-point
                textures will store the sub-pixel displacement and
                signed integers (e.g., ImageFormat::RG8UI) will round
                to the nearest pixel.
            */
            SS_POSITION_CHANGE,

            /** Texture storing exposure-interval weighted velocity,
                which may be specified to discount a fraction of the camera's
                velocity.  This is the input to G3D::MotionBlur.
            */
            SS_EXPRESSIVE_MOTION,

            /** Camera-space Z.  Must be a floating-point, R-only texture. This is always a negative value
            if a perspective transformation has been applied. */
            CS_Z,

            /** The depth buffer, used for depth write and test.  Not camera-space Z.
            This may include stencil bits. */
            DEPTH_AND_STENCIL,

            /** Shading normal, after interpolation and bump-mapping, in tangent-space.
                Note that tangent space is based on interpolated vertex normals. If no bump mapping is applied
                the shading normal will always be (0, 0, 1). */
            TS_NORMAL,

            /** Position within the normalized [0, 1] oct tree cube for SVO. */
            SVO_POSITION,

            /** Application-specific flags (e.g., "anisotropic", "water shader", "ray trace this pixel". 
                These may actually use individual bits, or be
                four values that can be 0, 1, or interpolated between them.
                \sa UniversalMaterial::Specification */
            FLAGS,
        
			/** Covariance matrix for voxel based filtering. */
            SVO_COVARIANCE_MAT1,
			SVO_COVARIANCE_MAT2,

            /** Not a valid enumeration value*/
            COUNT
        } value;
  
        static const char* toString(int i, Value& v);

        bool isUnitVector() const {
            return (value == WS_NORMAL) || (value == CS_NORMAL) || (value == TS_NORMAL);
        }

        G3D_DECLARE_ENUM_CLASS_METHODS(Field);
    }; // class Field


    class Specification {
    public:
        virtual ~Specification() {}

        /** Indexed by Field%s.*/
        Texture::Encoding       encoding[Field::COUNT];

        /** Reserved for future use--not currently supported */
        DepthEncoding           depthEncoding;

        /** Number of layers in each texture. Default is 1 */
        int                     depth;
        
		Texture::Dimension		dimension;

		bool					genMipMaps = false;

		/** Number of MSAA samples */
		int						numSamples;

        /** All fields for specific buffers default to NULL.  In the
            future, more buffers may be added, which will also default
            to NULL for backwards compatibility. */
        Specification();

        size_t hashCode() const {
			size_t h = depthEncoding.hashCode() + depth;
            for (int i = 0; i < Field::COUNT; ++i) {
                h += encoding[i].hashCode();
            }
			return h + int(genMipMaps) + int(dimension);
        }

		Specification& operator=(const Specification& other) {
			for (int i = 0; i < Field::COUNT; ++i) {
                encoding[i] = other.encoding[i];					 
			}
			depthEncoding = other.depthEncoding;
			depth     = other.depth;
			dimension     = other.dimension;
			genMipMaps    = other.genMipMaps;
            return *this;
		}

		/** Are these two specifications exactly the same */
		bool operator==(const Specification& other) const {
			for (int i = 0; i < Field::COUNT; ++i) {
				if (encoding[i] != other.encoding[i]) { return false; }					 
			}
			return (depthEncoding == other.depthEncoding) && (depth == other.depth) && (genMipMaps == other.genMipMaps) && (dimension == other.dimension);
		}
        

		bool operator!=(const Specification& other) const {
			return !(*this == other);
		}

		/** Memory Size in Bytes. */
		size_t memorySize() const {
			size_t res = 0;

			for (int i = 0; i < Field::COUNT; ++i) {
				if ( encoding[i].format != NULL){
					res += encoding[i].format->openGLBitsPerPixel / 8;
				}
			}
			return res;
		}

        /** Can be used with G3D::Table as an Equals and Hash function for a GBuffer::Specification.

          For example, 
        \code
        typedef Table<GBuffer::Specification, shared_ptr<Shader>, GBuffer::Specification::SameFields, GBuffer::Specification::SameFields> ShaderCache;
        \endcode

        Does not compare encoding or depthEncoding.
        */
        class SameFields {
        public:
            static size_t hashCode(const Specification& s) {
                size_t h = 0;
                for (int f = 0; f < Field::COUNT; ++f) {
                    h = (h << 1) | (s.encoding[f].format != NULL);
                }
                return h;
            }

            static bool equals(const Specification& a, const Specification& b) {
                return hashCode(a) == hashCode(b);
            }
        };

    };

protected:

    /** Used for debugging and visualization */
    String                      m_name;

    Specification               m_specification;
        
    shared_ptr<Camera>          m_camera;

    float                       m_timeOffset;

    float                       m_velocityStartTimeOffset;

    /** The other buffers are permanently bound to this framebuffer */
    shared_ptr<Framebuffer>     m_framebuffer;

    Framebuffer::AttachmentPoint m_fieldToAttachmentPoint[Field::COUNT];

    String                      m_readDeclarationString;
    String                      m_writeDeclarationString;

	Table<String, String>		m_readShaderStringCache;
	Table<String, String>		m_writeShaderStringCache;
	Table<String, String>		m_readwriteShaderStringCache;

    /** True when the textures have been allocated */
    bool                        m_allTexturesAllocated;

    bool                        m_depthOnly;

    bool                        m_hasFaceNormals;

    Vector2int16                m_depthGuardBandThickness;

    Vector2int16                m_colorGuardBandThickness;

	Vector3int16				m_resolution;

    bool                        m_useImageStore;

	/** Settings used to connect the GBuffer to a Shader */
	Sampler						m_textureSettings;
    GBuffer(const String& name, const Specification& specification);

    // Intentionally unimplemented
    GBuffer(const GBuffer&);

    // Intentionally unimplemented
    GBuffer& operator=(const GBuffer&);

    void setSpecification(const Specification& s, bool forceAllFieldsToUpdate);

    /** \deprecated Used by SVO */
	String& getShaderString(const String &gbufferName, Args& args, Access access, bool &needCreation);

public:

    /** \brief Returns true if GBuffer is supported on this GPU */
    static bool supported();

    static shared_ptr<GBuffer> create
    (const Specification&       specification,
     const String&              name = "G3D::GBuffer");

    int width() const;
    int height() const;

    int depth() const;

    /** The full bounds, including the depth guard band */
    Rect2D rect2DBounds() const;

    /** The actual framebuffer bounds out to the edge of the depth guard band
        \sa colorRect, finalRect */
    Rect2D rect() const {
        return rect2DBounds();
    }

    /** The region within the color guard band */
    Rect2D colorRect() const {
        return Rect2D::xyxy(Vector2(m_depthGuardBandThickness - m_colorGuardBandThickness), framebuffer()->vector2Bounds() - Vector2(m_depthGuardBandThickness - m_colorGuardBandThickness));
    }

    /** The region that will affect the final image, i.e., with the depth and color band stripped off. */
    Rect2D finalRect() const {
        return Rect2D::xyxy(Vector2(m_depthGuardBandThickness), framebuffer()->vector2Bounds() - Vector2(m_depthGuardBandThickness));
    }

    /**
    \brief A series of macros to prepend before a Surface's shader
    for rendering <b>to</b> GBuffers.  

    Note that the framebuffer() automatically binds all write declarations to any 
    shader invoked on it.
    
    This defines each of the fields
    in use and maps it to a GLSL output variable.  For example,
    it might contain:

    \code
    #define WS_NORMAL   gl_FragData[0]
    uniform vec2 WS_NORMAL_writeScaleBias;

    #define LAMBERTIAN  gl_FragData[1]
    uniform vec2 LAMBERTIAN_writeScaleBias;

    #define DEPTH       gl_FragDepth
    \endcode
    */
    const String writeDeclarations() const {
        return m_writeDeclarationString;
    }

   /**
    Example:

    \code
    #define WS_NORMAL  
    uniform vec2 WS_NORMAL_readScaleBias;

    #define LAMBERTIAN
    uniform vec2 LAMBERTIAN_readScaleBias;

    #define DEPTH
    \endcode

    This helper is not always desirable--you may wish to manually bind the scaleBias values
    */
    const String readDeclarations() const {
        return m_readDeclarationString;
    }

    /**  Binds the writeScaleBias values defined in writeDeclarations() in the form 
         [prefix]FIELD_writeScaleBias = vec2(writeMultiplyFirst, writeAddSecond)
         
         Note that the framebuffer() automatically binds all write uniform arguments.
        */
    virtual void setShaderArgsWrite(UniformTable& args, const String& prefix = "") const;

    /** Binds the readScaleBias values defined in readDeclarations(). in the form
        [prefix]FIELD_...
        
        
        Inside a shader, these arguments can be automatically defined using the macro:
        
        \code
        #include <GBuffer/GBuffer.glsl> 
        uniform_GBuffer(name)
        \endcode

        \sa uniform_GBuffer, GBuffer::setShaderArgsWrite, Texture::SetShaderArgs
        */
    virtual void setShaderArgsRead(UniformTable& args, const String& prefix = "") const;


	String getImageString(const Specification &spec, const ImageFormat* format){
		Texture::Dimension dim=spec.dimension;

		String res;

		if(spec.numSamples==1){
			switch(dim){
			case Texture::DIM_2D:
				res = String("image2D");
				break;
			case Texture::DIM_3D:
				res =  String("image3D");
				break;
			case Texture::DIM_2D_RECT:
				res =  String("image2DRect");
				break; 
			case Texture::DIM_CUBE_MAP:
				res =  String("imageCube");
				break;
			default:
				alwaysAssertM(false, "Unrecognised dimension");
			}
		}else{
			switch(dim){
			case Texture::DIM_2D:
				res =  String("image2DMS");
				break;
			default:
				alwaysAssertM(false, "Unrecognised dimension");
        
			}
		}

		if(format->isIntegerFormat())
			res =  String("i")+res;
		
		return res;
	}


	String getSamplerStringFromTexDimension(const Specification& spec){
            Texture::Dimension dim = spec.dimension;
            if (spec.numSamples == 1) {
                switch(dim){
                case Texture::DIM_2D:
                    return "sampler2D";
                
                case Texture::DIM_3D:
                    return "sampler3D";

                case Texture::DIM_2D_RECT:
                    return "sampler2DRect";

                case Texture::DIM_CUBE_MAP:
                    return "samplerCube";

                default:
                    alwaysAssertM(false, "Unrecognised dimension");
                    return "unknown";
                }
            } else {
                switch(dim) {
                case Texture::DIM_2D:
                    return "sampler2DMS";

                default:
                    alwaysAssertM(false, "Unrecognised dimension");
                    return "unknown";
                }
            }
	}

	String getSwizzleComponents(int numComponents){
		switch(numComponents){
		case 1:
			return String("x");
		case 2:
			return String("xy");
		case 3:
			return String("xyz");
		default:
			return String("xyzw");
		}
	}

	int getTexDimensionInt(Texture::Dimension dim){
		switch(dim){
		case Texture::DIM_2D:
			return 2;
		case Texture::DIM_3D:
			return 3;
		case Texture::DIM_2D_RECT:
			return 2;
		case Texture::DIM_CUBE_MAP:
			return 2;
		default:
			alwaysAssertM(false, "Unrecognised dimension");
			return 0;
		}
	}

	//Are the string manipulation efficient enough ?
	void connectToShader(String gbufferName, Args& args, Access access, const Sampler&	textureSettings, int mipLevel=0);


    /** Returns the attachment point on framebuffer() for \a field.*/
    Framebuffer::AttachmentPoint attachmentPoint(Field field) const {
        return m_fieldToAttachmentPoint[field.value];
    }


    /** sets the clear value of the given field */
    void setColorClearValue(Field field, const Color4& c) {
        Framebuffer::AttachmentPoint x = attachmentPoint(field);
        framebuffer()->setClearValue(x, c);
    }


    /** get the clear value of the given field */
    const Color4 getClearValue(Field field) const {
        return framebuffer()->getClearValue(attachmentPoint(field));
    }


    const Specification& specification() const {
        return m_specification;
    }


    /** 
      Changes the specification. If nothing changed from the previous 
      specification, then no allocation or pointer changes will occur.
      
      Otherwise, the GBuffer *may* optimize texture allocation, however
      the current implementation does not guarantee this.
     */
    void setSpecification(const Specification& s);

    /** The other buffers are permanently bound to this framebuffer */
    const shared_ptr<Framebuffer>& framebuffer() const {
        return m_framebuffer;
    }

    /** \copydoc framebuffer()const */
    shared_ptr<Framebuffer> framebuffer() {
        return m_framebuffer;
    }

    /** The camera from which these buffers were rendered. */
    const shared_ptr<Camera> camera() const {
        return m_camera;
    }

    /** For debugging in programs with many GBuffers. */
    const String& name() const {
        return m_name;
    }

    /** Reallocate all buffers to this size if they are not already. */
    virtual void resize(int width, int height, int depth=1);


    /** Explicitly override the camera stored in the GBuffer. */
    void setCamera(const shared_ptr<Camera>& camera) {
        m_camera = camera;
    }

    /** \sa renderIntoGBufferHomogeneous */
    void setTimeOffsets(float timeOffset, float velocityStartTimeOffset) {
        m_timeOffset = timeOffset;
        m_velocityStartTimeOffset = velocityStartTimeOffset;
    }

    /** \sa renderIntoGBufferHomogeneous */
    float timeOffset() const {
        return m_timeOffset;
    }

    /** \sa renderIntoGBufferHomogeneous */
    float velocityStartTimeOffset() const {
        return m_velocityStartTimeOffset;
    }

    /** True iff this G-buffer renders only depth and stencil. */
    bool isDepthAndStencilOnly() const {
        return m_depthOnly;
    }

    /** True if this contains non-NULL Field::CS_FACE_NORMAL or Field::WS_FACE_NORMAL in the specificationl. */
    bool hasFaceNormals() const {
        return m_hasFaceNormals;
    }

    /** Returns the Texture bound to \a f, or NULL if there is not one */
    shared_ptr<Texture> texture(Field f) const;

    /** Bind the framebuffer and clear it, then set the camera and time offsets */
    void prepare(RenderDevice* rd, const shared_ptr<Camera>& camera, float timeOffset, float velocityStartTimeOffset, Vector2int16 depthGuardBandThickness, Vector2int16 colorGuardBandThickness);

	/** Bind the framebuffer and clear it, then set the time offsets 
		No camera version.
    */
	void prepare(RenderDevice* rd, float timeOffset, float velocityStartTimeOffset, Vector2int16 depthGuardBandThickness, Vector2int16 colorGuardBandThickness);

    /** \sa GApp::Settings::depthGuardBandThickness
        \image html guardBand.png */
    Vector2int16 depthGuardBandThickness() const {
        return m_depthGuardBandThickness;
    }

    /** \sa GApp::Settings::colorGuardBandThickness 
        \image html guardBand.png */
    Vector2int16 colorGuardBandThickness() const {
        return m_colorGuardBandThickness;
    }

	/** Enable/disable the image store api */
	void setImageStore(bool state){
		m_useImageStore=state;
	}

    /**
      \brief Binds a single argument as [prefix] + fieldName + "_buffer" and fieldName + "_readScaleBias".

      Used by setShaderArgsRead(), and available for writing your own shaders that use GBuffer encodings but may not 
      directly use a GBuffer or all of its fields. Note that you can
      simply use setShaderArgsRead() to bind all GBuffer fields; G3D::Shader will ignore the ones that are not used by the actual
      shader.

      \param texture If NULL, no binding is made.  Use Texture::blackIfNull and Texture::whiteIfNull to avoid this behavior.
      \param encoding Used for non-NULL, non- Field::DEPTH_AND_STENCIL input
    */
    static void bindReadArgs(UniformTable& args, Field field, const shared_ptr<Texture>& texture, const String& prefix = "");

    /** Note that the framebuffer() automatically binds all write uniform arguments */
    static void bindWriteUniform(UniformTable& args, Field f, const Texture::Encoding& encoding, const String& prefix = "");
};

} // namespace G3D

G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::GBuffer::Field);

#endif
