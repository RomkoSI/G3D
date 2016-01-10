#include "G3D/G3DAll.h"
#include "testassert.h"

void testFileSystem() {
    printf("FileSystem...");

    testAssert(g3dfnmatch("*.zip", "hello.not", FNM_PERIOD | FNM_NOESCAPE | FNM_PATHNAME) == FNM_NOMATCH);
    testAssert(g3dfnmatch("*.zip", "hello.zip", FNM_PERIOD | FNM_NOESCAPE | FNM_PATHNAME) == 0);

    testAssert(FilePath::matches("hello", "*", false));
    testAssert(FilePath::matches("hello", "*", true));

    chdir("TestDir");
    String cwd = FileSystem::currentDirectory();
    testAssert(endsWith(cwd, "TestDir"));
    chdir("..");

    // Directory listing
    Array<String> files;
    FileSystem::getFiles("*", files);
    testAssert(files.contains("Any-load.txt"));
    testAssert(files.contains("apiTest.zip"));

    // Directory listing
    files.clear();
    FileSystem::getFiles("*.zip", files);
    testAssert(files.contains("apiTest.zip"));
    testAssert(files.size() == 1);

    // Directory listing inside zipfile
    files.clear();
    testAssert(FileSystem::exists("apiTest.zip"));
    testAssert(FileSystem::isZipfile("apiTest.zip"));
    FileSystem::getFiles("apiTest.zip/*", files);
    testAssert(files.size() == 1);
    testAssert(files.contains("Test.txt"));

    files.clear();
    FileSystem::getDirectories("apiTest.zip/*", files);
    testAssert(files.size() == 1);
    testAssert(files.contains("zipTest"));

    testAssert(! FileSystem::exists("nothere"));
    testAssert(FileSystem::exists("apiTest.zip/Test.txt"));
    testAssert(! FileSystem::exists("apiTest.zip/no.txt"));

    testAssert(FileSystem::size("apiTest.zip") == 488);

    printf("passed\n");
}
