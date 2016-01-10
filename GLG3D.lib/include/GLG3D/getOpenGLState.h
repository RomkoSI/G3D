/**
  @file getOpenGLState.h

  @maintainer Morgan McGuire, http://graphics.cs.williams.edu
  @cite       Created by Morgan McGuire & Seth Block
  @created 2001-08-05
  @edited  2006-02-07
*/

#ifndef G3D_GETOPENGLSTATE_H
#define G3D_GETOPENGLSTATE_H

#include "G3D/platform.h"
#include "./glheaders.h"
#include "./glcalls.h"

namespace G3D {

/** 
 Returns all OpenGL state as a formatted string of C++
 code that will reproduce that state.   Leaves all OpenGL
 state in exactly the same way it found it.   Use this for 
 debugging when OpenGL doesn't seem to be in the same state 
 that you think it is in. 

  A common idiom is:
  {String s = getOpenGLState(false); debugPrintf("%s", s.c_str();}

 @param showDisabled if false, state that is not affecting
  rendering is not shown (e.g. if lighting is off,
  lighting information is not shown).
 */
String getOpenGLState(bool showDisabled=true);


/**
 Returns the number of bytes occupied by a value in
 an OpenGL format (e.g. GL_FLOAT).  Returns 0 for unknown
 formats.
 */
int sizeOfGLFormat(GLenum format);

/**
 Pretty printer for GLenums.  Useful for debugging OpenGL
 code.
 */
const char* GLenumToString(GLenum i);

}

#endif
