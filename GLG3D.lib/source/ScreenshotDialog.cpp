/**
\file ScreenshotDialog.cpp

\maintainer Morgan McGuire, http://graphics.cs.williams.edu

\created 2007-10-01
\edited  2014-07-19

 G3D Innovation Engine
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */
#include "G3D/svn_info.h"
#include "G3D/svnutils.h"
#include "GLG3D/ScreenshotDialog.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/GuiTabPane.h"
#include "G3D/FileSystem.h"
#include "G3D/fileutils.h"
#include "G3D/Journal.h"
#include <time.h>

#ifdef G3D_WINDOWS
#   include <sys/timeb.h>
#else
#	include <sys/time.h>
#endif

namespace G3D {

ScreenshotDialog::ScreenshotDialog(OSWindow* osWindow, const shared_ptr<GuiTheme>& theme, const String& journalDirHint, const bool flip) : 
    GuiWindow("", theme, Rect2D::xywh(400, 100, 10, 10), GuiTheme::DIALOG_WINDOW_STYLE, HIDE_ON_CLOSE), 
    ok(false), m_flip(flip), m_osWindow(osWindow), m_location(APPEND), 
    m_addToSVN(false) {

    static int projectRevision = getSVNRepositoryRevision(FileSystem::currentDirectory());

    GuiPane* rootPane = pane();

    GuiTabPane* tabPane = pane()->addTabPane(&m_currentTab);
    GuiPane* journalPane = tabPane->addTab("Journal");
    GuiPane* filePane = tabPane->addTab("File Only");

    const String& date = System::currentDateString();
    m_journalFilename = Journal::findJournalFile(journalDirHint);

    String currentEntryName;

    if (! m_journalFilename.empty()) {
        currentEntryName = Journal::firstSectionTitle(m_journalFilename);
    }
    GuiRadioButton* appendButton = journalPane->addRadioButton("Append to `" + currentEntryName + "'", APPEND, &m_location);
    (void)appendButton;

    journalPane->beginRow();

    m_newEntryName = date + ": ???";
    GuiRadioButton* newEntryButton = journalPane->addRadioButton("New entry", NEW, &m_location);
    newEntryButton->setWidth(90);
    GuiTextBox* newEntryNameBox = journalPane->addTextBox(GuiText(), &m_newEntryName);
    newEntryNameBox->setWidth(400);
    journalPane->endRow();

	m_caption = "";
    GuiTextBox* captionBox = journalPane->addTextBox("Caption", &m_caption);
    captionBox->setCaptionWidth(90);
    captionBox->moveBy(0, 15);
    captionBox->setWidth(490);

    journalPane->beginRow(); {
        journalPane->addLabel("Explanation")->setWidth(90);
        GuiTextBox* discussionBox = journalPane->addTextBox("", &m_discussion);
        discussionBox->setSize(400, 100);
    } journalPane->endRow();



    journalPane->beginRow(); {
        GuiCheckBox* box = journalPane->addCheckBox("Add to SVN", &m_addToSVN);
        box->setEnabled(G3D::hasCommandLineSVN() && (projectRevision >= 0));
        m_addToSVN = G3D::hasCommandLineSVN() && (projectRevision >= 0);
    } journalPane->endRow();

    journalPane->addLabel("Include in filename:");

    journalPane->beginRow(); {
        static bool showDate = true;
        static String filenameDate = date;
        
        GuiControl* c = journalPane->addCheckBox("Date", &showDate);
        c->setWidth(150);
        c->moveBy(15, 0);
        journalPane->addTextBox("", &filenameDate)->setWidth(130);
    } journalPane->endRow();

    journalPane->beginRow(); {
        static bool showG3DSVNRevision = true;
        static int g3dRevision = SVN_REVISION_NUMBER;
        
        GuiControl* c = journalPane->addCheckBox("G3D SVN Revision", &showG3DSVNRevision);
        c->setWidth(150);
        c->moveBy(15, 0);
        GuiNumberBox<int>* n = journalPane->addNumberBox("", &g3dRevision);
        n->setUnitsSize(0);
        n->setWidth(130);
    } journalPane->endRow();

    journalPane->beginRow(); {
        static bool showProjectSVNRevision = (projectRevision >= 0);

        GuiControl* c = journalPane->addCheckBox("Project SVN Revision", &showProjectSVNRevision);
        c->setWidth(150);
        c->moveBy(15, 0);
        c->setEnabled(projectRevision >= 0);

        GuiNumberBox<int>* n = journalPane->addNumberBox("", &projectRevision);
        n->setUnitsSize(0);
        n->setWidth(130);
    } journalPane->endRow();


    journalPane->pack();

    if (beginsWith(currentEntryName, date)) {
        // It is the same day as the previous entry, so default to appending to it
        m_location = APPEND;
    } else {
        m_location = NEW;
    }

    textBox = filePane->addTextBox("Filename", &m_filename, GuiTextBox::IMMEDIATE_UPDATE);
    textBox->setSize(Vector2(min(osWindow->width() - 100.0f, 500.0f), textBox->rect().height()));
    textBox->setFocused(true);

    filePane->addButton("...", [this]() {FileDialog::getFilename(m_filename, "jpg"); });
    tabPane->pack();


    m_textureBox = rootPane->addTextureBox(NULL, shared_ptr<Texture>(), true, m_flip);
    m_textureBox->setSize(256, 256 * min(3.0f, float(osWindow->height()) / osWindow->width()));
    m_textureBox->moveRightOf(tabPane);
    m_textureBox->moveBy(0, 18);

    cancelButton = rootPane->addButton("Cancel");
    cancelButton->setPosition(tabPane->rect().x1y1() + Vector2(50, -25));
    okButton     = rootPane->addButton("Ok");
    okButton->moveRightOf(cancelButton);

    if (! m_journalFilename.empty()) {
        m_currentTab = JOURNAL_TAB;
        captionBox->setFocused(true);
    } else {
        journalPane->setEnabled(false);
        m_currentTab = FILE_ONLY_TAB;
        okButton->setFocused(true);
    }
    
    pack();
    moveToCenter();
}


String ScreenshotDialog::nextFilenameBase(const String& prefix) {
    const String& g3dRevisionString = ((SVN_REVISION_NUMBER != 0) ? format("_g3d_r%d", SVN_REVISION_NUMBER) : "");
    const int projectRevision = getSVNRepositoryRevision(FileSystem::currentDirectory());
    const String& projectRevisionString = projectRevision < 0 ? "" : format("_r%d", projectRevision);
    return generateFileNameBaseAnySuffix(prefix) + "_" + System::appName() + projectRevisionString + g3dRevisionString;
}


bool ScreenshotDialog::getFilename(String& filename, bool& addToSVN, const String& caption, const shared_ptr<Texture>& texture) {
    setCaption(caption);
    m_textureBox->setTexture(texture);
    m_textureBox->setVisible(notNull(texture));
    m_filename = filename;

    showModal(m_osWindow);

    if (ok) {
        filename = m_filename;
        addToSVN = m_addToSVN;
    }
    return ok;
}


void ScreenshotDialog::close() {
    setVisible(false);
    m_manager->remove(dynamic_pointer_cast<Widget>(shared_from_this()));
}



void ScreenshotDialog::onCommit() {
    if (m_currentTab == JOURNAL_TAB) {
        // Create a filename:
        const String& path = FilePath::parent(m_filename);
        const String& ext  = FilePath::ext(m_filename);
        const String& s    = FilePath::base(m_filename) + "__" + m_caption;

        m_filename         = FilePath::concat(path, FilePath::makeLegalFilename(s) + "." + ext);
        
        const String& text = Journal::formatImage(m_journalFilename, m_filename, m_caption, wordWrap(m_discussion, 80));
        
        if (m_location == APPEND) {
            Journal::appendToFirstSection(m_journalFilename, text);
        } else {
            debugAssert(m_location == NEW);
            Journal::insertNewSection(m_journalFilename, m_newEntryName, text);
        }

        debugPrintf("Added image thumbnail to %s\n", m_journalFilename.c_str());
    }
}


bool ScreenshotDialog::onEvent(const GEvent& e) {
    const bool handled = GuiWindow::onEvent(e);

    // Check after the (maybe key) event is processed normally
    okButton->setEnabled(trimWhitespace(m_filename) != "");

    if (handled) {
        return true;
    }

    if ((e.type == GEventType::GUI_ACTION) && 
        ((e.gui.control == cancelButton) ||
         (e.gui.control == textBox) ||
         (e.gui.control == okButton))) {
        ok = (e.gui.control != cancelButton);
        
        if (ok) {
            onCommit();
        }

        close();
        return true;

    } else if ((e.type == GEventType::KEY_DOWN) && (e.key.keysym.sym == GKey::ESCAPE)) {
        // Cancel the dialog
        ok = false;
        close();
        return true;
    }

    return false;
}

} // namespace G3D
