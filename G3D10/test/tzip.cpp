#include "G3D/G3DAll.h"
#include "testassert.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;


static bool isZipfileTest(const String& filename) {
    return FileSystem::isZipfile(filename);
}


static bool zipfileExistsTest(const String& filename) {
	String path, contents;
	return zipfileExists(filename, path, contents);
}


void testZip() {
	
	printf("zip API ");

	int x; // for loop iterator

	// isZipfile()
	bool isZipTest = isZipfileTest("apiTest.zip");
	testAssertM(isZipTest, "isZipfile failed.");

	// zipfileExists()
	bool zipExistsTest = zipfileExistsTest("apiTest.zip/Test.txt");
	testAssertM(zipExistsTest, "zipfileExists failed.");

	// getFiles() - normal
	bool normalFiles = true;
	Array<String> files;
	FileSystem::getFiles("TestDir/*", files);

	if (files.length() != 1) {
		normalFiles = false;
	}
	for (x = 0; x < files.length(); ++x) {
		if(files[x] != "Test.txt") {
			normalFiles = false;
		}
	}
	testAssertM(normalFiles, "Normal getFiles failed.");

	// getDirs() - normal
	Array<String> dirs;
	FileSystem::getDirectories("TestDir/*", dirs);

	bool normalDirs = 	(dirs.length() == 1 && dirs[0] == "Folder") ||
		(dirs.length() == 2 && (dirs[0] == "Folder" || dirs[1] == "Folder"));

	testAssertM(normalDirs, "Normal getDirs failed.");

	// getFiles() + getDirs() - invalid
	Array<String> emptyTest;
	FileSystem::getFiles("nothing", emptyTest);
	FileSystem::getDirectories("nothing", emptyTest);
	bool noFile = emptyTest.length() == 0;
	testAssertM(noFile, "Improper response to a file that does not exist.");

	// getFiles() - zip
	bool zipFiles = true;
	String zipDir = "apiTest.zip/*";
	Array<String> zFiles;

	FileSystem::getFiles(zipDir, zFiles);

	if (zFiles.length() != 1) {
		zipFiles = false;
	}
	for (x = 0; x < zFiles.length(); ++x) {
		if(zFiles[x] != "Test.txt") {
			zipFiles = false;
		}
	}
	testAssertM(zipFiles, "Zip getFiles failed.");

	// getDirs() - zip
	bool zipDirs = true;
	Array<String> zDirs;

	FileSystem::getDirectories(zipDir, zDirs);

	if (zDirs.length() != 1) {
		zipDirs = false;
	}
	for (x = 0; x < zDirs.length(); ++x) {
		if (zDirs[x] != "zipTest") {
			zipDirs = false;
		}
	}
	testAssertM(zipDirs, "Zip getDirs failed.");

	// fileLength() - normal
	bool normalLength = false;
    if (FileSystem::size("TestDir/Test.txt") == 69) {
		normalLength = true;
	}
	testAssertM(normalLength, "Normal fileLength failed.");

	// fileLength() - nonexistent
	bool noLength = false;
	if (FileSystem::size("Grawk") == -1) {
		noLength = true;
	}
	testAssertM(noLength, "Nonexistent fileLength failed.");

	// fileLength() - zip
	bool zipLength = false;
	if(FileSystem::size("apiTest.zip/Test.txt") == 69) {
		zipLength = true;
	}
	testAssertM(zipLength, "Zip fileLength failed.");


	printf("passed\n");
}
