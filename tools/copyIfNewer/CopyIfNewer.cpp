#include "G3D/G3D.h"

using namespace G3D;

void printHelp();
bool isDirectory(const String& filename);
/** Adds a slash to a directory, if not already there. */
String maybeAddSlash(const String& in);
int main(int argc, char** argv);
bool excluded(bool exclusions, bool superExclusions, const String& filename);


void copyIfNewer
   (bool exclusions, 
    bool superExclusions, 
    String sourcespec,
    String destspec) {

	if (FileSystem::isDirectory(sourcespec)) {
		// Copy an entire directory.  Change the arguments so that
		// we copy the *contents* of the directory.

		sourcespec = maybeAddSlash(sourcespec);
		sourcespec = sourcespec + "*";
	}

	String path = filenamePath(sourcespec);

	Array<String> fileArray;
	Array<String> dirArray;

	FileSystem::getDirectories(sourcespec, dirArray);
	FileSystem::getFiles(sourcespec, fileArray);

	destspec = maybeAddSlash(destspec);

	if (FileSystem::exists(destspec, false) && ! FileSystem::isDirectory(destspec)) {
		printf("A file already exists named %s.  Target must be a directory.", 
			destspec.c_str());
		exit(-2);
	}
	FileSystem::createDirectory(destspec);

	for (int f = 0; f < fileArray.length(); ++f) {
		if (! excluded(exclusions, superExclusions, fileArray[f])) {
			String s = path + fileArray[f];
			String d = destspec + fileArray[f];
			if (FileSystem::isNewer(s, d)) {
				printf("copy %s %s\n", s.c_str(), d.c_str());
				FileSystem::copyFile(s, d);
			}
		}
	}

	// Directories just get copied; we don't check their dates.
	// Recurse into the directories
	for (int d = 0; d < dirArray.length(); ++d) {
		if (! excluded(exclusions, superExclusions, dirArray[d])) {
			copyIfNewer(exclusions, superExclusions, path + dirArray[d], destspec + dirArray[d]);
		}
	}
}



int main(int argc, char** argv) {

    initG3D();

    if (((argc == 2) && (String("--help") == argv[1])) || (argc < 3) || (argc > 4)) {
        printHelp();
        return -1;
    } else {
        bool exclusions = false;
        bool superExclusions = false;

        String s, d;
        if (String("--exclusions") == argv[1]) {
            exclusions = true;
            s = argv[2];
            d = argv[3];
        } else if (String("--super-exclusions") == argv[1]) {
            exclusions = true;
            superExclusions = true;
            s = argv[2];
            d = argv[3];
        } else {
            s = argv[1];
            d = argv[2];
        }

        copyIfNewer(exclusions, superExclusions, s, d);
    }
    
    return 0;
}


void printHelp() {
    printf("COPYIFNEWER\n\n");
    printf("SYNTAX:\n\n");
    printf(" copyifnewer [--help] [--exclusions | --super-exclusions] <source> <destdir>\n\n");
    printf("ARGUMENTS:\n\n");
    printf("  --exclusions  If specified, exclude CVS, svn, and ~ files. \n\n");
    printf("  --super-exclusions  If specified, exclude CVS, svn, ~, .ncb, .pyc, .sdf, .ncb, .suo Release, Debug, build, temp files. \n\n");
    printf("  source   Filename or directory name (trailing slash not required).\n");
    printf("           May include standard Win32 wild cards in the filename.\n");
    printf("  dest     Destination directory, no wildcards allowed.\n\n");
    printf("PURPOSE:\n\n");
    printf("Copies files matching the source specification to the dest if they\n");
    printf("do not exist in dest or are out of date (according to the file system).\n\n");
    printf("Compiled: " __TIME__ " " __DATE__ "\n"); 
}


String maybeAddSlash(const String& sourcespec) {
    if (sourcespec.length() > 0) {
        char last = sourcespec[sourcespec.length() - 1];
        if ((last != '/') && (last != ':') && (last != '\\')) {
            // Add a trailing slash
            return sourcespec + "/";
        }
    }
    return sourcespec;
}

bool excluded(bool exclusions, bool superExclusions, const String& filename) {
    
    if (exclusions) {
        if (filename[filename.length() - 1] == '~') {
            return true;
        } else if ((filename == "CVS") || (filename == "svn") || (filename == ".svn") || (filename == ".cvsignore")) {
            return true;
        }
    }

    if (superExclusions) {
        String f = toLower(filename);
        if ((f == "release") || 
            (f == "debug") ||
            (f == "build") ||
            (f == "graveyard") ||
            (f == "temp") ||
            endsWith(f, ".pyc") ||
            endsWith(f, ".sbr") ||
            endsWith(f, ".ncb") ||
            endsWith(f, ".opt") ||
            endsWith(f, ".bsc") ||
            endsWith(f, ".suo") ||
            endsWith(f, ".ncb") ||
            endsWith(f, ".sdf") ||
            endsWith(f, ".pch") ||
            endsWith(f, ".ilk") ||
            endsWith(f, ".pdb")) {

            return true;
        }
    }

    return false;
}
