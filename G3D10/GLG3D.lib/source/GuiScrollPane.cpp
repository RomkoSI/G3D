/**
 \file GLG3D/GuiScrollPane.h

 \created
 \edited  

 Copyright 2000-2015, Morgan McGuire, http://graphics.cs.williams.edu
 All rights reserved.
 */
#include "GLG3D/GuiPane.h"
#include "GLG3D/GuiScrollPane.h"
#include "GLG3D/RenderDevice.h"

namespace G3D {

    static const float MIN_WINDOW_SIZE = 100;

GuiScrollPane::GuiScrollPane(GuiContainer* parent, bool verticalScroll, bool horizontalScroll, GuiTheme::ScrollPaneStyle style) : 
    GuiContainer(parent, ""), m_style(style), horizontalEnabled(horizontalScroll), verticalEnabled(verticalScroll) {
    
    m_viewPane            = new GuiPane(this, "", m_rect, GuiTheme::NO_PANE_STYLE);

    m_verticalScrollBar = NULL;
    if(verticalEnabled) {
        m_verticalScrollBar   = new GuiScrollBar(this, &m_verticalOffset, 0, m_rect.height(), true, this);
    }

    m_horizontalScrollBar = NULL;
    if(horizontalEnabled) {
        m_horizontalScrollBar = new GuiScrollBar(this, &m_horizontalOffset, 0, m_rect.width(), false, this);
    }

    setRect(Rect2D::xywh(m_rect.x0y0(), Vector2(MIN_WINDOW_SIZE, MIN_WINDOW_SIZE)));
    m_verticalOffset = 0;
    m_horizontalOffset = 0;
}   

void GuiScrollPane::findControlUnderMouse(Vector2 mouse, GuiControl*& control) {
    if (! m_rect.contains(mouse) || ! m_visible) {
        return;
    } 

    m_viewPane->findControlUnderMouse(mouse - m_rect.x0y0() + Vector2(m_horizontalOffset, m_verticalOffset * 0), control);     
    if (verticalEnabled) { 
        m_verticalScrollBar->findControlUnderMouse(mouse - m_rect.x0y0(), control);
    } 
    if (horizontalEnabled) {
        m_horizontalScrollBar->findControlUnderMouse(mouse - rect().x0y0(), control);
    }
}


void GuiScrollPane::setRect(const Rect2D& rect) {
    float scrollBarWidth = theme()->scrollBarWidth();
    float borderWidth = scrollBarStartBump();

    float verticalEnabledOffset = verticalEnabled ? scrollBarWidth : 0;

    float horizontalEnabledOffset = horizontalEnabled ? scrollBarWidth : 0;

   if (m_viewPane->rect().width() < m_rect.width() - verticalEnabledOffset) {
        horizontalEnabledOffset = 0;
    }

    if (m_viewPane->rect().height() < m_rect.height() - horizontalEnabledOffset) {
        verticalEnabledOffset = 0;
    }

    m_rect = rect; Rect2D::xywh(rect.x0() - verticalEnabledOffset, rect.y0() - horizontalEnabledOffset, rect.width() + verticalEnabledOffset, rect.width() + horizontalEnabledOffset);
    m_clientRect = m_rect;
    if (verticalEnabled) {
        m_verticalScrollBar->setRect(Rect2D::xywh(m_rect.width() - scrollBarWidth, borderWidth, scrollBarWidth, m_rect.height() - scrollBarDimesionsBump() - horizontalEnabledOffset));
        m_verticalScrollBar->setExtent(m_rect.height() - horizontalEnabledOffset);
    }
    if (horizontalEnabled) {
        m_horizontalScrollBar->setRect(Rect2D::xywh(borderWidth, m_rect.height() - scrollBarWidth, m_rect.width() - scrollBarDimesionsBump() - verticalEnabledOffset, scrollBarWidth));
        m_horizontalScrollBar->setExtent(m_rect.width() - verticalEnabledOffset);
     }
}

void GuiScrollPane::pack() {
    float scrollBarWidth = theme()->scrollBarWidth();
    m_viewPane->setPosition(0 , 0);
    m_viewPane->pack();
    Rect2D viewRect = m_viewPane->rect();
    if (verticalEnabled) {
        if (viewRect.width() > MIN_WINDOW_SIZE) {
            setWidth(viewRect.width() + scrollBarWidth + borderDimensionsBump());
        }
    }
    if (horizontalEnabled) {
        if(viewRect.height() < m_rect.height() && viewRect.height() > MIN_WINDOW_SIZE) {
            setHeight(viewRect.height() + scrollBarWidth + borderDimensionsBump());
        }
     }
}


void GuiScrollPane::render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const {
    if (! m_visible) {
        return;
    }
    
    float scrollBarWidth   = theme->scrollBarWidth();
    float borderDimensions = borderDimensionsBump();
    float borderWidth      = theme->textBoxBorderWidth();

    bool verticalNeeded = true;
    float verticalEnabledOffset = verticalEnabled ? scrollBarWidth : 0;

    bool horizontalNeeded = true;
    float horizontalEnabledOffset = horizontalEnabled ? scrollBarWidth : 0;

   if (m_viewPane->rect().width() < m_rect.width() - verticalEnabledOffset) {
        horizontalEnabledOffset = 0;
        horizontalNeeded = false;
    }

    if (m_viewPane->rect().height() < m_rect.height() - horizontalEnabledOffset) {
        verticalEnabledOffset = 0;
        verticalNeeded = false;
    }

    m_viewPane->setPosition(-m_horizontalOffset, -m_verticalOffset);
    if (verticalEnabled && verticalNeeded) {
        m_verticalScrollBar->setExtent(m_rect.height() - borderDimensions - verticalEnabledOffset);
        m_verticalScrollBar->setMax(m_viewPane->rect().height());
    }

    if (horizontalEnabled && horizontalNeeded) {
        m_horizontalScrollBar->setExtent(m_rect.width() - borderDimensions - horizontalEnabledOffset);
        m_horizontalScrollBar->setMax(m_viewPane->rect().width());
    }
    theme->pushClientRect(m_rect); {
        if (m_style == GuiTheme::BORDERED_SCROLL_PANE_STYLE) {
            theme->renderTextBoxBorder(Rect2D::xywh(0, 0, m_rect.width() - verticalEnabledOffset, m_rect.height() - horizontalEnabledOffset), m_enabled && ancestorsEnabled, false);
        }

        if (verticalEnabled && verticalNeeded) {
            m_verticalScrollBar->render(rd, theme, ancestorsEnabled && m_enabled);
        }

        if (horizontalEnabled && horizontalNeeded) {
            m_horizontalScrollBar->render(rd, theme, ancestorsEnabled && m_enabled);
        }
    } theme->popClientRect();

    theme->pushClientRect(Rect2D::xywh(m_rect.x0() + borderWidth, m_rect.y0() + borderWidth, m_rect.width() - borderDimensions - verticalEnabledOffset, m_rect.height() - borderDimensions - horizontalEnabledOffset)); {
        m_viewPane->render(rd, theme, ancestorsEnabled && m_enabled);
    } theme->popClientRect();


}

}
