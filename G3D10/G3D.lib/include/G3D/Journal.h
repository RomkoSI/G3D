/**
  \file G3D/Journal.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2014-07-17
  \edited  2014-07-17

 G3D Innovation Engine
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */
#ifndef G3D_Journal_h
#define G3D_Journal_h

#include "G3D/platform.h"
#include "G3D/G3DString.h"

namespace G3D {

/** Routines for programmatically working with journal.dox files.

  \sa G3D::GApp::Settings::screenshotDirectory, G3D::VideoRecordDialog, G3D::ScreenshotDialog, G3D::Log, G3D::System::findDataFile 
  */
class Journal {
private:
    // Currently an abstract class
    Journal() {}

public:

    /** Locates journal.dox and returns the fully-qualified filename, starting a search from \a hint. 
        Returns the empty string if not found.
     */
    static String findJournalFile(const String& hint = ".");

    /** Returns title of the first Doxygen "section" command the journal at \a journalFilename, or the empty
        string if no section title is found. Assumes that \a journalFilename exists. */
    static String firstSectionTitle(const String& journalFilename);

    /** Adds \a text to the bottom of the first section in the .dox file at \a journalFilename. */
    static void appendToFirstSection(const String& journalFilename, const String& text);

    /** Inserts \a text immediately before the first "section" command in the .dox file at \a journalFilename. */
    static void insertBeforeFirstSection(const String& journalFilename, const String& text);
};

}

#endif