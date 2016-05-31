/**
 \file GLG3D/Shader_ShaderProgram.cpp
  
 \maintainer Morgan McGuire, Michael Mara http://graphics.cs.williams.edu
 
 \created 2012-06-13
 \edited  2012-06-07

 */
#include "G3D/platform.h"
#include "GLG3D/Shader.h"
#include "GLG3D/GLCaps.h"
#include "G3D/fileutils.h"
#include "G3D/Log.h"
#include "G3D/FileSystem.h"
#include "G3D/prompt.h"
#include "GLG3D/RenderDevice.h"

namespace G3D {

static GLenum glShaderType(int s) {
    switch(s) {
    case Shader::VERTEX:               return GL_VERTEX_SHADER;
    case Shader::TESSELLATION_CONTROL: return GL_TESS_CONTROL_SHADER;
    case Shader::TESSELLATION_EVAL:    return GL_TESS_EVALUATION_SHADER;
    case Shader::GEOMETRY:             return GL_GEOMETRY_SHADER;
    case Shader::PIXEL:                return GL_FRAGMENT_SHADER;
    case Shader::COMPUTE:              return GL_COMPUTE_SHADER;
    default:
        alwaysAssertM(false, format("Invalid shader type %d given to glShaderType", s));
        return -1;
    }
}


static void readAndAppendShaderLog(const char* glInfoLog, String& messages, const String& name, const Table<int, String>& indexToNameTable){
    int c = 0;
    // Copy the result to the output string
    while (glInfoLog[c] != '\0') {       

        // Copy until the next newline or end of string
        String line;
        while (glInfoLog[c] != '\n' && glInfoLog[c] != '\r' && glInfoLog[c] != '\0') {
            line += glInfoLog[c];
            ++c;
        }

        if (beginsWith(line, "ERROR: ")) {
            // NVIDIA likes to preface messages with "ERROR: "; strip it off
            line = line.substr(7);
        }

        TextInput::Settings settings;
        settings.simpleFloatSpecials = false;
        TextInput t(TextInput::FROM_STRING, line, settings);

        if ((t.peek().extendedType() == Token::INTEGER_TYPE) || (t.peek().extendedType() == Token::HEX_INTEGER_TYPE)) {
            // Now parse the file index into a file name
            const int index = t.readInteger();
            line = t.readUntilNewlineAsString();
            line = indexToNameTable.get(index) + line;
        } else {
            line = ": " + line;
        }

        messages += line;
        
        if (glInfoLog[c] == '\r' && glInfoLog[c + 1] == '\n') {
            // Windows newline
            c += 2;
        } else if ((glInfoLog[c] == '\r' && glInfoLog[c + 1] != '\n') || // Dangling \r; treat it as a newline
            glInfoLog[c] == '\n') {
            ++c;
        }
       
        messages += NEWLINE;
    }
    
}


shared_ptr<Shader::ShaderProgram> Shader::ShaderProgram::create
(const Array<PreprocessedShaderSource>& preprocessedSource, 
 const String&                     preambleAndMacroString, 
 const Args&                            args, 
 const Table<int, String>&         indexToNameTable){

    shared_ptr<ShaderProgram> s(new ShaderProgram());
    s->init(preprocessedSource, preambleAndMacroString, args, indexToNameTable);
    return s;
}


void Shader::ShaderProgram::init(const Array<PreprocessedShaderSource>& pss, const String& preambleAndMacroString, const Args& args, const Table<int, String>& indexToNameTable){
    ok = true;
    debugAssertGLOk();
    
    if (! GLCaps::supports_GL_ARB_shader_objects()) {
        messages = "This graphics card does not support GL_ARB_shader_objects.";
        ok = false;
        return;
    }
    
    debugAssertGLOk();
    Array<String> fullCode;
    compile(pss, preambleAndMacroString, args, indexToNameTable, fullCode);
    debugAssertGLOk();
    if (ok) {
       link();
       if ( !ok) {
           debugPrintf("Shader code:\n");
           for (int i = 0; i < STAGE_COUNT; ++i) {
               debugPrintf("Stage %d:\n", i);
               debugPrintf("%s\n\n", fullCode[i].c_str());
           }
       }
    }

    debugAssertGLOk();
    if (ok) {
        addActiveUniformsFromProgram();
        debugAssertGLOk();
        addUniformsFromSource(pss, args);
        debugAssertGLOk();
    }

    if (ok) {
        addActiveAttributesFromProgram();
        debugAssertGLOk();
        addVertexAttributesFromSource(pss);
        debugAssertGLOk();
    }
    debugAssertGLOk();

    logPrintf("%s\n", messages.c_str());
}


void Shader::ShaderProgram::link() {
    glProgramObject = glCreateProgram();
    debugAssertGLOk();
    // Attach
    for (int s = 0; s < STAGE_COUNT; ++s) {
        if (glShaderObject[s]) {
            glAttachShader(glProgramObject, glShaderObject[s]);
        }
        debugAssertGLOk();
    }

    // Link
    glLinkProgram(glProgramObject);
    debugAssertGLOk();

    // Read back messages
    GLint linked;
    glGetProgramiv(glProgramObject, GL_LINK_STATUS, &linked);
    debugAssertGLOk();

    GLint maxLength = 0, length = 0;
    glGetProgramiv(glProgramObject, GL_INFO_LOG_LENGTH, &maxLength);
    GLchar* pInfoLog = (GLchar *)malloc((maxLength + 1) * sizeof(GLcharARB));
    glGetProgramInfoLog(glProgramObject, maxLength, &length, pInfoLog);
    debugAssertGLOk();

    messages += String(pInfoLog);
    ok = ok && (linked == GL_TRUE);

    free(pInfoLog);
    pInfoLog = nullptr;
}


void Shader::ShaderProgram::compile
   (const Array<PreprocessedShaderSource>&  pss, 
    const String&                           preambleAndMacroArgs, 
    const Args&                             args,
    const Table<int, String>&               indexToNameTable,
    Array<String>&                          codeArray) {
    
    debugAssertGLOk();
    codeArray.fastClear();
    codeArray.resize(STAGE_COUNT);
    for (int s = 0; s < STAGE_COUNT; ++s) {
        const PreprocessedShaderSource& pSource = pss[s];
        if (pSource.preprocessedCode != "") {
            String fullyProcessedCode = pSource.preprocessedCode;

            const bool processSuccess = Shader::expandForPragmas(fullyProcessedCode, args, indexToNameTable, messages);
            ok = ok && processSuccess;

            if (processSuccess) {
				String& code = codeArray[s] = pSource.versionString + pSource.extensionsString + preambleAndMacroArgs + pSource.g3dInsertString + fullyProcessedCode;


                GLint compiled = GL_FALSE;
                GLuint& glShader = glShaderObject[s];
                glShader = glCreateShader(glShaderType(s));

                // Compile the shader
                GLint length = (GLint)code.length();

				// debugPrintf("Compiling a shader of %d kB\n", iRound(length / 1000.0f));
                const GLchar* codePtr = static_cast<const GLchar*>(code.c_str());
                
                int count = 1;
                glShaderSource(glShader, count, &codePtr, &length);
                glCompileShader(glShader);
                glGetShaderiv(glShader, GL_COMPILE_STATUS, &compiled);

                // Read the result of compilation
                GLint maxLength;
                glGetShaderiv(glShader, GL_INFO_LOG_LENGTH, &maxLength);

                debugAssertGLOk();
                if (maxLength > 0) {
                    GLchar* pInfoLog = (GLchar*)System::malloc(maxLength * sizeof(GLchar));
                    glGetShaderInfoLog(glShader, maxLength, &length, pInfoLog);
                    readAndAppendShaderLog(pInfoLog, messages, pSource.filename, indexToNameTable);
                    System::free(pInfoLog);
                }
            
                ok = ok && (compiled == GL_TRUE);

#               ifdef G3D_DEBUG
                if (! ok) {
                    debugPrintf("Shader source:\n%s\n", codePtr);
                }
#               endif
            }

        } else {
            // No code to compile from, so the shader object does not exist
            glShaderObject[s] = 0; 
        }    
    } // for each stage
    
}


void Shader::ShaderProgram::addUniformsFromSource(const Array<PreprocessedShaderSource>& preprocessedSource, const Args& args){
    for (int s = 0; s < STAGE_COUNT; ++s) {
        addUniformsFromCode(preprocessedSource[s].preprocessedCode, args);
    }
}


static bool parseTokenIntoIntegerLiteral(const Token& t, const Args& args, int& result){
    if ((t.extendedType() == Token::INTEGER_TYPE) || (t.extendedType() == Token::HEX_INTEGER_TYPE)) {
        result = (int)t.number();
        return true;
    } else { 
        if (t.extendedType() == Token::SYMBOL_TYPE) {
            String macroValue;
            if (args.getMacro(t.string(), macroValue)) {
                TextInput ti(TextInput::FROM_STRING, macroValue);
                const Token& intToken = ti.read();
                if ((intToken.extendedType() == Token::INTEGER_TYPE) || (intToken.extendedType() == Token::HEX_INTEGER_TYPE)) {
                    result  = (int)intToken.number();
                    return true;
                } 
            } 
        } 
    }

    return false;    
}


void Shader::ShaderProgram::addUniformsFromCode(const String& code, const Args& args) {

    TextInput ti(TextInput::FROM_STRING, code);
    while (ti.hasMore()) {
        const Token nextToken = ti.peek();
        if ((nextToken.type() == Token::SYMBOL) && (nextToken.string() != "#")) {
            bool isUniform = false;
            const GLenum type = getDeclarationType(ti, isUniform);
            if (isUniform && (type != GL_NONE)) {
                // Read the name
                const String& name = ti.readSymbol();
                if (isValidIdentifier(name) && !beginsWith(name, "_noset_")) {
                    int arraySize = -1;

                    if ((ti.peek().type() == Token::SYMBOL) && (ti.peek().string() == "[")) {
                        ti.readSymbol("[");
                        parseTokenIntoIntegerLiteral(ti.read(), args, arraySize);
                        ti.readSymbol("]");
                    }

                    // Read until the semi-colon
                    while (ti.read().string() != ";");

                    if (arraySize < 0) { 
                        // Not an array
                        bool created;
                        UniformDeclaration& d = uniformDeclarationTable.getCreate(name, created);

                        // See if this variable is already declared.
                        if (created) {
                            int index = -1;
                            d.fillOutDummy(name, index, type);
                        }

                    } else { 
                        // An array
                        bool created;

                        for (int i = 0; i < arraySize; ++i) {
                            UniformDeclaration& d = uniformDeclarationTable.getCreate(name + format("[%d]", i), created);
                            // See if this variable is already declared.
                            if (created) {
                                d.fillOutDummy(name, i, type);
                            }
                        }
                    }
                }
            } else {
                ti.readUntilNewlineAsString();
            }
        } else {
            // Consume the entire line
            ti.readUntilNewlineAsString();
        }
    }
}


bool Shader::ShaderProgram::isQualifier(const String& s) {
    static bool initialized = false;
    //except for memory qualifiers, there should only be one qualifier of each type - the types are broken up here for readability of code, but this restriction is not enforced
    //accepts all possible qualifiers, http://www.opengl.org/registry/doc/GLSLangSpec.4.30.6.pdf (pg46)
    static Array<String> qualifiers("const" , "in" , "out" , "attribute"); //storage qualifiers
    if (! initialized) {
        qualifiers.append("uniform" , "varying" , "buffer" , "shared");//more storage qualifiers
        qualifiers.append("centroid" , "sample" , "patch");//Auxiliary storage qualifiers
        qualifiers.append("noperspective" , "flat" , "smooth");//Interpolation Qualifiers
        qualifiers.append("precise" , "invariant");//Variance Qualifiers
        qualifiers.append("lowp" , "mediump" , "highp");//Precision Qualifiers
        qualifiers.append("coherent" , "volatile" , "restrict" , "readOnly");//Memory Qualifiers
        qualifiers.append("writeOnly");
        initialized = true;
    }

    return qualifiers.contains(s);
}


GLenum Shader::ShaderProgram::getDeclarationType(TextInput& ti) {
    bool b = false;
    return getDeclarationType(ti, b);
}


GLenum Shader::ShaderProgram::getDeclarationType(TextInput& ti, bool& uniform) {
    uniform = false;
    if (ti.peek().type() == Token::SYMBOL) {
        bool readyForType = false;
        String s = ti.peek().string();
        //Parse all qualifiers before the type: 
        while (! readyForType) {
            if (isQualifier(s)) {
                uniform = (uniform || (s == "uniform"));
                ti.readSymbol(s);
                s = ti.peek().string();
            } else if (s == "layout") {
                //This should properly parse through all possible layout inputs
                //http://www.opengl.org/registry/doc/GLSLangSpec.4.30.6.pdf (pg52)
                ti.readSymbol(s);
                ti.readSymbol("(");
                Token t = ti.read();
                while ((t.type() != Token::SYMBOL) || (t.string() != ")")) {
                    t = ti.read();  
                }
                s = ti.peek().string();
            } else {
                // The next token is not a qualifier of any sort, so it is probably the type, if this is a declaration
                readyForType = true;
            }
        }

        // Read the type
        String variableSymbol = ti.readSymbol();

        // check for unsigned int, short, long long, etc
        if (variableSymbol == "unsigned") {
            while (toGLType(variableSymbol + " " + ti.peek().string()) != GL_NONE) {
                variableSymbol += " " + ti.readSymbol();
            }
        }
        // If the variableSymbol is not a valid type, then this is not a variable declaration, and GL_NONE will be returned
        // Checking for not being a declaration cannot be done earlier, as a declaration could have no qualifiers
        return toGLType(variableSymbol);
    } else {
        return GL_NONE;
    }
}

            
void Shader::ShaderProgram::addVertexAttributesFromSource(const Array<PreprocessedShaderSource>& preprocessedSource) {
    
    const String& code = preprocessedSource[VERTEX].preprocessedCode;

    TextInput::Settings settings;
    settings.simpleFloatSpecials = false;
    TextInput ti(TextInput::FROM_STRING, code, settings);

    while (ti.hasMore()) {
        Token nextToken = ti.peek();
        if ((nextToken.type() == Token::SYMBOL) && (nextToken.string() != "#")) {
            const GLenum type = getDeclarationType(ti);
            if (type != GL_NONE) {
                // Read the name
                const String& name = ti.readSymbol();
            
                // If there is not a variable name following the type, then 
                // this is not a variable declaration. It may be a geometry shader declaration.
                if (isValidIdentifier(name)) {
                    int elementNum = 1;
                    if ((ti.peek().type() == Token::SYMBOL) && (ti.peek().string() == "[")) {
                        ti.readSymbol("[");
                        elementNum = (int)ti.readNumber();
                        ti.readSymbol("]");
                    }

                    bool created;
                    // See if this variable is already declared.
                    AttributeDeclaration& d = attributeDeclarationTable.getCreate(name, created);
                
                    if (created) {
                        d.location = -1;
                        d.name = name;
                        d.elementNum = elementNum;
                        d.type = type;
                    }
                }

                // Read until the semicolon
                while (ti.read().string() != ";");

            } else {
                ti.readUntilNewlineAsString();
            }
           
        } else {
            // Consume the entire line
            ti.readUntilNewlineAsString();
        }
    }
}


void Shader::ShaderProgram::addActiveAttributesFromProgram() {
    // Length of the longest variable name
    GLint maxLength;

    // Number of uniform variables
    GLint attributeCount;

    // Get the number of uniforms, and the length of the longest name.
    glGetProgramiv(glShaderProgramObject(), GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLength);
    glGetProgramiv(glShaderProgramObject(), GL_ACTIVE_ATTRIBUTES, &attributeCount);

    GLchar* name = (GLchar *) System::malloc(maxLength * sizeof(GLchar));
    
    // Get the sizes, types and names
    if (maxLength > 0) { // If the names are blank, don't try to get them.
        // Loop over glGetActiveAttribute and store the results away.
        for (GLuint i = 0; i < (GLuint)attributeCount; ++i) {   
            AttributeDeclaration d;
            glGetActiveAttrib(glShaderProgramObject(), i, maxLength, NULL, &d.elementNum, &d.type, name); 
            d.location = glGetAttribLocation(glShaderProgramObject(), name);

            const String n(name);
            // Ignore empty and incorrect variables, which are occasionally returned by the driver.
            const bool bogus = ((d.location == -1) && n.empty()) ||
                beginsWith(n, "_main") || beginsWith(n, "_noset_");
            if (! bogus) {
                d.name = n;
                debugAssert(!attributeDeclarationTable.containsKey(n));
                attributeDeclarationTable.set(name, d);
            }
        }
    }
    System::free(name);
    name = NULL;
}


void Shader::ShaderProgram::addActiveUniformsFromProgram() {
    // Length of the longest variable name
    GLint maxLength;

    // Number of uniform variables
    GLint uniformCount;
    // Get the number of uniforms, and the length of the longest name.
    glGetProgramiv(glShaderProgramObject(), GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);
    glGetProgramiv(glShaderProgramObject(), GL_ACTIVE_UNIFORMS, &uniformCount);
    GLchar* name = (GLchar *) malloc(maxLength * sizeof(GLchar));
    
    // Get the sizes, types and names
    int lastTextureUnit = -1;
    int lastImageUnit = -1;
    // Loop over glGetActiveUniform and store the results away.
    for (int i = 0; i < uniformCount; ++i) {
        const GLuint uniformIndex = i;
        GLuint programObject = glShaderProgramObject();

        GLint elementNum;
        GLenum type;
        glGetActiveUniform(programObject, uniformIndex, maxLength, NULL, &elementNum, &type, name);
        UniformDeclaration& d = uniformDeclarationTable.getCreate(name);
        d.name = name;
        d.location = glGetUniformLocation(glShaderProgramObject(), name);
        d.type = type;
        d.elementNum = elementNum;

        bool isGLBuiltIn = (d.location == -1) || 
            ((strlen(name) > 3) && beginsWith(String(name), "gl_"));

        d.dummy = isGLBuiltIn;
        d.index = -1;
        if (! isGLBuiltIn) {
            if (isSamplerType(d.type)) {
                ++lastTextureUnit;
                d.glUnit = lastTextureUnit;           
            } else if (isImageType(d.type)) {
                ++lastImageUnit;
                d.glUnit = lastImageUnit;
            } else if (d.elementNum == 1) { 
                // Not array
                d.glUnit = -1;
            } else { 
                // Is an array, remove from uniform declaration table, and add it's elements
                GLint type      = d.type;
                int arraySize   = (int)d.elementNum;
                int glUnit = -1;
                uniformDeclarationTable.remove(name);
                
                // Get rid of [0] if it exists (depends on driver)
                String arrayName = name;
                if (arrayName[arrayName.length()-1] == ']') {
                    size_t bracketLoc = arrayName.rfind('[');
                    // TODO: error handle if "[" doesn't exist
                    arrayName = arrayName.substr(0, bracketLoc);
                }
                
                
                for (int i = 0; i < arraySize; ++i) {
                    const String appendedName = format("%s[%d]", arrayName.c_str(), i);
                    UniformDeclaration& elementDeclaration = uniformDeclarationTable.getCreate(appendedName);
                    GLint location = glGetUniformLocation(glShaderProgramObject(), appendedName.c_str());
                    debugAssertGLOk();
		            bool dummy = (location == -1);
                    elementDeclaration.setAllFields(appendedName, i, type, location, dummy, glUnit);
                }
            }
        }
    }

    free(name);
}


} // namespace G3D
