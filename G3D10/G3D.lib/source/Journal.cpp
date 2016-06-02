/**
\file Journal.cpp

\maintainer Morgan McGuire, http://graphics.cs.williams.edu

\created 2014-07-17
\edited  2016-04-01

 G3D Innovation Engine
 Copyright 2000-2016, Morgan McGuire.
 All rights reserved.
 */

#include "G3D/Journal.h"
#include "G3D/debugAssert.h"
#include "G3D/FileSystem.h"
#include "G3D/stringutils.h"
#include "G3D/fileutils.h"
#include "G3D/TextInput.h"
#include "G3D/enumclass.h"
#include <regex>

namespace G3D {

G3D_DECLARE_ENUM_CLASS(JournalSyntax, DOXYGEN, MARKDEEP);
    
static JournalSyntax detectSyntax(const String& journalFilename) {
    return endsWith(toLower(journalFilename), ".dox") ? JournalSyntax::DOXYGEN : JournalSyntax::MARKDEEP;
}


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

        const String& t = FilePath::concat(searchPaths[i], "journal.md.html");
        if (FileSystem::exists(t)) {
            return t;
        }
    }

    return "";
}


static const std::regex& headerRegex() {
    // Find either "\n# <stuff>\n\n" or "\n<stuff>\n===...\n"
    // With newlines able to be either unix-style "\n" or windows style "\r\n".
    // (\r\n?|\n)
    static const std::regex HEADER_REGEX("(\r\n?|\n)#.*(\r\n?|\n)(\r\n?|\n)|(\r\n?|\n)[^(\r\n?|\n)#].*(\r\n?|\n)={3,}\\s*?(\r\n?|\n)", std::regex::ECMAScript);
    return HEADER_REGEX;
}


static size_t findSection(JournalSyntax syntax, const String& fileContents, size_t start = 0) {
    if (syntax == JournalSyntax::DOXYGEN) {
        // Search for the section title
        size_t pos = fileContents.find("\\section", start);
        const size_t pos2 = fileContents.find("@section", start);
        
        if (pos == String::npos) {
            pos = pos2;
        } else if (pos2 != String::npos) {
            pos = min(pos, pos2);
        }
        
        return (int)pos;
    } else {
        
        std::cmatch match;
        if (std::regex_search(fileContents.c_str(), match, headerRegex())) {
            // Found a match
            return match.position() + 1;
        } else {
            return String::npos;
        }
    }
}


String Journal::firstSectionTitle(const String& journalFilename) {
    alwaysAssertM(FileSystem::exists(journalFilename), journalFilename + " not found.");

    const JournalSyntax syntax = detectSyntax(journalFilename);

    const String& file = readWholeFile(journalFilename);

    // Search for the section title
    size_t pos = findSection(syntax, file);

    if (pos == String::npos) {
        // No section found
        return "";
    } else if (syntax == JournalSyntax::DOXYGEN) {
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
    } else {
        // Find either "\n# <stuff>\n\n" or "\n<stuff>\n===...\n"
        // The first character must be a newline. After that, determine whether it is an ATX or STL style header
        size_t end = file.find("\n", pos);
        if (end == String::npos) {
            end = file.length() - 1;
        }
        return trimWhitespace(file.substr((file[pos] == '#') ? pos + 1 : pos, end - pos + 1));
    }
}


void Journal::appendToFirstSection(const String& journalFilename, const String& text) {
    alwaysAssertM(FileSystem::exists(journalFilename), journalFilename + " not found.");

    const JournalSyntax syntax = detectSyntax(journalFilename);
    const String& file = readWholeFile(journalFilename);

    // Search for the section title
    size_t pos = findSection(syntax, file);

    String combined;
    if (syntax == JournalSyntax::DOXYGEN) {
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

    } else {
        // Get past the header
        if (pos != String::npos) {
            const bool isATX = (file[pos] == '#');

            pos = file.find('\n', pos);
            if (pos == String::npos) {
                pos = file.length() - 1;
            }

            if (! isATX) {
                // Jump over the === signs as well by looking for the NEXT newline
                pos = file.find('\n', pos + 1);
                if (pos == String::npos) {
                    pos = file.length() - 1;
                }
            }

            if (pos != String::npos) {
                // If there are more newlines, skip over them
                while ((pos < file.length() - 1) && (file[pos] == '\n')) {
                    ++pos;
                }
            }
        }
    }

    combined = file.substr(0, pos) + text + "\n" + file.substr(pos);

    writeWholeFile(journalFilename, combined);
}


void Journal::insertNewSection(const String& journalFilename, const String& title, const String& text) {
    alwaysAssertM(FileSystem::exists(journalFilename), journalFilename + " not found.");

    const JournalSyntax syntax = detectSyntax(journalFilename);
    const String& file = readWholeFile(journalFilename);

    // Search for the section title
    size_t pos = findSection(syntax, file);

    if (pos == String::npos) {
        // No section found: look for the beginning
        if (syntax == JournalSyntax::DOXYGEN) {
            pos = file.find("/*");
            if (pos == String::npos) {
                pos = 0;
            } else {
                pos += 2;
            }
        } else {
            // Skip over any opening meta tag and title
            pos = file.find("<meta");
            if (pos != String::npos) {
                // Jump past it:
                pos = file.find('>', pos) + 1;
            } else {
                pos = 0;
            }
            
            while ((pos < file.length() - 1) && isWhitespace(file[pos])) {
                ++pos;
            }

            // TODO: Jump past any indented lines as well
        }
    }

    String section;
    if (syntax == JournalSyntax::DOXYGEN) {
        time_t t1;
        ::time(&t1);
        tm* t = localtime(&t1);

        const String& sectionName = format("S%4d%02d%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
        section = "\\section " + sectionName + " " + title + "\n\n" + text + "\n";
    } else {
        section = "\n" + trimWhitespace(title) + "\n=============================================================\n" + text + "\n";
    }
    
    const String& combined = file.substr(0, pos) + section + "\n" + file.substr(pos);
    writeWholeFile(journalFilename, combined);
}


static String escapeDoxygenCaption(const String& s) {
    String r;
    for (size_t i = 0; i < s.length(); ++i) {
        const char c = s[i];
        if ((c == ',') || (c == '}') || (c == '{') || (c == '\"')) {
            // Escape these characters
            r += '\\';
        }
        r += c;
    }
    return r;
}


String Journal::formatImage(const String& journalFilename, const String& imageFilename, const String& caption, const String& discussion) {
    const bool isVideo = endsWith(toLower(imageFilename), ".mp4");

    const JournalSyntax syntax = detectSyntax(journalFilename);

    const String& journalFullPath = FileSystem::resolve(journalFilename);
    const String& imageFullPath = FileSystem::resolve(imageFilename);

    const String& prefix = G3D::greatestCommonPrefix(journalFullPath, imageFullPath);
    const String& relativeImagePath = imageFullPath.substr(prefix.length());

    if (syntax == JournalSyntax::DOXYGEN) {
        const String macroString = isVideo ? "video" : "thumbnail";
        return "\n\\" + macroString + "{" + relativeImagePath + ", " + escapeDoxygenCaption(caption) + "}\n\n" + discussion + "\n";
    } else {
        const String& discussionSection = (discussion.empty()) ? "" : "\n\n" + discussion;
        return "\n![" + caption + "](" + relativeImagePath + ")" + discussionSection + "\n";
    }
}

} // G3D
