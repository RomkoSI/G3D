/**
   @file IconSetViewer.cpp
 
 Viewer for .icn files

 \author Morgan McGuire
 
 \created 2010-01-04
 \edited  2010-01-04
*/
#include "IconSetViewer.h"


IconSetViewer::IconSetViewer(const shared_ptr<GFont>& captionFont) : m_font(captionFont) {
}


void IconSetViewer::onInit(const G3D::String& filename) {
    m_iconSet = IconSet::fromFile(filename);
}


void IconSetViewer::onGraphics2D(RenderDevice* rd, App* app) {
    app->colorClear = Color3::white();

    rd->push2D();

    rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
    const int N = m_iconSet->size();
    const Rect2D& viewport = rd->viewport();


    // Current x position (set negative to force drawing from an offset)
    int x = 0;//-3000;//-1775;
    int y = 2;
    // Highest x position of the current column
    int x1 = x;
    const int fontSize = 8;
    G3D::String currentPath;
    for (int i = 0; i < N; ++i) {
        const Icon& icon = m_iconSet->get(i);
        const G3D::String& filename = m_iconSet->filename(i);
        const int h = max(fontSize, icon.height());

        if (y + h > viewport.height()) {
            // New column
            y = 2;
            x = x1 + 12;
        }

        const G3D::String& path = filenamePath(filename);
        if (path != currentPath) {
            // Print the path
            Vector2 p((float)x, y + 10.0f);
            p += m_font->draw2D(rd, path, p, fontSize + 2, Color3::blue(), Color4::clear());
            x1 = max(x1, iCeil(p.x));
            y = iCeil(p.y) + 1;
            currentPath = path;
        }

        Draw::rect2D(Rect2D::xywh((float)x, (float)y, (float)icon.width(), (float)icon.height()), rd, Color3::white(), icon.texture());

        Vector2 p(float(x + max(icon.width(), 32)), (float)y);
        p += m_font->draw2D(rd, filenameBaseExt(filename), p, (float)fontSize, Color3::black(), Color4::clear());
        x1 = max(x1, iCeil(p.x));
        y = max(y + icon.height() + 2, iCeil(p.y));
    }

    rd->pop2D();
}
