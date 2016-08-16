@echo off
echo.
if not exist Doxyfile (
    echo Run builddoc from the directory containing your Doxyfile.
    echo G3D provides a Doxyfile and G3DDoxygen.sty in samples\starter.
    exit /b 1
)

echo %time% Building documentation into %cd%\build\doc

if not exist build (
    @echo on
    mkdir build
    @echo off
)

@echo on
doxygen
CopyIfNewer doc-files\* build\doc\
@echo off

echo.
echo %time% Documentation build complete 
