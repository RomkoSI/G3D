/**
  \file GLG3D/ScreenshotDialog.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2007-10-01
  \edited  2014-01-24
 
 G3D Innovation Engine
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */
#ifndef G3D_ScreenshotDialog_h
#define G3D_ScreenshotDialog_h

#include "G3D/platform.h"
#include "GLG3D/Widget.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/GuiTextBox.h"
#include "GLG3D/FileDialog.h"


namespace G3D {

class GuiTextureBox;

class ScreenshotDialog : public GuiWindow {
protected:
    friend class VideoRecordDialog;

    bool                ok;

    bool                m_flip;

    GuiButton*          okButton;
    GuiButton*          cancelButton;
    GuiTextBox*         textBox;

    String              m_filename;
    String              m_journalFilename;

    OSWindow*           m_osWindow;
   // shared_ptr<FileDialog>         m_dialog;

    enum {JOURNAL_TAB, FILE_ONLY_TAB};
    int			m_currentTab;
    
    
    enum Location {APPEND, NEW} location;	
    Location		m_location;
    
    String		m_newEntryName;
    String		m_caption;
    String		m_discussion;
    
    GuiTextureBox*      m_textureBox;
    
    ScreenshotDialog(OSWindow* osWindow, const shared_ptr<GuiTheme>& theme, const String& journalDirHint, const bool flip = true);

    void close();

    void onCommit();

public:

    /** Generate the next filename. */
    static String nextFilenameBase(const String& prefix = "");

    static shared_ptr<ScreenshotDialog> create(OSWindow* osWindow, const shared_ptr<GuiTheme>& theme, const String& journalDirHint, bool flip = true) {
        return shared_ptr<ScreenshotDialog>(new ScreenshotDialog(osWindow, theme, journalDirHint, flip));
    }

    static shared_ptr<ScreenshotDialog> create(const shared_ptr<GuiWindow>& parent, const String& journalDirHint, bool flip = true) {
        return create(parent->window(), parent->theme(), journalDirHint, flip);
    }

    /**
       @param filename  This is the initial filename shown, and unless cancelled, receives the final filename as well.
       @return True unless cancelled

       If the user selects the Journal options, this will create an entry in journal.dox in anticipation of
       the image/video file being written.
     */
    // filename is passed as a reference instead of a pointer because it will not be used after the
    // method ends, so there is no danger of the caller misunderstanding as there is with the GuiPane::addTextBox method.
    virtual bool getFilename(String& filename, const String& caption = "Save Screenshot", const shared_ptr<Texture>& texture = shared_ptr<Texture>());

    virtual bool onEvent(const GEvent& e);

};

} // namespace

#endif
