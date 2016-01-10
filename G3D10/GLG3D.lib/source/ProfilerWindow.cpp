/**
  \file ProfilerResultWindow.h

  \maintainer Cyril Crassin, http://www.icare3d.org

  \created 2013-07-09
  \edited  2013-07-19
*/

#include "G3D/FileSystem.h"
#include "GLG3D/ProfilerWindow.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/GuiTabPane.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Draw.h"
#include "GLG3D/IconSet.h"

namespace G3D {
static const int TREE_DISPLAY_WIDTH = 1000;

static const float HEIGHT = 15;
static const float INDENT = 16;
static const float HINT_COL = 300;
static const float CPU_COL = 600;
static const float GPU_COL = 665;
static const float LINE_COL = 750;

bool ProfilerWindow::ProfilerTreeDisplay::checkIfCollapsed(const size_t hash) const {
    return collapsedIfIncluded == m_collapsed.contains(hash);
}

void ProfilerWindow::ProfilerTreeDisplay::expandAll() {
    collapsedIfIncluded = true;
    m_collapsed.clear();
}

void ProfilerWindow::ProfilerTreeDisplay::collapseAll() {
    collapsedIfIncluded = false;
    m_collapsed.clear();
}

bool ProfilerWindow::ProfilerTreeDisplay::onEvent(const GEvent & event) {
    if (!m_visible) {
        return false;
    }
    ProfilerWindow* window = dynamic_cast<ProfilerWindow*>(this->window());
    Vector2 mousePositionDeBumped = event.mousePosition() - Vector2(window->m_scrollPane->horizontalOffset(), window->m_scrollPane->verticalOffset());
    if (event.type == GEventType::MOUSE_BUTTON_DOWN && (m_rect.contains(mousePositionDeBumped))) {
        Array< const Array<Profiler::Event>* > eventTreeArray;
        Profiler::getEvents(eventTreeArray);
        float y = 0;
        for (int t = 0; t < eventTreeArray.size(); ++t) {
            const Array<Profiler::Event>& tree = *(eventTreeArray[t]);

            for (int e = 0; e < tree.length(); ++e) {
                const Profiler::Event& profilerEvent = tree[e];
                if (Rect2D::xyxy(profilerEvent.level() * INDENT, y, float(TREE_DISPLAY_WIDTH), y + HEIGHT).contains(event.mousePosition())) {
                    if (Rect2D::xywh(profilerEvent.level() * INDENT, y, INDENT, HEIGHT).contains(event.mousePosition())) {
                        if (m_collapsed.contains(profilerEvent.hash())) {
                            m_collapsed.remove(profilerEvent.hash());
                            return true;
                        } else {
                            m_collapsed.insert(profilerEvent.hash());
                            return true;
                        }
                    } else {
                        m_selected = profilerEvent.hash();
                        return true;
                    }
                }  else if (checkIfCollapsed(profilerEvent.hash())) {
                    int level = profilerEvent.level();
                    ++e;
                    while (e < tree.length() && tree[e].level() > level) { ++e; }
                    --e;
                }
                y += HEIGHT;
            }
        }
    }
    return false;
}


ProfilerWindow::ProfilerTreeDisplay::ProfilerTreeDisplay(GuiWindow* w) : GuiControl(w) {
    m_icon = GFont::fromFile(System::findDataFile("icon.fnt"));
    shared_ptr<IconSet> iconSet = IconSet::fromFile(System::findDataFile("icon/tango.icn"));
    collapsedIfIncluded = true;
    m_collapsed.clear();
    m_selected = -1;
}


void ProfilerWindow::ProfilerTreeDisplay::render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const {

    float y = 0;

#   define SHOW_TEXT(x, t) theme->renderLabel(Rect2D::xywh(x + INDENT, y, float(TREE_DISPLAY_WIDTH), HEIGHT), (t), GFont::XALIGN_LEFT, GFont::YALIGN_BOTTOM, true, false);
    // Traverse the profile
    Array< const Array<Profiler::Event>* > eventTreeArray;
    Profiler::getEvents(eventTreeArray);

    for (int t = 0; t < eventTreeArray.size(); ++t) {
        const Array<Profiler::Event>& tree = *(eventTreeArray[t]);

        for (int e = 0; e < tree.length(); ++e) {
            const Profiler::Event& event = tree[e];
            if (m_selected == event.hash()) {
                theme->renderSelection(Rect2D::xywh(0, y, float(TREE_DISPLAY_WIDTH), HEIGHT));
            }
            if (theme->bounds(event.name()).x > HINT_COL - event.level() * INDENT) {
                float length = HINT_COL - event.level() * INDENT - INDENT;
                SHOW_TEXT(event.level() * INDENT, event.name().substr(0, theme->defaultStyle().font->wordSplitByWidth(length, event.name())) + "...")
            } else {
                SHOW_TEXT((event.level()) * INDENT, event.name())
            }

            if (theme->bounds(event.hint()).x > CPU_COL - HINT_COL) {
                float length = CPU_COL - HINT_COL;
                SHOW_TEXT(HINT_COL, event.hint().substr(0, theme->defaultStyle().font->wordSplitByWidth(length, event.hint())) + "...")
            } else {
                SHOW_TEXT(HINT_COL, event.hint())
            }
            SHOW_TEXT(CPU_COL, format("%6.3f ms", event.cpuDuration() / units::milliseconds()))
            SHOW_TEXT(GPU_COL, format("%6.3f ms", event.gfxDuration() / units::milliseconds()))
            SHOW_TEXT(LINE_COL, format("%s(%d)", FilePath::baseExt(event.file()).c_str(), event.line()))

            if (checkIfCollapsed(event.hash()) && event.numChildren() > 0) {
                theme->renderLabel(Rect2D::xywh(event.level() * INDENT,y,INDENT,HEIGHT),GuiText("4",m_icon), GFont::XALIGN_LEFT, GFont::YALIGN_BOTTOM, true, false);
                int level = event.level();
                ++e;
                while (e < tree.length() && tree[e].level() > level) { ++e; }
                --e;
            } else if (event.numChildren() > 0) {
                theme->renderLabel(Rect2D::xywh(event.level() * INDENT, y, INDENT, HEIGHT), GuiText("6", m_icon), GFont::XALIGN_LEFT, GFont::YALIGN_BOTTOM, true, false);
            }
            
            
            y += HEIGHT;
        } // e
	} // t

    // Make sure that the window is large enough.  Has to be at least the height of the containing window
    // or we aren't guaranteed to have render called again
    y = max(y, m_gui->rect().height()) + 40;
    const_cast<ProfilerWindow::ProfilerTreeDisplay*>(this)->setHeight(y);
    const_cast<GuiContainer*>(m_parent)->setHeight(y);
}


void ProfilerWindow::collapseAll() {
    m_treeDisplay->collapseAll();
}

void ProfilerWindow::expandAll() {
    m_treeDisplay->expandAll();
}

ProfilerWindow::ProfilerWindow
   (const shared_ptr<GuiTheme> &theme) : 
    GuiWindow("Profiler", 
              theme, 
              Rect2D::xywh(5, 5, TREE_DISPLAY_WIDTH + 50, 700),
              GuiTheme::NORMAL_WINDOW_STYLE,
              GuiWindow::HIDE_ON_CLOSE) {
   
    GuiPane* pane = GuiWindow::pane();

	pane->addCheckBox("Enable", Pointer<bool>(&Profiler::enabled, &Profiler::setEnabled));
    GuiButton* collapseButton = pane->addButton("Collapse All", this, &ProfilerWindow::collapseAll);
    pane->addButton("Expand All", this, &ProfilerWindow::expandAll)->moveRightOf(collapseButton);
    GuiLabel* a = pane->addLabel("Event"); a->setWidth(320);
    GuiLabel* b = pane->addLabel("Hint"); b->setWidth(300);  b->moveRightOf(a); a = b;
    b = pane->addLabel("CPU"); b->setWidth(65); b->moveRightOf(a); a = b;
    b = pane->addLabel("GPU"); b->setWidth(90); b->moveRightOf(a); a = b;
    b = pane->addLabel("File(Line)"); b->setWidth(200); b->moveRightOf(a); a = b;

    m_treeDisplay = new ProfilerTreeDisplay(this);
    m_treeDisplay->moveBy(0, -5);
    m_treeDisplay->setSize(float(TREE_DISPLAY_WIDTH), 400.0f);

    m_scrollPane = pane->addScrollPane(true, true);
    m_scrollPane->setSize(m_treeDisplay->rect().width() + 10, 400);
    m_scrollPane->viewPane()->addCustom(m_treeDisplay);
    pack();
}


shared_ptr<ProfilerWindow> ProfilerWindow::create(const shared_ptr<GuiTheme>& theme) {
    return shared_ptr<ProfilerWindow>(new ProfilerWindow(theme));
}


void ProfilerWindow::setManager(WidgetManager *manager) {
    GuiWindow::setManager(manager);
    if (manager) {
        // Move to the upper right
        ///float osWindowWidth = (float)manager->window()->width();
        ///setRect(Rect2D::xywh(osWindowWidth - rect().width(), 40, rect().width(), rect().height()));
    }
}


}
