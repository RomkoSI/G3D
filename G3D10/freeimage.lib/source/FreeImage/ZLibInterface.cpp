// ==========================================================
// ZLib library interface
//
// Design and implementation by
// - Floris van den Berg (flvdberg@wxs.nl)
//
// This file is part of FreeImage 3
//
// COVERED CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS, WITHOUT WARRANTY
// OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION, WARRANTIES
// THAT THE COVERED CODE IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE
// OR NON-INFRINGING. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE COVERED
// CODE IS WITH YOU. SHOULD ANY COVERED CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT
// THE INITIAL DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY NECESSARY
// SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL
// PART OF THIS LICENSE. NO USE OF ANY COVERED CODE IS AUTHORIZED HEREUNDER EXCEPT UNDER
// THIS DISCLAIMER.
//
// Use at your own risk!
// ==========================================================

#include "zlib.h"
#include "FreeImage.h"
#include "Utilities.h"
//#include "zutil.h"	/* must be the last header because of error C3163 in VS2008 (_vsnprintf defined in stdio.h) */

/**
Compresses a source buffer into a target buffer, using the ZLib library. 
Upon entry, target_size is the total size of the destination buffer, 
which must be at least 0.1% larger than source_size plus 12 bytes. 

@param target Destination buffer
@param target_size Size of the destination buffer, in bytes
@param source Source buffer
@param source_size Size of the source buffer, in bytes
@return Returns the actual size of the compressed buffer, returns 0 if an error occured
@see FreeImage_ZLibUncompress
*/
DWORD DLL_CALLCONV 
FreeImage_ZLibCompress(BYTE *target, DWORD target_size, BYTE *source, DWORD source_size) {
	uLongf dest_len = (uLongf)target_size;

	int zerr = compress(target, &dest_len, source, source_size);
	switch(zerr) {
		case Z_MEM_ERROR:	// not enough memory
		case Z_BUF_ERROR:	// not enough room in the output buffer
			FreeImage_OutputMessageProc(FIF_UNKNOWN, "Zlib error : %s", zError(zerr));
			return 0;
		case Z_OK:
			return dest_len;
	}

	return 0;
}

/**
Decompresses a source buffer into a target buffer, using the ZLib library. 
Upon entry, target_size is the total size of the destination buffer, 
which must be large enough to hold the entire uncompressed data. 
The size of the uncompressed data must have been saved previously by the compressor 
and transmitted to the decompressor by some mechanism outside the scope of this 
compression library.

@param target Destination buffer
@param target_size Size of the destination buffer, in bytes
@param source Source buffer
@param source_size Size of the source buffer, in bytes
@return Returns the actual size of the uncompressed buffer, returns 0 if an error occured
@see FreeImage_ZLibCompress
*/
DWORD DLL_CALLCONV 
FreeImage_ZLibUncompress(BYTE *target, DWORD target_size, BYTE *source, DWORD source_size) {
	uLongf dest_len = (uLongf)target_size;

	int zerr = uncompress(target, &dest_len, source, source_size);
	switch(zerr) {
		case Z_MEM_ERROR:	// not enough memory
		case Z_BUF_ERROR:	// not enough room in the output buffer
		case Z_DATA_ERROR:	// input data was corrupted
			FreeImage_OutputMessageProc(FIF_UNKNOWN, "Zlib error : %s", zError(zerr));
			return 0;
		case Z_OK:
			return dest_len;
	}

	return 0;
}

/**
Update a running crc from source and return the updated crc, using the ZLib library.
If source is NULL, this function returns the required initial value for the crc.

@param crc Running crc value
@param source Source buffer
@param source_size Size of the source buffer, in bytes
@return Returns the new crc value
*/
DWORD DLL_CALLCONV 
FreeImage_ZLibCRC32(DWORD crc, BYTE *source, DWORD source_size) {

    return crc32(crc, source, source_size);
}
