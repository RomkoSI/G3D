#!/usr/bin/env python
import sys
import os
# Script for automatically generating ImageFormat.cpp and ImageFormat.h
# Created 2014-07-31 By Sam Donow
mypath = os.path.abspath(__file__)
mydir  = os.path.dirname(mypath)
ImageFormat_h = os.path.join(mydir, "G3D.lib", "include", "G3D", "ImageFormat.h")
ImageFormat_cpp = os.path.join(mydir, "G3D.lib", "source" , "ImageFormat.cpp")
myLastModified = os.path.getmtime(mypath)
if (os.path.exists(ImageFormat_h) and os.path.getmtime(ImageFormat_h) > myLastModified) and (os.path.exists(ImageFormat_cpp) and os.path.getmtime(ImageFormat_cpp) > myLastModified):
    sys.exit(0)
#---------------------------------------------------------------data------------------------------------------#
AllFormats = [
{"name":        "L8",          "Implemented" : True, "AlphaVersion": "LA8"             , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_LUMINANCE8",      "GL_LUMINANCE",   8, 0, 0, 0, 0, 0, 0, 8, 8,                          "GL_UNSIGNED_BYTE", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "CODE_L8", "COLOR_SPACE_NONE"]}, 
{"name":        "L16",          "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_LUMINANCE16",     "GL_LUMINANCE",  16, 0, 0, 0, 0, 0, 0, 16, 16,                        "GL_UNSIGNED_SHORT", "OPAQUE_FORMAT", "INTEGER_FORMAT", "CODE_L16", "COLOR_SPACE_NONE"]},
{"name":        "L16F",         "Implemented" : True, "AlphaVersion": "LA16F"           , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_LUMINANCE16F_ARB","GL_LUMINANCE", 16, 0, 0, 0, 0, 0, 0, 16, 16,                         "GL_HALF_FLOAT", "OPAQUE_FORMAT", "FLOATING_POINT_FORMAT", "CODE_L16F", "COLOR_SPACE_NONE"]},
{"name":        "L32F",         "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_LUMINANCE32F_ARB","GL_LUMINANCE", 32, 0, 0, 0, 0, 0, 0, 32, 32,                         "GL_FLOAT", "OPAQUE_FORMAT", "FLOATING_POINT_FORMAT", "CODE_L32F", "COLOR_SPACE_NONE"]},
{"name":        "A8",           "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_ALPHA8",          "GL_ALPHA",   0, 8, 0, 0, 0, 0, 0, 8, 8,                              "GL_UNSIGNED_BYTE", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "CODE_A8", "COLOR_SPACE_NONE"]},
{"name":        "A16",          "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_ALPHA16",         "GL_ALPHA",   0, 16, 0, 0, 0, 0, 0, 16, 16,                           "GL_UNSIGNED_SHORT", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "CODE_A16", "COLOR_SPACE_NONE"]},
{"name":        "A16F",         "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_ALPHA16F_ARB",    "GL_ALPHA",   0, 16, 0, 0, 0, 0, 0, 16, 16,                           "GL_HALF_FLOAT", "CLEAR_FORMAT", "FLOATING_POINT_FORMAT", "CODE_A16F", "COLOR_SPACE_NONE"]},
{"name":        "A32F",         "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_ALPHA32F_ARB",    "GL_ALPHA",   0, 32, 0, 0, 0, 0, 0, 32, 32,                           "GL_FLOAT", "CLEAR_FORMAT", "FLOATING_POINT_FORMAT", "CODE_A32F", "COLOR_SPACE_NONE"]},
{"name":        "LA4",          "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [2, "UNCOMP_FORMAT",     "GL_LUMINANCE4_ALPHA4",       "GL_LUMINANCE_ALPHA", 4, 4, 0, 0, 0, 0, 0, 8, 8,              "GL_UNSIGNED_BYTE", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "CODE_LA4", "COLOR_SPACE_NONE"]},
{"name":        "LA8",          "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [2, "UNCOMP_FORMAT",     "GL_LUMINANCE8_ALPHA8",       "GL_LUMINANCE_ALPHA", 8, 8, 0, 0, 0, 0, 0, 16, 16,            "GL_UNSIGNED_BYTE", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "CODE_LA8", "COLOR_SPACE_NONE"]},
{"name":        "LA16",         "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [2, "UNCOMP_FORMAT",     "GL_LUMINANCE16_ALPHA16",     "GL_LUMINANCE_ALPHA", 16, 16, 0, 0, 0, 0, 0, 16*2, 16*2,      "GL_UNSIGNED_SHORT", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "CODE_LA16", "COLOR_SPACE_NONE"]},
{"name":        "LA16F",        "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [2, "UNCOMP_FORMAT",     "GL_LUMINANCE_ALPHA16F_ARB",  "GL_LUMINANCE_ALPHA", 16, 16, 0, 0, 0, 0, 0, 16*2, 16*2,      "GL_HALF_FLOAT", "CLEAR_FORMAT", "FLOATING_POINT_FORMAT", "ImageFormat::CODE_LA16F", "ImageFormat::COLOR_SPACE_NONE"]},
{"name":        "LA32F",        "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [2, "UNCOMP_FORMAT",     "GL_LUMINANCE_ALPHA32F_ARB",  "GL_LUMINANCE_ALPHA", 32, 32,  0,  0,  0,  0,  0, 32*2, 32*2, "GL_FLOAT", "CLEAR_FORMAT", "FLOATING_POINT_FORMAT", "ImageFormat::CODE_LA32F", "ImageFormat::COLOR_SPACE_NONE"]},
{"name":        "RGB5",         "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [3, "UNCOMP_FORMAT",     "GL_RGB5",            "GL_RGBA",           0,  0,  5,  5,  5,  0,  0, 16, 16,               "GL_UNSIGNED_BYTE", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_RGB5", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGB5A1",       "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [4, "UNCOMP_FORMAT",     "GL_RGB5_A1",         "GL_RGBA",           0,  1,  5,  5,  5,  0,  0, 16, 16,               "GL_UNSIGNED_BYTE", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_RGB5A1", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGB8",         "Implemented" : True, "AlphaVersion": "RGBA8"           , "hasSRGBVersion": True , "methodData": [3, "UNCOMP_FORMAT",     "GL_RGB8",            "GL_RGB",             0,  0,  8,  8,  8,  0,  0, 32, 24,              "GL_UNSIGNED_BYTE", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_RGB8", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGB10",        "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [3, "UNCOMP_FORMAT",     "GL_RGB10",           "GL_RGB",     0,  0, 10, 10, 10,  0,  0, 32, 10*3,                    "GL_UNSIGNED_SHORT", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_RGB10", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGB10A2",      "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [4, "UNCOMP_FORMAT",     "GL_RGB10_A2",        "GL_RGBA",    0,  2, 10, 10, 10,  0,  0, 32, 32,                      "GL_UNSIGNED_INT_10_10_10_2", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_RGB10A2", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGB16",        "Implemented" : True, "AlphaVersion": "RGBA16"          , "hasSRGBVersion": False, "methodData": [3, "UNCOMP_FORMAT",     "GL_RGB16",           "GL_RGB",     0,  0, 16, 16, 16,  0,  0, 16*3, 16*3,                  "GL_UNSIGNED_SHORT", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_RGB16", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGB16F",       "Implemented" : True, "AlphaVersion": "RGBA16F"         , "hasSRGBVersion": False, "methodData": [3, "UNCOMP_FORMAT",     "GL_RGB16F_ARB",      "GL_RGB",     0,  0, 16, 16, 16,  0,  0, 16*3, 16*3,                  "GL_HALF_FLOAT", "OPAQUE_FORMAT", "FLOATING_POINT_FORMAT", "ImageFormat::CODE_RGB16F", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGB32F",       "Implemented" : True, "AlphaVersion": "RGBA32F"         , "hasSRGBVersion": False, "methodData": [3, "UNCOMP_FORMAT",     "GL_RGB32F_ARB",      "GL_RGB",     0,  0, 32, 32, 32,  0,  0, 32*3, 32*3,                  "GL_FLOAT", "OPAQUE_FORMAT", "FLOATING_POINT_FORMAT", "ImageFormat::CODE_RGB32F", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "R11G11B10F",   "Implemented" : True, "AlphaVersion": "RGBA16F"         , "hasSRGBVersion": False, "methodData": [3, "UNCOMP_FORMAT",     "GL_R11F_G11F_B10F_EXT","GL_RGB",     0,  0, 11, 11, 10, 0, 0,   32,   32,                  "GL_FLOAT", "OPAQUE_FORMAT", "FLOATING_POINT_FORMAT", "ImageFormat::CODE_R11G11B10F", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGB9E5F",      "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [3, "UNCOMP_FORMAT",     "GL_RGB9_E5_EXT",      "GL_RGB",     0,  0, 14, 14, 14, 0, 0,   32,   32,                   "GL_FLOAT", "OPAQUE_FORMAT", "FLOATING_POINT_FORMAT", "ImageFormat::CODE_RGB9E5F", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGB8I",        "Implemented" : True, "AlphaVersion": "RGBA8I"          , "hasSRGBVersion": False, "methodData": [3, "UNCOMP_FORMAT",     "GL_RGB8I_EXT",        "GL_RGB_INTEGER",     0,  0,  8,  8,  8,  0,  0, 32, 24,             "GL_BYTE", "OPAQUE_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_RGB8I", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGB8UI",       "Implemented" : True, "AlphaVersion": "RGB8UI"          , "hasSRGBVersion": False, "methodData": [3, "UNCOMP_FORMAT",     "GL_RGB8UI_EXT",       "GL_RGB_INTEGER",     0,  0,  8,  8,  8,  0,  0, 32, 24,             "GL_UNSIGNED_BYTE", "OPAQUE_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_RGB8UI", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGBA8I",       "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [4, "UNCOMP_FORMAT",     "GL_RGBA8I_EXT",       "GL_RGBA_INTEGER",    0,  8,  8,  8,  8,  0,  0, 32, 32,             "GL_BYTE", "CLEAR_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_RGBA8I", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGBA8UI",      "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [4, "UNCOMP_FORMAT",     "GL_RGBA8UI_EXT",      "GL_RGBA_INTEGER",    0,  8,  8,  8,  8,  0,  0, 32, 32,             "GL_UNSIGNED_BYTE", "CLEAR_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_RGBA8UI", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGB8_SNORM",   "Implemented" : True, "AlphaVersion": "RGBA8_SNORM"     , "hasSRGBVersion": False, "methodData": [3, "UNCOMP_FORMAT",     "GL_RGB8_SNORM",       "GL_RGB",       0,  0,  8,  8,  8,  0,  0, 32, 32,                   "GL_BYTE", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_RGB8_SNORM", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGBA8_SNORM",  "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [4, "UNCOMP_FORMAT",     "GL_RGBA8_SNORM",      "GL_RGBA",      0,  8,  8,  8,  8,  0,  0, 32, 32,                   "GL_BYTE", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_RGBA8_SNORM", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGB16_SNORM",  "Implemented" : True, "AlphaVersion": "RGBA16_SNORM"    , "hasSRGBVersion": False, "methodData": [3, "UNCOMP_FORMAT",     "GL_RGB16_SNORM",      "GL_RGB",       0,  0, 16, 16, 16,  0,  0, 64, 64,                   "GL_SHORT", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_RGB16_SNORM", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGBA16_SNORM", "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [4,"UNCOMP_FORMAT",     "GL_RGBA16_SNORM",     "GL_RGB",      0, 16, 16, 16, 16,  0,  0, 64, 64,                     "GL_SHORT", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_RGBA16_SNORM", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "ARGB8",        "Implemented" :False, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": []},
{"name":        "BGR8",         "Implemented" : True, "AlphaVersion": "BGRA8"           , "hasSRGBVersion": False, "methodData": [3, "UNCOMP_FORMAT",     "GL_RGB8",            "GL_BGR",     0,  0,  8,  8,  8,  0,  0, 32, 24,                      "GL_UNSIGNED_BYTE", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_BGR8", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "BGRA8",        "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [4, "UNCOMP_FORMAT",     "GL_RGBA8",           "GL_BGRA",    0,  8,  8,  8,  8,  0,  0, 32, 32,                      "GL_UNSIGNED_BYTE", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_BGRA8", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "R8",           "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_R8",              "GL_RED",            0,  0,  8,  0,  0,  0,  0, 8, 8,                 "GL_UNSIGNED_BYTE", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_R8", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "R8I",          "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_R8I",             "GL_RED_INTEGER",    0,  0,  8,  0,  0,  0,  0, 8, 8,                 "GL_BYTE", "OPAQUE_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_R8I", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "R8UI",         "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_R8UI",            "GL_RED_INTEGER",    0,  0,  8,  0,  0,  0,  0, 8, 8,                 "GL_UNSIGNED_BYTE", "OPAQUE_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_R8UI", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "R8_SNORM",     "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_R8_SNORM",        "GL_RED",      0,  0,  8,  0,  0,  0,  0, 8, 8,                       "GL_UNSIGNED_BYTE", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_R8_SNORM", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "R16",          "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_R16",             "GL_RED",            0,  0,  16,  0,  0,  0,  0, 16, 16,              "GL_UNSIGNED_SHORT", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_R16", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "R16I",         "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_R16I",            "GL_RED_INTEGER",    0,  0,  16,  0,  0,  0,  0, 16, 16,              "GL_SHORT", "OPAQUE_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_R16I", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "R16UI",        "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_R16UI",           "GL_RED_INTEGER",    0,  0,  16,  0,  0,  0,  0, 16, 16,              "GL_UNSIGNED_SHORT", "OPAQUE_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_R16UI", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "R16_SNORM",    "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_R16_SNORM",       "GL_RED",           0,  0,  16,  0,  0,  0,  0, 16, 16,               "GL_UNSIGNED_SHORT", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_R16_SNORM", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "R16F",         "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_R16F",            "GL_RED",            0,  0,  16, 0,  0,  0,  0, 16, 16,               "GL_HALF_FLOAT", "OPAQUE_FORMAT", "FLOATING_POINT_FORMAT", "ImageFormat::CODE_R16F", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "R32I",         "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_R32I",            "GL_RED_INTEGER",    0,  0,  32,  0,  0,  0,  0, 32, 32,              "GL_INT", "OPAQUE_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_R32I", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "R32UI",        "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_R32UI",           "GL_RED_INTEGER",    0,  0,  32,  0,  0,  0,  0, 32, 32,              "GL_UNSIGNED_INT", "OPAQUE_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_R32UI", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RG8",          "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [2, "UNCOMP_FORMAT",     "GL_RG8",             "GL_RG",             0,  0,  8,  8,  0,  0,  0, 16, 16,               "GL_UNSIGNED_BYTE", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_RG8", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RG8I",         "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [2, "UNCOMP_FORMAT",     "GL_RG8I",            "GL_RG_INTEGER",     0,  0,  8,  8,  0,  0,  0, 16, 16,               "GL_UNSIGNED_BYTE", "OPAQUE_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_RG8I", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RG8UI",        "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [2, "UNCOMP_FORMAT",     "GL_RG8UI",           "GL_RG_INTEGER",     0,  0,  8,  8,  0,  0,  0, 16, 16,               "GL_UNSIGNED_BYTE", "OPAQUE_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_RG8UI", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RG8_SNORM",    "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [2, "UNCOMP_FORMAT",     "GL_RG8_SNORM",       "GL_RG",             0,  0,  8,  8,  0,  0,  0, 16, 16,               "GL_UNSIGNED_BYTE", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_RG8_SNORM", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RG16",         "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [2, "UNCOMP_FORMAT",     "GL_RG16",            "GL_RG",             0,  0,  16,  16,  0,  0,  0, 16*2, 16*2,         "GL_UNSIGNED_SHORT", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_RG16", "ImageFormat::COLOR_SPACE_RGB"]            },
{"name":        "RG16I",        "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [2, "UNCOMP_FORMAT",     "GL_RG16I",           "GL_RG_INTEGER",     0,  0,  16,  16,  0,  0,  0, 16*2, 16*2,         "GL_SHORT", "OPAQUE_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_RG16I", "ImageFormat::COLOR_SPACE_RGB"]                                   },
{"name":        "RG16UI",       "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [2, "UNCOMP_FORMAT",     "GL_RG16UI",          "GL_RG_INTEGER",     0,  0,  16,  16,  0,  0,  0, 16*2, 16*2,         "GL_UNSIGNED_SHORT", "OPAQUE_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_RG16UI", "ImageFormat::COLOR_SPACE_RGB"]                         },
{"name":        "RG16_SNORM",   "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [2, "UNCOMP_FORMAT",     "GL_RG16_SNORM",      "GL_RG",             0,  0,  16,  16,  0,  0,  0, 16*2, 16*2,         "GL_UNSIGNED_SHORT", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_RG16_SNORM", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RG16F",        "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [2, "UNCOMP_FORMAT",     "GL_RG16F",           "GL_RG",             0,  0,  16, 16,  0,  0,  0, 32, 32,              "GL_HALF_FLOAT", "OPAQUE_FORMAT", "FLOATING_POINT_FORMAT", "ImageFormat::CODE_RG16F", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RG32I",        "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [2, "UNCOMP_FORMAT",     "GL_RG32I",           "GL_RG_INTEGER",     0,  0,  32,  32,  0,  0,  0, 32*2, 32*2,         "GL_INT", "OPAQUE_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_RG32I", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RG32UI",       "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [2, "UNCOMP_FORMAT",     "GL_RG32UI",          "GL_RG_INTEGER",     0,  0,  32,  32,  0,  0,  0, 32*2, 32*2,          "GL_UNSIGNED_INT", "OPAQUE_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_RG32UI", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "R32F",         "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_R32F",            "GL_RED",            0,  0,  32, 0,  0,  0,  0, 32, 32,             "GL_FLOAT", "OPAQUE_FORMAT", "FLOATING_POINT_FORMAT", "ImageFormat::CODE_R32F", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RG32F",        "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [2, "UNCOMP_FORMAT",     "GL_RG32F",           "GL_RG",             0,  0,  32, 32,  0,  0,  0, 64, 64,              "GL_FLOAT", "OPAQUE_FORMAT", "FLOATING_POINT_FORMAT", "ImageFormat::CODE_RG32F", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGBA8",        "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": True , "methodData": [4, "UNCOMP_FORMAT",     "GL_RGBA8",           "GL_RGBA",    0,  8,  8,  8,  8,  0,  0, 32, 32,                      "GL_UNSIGNED_BYTE", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_RGBA8", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGBA16",       "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [4, "UNCOMP_FORMAT",     "GL_RGBA16",          "GL_RGBA",    0, 16, 16, 16, 16, 0, 0, 16*4, 16*4,                    "GL_UNSIGNED_SHORT", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_RGBA16", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGBA16F",      "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [4, "UNCOMP_FORMAT",     "GL_RGBA16F_ARB",     "GL_RGBA",    0, 16, 16, 16, 16, 0, 0, 16*4, 16*4,                    "GL_HALF_FLOAT", "CLEAR_FORMAT", "FLOATING_POINT_FORMAT", "ImageFormat::CODE_RGBA16F", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGBA32F",      "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [4, "UNCOMP_FORMAT",     "GL_RGBA32F_ARB",     "GL_RGBA",    0, 32, 32, 32, 32, 0, 0, 32*4, 32*4,                    "GL_FLOAT", "CLEAR_FORMAT", "FLOATING_POINT_FORMAT", "ImageFormat::CODE_RGBA32F", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGBA16I",      "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [4, "UNCOMP_FORMAT",     "GL_RGBA16I",         "GL_RGBA_INTEGER",    0, 16, 16, 16, 16, 0, 0, 16*4, 16*4,            "GL_SHORT", "CLEAR_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_RGBA16I", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGBA16UI",     "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [4, "UNCOMP_FORMAT",     "GL_RGBA16UI",        "GL_RGBA_INTEGER",    0, 16, 16, 16, 16, 0, 0, 16*4, 16*4,            "GL_UNSIGNED_SHORT", "CLEAR_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_RGBA16UI", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGB32I",       "Implemented" : True, "AlphaVersion": "RGBA32I"         , "hasSRGBVersion": False, "methodData": [3, "UNCOMP_FORMAT",     "GL_RGB32I",          "GL_RGBA_INTEGER",    0, 32, 32, 32,  0, 0, 0, 32*3, 32*3,            "GL_INT", "OPAQUE_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_RGB32I", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGB32UI",      "Implemented" : True, "AlphaVersion": "RGBA32UI"        , "hasSRGBVersion": False, "methodData": [3, "UNCOMP_FORMAT",     "GL_RGB32UI",         "GL_RGBA_INTEGER",    0, 32, 32, 32,  0, 0, 0, 32*3, 32*3,            "GL_UNSIGNED_INT", "OPAQUE_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_RGB32UI", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGBA32I",      "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [4, "UNCOMP_FORMAT",     "GL_RGBA32I",         "GL_RGBA_INTEGER",    0, 32, 32, 32, 32, 0, 0, 32*4, 32*4,            "GL_INT", "CLEAR_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_RGBA32I", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGBA32UI",     "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [4, "UNCOMP_FORMAT",     "GL_RGBA32UI",        "GL_RGBA_INTEGER",    0, 32, 32, 32, 32, 0, 0, 32*4, 32*4,            "GL_UNSIGNED_INT", "CLEAR_FORMAT", "INTEGER_FORMAT", "ImageFormat::CODE_RGBA32UI", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGBA4",        "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [4, "UNCOMP_FORMAT",     "GL_RGBA4",           "GL_RGBA",            0,  4,  4,  4,  4,  0,  0, 16, 16,              "GL_UNSIGNED_BYTE", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_RGBA4", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGBA2",        "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [4, "UNCOMP_FORMAT",     "GL_RGBA2",           "GL_RGBA",            0,  2,  2,  2,  2,  0,  0, 8, 8,                "GL_UNSIGNED_BYTE", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_RGBA2", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "BAYER_RGGB8",  "Implemented" :False, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": []},
{"name":        "BAYER_GRBG8",  "Implemented" :False, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": []},
{"name":        "BAYER_GBRG8",  "Implemented" :False, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": []},
{"name":        "BAYER_BGGR8",  "Implemented" :False, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": []},
{"name":     "BAYER_RGGB32F",   "Implemented" :False, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": []},
{"name":     "BAYER_GRBG32F",   "Implemented" :False, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": []},
{"name":     "BAYER_GBRG32F",   "Implemented" :False, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": []},
{"name":     "BAYER_BGGR32F",   "Implemented" :False, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": []},
{"name":        "HSV8",         "Implemented" :False, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": []},
{"name":        "HSV32F",       "Implemented" :False, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": []},
{"name":    "YUV420_PLANAR",    "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [3, "UNCOMP_FORMAT",  "GL_NONE",    "GL_NONE", 0, 0, 0, 0, 0, 0, 0, 12, 12,   "GL_UNSIGNED_BYTE", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_YUV420_PLANAR", "ImageFormat::COLOR_SPACE_YUV"]},
{"name":        "YUV422",       "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [3, "UNCOMP_FORMAT",  "GL_NONE",    "GL_NONE", 0, 0, 0, 0, 0, 0, 0, 16, 16,  "GL_UNSIGNED_BYTE", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_YUV422", "ImageFormat::COLOR_SPACE_YUV"]},
{"name":        "YUV444",       "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [3, "UNCOMP_FORMAT",  "GL_NONE",    "GL_NONE", 0, 0, 0, 0, 0, 0, 0, 24, 24,  "GL_UNSIGNED_BYTE", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_YUV444", "ImageFormat::COLOR_SPACE_YUV"]},
{"name":        "RGB_DXT1",     "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": True , "methodData": [3, "COMP_FORMAT",       "GL_COMPRESSED_RGB_S3TC_DXT1_EXT",    "GL_RGB",     0, 0, 0, 0, 0, 0, 0, 64, 64,            "GL_UNSIGNED_BYTE", "OPAQUE_FORMAT", "OTHER", "ImageFormat::CODE_RGB_DXT1", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGBA_DXT1",    "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": True , "methodData": [4, "COMP_FORMAT ",      "GL_COMPRESSED_RGBA_S3TC_DXT1_EXT",   "GL_RGBA",    0, 0, 0, 0, 0, 0, 0, 64, 64,            "GL_UNSIGNED_BYTE", "CLEAR_FORMAT", "OTHER", "ImageFormat::CODE_RGBA_DXT1", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGBA_DXT3",    "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": True , "methodData": [4, "COMP_FORMAT ",      "GL_COMPRESSED_RGBA_S3TC_DXT3_EXT",   "GL_RGBA",    0, 0, 0, 0, 0, 0, 0, 128, 128,          "GL_UNSIGNED_BYTE", "CLEAR_FORMAT", "OTHER", "ImageFormat::CODE_RGBA_DXT3", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "RGBA_DXT5",    "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": True , "methodData": [4, "COMP_FORMAT ",      "GL_COMPRESSED_RGBA_S3TC_DXT5_EXT",   "GL_RGBA",    0, 0, 0, 0, 0, 0, 0, 128, 128,          "GL_UNSIGNED_BYTE", "CLEAR_FORMAT", "OTHER", "ImageFormat::CODE_RGBA_DXT5", "ImageFormat::COLOR_SPACE_RGB"]},
{"name":        "SRGB8",        "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [3, "UNCOMP_FORMAT",     "GL_SRGB8",                           "GL_RGB",                0,  0,  8,  8,  8,  0,  0, 32, 24,      "GL_UNSIGNED_BYTE", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_SRGB8", "ImageFormat::COLOR_SPACE_SRGB"]},
{"name":        "SRGBA8",       "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [4, "UNCOMP_FORMAT",     "GL_SRGB8_ALPHA8",                    "GL_RGBA",            0,  8,  8,  8,  8,  0,  0, 32, 24,      "GL_UNSIGNED_BYTE", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_SRGBA8", "ImageFormat::COLOR_SPACE_SRGB"]},
{"name":        "SL8",          "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_SLUMINANCE8",                     "GL_LUMINANCE",        8,  0,  0,  0,  0,  0,  0, 8, 8,        "GL_UNSIGNED_BYTE", "OPAQUE_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_SL8", "ImageFormat::COLOR_SPACE_SRGB"]},
{"name":        "SLA8",         "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [2, "UNCOMP_FORMAT",     "GL_SLUMINANCE8_ALPHA8",              "GL_LUMINANCE_ALPHA",    8,  8,  0,  0,  0,  0,  0, 16, 16,      "GL_UNSIGNED_BYTE", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_SLA8", "ImageFormat::COLOR_SPACE_SRGB"]},
{"name":        "SRGB_DXT1",    "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [3, "COMP_FORMAT ",      "GL_COMPRESSED_SRGB_S3TC_DXT1_EXT",       "GL_RGB",            0, 0, 0, 0, 0, 0, 0, 64, 64,    "GL_UNSIGNED_BYTE", "OPAQUE_FORMAT", "OTHER", "ImageFormat::CODE_SRGB_DXT1", "ImageFormat::COLOR_SPACE_SRGB"]},
{"name":        "SRGBA_DXT1",   "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [4, "COMP_FORMAT ",      "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT", "GL_RGBA",        0, 0, 0, 0, 0, 0, 0, 64, 64,    "GL_UNSIGNED_BYTE", "CLEAR_FORMAT", "OTHER", "ImageFormat::CODE_SRGBA_DXT1", "ImageFormat::COLOR_SPACE_SRGB"]},
{"name":        "SRGBA_DXT3",   "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [4, "COMP_FORMAT ",      "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT", "GL_RGBA",        0, 0, 0, 0, 0, 0, 0, 128, 128,  "GL_UNSIGNED_BYTE", "CLEAR_FORMAT", "OTHER", "ImageFormat::CODE_SRGBA_DXT3", "ImageFormat::COLOR_SPACE_SRGB"]},
{"name":        "SRGBA_DXT5",   "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [4, "COMP_FORMAT ",      "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT", "GL_RGBA",        0, 0, 0, 0, 0, 0, 0, 128, 128,  "GL_UNSIGNED_BYTE", "CLEAR_FORMAT", "OTHER", "ImageFormat::CODE_SRGBA_DXT5", "ImageFormat::COLOR_SPACE_SRGB"]},
{"name":        "DEPTH16",      "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_DEPTH_COMPONENT16_ARB",           "GL_DEPTH_COMPONENT", 0, 0, 0, 0, 0, 16, 0, 16, 16,   "GL_UNSIGNED_SHORT", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_DEPTH16", "ImageFormat::COLOR_SPACE_NONE"]},
{"name":        "DEPTH24",      "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_DEPTH_COMPONENT24_ARB",           "GL_DEPTH_COMPONENT", 0, 0, 0, 0, 0, 24, 0, 32, 24,   "GL_UNSIGNED_INT", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_DEPTH24", "ImageFormat::COLOR_SPACE_NONE"]},
{"name":        "DEPTH32",      "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_DEPTH_COMPONENT32_ARB",           "GL_DEPTH_COMPONENT", 0, 0, 0, 0, 0, 32, 0, 32, 32,   "GL_UNSIGNED_INT", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_DEPTH32", "ImageFormat::COLOR_SPACE_NONE"]},
{"name":        "DEPTH32F",     "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_DEPTH_COMPONENT32_ARB",           "GL_DEPTH_COMPONENT", 0, 0, 0, 0, 0, 32, 0, 32, 32,   "GL_FLOAT", "CLEAR_FORMAT", "FLOATING_POINT_FORMAT", "ImageFormat::CODE_DEPTH32F", "ImageFormat::COLOR_SPACE_NONE"]},        
{"name":        "STENCIL1",     "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_STENCIL_INDEX1_EXT",              "GL_STENCIL_INDEX",  0, 0, 0, 0, 0, 0, 1, 1, 1,      "GL_UNSIGNED_BYTE", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_STENCIL1", "ImageFormat::COLOR_SPACE_NONE"]},
{"name":        "STENCIL4",     "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_STENCIL_INDEX4_EXT",              "GL_STENCIL_INDEX",  0, 0, 0, 0, 0, 0, 4, 4, 4,      "GL_UNSIGNED_BYTE", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_STENCIL4", "ImageFormat::COLOR_SPACE_NONE"]},
{"name":        "STENCIL8",     "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_STENCIL_INDEX8_EXT",              "GL_STENCIL_INDEX",  0, 0, 0, 0, 0, 0, 8, 8, 8,      "GL_UNSIGNED_BYTE", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_STENCIL8", "ImageFormat::COLOR_SPACE_NONE"]},
{"name":        "STENCIL16",    "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [1, "UNCOMP_FORMAT",     "GL_STENCIL_INDEX16_EXT",             "GL_STENCIL_INDEX", 0, 0, 0, 0, 0, 0, 16, 16, 16,   "GL_UNSIGNED_SHORT", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_STENCIL16", "ImageFormat::COLOR_SPACE_NONE"]},
{"name":"DEPTH24_STENCIL8" ,    "Implemented" : True, "AlphaVersion": ""                , "hasSRGBVersion": False, "methodData": [2, "UNCOMP_FORMAT", "GL_DEPTH24_STENCIL8_EXT",    "GL_DEPTH_STENCIL_EXT",0, 0, 0, 0, 0, 24, 8, 32, 32,  "GL_UNSIGNED_INT_24_8", "CLEAR_FORMAT", "NORMALIZED_FIXED_POINT_FORMAT", "ImageFormat::CODE_DEPTH24_STENCIL8", "ImageFormat::COLOR_SPACE_NONE"]}
    ]
CODE_NUM = len(AllFormats)

#----------------------------------------------ImageFormat.h------------------------------------------#
code_h = """/**
  \\file G3D/ImageFormat.h

  \\maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \\created 2003-05-23
  \\edited  2014-02-20
*/

/* DO NOT MODIFY THIS FILE DIRECTLY
 *  It is automatically generated by ImageFormat.py */
#ifndef GLG3D_ImageFormat_h
#define GLG3D_ImageFormat_h

#include "G3D/platform.h"
#include "G3D/Table.h"
#include "G3D/enumclass.h"
#include "G3D/Any.h"

namespace G3D {

/** Information about common image formats.  Don't construct these;
    use the methods provided to access the const instances.
    
    For most formats, the number indicates the number of bits per
    channel and a suffix of "F" indicates floating point (following
    OpenGL conventions).  This does not hold for the YUV and DXT
    formats.

    \\sa G3D::Image, G3D::Texture, G3D::ImageConvert
*/
class ImageFormat {
public:

    // Must update ImageFormat::name() when this enum changes.
    enum Code {
        CODE_AUTO = -2,
        CODE_NONE = -1,
        CODE_L8   =  0,
"""
for format in AllFormats[1:]:
    code_h += "        CODE_{:s},\n".format(format["name"])
code_h += """
        CODE_NUM
        };
"""

code_h += """
    enum ColorSpace {
        COLOR_SPACE_NONE,
        COLOR_SPACE_RGB,
        COLOR_SPACE_HSV,
        COLOR_SPACE_YUV,
        COLOR_SPACE_SRGB
    };

    enum BayerPattern {
        BAYER_PATTERN_NONE,
        BAYER_PATTERN_RGGB,
        BAYER_PATTERN_GRBG,
        BAYER_PATTERN_GBRG,
        BAYER_PATTERN_BGGR
    };


    enum NumberFormat { 
        FLOATING_POINT_FORMAT,
        INTEGER_FORMAT,
        NORMALIZED_FIXED_POINT_FORMAT,
        OTHER // e.g. DXT
    };

    /**  Number of channels (1 for a depth texture). */
    int                 numComponents;
    bool                compressed;

    /** Useful for serializing. */
    Code                code;

    ColorSpace          colorSpace;

    /** If this is a Bayer format, what is the pattern. */
    BayerPattern        bayerPattern;

    /** The OpenGL format equivalent to this one, e.g, GL_RGB8  Zero if there is no equivalent. This is actually a GLenum */
    int                 openGLFormat;

    /** The OpenGL base format equivalent to this one (e.g., GL_RGB, GL_ALPHA).  Zero if there is no equivalent.  */
    int                 openGLBaseFormat;

    int                 luminanceBits;

    /** Number of bits per pixel storage for alpha values; Zero for compressed textures and non-RGB. */
    int                 alphaBits;
    
    /** Number of bits per pixel storage for red values; Zero for compressed textures and non-RGB. */
    int                 redBits;

    /** Number of bits per pixel storage for green values; Zero for compressed textures and non-RGB. */
    int                 greenBits;
    
    /** Number of bits per pixel storage for blue values; Zero for compressed textures  and non-RGB. */
    int                 blueBits;

    /** Number of bits per pixel */
    int                 stencilBits;

    /** Number of depth bits (for depth textures; e.g. shadow maps) */
    int                 depthBits;

    /** Amount of CPU memory per pixel when packed into an array, discounting any end-of-row padding. */
    int                 cpuBitsPerPixel;

    /**
      Amount of GPU memory per pixel on most graphics cards, for formats supported by OpenGL. This is
      only an estimate--the actual amount of memory may be different on your actual card.

     This may be greater than the sum of the per-channel bits
     because graphics cards need to pad to the nearest 1, 2, or
     4 bytes.
     */
    int                 openGLBitsPerPixel;

    /** The OpenGL bytes (type) format of the data buffer used with this texture format, 
        e.g., GL_UNSIGNED_BYTE */
    int                 openGLDataFormat;

    /** True if there is no alpha channel for this texture. */
    bool                opaque;
    
    /** True if the bit depths specified are for float formats. TODO: Remove, 
        replace with function keying off numberFormat */
    bool                floatingPoint;

    /** Indicates whether this format treats numbers as signed/unsigned integers, 
        floating point, or signed/unsigned normalized fixed point */
    NumberFormat        numberFormat;

    /** Human readable name of this format.*/
    const String& name() const;

    /** True if data in otherFormat is binary compatible */
    bool canInterpretAs(const ImageFormat* otherFormat) const;

    /** Returns ImageFormat representing the same channels as \\a
        otherFormat plus an alpha channel, all with at least the same
        precision as \\a otherFormat, or returns NULL if an equivalent
        format is unavailable.  Will return itself if already contains
        an alpha channel. */
    static const ImageFormat* getFormatWithAlpha(const ImageFormat* otherFormat);

    static const ImageFormat* getSRGBFormat(const ImageFormat* otherFormat);

    /** Takes the same values that name() returns */
    static const ImageFormat* fromString(const String& s);

	/** Provide the OpenGL format that should be used to bind to a GLSL shader image. */
	int openGLShaderImageFormat() const;


private:

    ImageFormat
    (int             numComponents,
     bool            compressed,
     int             glFormat,
     int             glBaseFormat,
     int             luminanceBits,
     int             alphaBits,
     int             redBits,
     int             greenBits,
     int             blueBits,
     int             depthBits,
     int             stencilBits,
     int             openGLBitsPerPixel,
     int             cpuBitsPerPixel,
     int             glDataFormat,
     bool            opaque,
     NumberFormat    numberFormat,
     Code            code,
     ColorSpace      colorSpace,
     BayerPattern    bayerPattern = BAYER_PATTERN_NONE);

public:
"""
for format in AllFormats:
    if format["Implemented"]:
        code_h += "    static const ImageFormat* {:s}();\n\n".format(format["name"])
code_h += """

    /**
     NULL pointer; indicates that the G3D::Texture class should choose
     either RGBA8 or RGB8 depending on the presence of an alpha channel
     in the input.
     */
    static const ImageFormat* AUTO()  { return NULL; }

    /** Returns DEPTH16, DEPTH24, or DEPTH32 according to the bits
     specified. You can use "glGetInteger(GL_DEPTH_BITS)" to match 
     the screen's format.*/
    static const ImageFormat* depth(int depthBits = 24);

    /** Returns STENCIL1, STENCIL4, STENCIL8 or STENCIL16 according to the bits
      specified. You can use "glGetInteger(GL_STENCIL_BITS)" to match 
     the screen's format.*/
    static const ImageFormat* stencil(int bits = 8);

    /** Returns R32F, RG32F, RGB32F, or RGBA32F according to the channels specified. */
    static const ImageFormat* floatFormat(int numChannels);

    /** Returns the matching ImageFormat* identified by the Code.  May return NULL
      if this format's code is reserved but not yet implemented by G3D. */
    static const ImageFormat* fromCode(ImageFormat::Code code);


    /** For use with ImageFormat::convert. */
    class BayerAlgorithm {
    public:
        enum Value { 
            NEAREST,
            BILINEAR,
            MHC,
            BEST = MHC
        };
    private:
        static const char* toString(int i, Value& v) {
            static const char* str[] = {"NEAREST", "BILINEAR", "MHC", "BEST", NULL}; 
            static const Value val[] = {NEAREST, BILINEAR, MHC, BEST};
            const char* s = str[i];
            if (s) {
                v = val[i];
            }
            return s;
        }

        Value value;

    public:

        G3D_DECLARE_ENUM_CLASS_METHODS(BayerAlgorithm);
    };

    /** Converts between arbitrary formats on the CPU.  Not all format conversions are supported or directly supported.
        Formats without direct conversions will attempt to convert through RGBA first.

        A conversion routine might only support source or destination padding or y inversion or none.
        If support is needed and not available in any of the direct conversion routines, then no conversion is done.

        YUV422 expects data in YUY2 format (Y, U, Y2, v).  Most YUV formats require width and heights that are multiples of 2.

        Returns true if a conversion was available, false if none occurred.

       \deprecated
       \sa G3D::ImageConvert
    */
    static bool convert(const Array<const void*>& srcBytes, int srcWidth, int srcHeight, const ImageFormat* srcFormat, int srcRowPadBits,
                        const Array<void*>& dstBytes, const ImageFormat* dstFormat, int dstRowPadBits,
                        bool invertY = false, BayerAlgorithm bayerAlg = BayerAlgorithm::MHC);

    /**
       Checks if a conversion between two formats is available. 
       \deprecated
       \sa G3D::ImageConvert
     */
    static bool conversionAvailable(const ImageFormat* srcFormat, int srcRowPadBits, const ImageFormat* dstFormat, int dstRowPadBits, bool invertY = false);

    /** Does this contain exactly one unorm8 component? */
    bool representableAsColor1unorm8() const;

    /** Does this contain exactly two unorm8 components? */
    bool representableAsColor2unorm8() const;

    /** Does this contain exactly three unorm8 components? */
    bool representableAsColor3unorm8() const;

    /** Does this contain exactly four unorm8 components? */
    bool representableAsColor4unorm8() const;

    /** Returns a Color4 that masks off unused components in the format, given in RGBA
        For example, the mask for R32F is (1,0,0,0), for A32F is (0,0,0,1), for RGB32F is (1,1,1,0).
        (Note that luminance is interpreted as using only the R channel, even though RGB would make more sense
        to me...)
      */
    Color4 channelMask() const;

    bool isIntegerFormat() const{
        return (numberFormat == INTEGER_FORMAT);
    }

    /** Returns true if these formats have the same components
        (possibly in different NumberFormat%s or sizes) */
    bool sameComponents(const ImageFormat* other) const;

};

typedef ImageFormat TextureFormat;

}

template <>
struct HashTrait<const G3D::ImageFormat*> {
    static size_t hashCode(const G3D::ImageFormat* key) { return reinterpret_cast<size_t>(key); }
};



#endif
"""
#----------------------------------------------ImageFormat.cpp----------------------------------------#
code_cpp = """
/**
 \\file ImageFormat.cpp
 
 \\maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
 \\created 2003-05-23
 \\edited  2014-02-20
 */
 
/* DO NOT MODIFY THIS FILE DIRECTLY
 *  It is automatically generated by ImageFormat.py
 */
#include "GLG3D/glheaders.h"
#include "G3D/ImageFormat.h"
#include "G3D/stringutils.h"

namespace G3D {

ImageFormat::ImageFormat(
    int             _numComponents,
    bool            _compressed,
    int             _glFormat,
    int             _glBaseFormat,
    int             _luminanceBits,
    int             _alphaBits,
    int             _redBits,
    int             _greenBits,
    int             _blueBits,
    int             _depthBits,
    int             _stencilBits,
    int             _openGLBitsPerPixel,
    int             _cpuBitsPerPixel,
    int             glDataFormat,
    bool            _opaque,
    NumberFormat    _numberFormat,
    Code            _code,
    ColorSpace      _colorSpace,
    BayerPattern    _bayerPattern) : 

    numComponents(_numComponents),
    compressed(_compressed),
    code(_code),
    colorSpace(_colorSpace),
    bayerPattern(_bayerPattern),
    openGLFormat(_glFormat),
    openGLBaseFormat(_glBaseFormat),
    luminanceBits(_luminanceBits),
    alphaBits(_alphaBits),
    redBits(_redBits),
    greenBits(_greenBits),
    blueBits(_blueBits),
    stencilBits(_stencilBits),
    depthBits(_depthBits),
    cpuBitsPerPixel(_cpuBitsPerPixel),
    openGLBitsPerPixel(_openGLBitsPerPixel),
    openGLDataFormat(glDataFormat),
    opaque(_opaque),
    numberFormat(_numberFormat){

    floatingPoint = (numberFormat == FLOATING_POINT_FORMAT);
    debugAssert(cpuBitsPerPixel <= openGLBitsPerPixel);
}


bool ImageFormat::sameComponents(const ImageFormat* other) const {
    return
        (numComponents == other->numComponents) &&
        ((alphaBits != 0) == (other->alphaBits != 0)) &&
        ((redBits != 0) == (other->redBits != 0)) &&
        ((greenBits != 0) == (other->greenBits != 0)) &&
        ((blueBits != 0) == (other->blueBits != 0)) &&
        ((luminanceBits != 0) == (other->luminanceBits != 0)) &&
        ((stencilBits != 0) == (other->stencilBits != 0)) &&
        ((depthBits != 0) == (other->depthBits != 0));
}


const ImageFormat* ImageFormat::depth(int depthBits) {

    switch (depthBits) {
    case 16:
        return DEPTH16();

    case 24:
        return DEPTH24();

    case 32:
        return DEPTH32();

    default:
        debugAssertM(false, "Depth must be 16, 24, or 32.");
        return DEPTH32();
    }
}


const ImageFormat* ImageFormat::stencil(int bits) {
    switch (bits) {
    case 1:
        return STENCIL1();

    case 4:
        return STENCIL4();

    case 8:
        return STENCIL8();

    case 16:
        return STENCIL16();

    default:
        debugAssertM(false, "Stencil must be 1, 4, 8 or 16.");
        return STENCIL16();
    }
}

const ImageFormat* ImageFormat::floatFormat(int numChannels) {
    switch (numChannels) {
    case 1:
        return ImageFormat::R32F(); 
    case 2: 
        return ImageFormat::RG32F(); 
    case 3: 
        return ImageFormat::RGB32F(); 
    case 4: 
        return ImageFormat::RGBA32F(); 
    default: 
        alwaysAssertM(false, "Num channels must be in [1,4]");
        return ImageFormat::RGBA32F();
    }
}
"""

code_cpp += "static const String nameArray[ImageFormat::CODE_NUM] = {\n"

i = 0
for format in AllFormats:
    if i < CODE_NUM - 1:
        code_cpp += '    "{:s}",\n'.format(format["name"])
    else:
        code_cpp += '    "{:s}"\n}};\n'.format(format["name"])
    i += 1 

code_cpp += """
const String& ImageFormat::name() const {
    debugAssert(code < CODE_NUM);
    return nameArray[code];
}


bool ImageFormat::canInterpretAs(const ImageFormat* otherFormat) const {
    if (this == otherFormat) {
        return true;
    }

    if (compressed || otherFormat->compressed) {
        return false;
    }

    if (colorSpace != otherFormat->colorSpace) {
        return false;
    }

    if (floatingPoint != otherFormat->floatingPoint) {
        return false;
    }

    if (numComponents != otherFormat->numComponents) {
        return false;
    }

    if (cpuBitsPerPixel != otherFormat->cpuBitsPerPixel) {
        return false;
    }

    if (openGLDataFormat != otherFormat->openGLDataFormat) {
        return false;
    }

    return true;
}
"""
code_cpp += """
const ImageFormat* ImageFormat::getSRGBFormat(const ImageFormat* otherFormat) {
    switch (otherFormat->code) {
"""
for format in AllFormats:
    if format["hasSRGBVersion"]:
        code_cpp += "        case CODE_{:s}:\n".format(format["name"])
        code_cpp += "            return S{:s}();\n\n".format(format["name"]) 

code_cpp += """        default:
        return otherFormat;
    }
}
"""

code_cpp += """
const ImageFormat* ImageFormat::getFormatWithAlpha(const ImageFormat* otherFormat) {
    if (! otherFormat->opaque) {
        return otherFormat;
    }

    switch (otherFormat->code) {
"""

for format in AllFormats:
    if format["AlphaVersion"] != "":
        code_cpp += "        case CODE_{:s}:\n".format(format["name"])
        code_cpp += "            return {:s}();\n".format(format["AlphaVersion"])
code_cpp += """        default:
       break;
    }

    return NULL;
}
"""

code_cpp += """
const ImageFormat* ImageFormat::fromString(const String& s) {
    if (toLower(s) == "auto") {
        return NULL;
    }

    for (int i = 0; i < CODE_NUM; ++i) {
        if (s == nameArray[i]) {
            return fromCode(ImageFormat::Code(i));
        }
    }
    return NULL;
}

"""

code_cpp += """
const ImageFormat* ImageFormat::fromCode(ImageFormat::Code code) {
    switch (code) {
"""
for format in AllFormats:
    code_cpp += "        case ImageFormat::CODE_{:s}:\n".format(format["name"])
    if format["Implemented"]:
        code_cpp += "            return ImageFormat::{:s}();\n".format(format["name"])
    else:
        code_cpp += "            // TODO\n"
        code_cpp += "            return NULL;\n"

code_cpp += """        default:
            return NULL;
    }
}
"""

code_cpp += """
bool ImageFormat::representableAsColor1unorm8() const {
    return (numComponents == 1) &&
        (cpuBitsPerPixel == 8) &&
        ((luminanceBits == 8) ||
         (redBits == 8) ||
         (alphaBits == 8));
}


bool ImageFormat::representableAsColor2unorm8() const {
    return (numComponents == 2) &&
        (cpuBitsPerPixel == 16) &&
        ((redBits == 8 && greenBits == 8) ||
         (luminanceBits == 8 && alphaBits == 8) ||
         (redBits == 8 && alphaBits == 8));
}


bool ImageFormat::representableAsColor3unorm8() const {
    return (numComponents == 3) &&
        (cpuBitsPerPixel == 24) &&
        (redBits == 8 && greenBits == 8 && blueBits == 8);
}


bool ImageFormat::representableAsColor4unorm8() const {
    return (numComponents == 4) &&
        (cpuBitsPerPixel == 32) &&
        (redBits == 8 && greenBits == 8 && blueBits == 8 && alphaBits == 8);
}


Color4 ImageFormat::channelMask() const{
    Color4 mask;
    mask.r = (redBits > 0 || luminanceBits > 0) ? 1.0f : 0.0f;
    mask.b = (blueBits > 0)                     ? 1.0f : 0.0f;
    mask.g = (greenBits > 0)                    ? 1.0f : 0.0f;
    mask.a = (alphaBits > 0)                    ? 1.0f : 0.0f;
    return mask;

}


int ImageFormat::openGLShaderImageFormat() const{
	switch(openGLFormat){
	case GL_DEPTH_COMPONENT32F:
		return GL_R32F;
	/*case GL_RGBA8: 
		return GL_R32UI;*/
	default:
		return openGLFormat;
	}
}


// Helper variables for defining texture formats

// Is opaque format (no alpha)
static const bool OPAQUE_FORMAT = true;
static const bool CLEAR_FORMAT  = false;

// Is compressed format (not raw component data)
static const bool COMP_FORMAT   = true;
static const bool UNCOMP_FORMAT = false;
"""

for format in AllFormats:
    if format["Implemented"]:
        code_cpp += """ 
    const ImageFormat* ImageFormat::"""
        code_cpp += "{:s}() {{\n".format(format["name"])
        code_cpp += "        static const ImageFormat format("
        i = 0
        for datum in format["methodData"]:
            code_cpp += str(datum) + (", " if i < len(format["methodData"]) - 1 else "")
            i += 1 
        code_cpp += ");\n        return &format;\n    }\n"   

code_cpp += """
}
"""
#----------------------------------------------------writing out----------------------------------------#

with open(ImageFormat_h, "w+") as IF_h:
    IF_h.write(code_h)
with open(ImageFormat_cpp, "w+") as IF_cpp:
    IF_cpp.write(code_cpp)
sys.stdout.write("Updated ImageFormat")