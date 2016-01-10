/**
\file Journal.cpp

\maintainer Morgan McGuire, http://graphics.cs.williams.edu

\created 2014-07-17
\edited  2014-07-19

 G3D Innovation Engine
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */

#include "G3D/Journal.h"
#include "G3D/debugAssert.h"
#include "G3D/FileSystem.h"
#include "G3D/stringutils.h"
#include "G3D/fileutils.h"
#include "G3D/TextInput.h"

namespace G3D {

String Journal::findJournalFile(const String& hint) {
    Array<String> searchPaths;
    
    if (endsWith(hint, ".dox")) {
        searchPaths.append(FilePath::parent(hint));
    } else {
        searchPaths.append(hint);
    } 
    
    searchPaths.append(
        FileSystem::currentDirectory(), 
        FilePath::concat(FileSystem::currentDirectory(), ".."),
        FilePath::concat(FileSystem::currentDirectory(), "../journal"),
        FilePath::concat(FileSystem::currentDirectory(), "../../journal"),
        FilePath::concat(FileSystem::currentDirectory(), "../../../journal"));
    
    for (int i = 0; i < searchPaths.length(); ++i) {
        const String& s = FilePath::concat(searchPaths[i], "journal.dox");
        if (FileSystem::exists(s)) {
            return s;
        }
    }

    return "";
}


static int findSection(const String& fileContents, size_t start = 0) {
    // Search for the section title
    size_t pos = fileContents.find("\\section", start);
    const size_t pos2 = fileContents.find("@section", start);

    if (pos == String::npos) {
        pos = pos2;
    } else if (pos2 != String::npos) {
        pos = min(pos, pos2);
    }

    return (int)pos;
}


String Journal::firstSectionTitle(const String& journalFilename) {
    alwaysAssertM(FileSystem::exists(journalFilename), journalFilename + " not found.");

    const String& file = readWholeFile(journalFilename);

    // Search for the section title
    size_t pos = findSection(file);

    if (pos == String::npos) {
        // No section found
        return "";
    } else {
        pos += String("@section").length();

        // Read that line
        size_t end = file.find("\n", pos);
        if (end == String::npos) {
            // Back up by the necessary end-comment character
            end = file.length() - 2;
        }

        const String& sectionStatement = file.substr(pos, end - pos + 1);

        try {
            TextInput parser(TextInput::FROM_STRING, sectionStatement);
            // Skip the label
            parser.readSymbol();

            return trimWhitespace(parser.readUntilNewlineAsString());
        } catch (...) {
            // Journal parsing failed
            return "";
        }
    } // if section found
}


void Journal::appendToFirstSection(const String& journalFilename, const String& text) {
    alwaysAssertM(FileSystem::exists(journalFilename), journalFilename + " not found.");

    const String& file = readWholeFile(journalFilename);

    // Search for the section title
    size_t pos = findSection(file);

    if (pos != String::npos) {
		// Skip to the end of the section line
		pos = file.find('\n', pos);
		if (pos != String::npos) {
			// Go *past* that character
			++pos;
		}
	} 

    if (pos == String::npos) {
        // No section found: look for the end of the documentation comment
        pos = file.find("*/");
        if (pos == String::npos) {
            pos = file.length() - 1;
        }
   	}

    const String& combined = file.substr(0, pos) + text + "\n" + file.substr(pos);
    writeWholeFile(journalFilename, combined);
}


void Journal::insertBeforeFirstSection(const String& journalFilename, const String& text) {
    alwaysAssertM(FileSystem::exists(journalFilename), journalFilename + " not found.");

    const String& file = readWholeFile(journalFilename);

    // Search for the section title
    size_t pos = findSection(file);

	if (pos == String::npos) {
        // No section found: look for the beginning
        pos = file.find("/*");
        if (pos == String::npos) {
            pos = 0;
        } else {
			pos += 2;
		}
	}

	// Insert right before it 
    const String& combined = file.substr(0, pos) + text + "\n" + file.substr(pos);
	writeWholeFile(journalFilename, combined);
}

}
