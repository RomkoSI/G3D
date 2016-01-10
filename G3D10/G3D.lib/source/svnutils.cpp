#include "G3D/svnutils.h"

#include "G3D/stringutils.h"
#include "G3D/g3dmath.h"
#include "G3D/platform.h"
#include "G3D/TextInput.h"

#include <iostream>
#include <stdio.h>
namespace G3D {


static FILE* g3d_popen(const char* cmd, const char* mode) {
#   ifdef G3D_WINDOWS
        return _popen(cmd, mode);
#   else
        return popen(cmd, mode);
#   endif
}

static int g3d_pclose(FILE* file) {
#   ifdef G3D_WINDOWS
        return _pclose(file);
#   else
        return pclose(file);
#   endif
}

/** Go up one directory if possible */
static String maybeUpOneDirectory(const String& directory) {
    size_t lastSlash = findLastSlash(directory);
    if (lastSlash == String::npos) {
        return directory;
    } else {
        const String& newPath = directory.substr(0, lastSlash);
        if (endsWith(newPath,":")) {
            return newPath + "\\";
        } else {
            return newPath;
        }
    }
}

static String g3d_exec(const char* cmd) {
    FILE* pipe = g3d_popen(cmd, "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    String result = "";
    while(!feof(pipe)) {
    	if(fgets(buffer, 128, pipe) != NULL)
    		result += buffer;
    }
    g3d_pclose(pipe);
    return result;
}

/** 
    Returns the highest revision of svn version files under \param path
    Return -1 if no svn information is found
    */
int getSVNDirectoryRevision(const String& path) {
#   ifdef G3D_WINDOWS
        const String& rawString = g3d_exec(("SubWCRev " + path).c_str());
        TextInput ti(TextInput::FROM_STRING, rawString);
        while (ti.hasMore()) {
            Token t = ti.read();
            if (t.type() == Token::NUMBER) {
                return (int)t.number();
            }
        }
        return -1;
#   else
        const String& rawString = g3d_exec(("svnversion " + path).c_str());
        String input = rawString;
        size_t separatorIndex  = input.find(':');
        if (separatorIndex != String::npos) {
            int newStart = separatorIndex + 1;
            input = input.substr(newStart, input.length() - newStart);
        }
        TextInput ti(TextInput::FROM_STRING, input);
        Token t = ti.read();
        if (t.type() == Token::NUMBER) {
            return (int)t.number();
        }
        return -1;
#   endif
}

/** 
    Fails if the path is over 4 parents away from the versioned part of the repository.
    Works by finding the revision of all parent directories of rawPath (and rawPath itself).
    Return -1 if no svn information is found. 
    */
int getSVNRepositoryRevision(const String& rawPath) {
    static Table<String, int> revisionTable;
    int revisionNumber = -1;
    if (revisionTable.get(rawPath, revisionNumber)) {
        return revisionNumber;
    }
    String currentPath = rawPath;
    // Heuristic: check the first 4 parent directories. If none of them are under revision control, give up
    for(int i = 0; i < 4; ++i) { 
        int newResult = getSVNDirectoryRevision(currentPath);
        if (newResult >= 0) {
            revisionNumber = newResult;
            break;
        }
        currentPath = maybeUpOneDirectory(currentPath);
    }
    if (revisionNumber >= 0) {
        while(true) {
            currentPath = maybeUpOneDirectory(currentPath);
            int newResult = getSVNDirectoryRevision(currentPath);
            revisionNumber = iMax(revisionNumber, newResult);
            if (newResult < 0) {
                break;
            }
        }
    }
    revisionTable.set(rawPath, revisionNumber);
    return revisionNumber;
}

}
