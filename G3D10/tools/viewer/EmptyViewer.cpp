/**
 \file EmptyViewer.cpp
 
 If the user tried to load a file that wasn't valid, this default viewer is displayed
 
 \maintainer Morgan McGuire
 \author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 \created 2007-05-31
 \edited  2014-10-01
 */
#include "EmptyViewer.h"

EmptyViewer::EmptyViewer(){}


void EmptyViewer::onInit(const G3D::String& filename) {}


void EmptyViewer::onGraphics2D(RenderDevice* rd, App* app) {
    screenPrintf("G3D Asset Viewer");
    screenPrintf("\n");
    screenPrintf("\n");
    screenPrintf("Built " __TIME__ " " __DATE__);
    screenPrintf("%s", System::version().c_str());
    screenPrintf("http://g3d.codeplex.com");
    screenPrintf("\n");
    screenPrintf("\n");
    screenPrintf("Drag and drop a media file here to view it.\n");
    screenPrintf("\n");
    screenPrintf("Image Formats:    png, jpg, bmp, tga, pcx, dds, psd, cut, exr, hdr, iff, mng, tiff, xbm,");
    screenPrintf("                  xpm, pfm, pict, jbig, ppm, ico, gif, (+ cube maps...just drop one face)");
    screenPrintf("\n");
    screenPrintf("3D Formats:       obj, fbx, dae, 3ds, pk3, md2, md3, bsp, off, ply, ply2, ifs, stl, stla,");
    screenPrintf("                  ArticulatedModel.Any");
    screenPrintf("\n");
    screenPrintf("Material Formats: Material.Any");
    screenPrintf("\n");
    screenPrintf("GUI Formats:      fnt, gtm");
#   ifndef G3D_NO_FFMPEG
    screenPrintf("Video Formats: mp4, avi, mp4, mov, ogg, asf, wmv");
#   endif
    screenPrintf("\n");
    screenPrintf("Press 'V' to view G3D::OSWindow events");
}
