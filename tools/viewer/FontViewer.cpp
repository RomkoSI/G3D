/**
   @file FontViewer.cpp
 
   Viewer for .fnt files, with a default font displayed for comparison
 
   @maintainer Eric Muller 09edm@williams.edu
   @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
   @created 2007-05-31
   @edited  2014-10-08
*/
#include "FontViewer.h"


FontViewer::FontViewer(shared_ptr<GFont> base) :
    m_fontfilename(""),
    m_basefont(base)
{}


void FontViewer::onInit(const String& filename) {
    m_fontfilename = filename;
    m_font = GFont::fromFile( m_fontfilename );
}


void FontViewer::onGraphics2D(RenderDevice* rd, App* app) {
    app->colorClear = Color3::white();

    rd->push2D();

    Rect2D windowBounds = rd->viewport();
    SlowMesh slowMesh(PrimitiveType::LINES);
    slowMesh.setColor(Color3::black());
    for (int i = 0; i <= 16; ++i) {
        // Horizontal line
        float y = i * windowBounds.height() / 16;
        slowMesh.makeVertex(Vector2(0, y));
        slowMesh.makeVertex(Vector2(windowBounds.width(), y));

        // Vertical line
        float x = i * windowBounds.width() / 16;
        slowMesh.makeVertex(Vector2(x, 0));
        slowMesh.makeVertex(Vector2(x, windowBounds.height()));
    }
    slowMesh.render(rd);

    double  size			= windowBounds.height()/16.0 /2.0 ;
    const Color4  color		= Color3::black();
    const Color4 outline	= Color4::clear();
    for (int y = 0; y < 16; ++y ) {
        for (int x = 0; x < 16; ++x ) {
            int i = x + y*16;
            G3D::String s = "";
            s += (char)i;

            // Character in the current font
            m_font->draw2D( rd, s, Vector2( x* windowBounds.width()/16.0f + windowBounds.width()/32.0f, 
                                            y*windowBounds.height()/16.0f + windowBounds.height()/32.0f), 
                            (float)size, color, outline, GFont::XALIGN_CENTER, GFont::YALIGN_CENTER ); 
			
            // Character in the normal font
            m_basefont->draw2D( rd, s, Vector2( x* windowBounds.width()/16.0f + windowBounds.width()/64.0f, 
                                                y* windowBounds.height()/16.0f + windowBounds.height()/20.0f), 
                                (float)size/2.0f, color, outline, GFont::XALIGN_CENTER, GFont::YALIGN_CENTER ); 

            // Character number
            m_basefont->draw2D( rd, format("\\x%x", i), Vector2( x* windowBounds.width()/16.0f + windowBounds.width()/20.0f-(float)size * 0.5f, 
                                                                 y* windowBounds.height()/16.0f + windowBounds.height()/20.0f), 
                                (float)size/2.0f, color, outline, GFont::XALIGN_CENTER, GFont::YALIGN_CENTER );

        }
    }


    rd->pop2D();
}
