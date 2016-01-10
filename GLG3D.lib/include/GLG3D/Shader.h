/**
 \file GLG3D/Shader.h
  
 \maintainer Morgan McGuire http://graphics.cs.williams.edu, Michael Mara http://www.illuminationcodified.com/
 
 \created 2012-06-13
 \edited  2014-03-25
 */

#ifndef GLG3D_Shader_h
#define GLG3D_Shader_h

#include "G3D/platform.h"
#include "G3D/G3DString.h"
#include "GLG3D/glheaders.h"
#include "G3D/Matrix2.h"
#include "G3D/Matrix3.h"
#include "G3D/Matrix4.h"
#include "G3D/Vector4.h"
#include "G3D/Vector3.h"
#include "G3D/Vector2.h"
#include "G3D/CoordinateFrame.h"
#include "G3D/Color4.h"
#include "G3D/Color3.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/AttributeArray.h"
#include "GLG3D/Texture.h"
#include "GLG3D/Args.h"
#include "G3D/SmallArray.h"
#include "G3D/constants.h"
#include "GLG3D/Profiler.h"


namespace G3D {

/**
  \brief Abstraction of the programmable hardware pipeline with G3D
  preprocessor extensions.

  Shaders are almost always invoked with the LAUNCH_SHADER macro, with
  arguments provided by the G3D::Args class. Many classes (including 
  Framebuffer, ArticulatedModel::Pose, and LightingEnvironment) provide
  a UniformTable field that allows them to impose additional values on the
  Args used for shaders that incorporate them.

  Shader supports macro arguments in addition to uniform and streaming
  (attribute, index) arguments.  It caches all needed permutations of
  a shader that uses macros and then provides the correct instance at
  invokation time.

  <h2>Preprocessor</h2>
   %G3D introduces additional preprocessor commands GLSL: 

   <ul>
   <li> \#version anywhere in the program, with multiple fallbacks
   <li> \#include, relative to the current file or system directory
   <li> \#expect
   <li> \#for
   <li> \#foreach
   </ul>

   <h3>\#version</h3> The preprocessor automatically moves GLSL
   \#version directives to the first line of a program before compilation.  

   The extended syntax "\#version v1 or v2 or ..." allows writing shaders
   that simultaneously target multiple GLSL versions.
   Shader will compie for the highest listed version number that the driver provides at runtime. 
   Within the shader code, you can test against the __VERSION__ macro to discover
   which version version is in use.

   <h3>\#expect</h3>
   Shader expands

   \code
   #expect MACRO_NAME "description"
   \endcode

   to

   \code
   #ifndef MACRO_NAME
   #  error description
   #endif
   \endcode

   <h3>\#for</h3>

     \code
        #for (int i = 0; i < macroArgOrIntegerLiteral; ++i)
            // ...use $(i) to refer to the index inside of the loop body...
        #endfor
     \endcode

     The upper bound cannot be a value set via \#define...if it is a macro,
     then the macro must have been passed from C++ as an argument.
    
     Example:

      \code
        #for (int j = 0; j < NUM_LIGHTS; ++j)
            uniform vec3 lightPos$(j);
        #endfor
      \endcode

   with <code>NUM_LIGHTS</code> a macro arg with value "4", this code expands to:

      \code
            uniform vec3 lightPos0;
            uniform vec3 lightPos1;
            uniform vec3 lightPos2;
            uniform vec3 lightPos3;
       \endcode

   The expression inside the $(...) can also perform a single arithmetic expression 
   of the form $(<identifier> <operator> <number>), where operator
   may be +, -, /, or *. If the number is an integer
   and the operator is /, then integer division is performed. Otherwise, the operation
   is performed at double precision.

   The preprocessor inserts \#line pragmas before each unrolled loop body,
   so errors will report correct line numbers.

   <h3>\#foreach</h3>
   \code    
   #foreach field in (value0), (value1), (value2)
       // ...loop body...
   #endforeach
   \endcode

   The inner code then gets repeated for each value of field, and any
   occurence of <code>$(field)</code> in the inner code is replaced with the
   current value of field.  Note that field can be replace with any
   string, and value* can be regular symbols or quoted strings.

        \code
        #foreach v, w in (v0, w0), (v1, w1), (v2, w2)
            // loop body
        #endforeach
        \endcode


   Example:
        \code
         #foreach (name, componentCount) in (lambertian, 4), (glossy, 4), (emissive, 3)
            uniform vec$(componentCount)    $(name)_constant;
            uniform sampler2D               $(name)_buffer;
         #endforeach
        \endcode

    becomes:

     \code
     uniform vec4        lambertian_constant;
     uniform sampler2D   lambertian_buffer;
     uniform vec4        glossy_constant;
     uniform sampler2D   glossy_buffer;
     uniform vec3        emissive_constant;
     uniform sampler2D   emissive_buffer;
     \endcode

     <h2>%G3D Built-in Uniforms and Macros</h2>

     \code
        G3D_SHADER_STAGE will be equal to one of:d G3D_VERTEX_SHADER, G3D_TESS_CONTROL_SHADER, G3D_TESS_EVALUATION_SHADER, G3D_GEOMETRY_SHADER, G3D_FRAGMENT_SHADER, G3D_COMPUTE_SHADER (which are integers equal to their OpenGL counterparts)
     
        One of the following is defined: G3D_OSX, G3D_WINDOWS, G3D_LINUX, G3D_FREEBSD

        One of the following may be defined: G3D_NVIDIA, G3D_MESA, G3D_AMD

        // RenderDevice properties
        uniform mat4x3 g3d_WorldToObjectMatrix;
        uniform mat4x3 g3d_ObjectToWorldMatrix;
        uniform mat4x4 g3d_ProjectionMatrix;
        uniform mat4x4 g3d_ProjectToPixelMatrix;
        uniform mat3   g3d_WorldToObjectNormalMatrix;
        uniform mat3   g3d_ObjectToWorldNormalMatrix;
        uniform mat3   g3d_ObjectToCameraNormalMatrix;
        uniform mat3   g3d_CameraToObjectNormalMatrix;
        uniform mat4   g3d_ObjectToScreenMatrix; // includes invertY information
        uniform mat4   g3d_ObjectToScreenMatrixTranspose;
        uniform mat4x3 g3d_WorldToCameraMatrix;
        uniform mat4x3 g3d_CameraToWorldMatrix;
        uniform bool   g3d_InvertY;
        uniform mat3   g3d_WorldToCameraNormalMatrix;
        uniform float  g3d_SceneTime; // GApp::lastGApp->scene()->time() if valid, otherwise time since first binding of this variable

        // Rect2D bounds from the draw call
        uniform vec2   g3d_FragCoordExtent; // Only if Args::setRect was invoked
        uniform vec2   g3d_FragCoordMin;   // Only if Args::setRect was invoked
        uniform vec2   g3d_FragCoordMax;   // Only if Args::setRect was invoked

        uniform int    g3d_NumInstances;
        \endcode

  <h2>Implementation</h2> G3D has two distinct preprocessing steps for
    shaders: load-time (e.g. include) and compile-time (for, foreach).

    The load-time preprocessor is documented in Shader::g3dLoadTimePreprocessor().

    The compile-time preprocessor currently only prepends
    Args::preambleAndMacroString() and evaluates the \#for and
    \#foreach commands ( Shader::expandForPragmas() )
*/ 
class Shader : public ReferenceCountedObject {
public:
    friend class RenderDevice;
    
    enum ShaderStage {
        VERTEX, 
        
        /** In DirectX, this is called the Hull Shader */
        TESSELLATION_CONTROL, 
        
        /** In DirectX, this is called the Domain Shader */
        TESSELLATION_EVAL, 
        
        GEOMETRY, 
        
        /** Sometimes referred to as the Fragment Shader. */
        PIXEL,

        COMPUTE,

        STAGE_COUNT
    };

    static GLuint toGLEnum(ShaderStage s);

    enum RecoverableErrorType {LOAD_ERROR, COMPILATION_ERROR};

    enum SourceType {FILE, STRING};

    enum DomainType {
        STANDARD_INDEXED_RENDERING_MODE, 
        STANDARD_NONINDEXED_RENDERING_MODE, 
        MULTIDRAW_INDEXED_RENDERING_MODE,
        MULTIDRAW_NONINDEXED_RENDERING_MODE,
        INDIRECT_RENDERING_MODE,
        STANDARD_COMPUTE_MODE, 
        INDIRECT_COMPUTE_MODE, 
        RECT_MODE,
        ERROR_MODE
    };

    /** The mode to use when applying the shader and args */
    static DomainType domainType(const shared_ptr<Shader>& s, const Args& args);

    /** Stores either a filename or a shader source string (that has not gone through the g3d preprocessor) for a single shader stage */
    class Source {
        friend class Shader;
    protected:
        SourceType type;
        String val;

    public:

        Source();

        Source(SourceType t, const String& value);

        /** Throws an error at runtime if the source string appears to be a filename instead of GLSL code */
        Source(const String& sourceString);
    };

    /** Consists of up to one Shader::Source per shader stage */
    class Specification {
    friend class Shader;
    protected:
        Source shaderStage[STAGE_COUNT];
        void setStages(const Array<String>& filenames);

    public:
        Specification(const Any& any);
        Specification();

        /** Take in between 1 and 5 filenames, each uniquely corresponding to one of 
            the 5 stages of the shading pipeline.
            We parse each filename, and based on the extension, load it into the specification at the corresponding stage
            The valid extensions are as follows:
            ------------------------------------------
            Shader Stage            |   Extension(s)
            ------------------------------------------
            Vertex                  |   .vrt or .vtx
            ------------------------------------------
            Tesselation Control     |
            (Hull) Shader           |   .ctl or .hul
            ------------------------------------------
            Tesselation Evaluation  |
            (Domain) Shader         |   .evl or .dom
            ------------------------------------------
            Geometry Shader         |       .geo
            ------------------------------------------
            Pixel (Fragment) Shader |   .pix or .frg     
            ------------------------------------------
            Compute Shader          |   .glc or .glsl
            ------------------------------------------

            If any of the strings passed in is not the empty string or a filename with this extension, an error will be thrown.    
        */
        Specification(
            const String& f0, 
            const String& f1 = "", 
            const String& f2 = "", 
            const String& f3 = "", 
            const String& f4 = "");

        Specification(const Array<String>& filenames);

        Source& operator[](ShaderStage s);
        const Source& operator[](ShaderStage s) const;
    };
    
protected:

    /**
       True iff a and b have the same SouceType and value.
     */
    static bool sameSource(const Source& a, const Source& b);

    /**
       True iff every stage of a and b satisfy sameSource.
     */
    static bool sameSpec(const Specification& a, const Specification& b);

    /**
       If a shader of the given Specification exists, return it,
       otherwise create a new one.
     */
    static shared_ptr<Shader> getShaderFromCacheOrCreate(const Specification& spec);

    /** Converts a type name to a GL enum */
    static GLenum toGLType(const String& s);

    /** 
        A structure containing the individual parts of the code of a load-time preprocessed (by g3d, not the driver) shader. 
        This is combined with the macro argument and preamble string at compilation time
        in the following manner: versionString + g3dInsertString + expandForPragmas(preprocessedCode).
      */
    class PreprocessedShaderSource {
    public:

        /** The original code, with \#includes evaluated, and the
            version line (if existant) replaced with an empty
            line. This is <B>before</b> the actual driver preprocessing and
            \#for pragma expansion, so \#defines, \#ifs, and \#fors
            have not yet been evaluated */
        String preprocessedCode;

        /** 
            The filename of the file where the code was originally loaded from.
            Note that this does *not* somehow account for \#include pragmas. 
          */
        String filename;

        /** The \#defines and uniform args added by G3D. Does not include user-defined macro arguments or preamble */
        String g3dInsertString;

        /** 
            A line of the form "\#version XXX" where XXX is a three digit number denoting the GLSL 
            version for the graphics driver to use. Inserted at the beginning of the final code submitted to the driver.
          */
        String versionString;

		/**
			Multiple lines containing "\#extension ..." definitions.
			Inserted at the beginning of the final code submitted to the driver, right after the version line.
		*/
		String extensionsString;

    };


	/** A wrapper around an actual openGL shader program object. Contains the shader objects and
	 * declarations of variables found by querying the program object */
    class ShaderProgram : public ReferenceCountedObject {
        friend class Shader;
        public:

        /** Uniform variable declaration discovered in the program */
        class UniformDeclaration {
        public:
            /** OpenGL location returned by glGetUniformLocation, -1 if this uniform is inactive */
            GLint       location;
            
            String name;

            /** OpenGL type of the variable (e.g. GL_INT) */
            GLenum      type;

            /** The number of elements in this variable. For non-arrays, this is 1. For now, we store all array elements separately, so this is always 1. Left for future use. */
            GLuint      elementNum;
            
            /** The index of this element in it's array. For non-array elements, -1. */
            GLuint      index;

            /** The texture, image, or shader buffer unit this uniform is bound to */
            GLint       glUnit;

            /** If true, this uniform is not used by the shader program, so we never try to bind a value to it */
            bool        dummy;

            /** Shorthand to avoid having to set these fields one-by-one. Not in a constructor since UniformDeclaration 
                is a G3D::Table key in its main role and thus gets initialized by the default constructor. */
            void setAllFields(const String& name, int index, GLenum type, GLint location, bool dummy, GLint glUnit){
                this->dummy     = dummy;
                this->location  = location;
                this->name      = name;
                this->index     = index;
                this->type      = type;

                // TODO: Don't allocate texture units for unused variables
                this->glUnit    = glUnit;

                // Currently always 1
                this->elementNum = 1;
            }

            /** Sets the \a name, \a index, and \a type fields, and provides proper defaults for all other field for a dummy variable */
            void fillOutDummy(const String& name, int index, GLenum type){
                setAllFields(name, index, type, -1, true, -1);
            }
        };



        /** Vertex Attribute (stream) variable declaration discovered in the program. Array-valued attributes are untested */
        class AttributeDeclaration {
        public:
            /** OpenGL location returned by glGetAttribLocation, -1 if this uniform is inactive */
            GLint       location;

            /** OpenGL type of the variable (e.g. GL_INT) */
            GLenum      type;

            /** The number of elements in this variable. For non-arrays, this is 1. */
            GLint      elementNum;

            String name;

            /** If true, this attribute is not used by the shader program, so we never try to bind a value to it */
            bool dummy;
        };

        
        typedef Table<String, UniformDeclaration>   UniformDeclarationTable;

        typedef Table<String, AttributeDeclaration> AttributeDeclarationTable;
        
        /** The underlying OpenGL Shader Objects for all possible shader stages */
        GLuint                              glShaderObject[STAGE_COUNT];

        UniformDeclarationTable             uniformDeclarationTable;
        AttributeDeclarationTable           attributeDeclarationTable;

        /** The underlying OpenGL Program Object */
        GLuint                              glProgramObject;

        /** False if compilation and/or linking failed */
        bool                                ok;
        
        /** Warning and error messages generated during compilation and/or linking */
        String                         messages;

        ShaderProgram() {}

    public:
        
        /** \copydoc glProgramObject */
        GLuint glShaderProgramObject() {
            return glProgramObject;
        }

        /** False if and only if there was an error in compilation or linking */
        bool isOk() {
            return ok;
        }

        /** True if and only if the uniform declaration table contains a non-dummy entry \a name. */
        bool containsNonDummyUniform(const String& name);

        /** Called from the constructor */
        void addActiveUniformsFromProgram();

        /** Called from the constructor */
        void addActiveAttributesFromProgram();

        /** 
            Finds any vertex attribute variables in the code for the vertex shader that are not already
            in the attribute list that OpenGL returned and adds them to
            the attribute declaration table.  This causes ShaderProgram to surpress
            warnings about setting variables that have been compiled
            away--those warnings are annoying when temporarily commenting
            out code.   
          */
        void addVertexAttributesFromSource(const Array<PreprocessedShaderSource>& preprocessedSource);

        /** 
            Finds any uniform variables in the code for all shader stages that are not already
            in the uniform list that OpenGL returned and adds them to
            the uniform declaration table.  This causes ShaderProgram to surpress
            warnings about setting variables that have been compiled
            away--those warnings are annoying when temporarily commenting
            out code. 

            \param args used to evaluate pragmas in array bounds
          */
        void addUniformsFromSource(const Array<PreprocessedShaderSource>& preprocessedSource, const Args& args);

        /** 
            Finds any uniform variables in the code string, and adds them to the uniform table. 

            \param args used to evaluate pragmas in array bounds
            \sa addUniformsFromSource 
          */
        void addUniformsFromCode(const String& code, const Args& args);

        /** Compile using the current macro args, preamble, and source code. 
        
            \param code Code after the G3D preprocessor, preamble and macroArgs, for use in reporting link errors. */
        void compile(const Array<PreprocessedShaderSource>& preprocessedSource, const String& preambleAndMacroArgs, const Args& args, const Table<int, String>& indexToNameTable, Array<String>& code);

        /** Link the program object */
        void link();
        
        /** Compile and link the shader, and set up the formal parameter lists */
        void init(const Array<PreprocessedShaderSource>& preprocessedSource, const String& preambleAndMacroArgs, const Args& args, const Table<int, String>& indexToNameTable);

        /** Compile and link the shader, returning a reference to it. \param indexToNameTable is for converting file indices to filenames, for useful error/warning messages */
        static shared_ptr<ShaderProgram> create(const Array<PreprocessedShaderSource>& preprocessedSource, const String& preambleAndMacroArgs, const Args& args, const Table<int, String>& indexToNameTable);
    private:
        /** Parses through any qualifiers and gets the type of variable in the declaration in the current line of ti - a helper method for addVariableAttributesFromSource and AddUniformsFromSource
            uniform is changed to true if the variable in the declaration is a uniform*/
        GLenum getDeclarationType(TextInput& ti, bool& uniform);

        GLenum getDeclarationType(TextInput& ti);

        /** Helper Method for the above two methods */
        bool isQualifier(const String& s);

    };
    
    /** The source code for a shader from the STAGE_COUNT stages. */
    Array<PreprocessedShaderSource>             m_preprocessedSource;

    /** Maps preamble + macro definitions to compiled shaders */
    Table<String, shared_ptr<ShaderProgram> >   m_compilationCache;

    String                                      m_name;

    /** Contains the filenames or hardcoded source strings this shader
        was constructed from */
    Specification                               m_specification;

    /** 
        A set of args solely for g3d uniforms. We use this instead of
        modifying the args passed into compileAndBind().  This needs
        to be cleared every time we reload, or we will accidentally
        bind variables that don't exist in the compiled shader
      */
    Args                                        m_g3dUniformArgs;

    bool                                        m_isCompute;

    /** Maps indices to filenames, so that we can correctly output
        filenames when we get file indices back from the shader on
        error messages.
    */
    Table<int, String>                          m_indexToFilenameTable;     

    /** Maps filenames to indices, so that we can correctly print
        \#line directives */
    Table<String, int>                          m_fileNameToIndexTable;

    /** The next index to be used by a previously unprocessed file
        when \#including, so that we can produce proper \#line
        directives */
    int                                         m_nextUnusedFileIndex;
    
    /** Returns a line directive in the format "#line X Y\n", where X
        is the lineNumber, and Y is an integer that maps to
        filename */
    String getLinePragma(int lineNumber, const String& filename);

    /** Handle a compilation or loading error as specified by
        s_failureBehavior. If this handles a compilation error,
        \a program can be overwritten with a correctly compiled shader
        (if s_failureBehaviord is PROMPT and the user reloads). */
    void handleRecoverableError(RecoverableErrorType eType, const Args& args, const String& message, shared_ptr<ShaderProgram>& program);

    /** No compilation, or even loading from files is performed at construction */
    Shader(const Specification& s);

    /** Finds a shader program in the cache that matches the supplied
        args, recompiling if necessary.  if compilation fails, returns
        NULL and fills in messages with a report of what went wrong.
        Primarily intended for internal use by Shader; called from
        compileAndBind() and its variants.  */
    shared_ptr<Shader::ShaderProgram> shaderProgram(const Args& args, String& messages);

    /** 
        Expands our \#for pragmas found in \a processedSource into
        valid GLSL code. Nested \#for loops are processed from the
        outside in.

        errorMessages is appended to if any errors are encountered with details.

        The return value is true if no errors were encountered.

        This is a *load-time* preprocessing step.
    */
    static bool expandForPragmas(String& processedSource, const Args& args, const Table<int, String>& indexToNameTable, String& errorMessages);

    /** Expands %G3D \#foreach pragmas found in \a processedSource
        into valid GLSL code. Nested \#foreach loops are processed
        from the outside in.

        errorMessages is appended to if any errors are encountered with details.

        The return value is true if not errors were encountered.

        This is a *load-time* preprocessing step.
        \sa expandForPragmas
    */
    static bool expandForEachPragmas(String& processedSource, const Table<int, String>& indexToNameTable, String& errorMessages);

    /** Expands G3D \#expect preprocessor instructions.
        This is a *load-time* preprocessing step.
        \sa expandForEachPragmas
    */
    static bool expandExpectPragmas(String& source, const Table<int, String>& indexToNameTable, String& errorMessages);

    /** Array of all shaders ever created.  NULL elements are flushed during reloadAll() */
    static Array< weak_ptr<Shader> >           s_allShaders;

public:

    /** Determines the behavior upon a load-time or compile-time error for all shaders */
    enum FailureBehavior {
        /** Throw an exception on load or compilation failure */
        EXCEPTION, 
        
        /** Prompt the user to throw an exception, abort the program,
            or retry loading on load or compilation
            failure. (default) */
        PROMPT, 

        /** ok() will be false if an error occurs */
        SILENT
    };

    /** A name for this shader.  By default, this is the filename with
        a star instead of the extension, e.g., blur.* \sa setName */
    const String& name() const {
        return m_name;
    }

    void setName(const String& n) {
        m_name = n;
    }
    
    static FailureBehavior s_failureBehavior;

    /** True if \a type corresponding to an OpenGL sampler type. */
    static bool isSamplerType(GLenum type);

    /** True if \a type corresponding to an OpenGL image type. */
    static bool isImageType(GLenum type);

    /** Set the global failure behavior. See Shader::FailureBehavior */
    static void setFailureBehavior(FailureBehavior f);

    /** Creates a shader from the given specification, loads it from disk, and applies the g3d preprocessor */
    static shared_ptr<Shader> create(const Specification& s);

    /** \copydoc Shader::Source::Source() */
    static shared_ptr<Shader> fromFiles(
        const String& f0, 
        const String& f1 = "", 
        const String& f2 = "", 
        const String& f3 = "", 
        const String& f4 = "");

    /**
       Create a shader using all shaders specified by the given pattern.

       Used to implement \link LAUNCH_SHADER() LAUNCH_SHADER\endlink.

       \code
       //Example:
       shared_ptr<Shader> s = getShaderFromPattern("SVO_updateLevelIndexBuffer.*");
       rd->apply(s, args);
       \endcode
     */
    static shared_ptr<Shader> getShaderFromPattern(const String& pattern);
    
    /** 
        Sets the g3d uniform variables such as g3d_objectToWorldMatrix (using values from \a renderDevice) \a args. See Shader::g3dLoadTimePreprocessor for the declarations
        of all of the variables of this form that can be set by this function. Also binds them immediately, to avoid overhead of writing and reading into a datastructure

        Also sets all uniforms of the form g3d_sz2D_textureName, where textureName is the name of any sampler type bound in sourceArgs. g3d_sz2D_textureName is a vec4
        of the form (w, h, 1.0/w, 1.0/h), where w and h are the width and height of the texture named textureName.
      */
    void bindG3DArgs(const shared_ptr<ShaderProgram>& p, RenderDevice* renderDevice, const Args& sourceArgs, int& maxModifiedTextureUnit);

    /** Reload this shader from the files on disk, if it was loaded from disk. */
    void reload();

    /** Reload from files on disk (if it was loaded from disk), run all steps of the g3d preprocessor and recompile */
    shared_ptr<ShaderProgram> retry(const Args& args);

    /** Bind a single vertex attribute */
    void bindStreamArg(const String& name, const AttributeArray& vertexRange, const ShaderProgram::AttributeDeclaration& decl);

    /** Iterate over all formal parameters and bind all vertex attributes appropriately. Also sets the glPatchParameter if the primitive type is patch. */
    void bindStreamArgs(const shared_ptr<ShaderProgram>& program, const Args& args, RenderDevice* rd);

    /** Counterpart to bindStreamArgs. Unbinds all vertex attributes that were set. */
    void unbindStreamArgs(const shared_ptr<ShaderProgram>& program, const Args& args, RenderDevice* rd);

    /** Bind a single uniform variable 
    */
    void bindUniformArg(const Args::Arg& arg, const ShaderProgram::UniformDeclaration& decl, int& maxModifiedTextureUnit);

    /** 
        Iterate over all formal parameters and bind all non-dummy variables appropriately.

        Writes the maximum value of the texture unit that had a sampler bound to it to maxModifiedTextureUnit, 
        so the caller can revert after the shading pass. (-1 if no samplers were bound).
     */
    void bindUniformArgs(const shared_ptr<ShaderProgram>& program, const Args& args, bool allowG3DArgs, int& maxModifiedTextureUnit);

    /** 
        Compile the shader and bind the arguments as necessary. Adds the necessary g3d uniforms to args.
        Writes the maximum value of the texture unit that had a sampler bound to it to maxModifiedTextureUnit, 
        so the caller can revert after the shading pass. (-1 if no samplers were bound).
    */
    shared_ptr<ShaderProgram> compileAndBind(const Args& args, RenderDevice* rd, int& maxModifiedTextureUnit);

    /** Load the shaders from files on disk, and run the preprocessor */
    void load();

    bool isCompute() const {
        return m_isCompute;
    }

    /** Replaces all \#includes in @a code with the contents of the appropriate files.
        It is called recursively, so included files may have includes themselves.
        This is called automatically by the preprocessor, but is public so as to be
        accessible to code that directly manipulates source strings.

        \param dir The directory from which the parent was loaded.
		\param messages Any errors regarding inclusion of files are written here

		\return True if no error occured during inclusion of other files
      */
    bool processIncludes(const String& dir, String& code, String& messages);


    /** 
        Execute all steps of the G3D load-time preprocessor engine

        Process Includes (adding \#line directives where neccessary)
        Process Foreach macros (adding \#line directives where neccessary)
        Process expect macros (adding \#line directives where neccessary)

        Process Version Line

        \param messages Any errors regarding inclusion of files are written here
        
        \return True if no error occured during inclusion of other files
        
    */
    bool g3dLoadTimePreprocessor(const String& dir, PreprocessedShaderSource& source, String& messages, GLuint stage);

    /** Reads the code looking for a \#version line (spaces allowed after "#"). If one is found, 
        remove it from \a code and put it in \a versionLine, otherwise set versionLine
        to "#version 330\n".
    
        @return True if we found and removed a version line from the code.
    */
    static bool processVersion(String& code, String& versionLine);


    /** Reads the code looking for a \#extension line (spaces allowed after "#"). If one is found,
	remove it from \a code and put it in \a extensionLine.
        
	@return True if we found and removed a version line from the code.

        \deprecated
    */
    static void processExtensions(String& code, String& extensionLines);


    /** Reloads all shaders throughout G3D. */
    static void reloadAll();

    /** Converts texture types to their canonical value (anything that can be a sampler2D becomes GL_TEXTURE_2D, etc)
        all others are unmodified.*/
    static GLenum canonicalType(GLenum e);

    /** A shader that accepts color and textureMap arguments and applies them directly without lighting. 
        This is like the old OpenGL fixed function, but supports colors and textures outside of the range [0, 1].

        e.g., used by GFont and Draw::rect.
      */
    static shared_ptr<Shader> unlit();

}; // class Shader

/** \def LAUNCH_SHADER

Get a shader from the cache, loading it if necessary, and launch it on the current RenderDevice using
\a args.  Avoids much of the boilerplate of declaring and initializing shader member variables.

The first argument is a pattern to match all source files for the shader. The pattern is first passed directly to
System::findDataFile. If this fails, the pattern is searched for an underscore. If there is an underscore, everything
up to the first underscore is used as a directory name. For example,
    LAUNCH_SHADER("Film_foo.*", args)
first searches for files matching "Film_foo.*". If no files match, then the pattern "Film/Film_foo.*" is tried.
The part of the pattern up to the first underscore is both the directory name and part of the filename.

It is an error if neither of the above two patterns matches any files. It is also an error if the first pattern
doesn't match any files and the pattern begins with an underscore.

Note that the underlying Shader class and RenderDevice::apply call are available for usage scenarios that do not
fit the common pattern.

\code
  // Example of use:
  LAUNCH_SHADER("SVO_updateLevelIndexBuffer.*", args);
  \endcode

  \sa LAUNCH_SHADER_PTR
*/

#define LAUNCH_SHADER_WITH_HINT(pattern, args, hint) {\
    static const shared_ptr<G3D::Shader> __theShader = G3D::Shader::getShaderFromPattern(pattern); \
    static const size_t _profilerHashBase = ::HashTrait<G3D::String>::hashCode(__theShader->name()) ^ ::HashTrait<G3D::String>::hashCode(G3D::String(__FILE__)) ^ size_t(__LINE__); \
    bool LAUNCH_SHADER_timingEnabled = G3D::Profiler::LAUNCH_SHADER_timingEnabled();\
    if (LAUNCH_SHADER_timingEnabled) {\
	    G3D::Profiler::beginEvent(__theShader->name(), __FILE__, __LINE__, _profilerHashBase, hint);\
    }\
	G3D::RenderDevice::current->apply(__theShader, (args)); \
    if (LAUNCH_SHADER_timingEnabled) {\
	    G3D::Profiler::endEvent();\
    }\
}

#define LAUNCH_SHADER(pattern, args) { \
    LAUNCH_SHADER_WITH_HINT(pattern, args, "") \
}

/** \def LAUNCH_SHADER_PTR
  Launch an already-loaded shader
  \code
    LAUNCH_SHADER_PTR(m_shader, args);
  \endcode
  \sa LAUNCH_SHADER
*/
#define LAUNCH_SHADER_PTR_WITH_HINT(shader, args, hint) { \
    bool LAUNCH_SHADER_timingEnabled = G3D::Profiler::LAUNCH_SHADER_timingEnabled();\
    static const size_t _profilerHashBase = ::HashTrait<G3D::String>::hashCode(shader->name()) ^ ::HashTrait<G3D::String>::hashCode(G3D::String(__FILE__)) ^ size_t(__LINE__); \
    if (LAUNCH_SHADER_timingEnabled) {\
	    G3D::Profiler::beginEvent(shader->name(), __FILE__, __LINE__, _profilerHashBase, hint);\
    }\
	G3D::RenderDevice::current->apply(shader, (args)); \
    if (LAUNCH_SHADER_timingEnabled) {\
	    G3D::Profiler::endEvent();\
    }\
}

#define LAUNCH_SHADER_PTR(shader, args) { \
    LAUNCH_SHADER_PTR_WITH_HINT(shader, args, "") \
}


}

#endif
