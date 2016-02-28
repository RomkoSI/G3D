/**
 \file license.cpp
 
 \author Morgan McGuire, http://graphics.cs.williams.edu
 
 \created 2004-04-15
 \edited  2004-05-16
*/

#include "G3D/format.h"
#include "G3D/G3DString.h"

namespace G3D {

String license() {
    return format(
"--------------------------------------------------------------------------\n"
"This program uses the G3D Innovation Engine (http://g3d.cs.williams.edu), which\n"
"is licensed under the \"Modified BSD\" Open Source license below.  The G3D Innovation Engine\n"
"source code is Copyright (C) 2000-2014, Morgan McGuire, All rights reserved.\n"
"%s\n"
"--------------------------------------------------------------------------\n"
"This program uses The OpenGL Extension Wrangler Library, which \n"
"is licensed under the \"Modified BSD\" Open Source license below.  \n"
"The OpenGL Extension Wrangler Library source code is\n"
"Copyright (C) 2002-2008, Milan Ikits <milan ikits@ieee.org>\n"
"Copyright (C) 2002-2008, Marcelo E. Magallon <mmagallo@debian.org>\n"
"Copyright (C) 2002, Lev Povalahev\n"
"All rights reserved.\n"
"-----------------------------------------------------------------------------------\n"
"This program uses the FreeImage library, which is licensed under the \n"
"FreeImage Public License - Version 1.0 (http://freeimage.sourceforge.net/freeimage-license.txt)\n"
"found in freeimage-license.txt.\n"
"-----------------------------------------------------------------------------------\n"
"This program uses the FFMPEG library, which is licensed under the \n"
"GNU Lesser General Public License, version 2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)\n"
"-----------------------------------------------------------------------------------\n"
"The Modified BSD license is below, and requires the following statement:\n"
"\n"
"Redistribution and use in source and binary forms, with or without \n"
"modification, are permitted provided that the following conditions are met:\n"
"\n"
"* Redistributions of source code must retain the above copyright notice, \n"
"  this list of conditions and the following disclaimer.\n"
"* Redistributions in binary form must reproduce the above copyright notice, \n"
"  this list of conditions and the following disclaimer in the documentation \n"
"  and/or other materials provided with the distribution.\n"
"* The name of the author may be used to endorse or promote products \n"
"  derived from this software without specific prior written permission.\n"
"\n"
"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" \n"
"AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE \n"
"IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n"
"ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE \n"
"LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR \n"
"CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF \n"
"SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\n"
"INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN\n"
"CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)\n"
"ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF\n"
"THE POSSIBILITY OF SUCH DAMAGE.\n"
"-------------------------------------------------------------------------\n"
"This program uses the Assimp library under the terms of a 3-clause BSD license\n"
"(http://assimp.sourceforge.net/main_license.html):\n"
"\n"
"Assimp source code\n"
"Copyright (c) 2006-2012 assimp team\n"
"All rights reserved.\n"
"\n"
"Redistribution and use in source and binary forms, with or without modification, are \n"
"permitted provided that the following conditions are met:\n"
"\n"
"Redistributions of source code must retain the above copyright notice, this list of \n"
"conditions and the following disclaimer.\n"
"Redistributions in binary form must reproduce the above copyright notice, this list \n"
"of conditions and the following disclaimer in the documentation and/or other materials\n"
"provided with the distribution.\n"
"Neither the name of the assimp team nor the names of its contributors may be used to \n"
"endorse or promote products derived from this software without specific prior written \n"
"permission.\n"
"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY\n"
"EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n"
"MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL\n"
"THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, \n"
"SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT\n"
"OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)\n"
"HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR\n"
"TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,\n"
"EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"
"-------------------------------------------------------------------------------------\n"
"This program uses the zlib general purpose compression library under the zlib license:\n"
"zlib source code\n"
"Copyright (C) 1995-2013 Jean-loup Gailly and Mark Adler\n"
"\n"
"This software is provided 'as-is', without any express or implied\n"
"warranty.  In no event will the authors be held liable for any damages\n"
"arising from the use of this software.\n"
"\n"
"Permission is granted to anyone to use this software for any purpose,\n"
"including commercial applications, and to alter it and redistribute it\n"
"freely, subject to the following restrictions:\n"
"\n"
"1. The origin of this software must not be misrepresented; you must not\n"
"   claim that you wrote the original software. If you use this software\n"
"   in a product, an acknowledgment in the product documentation would be\n"
"   appreciated but is not required.\n"
"2. Altered source versions must be plainly marked as such, and must not be\n"
"   misrepresented as being the original software.\n"
"3. This notice may not be removed or altered from any source distribution.\n"
"\n"
"  Jean-loup Gailly        Mark Adler\n"
"  jloup@gzip.org          madler@alumni.caltech.edu\n"
"\n\n",

#ifndef G3D_NO_FMOD
"-------------------------------------------------------------------------------------\n"
"FMOD Sound System, copyright (C) Firelight Technologies Pty, Ltd., 1994-2014.\n"
"Used under the conditions specificed in http://www.fmod.com/files/public/LICENSE.TXT\n"
#else
""
#endif
);
}

}