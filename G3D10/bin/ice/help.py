# help.py
#
# Help Information and Printing Routines for iCompile
from __future__ import print_function

from .variables import *
from .utils import *
from . import copyifnewer

##############################################################################
#                                  Version                                   #
##############################################################################

def printVersion(version):
    print("iCompile " + versionToString(version))
    print("Copyright 2003-2013 Morgan McGuire")
    print("All rights reserved")
    print()
    print("http://ice.sf.net")
    print()

##############################################################################
#                                    Help                                    #
##############################################################################

def printHelp():
    print(("""
iCompile: the zero-configuration build system

icompile  [--doc] [--opt | --debug] [--clean] [--version]
          [--config <custom .icompile>] [--verbosity n]
          [--help] [--noprompt [--template <tname>]] [--info] 
          [--deploy | --run <args> | --lldb <args>]

iCompile can build most C++ projects without options or manual
configuration.  Just type 'icompile' with no arguments.  Run in an
empty directory to generate a set of starter files.

Options:
 --config <file>  Use <file> instead of ~/.icompile as the user configuration
                  file.  This allows you to build certain projects with
                  a different compiler or include paths without changing the
                  project ice.txt file, e.g., when installing a 3rd party library
                  
 --debug          (Default) Create a debug executable (define _DEBUG,
                  disable optimizations).

 --deploy         Create a distributable application in the build directory.
                  This should only be used for GUI applications, not command-line
                  tools. Changes the target default to --opt.

                  OS X Specific: data-files/icons.icns  will automatically become
                  the application icon.  Your program will launch with Applications
                  as the current working directory.  Look at argv[0] to change to
                  the directory of your project.
 
 --doc            Generate documentation before building.
 
 --lldb           Run the program under lldb if compilation succeeds, passing all
                  further arguments (...) to the program.  lldb will
                  look in the directories in the ICE_EXTRA_SOURCE
                  environment variable for library and other source
                  files, in addition to the ones from your program.
                  You can also just run lldb yourself after using
                  iCompile.

 --info           Read configuration files and command line options, but instead
                  of building, print out information about where the generated
                  file will go.

 --noprompt       Run even if there is no ice.txt file, don't prompt the
                  user for input.  This is handy for launching iCompile
                  from automated build shell scripts.  If
                  --template <tname> is specified as well, a default
                  set of files will be built.  The only legal templates
                  are 'hello', 'G3D', 'tinyG3D', and 'empty' (default).

 --opt or -O      Generate an optimized executable.
 
 --run            Run the program if compilation succeeds, passing all
                  further arguments (...) to the program.

 --verbosity n    Change the amount of information printed by icompile

                  n   |   Result
                  ----|---------------------------------------------------
                  0   |   Quiet:  Only errors and prompts are displayed.
                      |
                  1   |   Normal (Default): Filenames and progress information
                      |   are also displayed
                  2   |   Verbose: Executed commands are also displayed
                      |
                  3   |   Trace: Additional debugging information is also
                      |   displayed.


Exclusive options:
 --help           Print this information.
 
 --clean          Delete all generated files (but not library generated files).
 
 --version        Print the version number of iCompile.

Special file and directory names:
  build            Output directory
  data-files       Files that will be needed at runtime
  doc-files        Files needed by your documentation (Doxygen output)
  tmp              Object files are put here
  icon.*           Becomes the program icon on OS X
  
iCompile will not look for source files in directories matching: """ +
           str(copyifnewer._excludeDirPatterns) +
"""

Generated file ice-stats.csv contains a history of the size of your files at
compilation times that is interesting for tracking development progress.

For convenience, if run from the source or data-files directory, iCompile
will automatically step up to the parent directory before compilation.

Edit ice.txt and ~/.icompile if your project has specific configuration needs.
See icompile.html at http://g3d.cs.williams.edu for full information. iCompile
was created by Morgan McGuire with additional programming by Robert Hunter, 
Mike Mara, Sam Donow, and Corey Taylor.
"""))
    sys.exit(0)



""" If the exact warning string passed in hasn't been printed
in the past 72 hours, it is printed and listed in the cache under
the "warnings" key.  Otherwise it is supressed."""
def maybeWarn(warning, state):

    MINUTE = 60
    HOUR = MINUTE * 60
    DAY = HOUR * 12
    WARNING_PERIOD = 3 * DAY
    now = time.time()

    if state == None or state.cache == None:
        # the cache has not been loaded yet
        colorPrint(warning, WARNING_COLOR)
        return

    allWarnings = state.cache.warnings
    if (not warning in allWarnings or
        ((allWarnings[warning] + WARNING_PERIOD) < time.time())):

        # Either this key is not in the dictionary or
        # the warning period has expired.  Print the warning
        # and update the time in the cache
        allWarnings[warning] = time.time()
        colorPrint(warning, WARNING_COLOR)
