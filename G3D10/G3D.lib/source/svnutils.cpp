#include "G3D/svnutils.h"

#include "G3D/stringutils.h"
#include "G3D/g3dmath.h"
#include "G3D/platform.h"
#include "G3D/TextInput.h"
#include "G3D/FileSystem.h"

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

#ifdef G3D_WINDOWS
/** Read output from the child process's pipe for STDOUT.
 *  Stop when there is no more data.
 */
String ReadFromPipe(HANDLE& pipe) { 
    const int BUFSIZE = 4096;
    DWORD dwRead; 
    CHAR chBuf[BUFSIZE]; 
    BOOL bSuccess = FALSE;

    String result;
    for (;;) {
        debugPrintf("Reading from child pipe...\n");
        bSuccess = ReadFile( pipe, chBuf, BUFSIZE, &dwRead, NULL);
        if( ! bSuccess || dwRead == 0 ) break; 
        
        result.append(chBuf, dwRead);
        debugPrintf("Read from child pipe: '%s'\n", result.c_str());
    }
    debugPrintf("Read full string from child pipe: '%s'\n", result.c_str());

    return result;
} 

void CreateChildProcess(LPWSTR& cmd, HANDLE& err, HANDLE& out, HANDLE& in) {
    BOOL bSuccess = FALSE; 
 
    // Set up members of the PROCESS_INFORMATION structure.  
    PROCESS_INFORMATION piProcInfo; 
    ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
 
    // Set up members of the STARTUPINFO structure. 
    // This structure specifies the STDIN and STDOUT handles for redirection. 
    STARTUPINFO siStartInfo;
    ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = &err;
    siStartInfo.hStdOutput = &out;
    siStartInfo.hStdInput = &in;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
 
    // Create the child process. 
    bSuccess = CreateProcess(
        NULL,
        cmd,           // command line 
        NULL,          // process security attributes 
        NULL,          // primary thread security attributes 
        TRUE,          // handles are inherited 
        NULL,             // creation flags 
        NULL,          // use parent's environment 
        NULL,          // use parent's current directory 
        &siStartInfo,  // STARTUPINFO pointer 
        &piProcInfo    // receives PROCESS_INFORMATION 
    );  
   
    // If an error occurs, exit the application. 
    if (!bSuccess) {
        alwaysAssertM(false, "Failed to create child process.");
    } else {
        // Close handles to the child process and its primary thread.
        // Some applications might keep these handles to monitor the status
        // of the child process, for example.
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
    }
}
#endif

static String g3d_exec(const char* cmd) {
#ifdef SOMETHING_THAT_IS_DEFINITELY_NOT_DEFINED
    HANDLE childInRead = NULL;
    HANDLE childInWrite = NULL;
    HANDLE childOutRead = NULL;
    HANDLE childOutWrite = NULL;

    SECURITY_ATTRIBUTES saAttr; 
 
    // Set the bInheritHandle flag so pipe handles are inherited. 
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 

    // Create a pipe for the child process's STDOUT.
    if (!CreatePipe(&childOutRead, &childOutWrite, &saAttr, 0)) {
        alwaysAssertM(false, "Couldn't create a stdout pipe for child process.");
    }

    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if (!SetHandleInformation(childOutRead, HANDLE_FLAG_INHERIT, 0)) {
        alwaysAssertM(false, "Couldn't set stdout handle information for child process.");
    }

    // Create a pipe for the child process's STDIN. 
    if (! CreatePipe(&childInRead, &childInWrite, &saAttr, 0)) 
        alwaysAssertM(false, "Couldn't create a stdin pipe for child process.");

    // Ensure the write handle to the pipe for STDIN is not inherited.
    if ( ! SetHandleInformation(childInWrite, HANDLE_FLAG_INHERIT, 0) )
        alwaysAssertM(false, "Couldn't set stdin handle information for child process.");

  
    // Adapted from http://stackoverflow.com/a/19717944
    // Convert command string to Windows string
    debugPrintf("running command '%s'\n", cmd);
    wchar_t* wString = new wchar_t[4096];
    MultiByteToWideChar(CP_ACP, 0, cmd, -1, wString, 4096);
    
    CreateChildProcess(wString, childOutWrite, childOutWrite, childInRead);

    // We don't need to write anything, so close the input write handle
    if (!CloseHandle(childInWrite)) {
        alwaysAssertM(false, "Failed to close child process stdin write handle.");
    }

    // Actually read output from child process
    String result = ReadFromPipe(childOutRead);
    
    // Clean up
    if (!CloseHandle(childOutWrite)) {
        alwaysAssertM(false, "Failed to close child process stdout write handle.");
    }
    if (!CloseHandle(childOutRead)) {
        alwaysAssertM(false, "Failed to close child process child stdout read handle.");
    }
    if (!CloseHandle(childInRead)) {
        alwaysAssertM(false, "Failed to close child process stdin read handle.");
    }    

    return result;
#else
    FILE* pipe = g3d_popen(cmd, "r");
    if (!pipe) { return "ERROR"; }
    char buffer[128];
    String result;

    while (!feof(pipe)) {
    	if (fgets(buffer, 128, pipe) != nullptr) {
    		result += buffer;
        }
    }

    g3d_pclose(pipe);
    pipe = nullptr;
    return result;
#endif
}


static bool commandExists(const String& cmd) {
    const char* path = System::getEnv("PATH");

    if (notNull(path)) {
        Array<String> pathDirs = G3D::stringSplit(path, 
#       ifdef G3D_WINDOWS
            ';'
#       else
            ':'
#       endif
            );

        for (const String& p : pathDirs) {
            if (FileSystem::exists(FilePath::concat(p, cmd))) {
                return true;
            }
        }
    }

    return false;
}


static String svnName() {
#   ifdef G3D_WINDOWS
        return "svn.exe";
#   else
        return "svn";
#   endif
    
}


bool hasCommandLineSVN() {
    static bool initialized = false;
    static bool hasSVN = false;
    if (! initialized) {
        hasSVN = commandExists(svnName());
        initialized = true;
    }
    return hasSVN;
}


int svnAdd(const String& path) {
    if (! hasCommandLineSVN()) { return -1; }

    const String& command = (svnName() + " add \"" + path + "\"");
    debugPrintf("Command: %s\n", command.c_str());
    const String& result = g3d_exec(command.c_str());

    if (result == "ERROR") {
        return -1;
    } else {
        return 0;
    }
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
        return 0;
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
        return 0;
#   endif
}


/** 
    Fails if the path is over 4 parents away from the versioned part of the repository.
    Works by finding the revision of all parent directories of rawPath (and rawPath itself).
    Return -1 if no svn information is found. 
    */
int getSVNRepositoryRevision(const String& rawPath) {
    static Table<String, int> revisionTable;
    int revisionNumber = 0;
    if (revisionTable.get(rawPath, revisionNumber)) {
        return revisionNumber;
    }
    String currentPath = rawPath;
    // Heuristic: check the first 4 parent directories. If none of them are under revision control, give up
    for(int i = 0; i < 4; ++i) { 
        int newResult = getSVNDirectoryRevision(currentPath);
        if (newResult > 0) {
            revisionNumber = newResult;
            break;
        }
        currentPath = maybeUpOneDirectory(currentPath);
    }
    if (revisionNumber > 0) {
        while(true) {
            currentPath = maybeUpOneDirectory(currentPath);
            int newResult = getSVNDirectoryRevision(currentPath);
            revisionNumber = iMax(revisionNumber, newResult);
            if (newResult == 0) {
                break;
            }
        }
    }
    revisionTable.set(rawPath, revisionNumber);
    return revisionNumber;
}

} // namespace G3D
