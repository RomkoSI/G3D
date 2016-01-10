#!/bin/bash          
SRC=/Users/michaelmara/Desktop/boost_1_53_0/
DST=/Users/michaelmara/g3d/G3D9/assimp.lib/code/

cp $SRC$1 $DST$1
/usr/bin/clang++ -msse2 -D_DEBUG -g -D__cdecl= -D__stdcall= -D__fastcall= -Qunused-arguments -fasm-blocks -msse2 -msse3 -pipe -c -I include/assimp -I include -I code/ -I /usr/X11R6/include/ -I /Users/michaelmara/g3d/G3D9/build/include/ /Users/michaelmara/g3d/G3D9/assimp.lib/code/3DSConverter.cpp 