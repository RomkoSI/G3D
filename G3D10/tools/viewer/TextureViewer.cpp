/**
 @file TextureViewer.cpp
 
 Viewer for image files
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#include "TextureViewer.h"

static bool allCubeMapFacesExist(const G3D::String& base, const G3D::String& ext, G3D::String& wildcardBase) {

	CubeMapConvention matchedConv = CubeMapConvention::G3D;
	bool foundMatch = false;

	// Check of filename ends in one of the convensions
	for (int convIndex = 0; convIndex < CubeMapConvention::COUNT; ++convIndex) {
		const CubeMapConvention::CubeMapInfo& info = Texture::cubeMapInfo(static_cast<CubeMapConvention>(convIndex));

		for (int faceIndex = 0; faceIndex < 6; ++faceIndex) {
			if (endsWith(base, info.face[faceIndex].suffix)) {
				foundMatch = true;
				matchedConv = static_cast<CubeMapConvention>(convIndex);
				wildcardBase = base.substr(0, base.length() - info.face[faceIndex].suffix.length());
				break;
			}
		}
	}

	// Texture isn't the start of a cube map set at all
	if (! foundMatch) {
		return false;
	}

    bool success = true;
	const CubeMapConvention::CubeMapInfo& info = Texture::cubeMapInfo(matchedConv);


	// See if all faces exist
	for (int faceIndex = 0; faceIndex < 6; ++faceIndex) {
		if (! FileSystem::exists(wildcardBase + info.face[faceIndex].suffix + "." + ext)) {
			success = false;
			break;
		}
    }

	return success;
}


TextureViewer::TextureViewer() :
    m_width(0),
    m_height(0), 
    m_isSky(false) {

}


void TextureViewer::onInit(const G3D::String& filename) {

	// Determine if texture is part of a cubemap set
	const G3D::String& path = FilePath::parent(filename);
	const G3D::String& base = FilePath::base(filename);
    const G3D::String& ext = FilePath::ext(filename);

	G3D::String wildcardBase;
	if (allCubeMapFacesExist(FilePath::concat(path, base), ext, wildcardBase)) {
		m_isSky = true;

		m_texture = Texture::fromFile(wildcardBase + "*." + ext, ImageFormat::AUTO(), Texture::DIM_CUBE_MAP);
	} else {
		m_texture = Texture::fromFile( filename, ImageFormat::AUTO(), Texture::DIM_2D, false);
		m_height = m_texture->height();
		m_width = m_texture->width();
	}
}


void TextureViewer::onGraphics2D(RenderDevice* rd, App* app) {
	if (! m_isSky) {
        screenPrintf("(Rendered with gamma=1.0 and no post-processing)");
        screenPrintf("Width: %d",  m_width);
        screenPrintf("Height: %d",  m_height);
            
        // Creates a rectangle the size of the window.
		float windowHeight = rd->viewport().height();
		float windowWidth = rd->viewport().width();
			
		Rect2D rect;
		if ((windowWidth > m_width) && (windowHeight > m_height)) {
			// creates a rectangle the size of the texture
			// centered in the window
			rect = Rect2D::xywh(windowWidth/2 - m_width/2,
								windowHeight/2 - m_height/2,
								(float)m_width,
								(float)m_height);
		} else {
			// Creates a rectangle the size of the texture
			// in the top left corner of the window, if the window
			// is smaller than the image currently
			rect = m_texture->rect2DBounds();
		}

        Draw::rect2D(rect, rd, Color3::white(), m_texture);
    }
}


void TextureViewer::onGraphics3D(RenderDevice* rd, App* app, const shared_ptr<LightingEnvironment>& lighting, Array<shared_ptr<Surface> >& surfaceArray) {
    if (m_isSky) {
        screenPrintf("(Rendered with gamma encoding and post-processing)");
        Draw::skyBox(rd, m_texture);
    }
}
