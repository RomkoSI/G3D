@REM Windows shell script for launching the Python build script.

SET command=python

where python 2> tmp

SET /p pythonLocation= < tmp



SET "commandName=%~dp0%ImageFormat.py"

%command% %commandName% %1 %2 %3 %4 %5 %6 %7 %8 %9
