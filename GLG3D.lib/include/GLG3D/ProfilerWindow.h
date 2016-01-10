/**
  \file ProfilerResultWindow.h

  \maintainer Cyril Crassin, http://www.icare3d.org

  \created 2013-07-09
  \edited  2013-07-19
*/
#ifndef GLG3D_ProfilerWindow_h
#define GLG3D_ProfilerWindow_h

#include "G3D/platform.h"
#include "GLG3D/Widget.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiLabel.h"
#include "GLG3D/GuiTextureBox.h"
#include "GLG3D/GuiDropDownList.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/Profiler.h"


namespace G3D {


/**
 \sa G3D::DeveloperWindow, G3D::GApp
 */
class ProfilerWindow : public GuiWindow {
protected:

    /** Inserted into the scroll pane */
    class ProfilerTreeDisplay : public GuiControl {
    public:

        bool collapsedIfIncluded;

        bool checkIfCollapsed(const size_t hash) const;

        void expandAll();

        void collapseAll();

        shared_ptr<GFont> m_icon;
        Set<size_t> m_collapsed;
        
        size_t m_selected;

        virtual bool onEvent(const GEvent& event) override;

        ProfilerTreeDisplay(GuiWindow* w);

        virtual void render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const override;
    };

    GuiScrollPane*              m_scrollPane;
    ProfilerTreeDisplay*        m_treeDisplay;

    void collapseAll();

    void expandAll();

    ProfilerWindow(const shared_ptr<GuiTheme>& theme);

public:

    virtual void setManager(WidgetManager* manager);

    static shared_ptr<ProfilerWindow> create(const shared_ptr<GuiTheme>& theme);

};

} // G3D

#endif
