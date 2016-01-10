@ECHO OFF
SET bits=32
IF "%ProgramW6432%" NEQ "" SET bits=64
SET viewer="%CD%\viewer%bits%.exe %%1"

echo. 
echo Permanently associate 3DS, OBJ, OFF, IFS, PLY, PLY2, BSP, MD2,
echo MD3, ANY, FNT, STL, STLA, and GTM files with the %bits%-bit G3D viewer in
echo %CD%?
echo. 
SET /P result="[Y/N] "

IF "%result%" NEQ "y" EXIT /B 1

ECHO ON
reg add HKEY_CURRENT_USER\Software\Classes\.3ds /ve /d "3D Model" /f
reg add HKEY_CURRENT_USER\Software\Classes\.obj /ve /d "3D Model" /f
reg add HKEY_CURRENT_USER\Software\Classes\.off /ve /d "3D Model" /f
reg add HKEY_CURRENT_USER\Software\Classes\.ifs /ve /d "3D Model" /f
reg add HKEY_CURRENT_USER\Software\Classes\.ply /ve /d "3D Model" /f
reg add HKEY_CURRENT_USER\Software\Classes\.ply2 /ve /d "3D Model" /f
reg add HKEY_CURRENT_USER\Software\Classes\.bsp /ve /d "3D Model" /f
reg add HKEY_CURRENT_USER\Software\Classes\.stl /ve /d "3D Model" /f
reg add HKEY_CURRENT_USER\Software\Classes\.stla /ve /d "3D Model" /f
reg add HKEY_CURRENT_USER\Software\Classes\.md2 /ve /d "3D Model" /f
reg add HKEY_CURRENT_USER\Software\Classes\.md3 /ve /d "3D Model" /f
reg add "HKEY_CURRENT_USER\Software\Classes\3D Model\shell\open\command" /ve /f /d %viewer%

reg add HKEY_CURRENT_USER\Software\Classes\.fnt /ve /d "G3D Font" /f
reg add "HKEY_CURRENT_USER\Software\Classes\G3D Font\shell\open\command" /ve /f /d %viewer%

reg add HKEY_CURRENT_USER\Software\Classes\.gtm /ve /d "G3D GUI Theme" /f
reg add "HKEY_CURRENT_USER\Software\Classes\G3D GUI Theme\shell\open\command" /ve /f /d %viewer%

reg add HKEY_CURRENT_USER\Software\Classes\.any /ve /d "G3D Data File" /f
reg add "HKEY_CURRENT_USER\Software\Classes\G3D Data File\shell\open\command" /ve /f /d %viewer%

@echo.
@echo Done!


