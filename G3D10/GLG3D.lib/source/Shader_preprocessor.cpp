/**
 \file GLG3D/Shader_preprocessor.cpp
  
 \maintainer Michael Mara http://illuminationcodified.com
 
 \created 2012-06-19
 \edited  2014-11-02

  G3D Library http://g3d.codeplex.com
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/
#include "G3D/platform.h"
#include "GLG3D/Shader.h"
#include "G3D/stringutils.h"
#include "G3D/FileSystem.h"
#include "G3D/fileutils.h"
#include "GLG3D/GLCaps.h"

namespace G3D {

static bool isNextToken(const String& macro, const String& code, size_t offset = 0) {
    if (offset == String::npos) {
        return false;
    }

    for (size_t i = offset; i < code.length() - macro.size() + 1; ++i) {
        const char c = code[i];
        if (c == macro[0]) {
            for (int j = 1; j < (int)macro.size(); ++j) {
                if (code[i+j] != macro[j]) {
                    return false;
                }
            }
            return true;
        }

        if (c != ' ' && c != '\t') {
            return false;
        }
    }
    return false; //CC: Was true, why ? ->caused bugs for me. Changed to false.
}


static bool precedingCharactersAreWhitespace(size_t offset, const String& code) {
    if( offset == 0 ) { 
        return true;
    }
    char c;
    for (size_t i = offset - 1; i > 0; --i) {
        c = code[i];
        if ( c == '\n' || c == '\r') {
            return true;
        }
        if ( (c != ' ') && (c != '\t') ) {
            return false;
        }
    }
    return true;
}


/** Returns the position of the last pragma of the form "#macro" (with any whitespace between the # and macro) in code, starting from the \param offset */
static size_t findLastPragmaWithSpaces(const String& macro, const String& code, size_t offset) {
    
    do {
        offset = code.rfind("#", offset);
        if(offset != String::npos) {
            if(precedingCharactersAreWhitespace(offset, code) && isNextToken(macro, code, (int)offset + 1)){
                return offset;
            }
            offset -= 1;
        } 

    } while(offset != String::npos);

    if (beginsWith(code, "#")) {
        if(isNextToken(macro, code, 1)) {
            return 0;
        }
    }
    
    return offset;
}


static size_t findPragmaWithSpaces(const String& macro, const String& code, size_t offset = 0) {
    if (beginsWith(code, "#")){
        if (isNextToken(macro, code, 1)) {
            return 0;
        }
    }

    do {
        offset = code.find("#", offset);

        if (offset != String::npos) {
            if (precedingCharactersAreWhitespace(offset, code) && isNextToken(macro, code, (int)offset + 1)) {

                // Look backwards to make sure that we aren't in a comment
                const int previousBlockCommentEnd(int(code.rfind("*/", offset)));
                const int previousBlockCommentBegin(int(code.rfind("/*", offset)));

                // Take advantage of std::npos == -1
                if (previousBlockCommentEnd >= previousBlockCommentBegin) {
                    // We are not in a comment
                    return offset;
                }

            }
            ++offset;
        } 

    } while (offset != String::npos);

    return offset;
}


String lineMacro(int lineNumber, int fileIndex) {
    return format("#line %d %d\n", lineNumber, fileIndex);
}


String Shader::getLinePragma(int lineNumber, const String& filename) {
    int fileIndex = m_nextUnusedFileIndex;
    if (m_fileNameToIndexTable.containsKey(filename)) {
        fileIndex = m_fileNameToIndexTable.get(filename);
    } else {
        // We need to map both directions.
        m_fileNameToIndexTable.set(filename, fileIndex);
        m_indexToFilenameTable.set(fileIndex, filename);
        ++m_nextUnusedFileIndex;
    }

    // Current GLSL specs set the __LINE__ macro to the line number; before 3.30 it also added one.
    return lineMacro(lineNumber, fileIndex);
}


/** If true, optimize shader compilation performance by only including files once within a shader. This is like forcing 
    "#pragma once" everywhere. It assumes that #includes are not inside of macro logic other than header guards,
	which is a good practice anyway.*/
static const bool includeFilesAtMostOnce = true;

bool Shader::processIncludes(const String& dir, String& code, String& messages) {
    // Including occured without a hitch
	bool ok = true;
	
	// Look for #include immediately after a newline.  If it is inside
    // a #if or a block comment, it will still be processed, but
    // single-line comments will properly disable it.
    bool foundPound = false;

    // Note that this is reloaded for each shader--it is just supposed to speed
    // repeated inclusions within a single complex shading program. 
	Table<String, String> includedFileContentsCache;

	size_t includeLoc = 0;
    do {
        foundPound = false;
        includeLoc = findPragmaWithSpaces("include", code, includeLoc);
        const size_t lineLoc    = findLastPragmaWithSpaces("line", code, includeLoc);

        if (includeLoc != String::npos) {
            // Remove this line
            size_t includeEnd = code.find("\n", includeLoc + 1);
            if (includeEnd == String::npos) {
                includeEnd = code.size();
            }
  
            const String& includeLine = code.substr(includeLoc, includeEnd - includeLoc + 1);
            String includedFilename;
            TextInput t (TextInput::FROM_STRING, includeLine);
            t.readSymbols("#", "include");


            if (t.peek().extendedType() == Token::SYMBOL_TYPE) {

                t.readSymbol("<");
                includedFilename = t.readUntilDelimiterAsString('>');
                t.readSymbol(">");
                includedFilename = System::findDataFile(includedFilename);

            } else {

                includedFilename = t.readString();
                includedFilename = FilePath::canonicalize(includedFilename);

                // Resolve non-absolute paths relative to the current file
                if (! beginsWith(includedFilename, "/")) {
                    includedFilename = pathConcat(dir, includedFilename);
                }
            }

            // Find the current filename
            const size_t  linePragmaEnd     = code.find("\n", lineLoc + 1);
            const String& lastLinePragma   = code.substr(lineLoc, linePragmaEnd - lineLoc + 1);

			TextInput     tlp(TextInput::FROM_STRING, lastLinePragma);
            tlp.readSymbols("#", "line");
            int lastLineNumber             = tlp.readInteger() + 1;

            const String& lastFile         = m_indexToFilenameTable.get(tlp.readInteger());

            // # of newLines between the include pragma and the closest line pragma before it
            int linesSinceLastLineNumber    = countNewlines(code.substr(linePragmaEnd + 1, includeLoc - linePragmaEnd - 1));

            String includedFile;

            try {
                bool created = false;
                String& contents = includedFileContentsCache.getCreate(includedFilename, created);

                if (created) {
                    contents = readWholeFile(includedFilename);
					includedFile = contents;
                } else if (! includeFilesAtMostOnce) {
					// If this wasn't the first time that we've loaded this include file, then just skip it.
					// If the shader file cache is ever made global (rather than per-shader), then this
					// logic needs to be a per-shader detection rather than a global cache detection.
					includedFile = contents;
				}

            } catch (...) { // All errors will be reported once loading is complete.
				ok = false;
                messages += format("%s(%d): #included file %s not found.\n", 
									lastFile.c_str(), lastLineNumber + linesSinceLastLineNumber,
                                    includedFilename.c_str());
            }

            if (! endsWith(includedFile, "\n")) {
                includedFile += "\n";
            }

            code = code.substr(0, includeLoc) + getLinePragma(1, includedFilename) + includedFile 
                + getLinePragma(lastLineNumber + linesSinceLastLineNumber - 1, lastFile) + code.substr(includeEnd);
            foundPound = true;
        }

    } while (foundPound);
	return ok;
}


static void extractCurrentLineInformation(size_t currentLocation, const String& source, int& currentLineNumber, int& currentFileIndex) {
    // Synthesize line pragma
    size_t linePragmaBegin              = findLastPragmaWithSpaces("line", source, currentLocation);
    size_t linePragmaEnd                = source.find("\n", linePragmaBegin + 1);
    const String& lastLinePragma   = source.substr(linePragmaBegin, linePragmaEnd - linePragmaBegin + 1);

    TextInput tlp (TextInput::FROM_STRING, lastLinePragma);
    tlp.readSymbols("#", "line");
    int lastLineNumber              = tlp.readInteger() + 1;
    // # of newLines between the include pragma and the closest line pragma before it
    int linesSinceLastLineNumber    = countNewlines(source.substr(linePragmaEnd + 1, currentLocation - linePragmaEnd - 1));

    currentFileIndex    = tlp.readInteger();
    currentLineNumber   = lastLineNumber + linesSinceLastLineNumber;
}


//    return replace(body, "$(" + counterToken + ")", format("%d", value));

static String expandForLoopBodyOnce(const String& body, const String& counterToken, int value) {
    const String& valueString = format("%d", value);
    String result;

    size_t prevEnd = 0;
    while (prevEnd < body.length() - 1) {
        const size_t start = body.find("$(", prevEnd);
        if (start == String::npos) {

            // No more replacements, copy the rest of the string
            result += body.substr(prevEnd, body.length() - prevEnd);
            prevEnd = body.length() - 1;

        } else {
            const size_t end = body.find(")", start + 2);

            const String& expr = body.substr(start + 2, end - start - 2);

            if (expr == counterToken) {
                // Simple replacement
                result += body.substr(prevEnd, start - prevEnd) + valueString;
                prevEnd = end + 1;
            } else {
                // Expression
                TextInput ti(TextInput::FROM_STRING, expr);
                Token t;
            
                String sym = ti.readSymbol();

                if (sym == counterToken) {
                    Token oper = ti.read();

                    if (oper.type() != Token::SYMBOL) {
                        throw String("Expected symbol inside $(") + counterToken + " ...)";
                    }

                    if (oper.string() == ")") {
                        // Simple replacement after all, but with whitespace around it
                        result += body.substr(prevEnd, start - prevEnd) + valueString;
                        prevEnd = end + 1;
                    } else {
                        double v = value;
                        const double d = ti.readNumber();
                        if (oper.string() == "+") {
                            v += d;
                        } else if (oper.string() == "-") {
                            v -= d;
                        } else if (oper.string() == "/") {
                            if ((oper.extendedType() == Token::INTEGER_TYPE) || (oper.extendedType() == Token::HEX_INTEGER_TYPE)) {
                                v = floor(v / d);
                            } else {
                                v /= d;
                            }
                        } else if (oper.string() == "*") {
                            v *= d;
                        } else {
                            throw String("Expected +, -, /, or * inside $(") + counterToken + " ...)";
                        }

                        result += body.substr(prevEnd, start - prevEnd) + format("%g", v);
                        prevEnd = end + 1;
                    }
                } else {
                    // We just read another #FOR-loop's variable--pass it on
                    result += body.substr(prevEnd, end - prevEnd + 1);
                    prevEnd = end + 1;
                } // if is counterToken
            } // if complex expression
        } // if counter token found
    } // while

    return result;
}


static void expandForBlock(String& expandedBlock, const String& innerBlock, const String& counterToken, int initValue, int endValue) {
    //const String& searchString = "$(" + counterToken + ")";
    for (int i = initValue; i < endValue; ++i) {
        expandedBlock += expandForLoopBodyOnce(innerBlock, counterToken, i);
    }
}


static void expandForEachBlock(String& expandedBlock, const String& innerBlock, const Array<String>& fieldTokens, const Array< Array<String> >& valueTuples) {
    for(int i = 0; i < valueTuples.size(); ++i) {
        String newBlock = innerBlock;
        const Array<String>& currentTuple = valueTuples[i];
        for (int j = 0; j < currentTuple.size(); ++j) {
            const String& searchString = "$(" + fieldTokens[j] + ")";
            newBlock = replace(newBlock, searchString, currentTuple[j]);
        }
        expandedBlock += newBlock;
    }
}

/** Includes everything after "#endfor[each]" (including the newline) in afterForBlockString */
static void fragmentSourceAroundForPragma(const String& source, size_t forLocation, size_t endForLocation, String& beforeForBlockString, 
                                        String& forLine, String& innerBlock, String& afterForBlockString){
    
    size_t afterEndOfForLine    = source.find('\n', forLocation) + 1;
    
                                            
    beforeForBlockString        = source.substr(0, forLocation);
    forLine                     = source.substr(forLocation, afterEndOfForLine - forLocation);
    innerBlock                  = source.substr(afterEndOfForLine, endForLocation - afterEndOfForLine);

    size_t indexOfNewlineAferEndFor = source.find('\n', endForLocation);
    if(indexOfNewlineAferEndFor == String::npos) {
        afterForBlockString = "";
    } else {
        size_t afterForBlockStart   = indexOfNewlineAferEndFor + 1;
        afterForBlockString         = source.substr(afterForBlockStart, source.length() - afterForBlockStart);
    }
}

static bool parseTokenIntoIntegerLiteral(const Token& t, const Args& args, int& result) {
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


/** Don't emit an error message if the #for loops macro arg bounds don't exist, but rather insert an #error macro into the source, so if its in a dead branch of an #ifdef it still works */
static String parseForLine(const String& forLine, const Args& args, String& counterToken, int& initValue, int& endValue, String& errorMessages){

    String emitCode = "";

    TextInput::Settings settings;
    TextInput ti(TextInput::FROM_STRING, forLine, settings);
    
    ti.readSymbols("#", "for", "(", "int"); 
    counterToken = ti.readSymbol(); 
    ti.readSymbol("=");

    const Token& initToken = ti.read();
    if (! parseTokenIntoIntegerLiteral(initToken, args, initValue)) {
        // The entire #for may have been inside a #ifdef, in which case this code should 
        // have been ignored.  We generate a compile-time error message so that GLSL can
        // detect whether it is an error or not.
        emitCode += format("\n#error Ill-formed FOR pragma, unable to parse initializer (%s) into integer literal\n", initToken.string().c_str());
        initValue = 0;
    }
    ti.readSymbol(";");
    ti.readSymbol(counterToken);
    ti.readSymbol("<");

    const Token& endToken = ti.read();
    if (! parseTokenIntoIntegerLiteral(endToken, args, endValue)) {
        // The entire #for may have been inside a #ifdef, in which case this code should 
        // have been ignored.  We generate a compile-time error message so that GLSL can
        // detect whether it is an error or not.
        endValue = 0;
        emitCode += format("\n#error Ill-formed FOR pragma, unable to parse endValue (%s) into integer literal\n", endToken.string().c_str());
    }

    ti.readSymbol(";");
    ti.readSymbol("++");
    ti.readSymbol(counterToken);
    ti.readSymbol(")");

    return emitCode;
}


static bool readFieldList(TextInput& ti, Array<String>& fieldTokens, String& emitCode) {
    bool hasParenthesis = ti.peek().string() == "(";
    if (hasParenthesis) {
        ti.read();
    }
    while(true) {
        String s = ti.peek().string();
        if (hasParenthesis && s == ")") {
            ti.read();
            break;
        } else if (!hasParenthesis && s == "in") {
            break;
        } else if (s == ",") { // skip these
            ti.read();
        } else {
            fieldTokens.append(ti.readSymbol());
        }
    }

    return false;

}

static String readUntilCommaOrClosParens(TextInput& ti) {
    String result;
    while (ti.peek().string() != "," && ti.peek().string() != ")") {
        result += ti.read().string();
    }
    return result;
}

static bool readValueTuples(TextInput& ti, Array<Array<String> >&  valueTuples, int fieldCount, String& emitCode) {
    while (true) {
        Array<String> currentTuple;
        ti.readSymbol("(");
        currentTuple.append( readUntilCommaOrClosParens(ti) );
        for (int i = 1; i < fieldCount; ++i) {
            ti.readSymbol(",");
            currentTuple.append( readUntilCommaOrClosParens(ti) );
        }
        ti.readSymbol(")");
        valueTuples.append(currentTuple);
        String nextToken = ti.peek().string();
        if ( nextToken == ",") {
            ti.read();
        } else {
            break;
        }
    } 
    return false;
}

// This occurs only on shader load, so performance is not a design goal
static String parseForEachLine(const String& forEachLine, Array<String>& fieldTokens, Array<Array<String> >& valueTuples){
    String emitCode;
    
    TextInput::Settings settings;
    TextInput ti(TextInput::FROM_STRING, forEachLine, settings);
    
    // e.g., #foreach (field0, field1) in (LAMBERTIAN, 2), (GLOSSY, 5)
    ti.readSymbols("#", "foreach"); 

    bool errorReadingFieldList = readFieldList(ti, fieldTokens, emitCode);
    if (errorReadingFieldList) {
        return emitCode;
    }    
    ti.readSymbol("in");
    int fieldCount = fieldTokens.size();

    readValueTuples(ti, valueTuples, fieldCount, emitCode);

    return emitCode;
}


static void findPragmaBlockStartAndEnd(const String& beginMacro, const String& endMacro, const String& source, size_t& beginLocation, size_t& endLocation) {
    beginLocation = findPragmaWithSpaces(beginMacro, source, beginLocation);
    if (beginLocation == String::npos) {
        endLocation = String::npos;
        return;
    }
    size_t currentLocation = beginLocation + 1;
    int openMacroCount = 1;
    while (openMacroCount > 0) {
        size_t nextBeginLocation    = findPragmaWithSpaces(beginMacro,  source, currentLocation);
        size_t nextEndLocation      = findPragmaWithSpaces(endMacro,    source, currentLocation);
        if ( nextEndLocation == String::npos ) {
            endLocation = String::npos;
            return;
        }
        if ( nextBeginLocation == String::npos || nextBeginLocation > nextEndLocation ) {
            endLocation     = nextEndLocation;
            currentLocation = nextEndLocation + 1;
            --openMacroCount;
        } else {
            currentLocation     = nextBeginLocation + 1;
            ++openMacroCount;
        }
    }
    return;

}


bool Shader::expandForEachPragmas(String& processedSource, const Table<int, String>& indexToNameTable, String& errorMessages) {

	const char* errorMsg = "%s(%d): No matching #endforeach found.\n";
    size_t forLocation = 0;
	size_t endForLocation;
    findPragmaBlockStartAndEnd("foreach", "endforeach", processedSource, forLocation, endForLocation);
    while (forLocation != String::npos) {
        // Synthesize line pragma
        int currentLineNumber, currentFileIndex;
        extractCurrentLineInformation(forLocation, processedSource, currentLineNumber, currentFileIndex);
        if (endForLocation == String::npos) {
            errorMessages += format(errorMsg, indexToNameTable.get(currentFileIndex).c_str(), currentLineNumber);
            return false;
        }

        const String& forBlockLinePragma = lineMacro(currentLineNumber, currentFileIndex);

        size_t endForLocation = findPragmaWithSpaces("endforeach", processedSource);
        if (endForLocation == String::npos) {
            errorMessages += format(errorMsg, indexToNameTable.get(currentFileIndex).c_str(), currentLineNumber);
            return false;
        }
        String beforeForBlockString, forLine, innerBlock, afterForBlockString;
        fragmentSourceAroundForPragma(processedSource, forLocation, endForLocation, beforeForBlockString, forLine, innerBlock, afterForBlockString);

        Array<Array<String> > valueTuples;
        Array<String> counterTokens;
        const String& emitCode = parseForEachLine(forLine, counterTokens, valueTuples);

        // Add #line pragma to the body of the loop
        innerBlock = forBlockLinePragma + innerBlock;

        String expandedBlock;
        expandForEachBlock(expandedBlock, innerBlock, counterTokens, valueTuples);
        // Newlines take the place of the old #for and #endfor lines
        processedSource = beforeForBlockString + "\n" + emitCode + expandedBlock + "\n" + afterForBlockString;
        
        findPragmaBlockStartAndEnd("foreach", "endforeach", processedSource, forLocation, endForLocation);
    }

    return true;
}


bool Shader::expandForPragmas(String& processedSource, const Args& args, const Table<int, String>& indexToNameTable, String& errorMessages){

    size_t forLocation = 0, endForLocation;
    findPragmaBlockStartAndEnd("for", "endfor", processedSource, forLocation, endForLocation);
    while (forLocation != String::npos) {

        // Synthesize line pragma
        int currentLineNumber, currentFileIndex;
        extractCurrentLineInformation(forLocation, processedSource, currentLineNumber, currentFileIndex);

        if (endForLocation == String::npos) {
            errorMessages += format("%s(%d): No matching #endfor found.\n", indexToNameTable.get(currentFileIndex).c_str(), currentLineNumber);
            return false;
        }

        const String& forBlockLinePragma = lineMacro(currentLineNumber, currentFileIndex);

        String beforeForBlockString, forLine, innerBlock, afterForBlockString;
        fragmentSourceAroundForPragma(processedSource, forLocation, endForLocation, beforeForBlockString, forLine, innerBlock, afterForBlockString);

        int initValue, endValue;
        String counterToken; 
        const String& emitErrorCode = parseForLine(forLine, args, counterToken, initValue, endValue, errorMessages);

        // Add #line pragma to the body of the loop
        innerBlock = forBlockLinePragma + innerBlock;

        String expandedBlock;
        expandForBlock(expandedBlock, innerBlock, counterToken, initValue, endValue);
        // Newlines take the place of the old #for and #endfor lines
        processedSource = beforeForBlockString + "\n" + emitErrorCode + expandedBlock + "\n" + afterForBlockString;
        
        findPragmaBlockStartAndEnd("for", "endfor", processedSource, forLocation, endForLocation);
    }

    return true;
}


static void fragmentSourceAroundExpectPragma(const String& source, size_t expectLocation, String& beforeExpectLine, 
                                            String& expectLine, String& afterExpectLine) {
    size_t afterEndOfExpectLine    = source.find('\n', expectLocation) + 1;
    beforeExpectLine    = source.substr(0, expectLocation);
    expectLine          = source.substr(expectLocation, afterEndOfExpectLine - expectLocation);
    afterExpectLine     = source.substr(afterEndOfExpectLine, source.length() - afterEndOfExpectLine + 1);
}


static bool expandExpectPragma(String& expandedExpectPragma, const String& expectLine) {
    TextInput::Settings settings;
    TextInput ti(TextInput::FROM_STRING, expectLine, settings);
    try {
        ti.readSymbols("#", "expect"); 
        const Token& macroToken         = ti.read();
        const String& macroName    = macroToken.string();
        const Token& descriptionToken   = ti.read();
        bool hasDescription             = descriptionToken.type() == Token::STRING;
        String description;
        if (hasDescription) {
           description  = ", " + descriptionToken.string();
        }
        expandedExpectPragma = format("#ifndef %s\n", macroName.c_str());
        expandedExpectPragma += format("#error Expected %s argument%s\n" , macroName.c_str(), description.c_str());
        expandedExpectPragma +="#endif\n";
    } catch (ParseError p) {
        return false;
    }

    return true;
}


bool Shader::expandExpectPragmas(String& source, const Table<int, String>& indexToNameTable, String& errorMessages) {
    size_t expectLocation = findPragmaWithSpaces("expect", source);
    while (expectLocation != String::npos) {
        // Synthesize line pragma
        int currentLineNumber, currentFileIndex;
        extractCurrentLineInformation(expectLocation, source, currentLineNumber, currentFileIndex);

        // So that the #error pragma has the same line number as the original #expect.
        const String& beforeExpectLinePragma = lineMacro(currentLineNumber - 2, currentFileIndex);

        // So that the next line after the #expect is numbered correctly
        const String& afterExpectLinePragma = lineMacro(currentLineNumber, currentFileIndex);

        
        String beforeExpectLine, expectLine, afterExpectLine;
        fragmentSourceAroundExpectPragma(source, expectLocation, beforeExpectLine, expectLine, afterExpectLine);

        String expandedExpectPragma;
        bool success = expandExpectPragma(expandedExpectPragma, expectLine);
        if (!success) {
            errorMessages += format("%s(%d): Malformed expect pragma. Use #expect SYMBOL_NAME \"optional description\"\n", 
									indexToNameTable[currentFileIndex].c_str(), currentLineNumber);
            return false;
        }
        source = beforeExpectLine + beforeExpectLinePragma + expandedExpectPragma + afterExpectLinePragma + afterExpectLine;

        expectLocation = findPragmaWithSpaces("expect", source);
    }

    return true;
}


bool Shader::g3dLoadTimePreprocessor(const String& dir, PreprocessedShaderSource& source, String& messages, GLuint stage) {
    String& code				= source.preprocessedCode;
    String& versionString		= source.versionString;
	String& extensionsString	= source.extensionsString;
    const String& name			= source.filename;
    String& insertString		= source.g3dInsertString;

    // G3D Preprocessor
    // Handle #include directives first, since they may affect
    // what preprocessing is needed in the code. 
    code = getLinePragma(1, name) + code;
    bool ok = processIncludes(dir, code, messages);     
    
    if (ok) {
        // Next up is for each preprocessing
        ok = expandForEachPragmas( code, m_indexToFilenameTable, messages);
    }

    if (ok) {
        // #expect pragmas...
        ok = expandExpectPragmas( code, m_indexToFilenameTable, messages );
    }

    
    if (ok) {
        // Standard uniforms.  We'll add custom ones to this below
        String uniformString = 
            STR(uniform mat4x3 g3d_WorldToObjectMatrix;
                uniform mat4x3 g3d_ObjectToWorldMatrix;
                uniform mat4   g3d_ProjectionMatrix;
                uniform mat4x4 g3d_ProjectToPixelMatrix;
                uniform mat3   g3d_WorldToObjectNormalMatrix;
                uniform mat3   g3d_ObjectToWorldNormalMatrix;
                uniform mat3   g3d_ObjectToCameraNormalMatrix;
                uniform mat4x3 g3d_ObjectToCameraMatrix;
                uniform mat3   g3d_CameraToObjectNormalMatrix;
                uniform mat4x3 g3d_WorldToCameraMatrix;
                uniform mat4x3 g3d_CameraToWorldMatrix;
                uniform float  g3d_SceneTime;
                uniform bool   g3d_InvertY;
                uniform mat3   g3d_WorldToCameraNormalMatrix;
                uniform mat4   g3d_ObjectToScreenMatrixTranspose;
                uniform mat4   g3d_ObjectToScreenMatrix;
                uniform vec2   g3d_FragCoordExtent;
                uniform vec2   g3d_FragCoordMin;
                uniform vec2   g3d_FragCoordMax;
                uniform int    g3d_NumInstances;);

        processVersion(code, versionString);

		processExtensions(code, extensionsString);


        // macros that we'll prepend onto the shader
        // These values are from the OpenGL specification. AMD can't parse hexadecimal constants
        String defineString =
            "\n#define G3D_VERTEX_SHADER (35633)"//0x8B31
            "\n#define G3D_TESS_CONTROL_SHADER (36488)"//0x8E88)"
            "\n#define G3D_TESS_EVALUATION_SHADER (0x8E87)"
            "\n#define G3D_GEOMETRY_SHADER (36487)"//0x8DD9)"
            "\n#define G3D_FRAGMENT_SHADER (35632)"//0x8B30)"
            "\n#define G3D_COMPUTE_SHADER (37305)\n";//0x91B9)\n";
       
        switch (GLCaps::enumVendor()) {
        case GLCaps::AMD:
            defineString += "#define G3D_ATI\n";
            defineString += "#define G3D_AMD\n";
            break;

        case GLCaps::NVIDIA:
            defineString += "#define G3D_NVIDIA\n";
            break;

        case GLCaps::MESA:
            defineString += "#define G3D_MESA\n";
            break;

        default:;
        }

#       ifdef G3D_OSX 
            defineString += "#define G3D_OSX\n";
#       endif
#       if defined(G3D_WINDOWS)
            defineString += "#define G3D_WINDOWS\n";
#       endif
#       if defined(G3D_LINUX)
            defineString += "#define G3D_LINUX\n";
#       endif
#       if defined(G3D_FREEBSD)
            defineString += "#define G3D_FREEBSD\n";
#       endif
#       if defined(G3D_64BIT)
            defineString += "#define G3D_64BIT\n";
#       endif

        switch (stage) {
        case GL_VERTEX_SHADER:
            defineString += "#define G3D_SHADER_STAGE G3D_VERTEX_SHADER\n";
            break;
        case GL_TESS_CONTROL_SHADER:
            defineString += "#define G3D_SHADER_STAGE G3D_TESS_CONTROL_SHADER\n";
            break;
        case GL_TESS_EVALUATION_SHADER:
            defineString += "#define G3D_SHADER_STAGE G3D_TESS_EVALUATION_SHADER\n";
            break;
        case GL_GEOMETRY_SHADER:
            defineString += "#define G3D_SHADER_STAGE G3D_GEOMETRY_SHADER\n";
            break;
        case GL_FRAGMENT_SHADER:
            defineString += "#define G3D_SHADER_STAGE G3D_FRAGMENT_SHADER\n";
            break;
        case GL_COMPUTE_SHADER:
            defineString += "#define G3D_SHADER_STAGE G3D_COMPUTE_SHADER\n";
            break;
        }

        insertString = defineString + uniformString + "\n";
        insertString +=  "// End of G3D::Shader inserted code\n";
        
        code += "\n";
    }
    return ok;
}


static bool canonicalizeVersionLine(String& versionLine) {
    
    Array<int> validGLSLVersions(110, 120, 130, 140, 150);
    validGLSLVersions.append(330, 400, 410, 420, 430);
    validGLSLVersions.append(440, 450);

    const int highestVersionSupportedOnThisGPU = iRound(GLCaps::glslVersion() * 100);

    TextInput ti (TextInput::FROM_STRING, versionLine);
    int version = -1;
    int nextVersion = -1;
    String chosenPhrase = "version 330";//default version line
    String lastPhrase = "";
    
    //get rid of leading "#"
    ti.readSymbol("#");
    while (ti.hasMore()) {
        Token t = ti.readSignificant(); // Read tokens, ignoring comments
        if (t.string() == "or") { 
            // "or" signifies the end of a version phrase:  the canonicalized version line 
            // will be made using the highest supported version in the version line

            if ((nextVersion <= highestVersionSupportedOnThisGPU) &&
                 validGLSLVersions.contains(nextVersion) && (nextVersion > version)) {
                     version      = nextVersion;
                     chosenPhrase = lastPhrase;
            }
            lastPhrase = "";

        } else if (t.type() == Token::NUMBER) { 

            // any number token is a GLSL version number
            nextVersion = (int)t.number();
            lastPhrase += t.string() + " ";

        } else { 
            //append other tokens (like "compatability" or the optional word "version") to the current version phrase
            lastPhrase += t.string() + " ";
        }
    }

    // test if the last phrase had the highest version (as there was
    // no "or" at the end of it)
    if ((nextVersion <= highestVersionSupportedOnThisGPU) &&
         validGLSLVersions.contains(nextVersion) && (nextVersion > version)) {
        version      = nextVersion;
        chosenPhrase = lastPhrase;
    }

    // construct the final versionLine (adding back the beginning "#" and the word "version" if it was omitted)
    versionLine = (beginsWith(chosenPhrase, "version") ? "#" : "#version ")  + chosenPhrase + "\n";

    // as long as version was set to something other than -1, this should be a valid versionLine
    return (version != -1);
}


bool Shader::processVersion(String& code, String& versionLine) {
    const size_t i = findPragmaWithSpaces("version", code);
        
    if (i != String::npos) {
        // Remove this line
        size_t end = code.find("\n", i + 1);
        if (end == String::npos) {
            end = code.size();
        }

        versionLine = code.substr(i, end - i);
        versionLine += "\n";
        if (! canonicalizeVersionLine(versionLine)) {
            alwaysAssertM(false, format("Invalid version line in \"%s\"\n", code.substr(i, end - i).c_str()));
        }

        code = code.substr(0, i) + code.substr(end); // Keep the "\n" to avoid changing line numbers
        return true;
    } else {
        // Insert #version 330
        versionLine = "#version 330\n";
        return false;
    }
    
}


void Shader::processExtensions(String& code, String& extensionLines) {

	//extensionLines = "\n";

	//Add useful exts by default.
	extensionLines = "\n#extension GL_NV_bindless_texture : enable\n";
	extensionLines += "#extension GL_ARB_bindless_texture : enable\n";
	extensionLines += "#extension GL_NV_gpu_shader5 : enable\n";
	extensionLines += "#extension GL_EXT_shader_image_load_formatted : enable\n";

	size_t i = findPragmaWithSpaces("extension", code);


	while (i != String::npos) {

		// Remove this line
		size_t end = code.find("\n", i + 1);
		if (end == String::npos) {
			end = code.size();
		}

		extensionLines += code.substr(i, end - i);
		extensionLines += "\n";

		code = code.substr(0, i) + code.substr(end); // Keep the "\n" to avoid changing line numbers


		i = findPragmaWithSpaces("extension", code);
	}
	
}


} // namespace G3D
