@echo This script will set your G3D10 and G3D10DATA environment variables, modify your 
@echo PATH variable, and install the G3D Visual Studio 2012 templates.
@echo.

set visualizerPath "c:\users\%USERNAME%\My Documents\Visual Studio 2013\Visualizers\"
mkdir %visualizerPath%
copy VisualStudioVisualizers/*.natvis %visualizerPath%

set itemTemplatePath "c:\users\%USERNAME%\My Documents\Visual Studio 2013\Templates\ItemTemplates\G3D\"
mkdir %itemTemplatePath%
copy VisualStudioItemTemplates/*.zip %itemTemplatePath%

@if exist ..\BUILD_INSTRUCTIONS.txt goto SourceInstall
setx G3D10DATA "%cd%\..\data"
setx PATH "%PATH%;%cd%"
@echo.
@echo Manually add this to your Visual Studio Include directories:
@echo %cd%\..\include
@echo.
@echo Manually add this to your Visual Studio Library directories:
@echo %cd%\..\lib
goto End

:SourceInstall
@REM this handles the case of running from G3D's own source tree, rather than its build tree

setx G3D10SOURCE "%cd%\.."
setx G3D10DATA "%G3D10SOURCE%\data-files"
setx PATH "%PATH%;%cd%\..\build\bin"
@echo.
@echo Manually add this to your Visual Studio Include directories:
@echo %G3D10SOURCE%\G3D.lib\include;%G3D10SOURCE%\GLG3D.lib\include;%G3D10SOURCE%\assimp.lib\include;%G3D10SOURCE%\civetweb.lib\include;%G3D10SOURCE%\freeimage.lib\include;%G3D10SOURCE%\glfw.lib\include;%G3D10SOURCE%\qrencode.lib\include;%G3D10SOURCE%\zip.lib\include;%G3D10SOURCE%\zlib.lib\include
@echo.
@echo Manually add this to your Visual Studio Library directories:
@echo %cd%\..\build\lib

:End
