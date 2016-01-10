@REM Windows shell script for launching the Python build script.
@echo off

SET command=python

where python 2> tmp

SET /p pythonLocation= < tmp

IF "%pythonLocation%"=="INFO: Could not find files for the given pattern(s)." (

  IF EXIST "c:\Python32\python.exe" (
      SET command="c:\Python32\python"
  ) ELSE ( 
      IF EXIST "c:\Python33\python.exe" (
		  SET command="c:\Python33\python"
	  ) ELSE (
	  	IF EXIST "c:\Python34\python.exe" (
	  	  SET command="c:\Python34\python"
	  	) ELSE (
		  echo Could not find python.exe--do you need to add C:\Python32\ or something similar to your PATH variable?
		  EXIT /B 1
		 )
	  )
  )
)

%command% buildg3d %1 %2 %3 %4 %5 %6 %7 %8 %9
