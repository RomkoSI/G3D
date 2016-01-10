# doticompile.py
# 
# Manage .icompile files
from __future__ import print_function

import string, os
try:
  import configparser
except ImportError:
  import ConfigParser as configparser
from . import copyifnewer, templateG3D, templateHello
from .utils import *
from .doxygen import *
from .variables import *
from .help import *

# If True, the project will not be rebuilt if all dependencies
# other than iCompile are up to date.  This is handy when
# working on iCompile and testing with large libraries.
_IGNORE_ICOMPILE_DEPENDENCY = False

##############################################################################
#                             Default .icompile                              #
##############################################################################

configHelp = """
# If you have special needs, you can edit per-project ice.txt
# files and your global ~/.icompile file to customize the
# way your projects build.  However, the default values are
# probably sufficient and you don't *have* to edit these.
#
# To return to default settings, just delete ice.txt and
# ~/.icompile and iCompile will generate new ones when run.
#
#
# In general you can set values without any quotes, e.g.:
#
#  compileoptions = -O3 -g --verbose $(CXXFLAGS) %(defaultcompileoptions)s
#
# Adds the '-O3' '-g' and '--verbose' options to the defaults as
# well as the value of environment variable CXXFLAGS.
# 
# These files have the following sections and variables.
# Values in ice.txt override those specified in .icompile.
#
# GLOBAL Section
#  compiler           Path to compiler. May also be <NEWESTGCC> or <NEWESTCOMPILER>
#
#  include            Semi-colon or colon (on Linux) separated
#                     include paths.
#
#  library            Same, for library paths.
#
#  defaultinclude     The initial include path.
#
#  defaultlibrary     The initial library path.
#
#  defaultcompiler    The initial compiler.
#
#  defaultexclude     Regular expression for directories to exclude
#                     when searching for C++ files.  Environment
#                     variables are NOT expanded for this expression.
#                     e.g. exclude: <EXCLUDE>|^win32$
# 
#  builddir           Build directory, relative to ice.txt.  Start with a 
#                     leading slash (/) to make absolute.
#
#  tempdir            Temp directory, relative to ice.txt. Start with a 
#                     leading slash (/) to make absolute.
#
#  beep               If True, beep after compilation
#
#  workdir            Directory to use as the current working directory
#                     (cwd) when launching the compiled program with
#                     the --lldb or --run flag
#
# DEBUG and RELEASE Sections
#
#  compileoptions                     
#  linkoptions        Options *in addition* to the ones iCompile
#                     generates for the compiler and linker, separated
#                     by spaces as if they were on a command line.
#
#
# The following special values are available:
#
#   %(localvar)s     Value of a variable set inside ice.txt
#                    or .icompile (Yes, you need that 's'--
#                    it is a Python thing.)
#   $(envvar)        Value of shell variable named envvar.
#                    Unset variables are the empty string.
#   $shell(...)      Runs the '...' and replaces the expression
#                    as if it were the value of an envvar.
#   $eval(...)       Evaluates the expression within the parenthesis using Python.
#                    This is useful for per-platform values. For example:
#                    $eval('-lX' if linux else os.getenv('HOME')). The following variables are
#                    bound:
#                           bool linux                    
#                           bool osx
#                           bool windows
#                           bool debug
#                           bool release
#   <NEWESTCOMPILER> The newest version of gcc or Visual Studio on your system.
#   <EXCLUDE>        Default directories excluded from compilation.
#
# The special values may differ between the RELEASE and DEBUG
# targets.  The default .icompile sets the 'default' variables
# and the default ice.txt sets the real ones from those, so you
# can chain settings.
#
#  Colors have the form:
#
#    [bold|underline|reverse|italic|blink|fastblink|hidden|strikethrough]
#    [FG] [on BG]
#
#  where FG and BG are each one of
#   {default, black, red, green, brown, blue, purple, cyan, white}
#  Many styles (e.g. blink, italic) are not supported on most terminals.
#
#  Examples of legal colors: "bold", "bold red", "bold red on white", "green",
#  "bold on black"
#
"""

defaultDotICompile = """
# This is a configuration file for iCompile (http://ice.sf.net)
# """ + configHelp + """
[GLOBAL]
defaultinclude:  $(INCLUDE);/opt/local/include;/usr/local/include/SDL11;/usr/include/SDL;/usr/X11R6/include;
defaultlibrary:  $(LIBRARY);$(LD_LIBRARY_PATH);$(DYLD_LIBRARY_PATH);/usr/X11R6/lib;
defaultcompiler: <NEWESTCOMPILER>
defaultexclude:  <EXCLUDE>
beep:            True
tempdir:         .ice-tmp
builddir:        build

[DEBUG]

[RELEASE]

"""

defaultProjectFileContents = """
# This project can be compiled by typing 'icompile'
# at the command line. Download the iCompile Python
# script from http://ice.sf.net
#
################################################################
""" + configHelp + """

################################################################
[GLOBAL]

compiler: %(defaultcompiler)s

include: %(defaultinclude)s

library: %(defaultlibrary)s

exclude: %(defaultexclude)s

workdir: data-files

# Colon-separated list of libraries on which this project depends.  If
# a library is specified (e.g., png.lib) the platform-appropriate 
# variation of that name is added to the libraries to link against.
# If a directory containing an iCompile ice.txt file is specified, 
# that project will be built first and then added to the include 
# and library paths and linked against.
uses:

################################################################
[DEBUG]

compileoptions:

linkoptions:

################################################################
[RELEASE]

compileoptions:

linkoptions:

"""




#################################################################
#                 Configuration & Project File                  #
#################################################################

""" Reads [section]name from the provided configuration, replaces
    <> and $() values with the appropriate settings.

    If exp is False, then $() variables are *not* expanded. 
"""    
def configGet(state, config, section, name, exp = True):
    try:
        val = config.get(section, name)
    except configparser.InterpolationMissingOptionError:
        maybeWarn('Variable \'' + name + '\' in ' + ' the [' + section + '] section of ' + 
                  state.rootDir + 'ice.txt may have an illegal value.  If that ice.txt ' +
                  'file is from a previous version of iCompile you should delete it.\n', state)
        return ''

    # Replace special values
    if '<' in val:
        if ('<NEWESTGCC>' in val):
            (compname, ver) = newestCompiler()
            val = val.replace('<NEWESTGCC>', compname)

        if ('<NEWESTCOMPILER>' in val):
            (compname, ver) = newestCompiler()
            val = val.replace('<NEWESTCOMPILER>', compname)

        val = val.replace('<EXCLUDE>', ('|'.join(copyifnewer._cppExcludePatterns + ['^CMakeFiles$'])).replace("$", "$$"))

    val = os.path.expanduser(val)
        
    if exp:
        val = expandvars(val, {'linux' :   (state.os == 'linux'),
                              'osx' :      (state.os == 'osx'),
                              'windows' :  (state.os == 'windows'),
                              'debug' :    (state.target == DEBUG),
                              'release' :  (state.target == RELEASE),
                              'getoutput' : getoutput
                            })

    return val


class FakeFile:
    _textLines = []
    _currentLine = 0

    def __init__(self, contents):
        self._currentLine = 0
        self._textLines = contents.split('\n')

    def __iter__(self):
        return self

    def __next__(self):
        if (self._currentLine >= len(self._textLines)):
            # end of file
            raise StopIteration
        else:
            self._currentLine += 1
            return self._textLines[self._currentLine - 1] + '\n'

    def readline(self):
        try:
            return self.__next__()
        except StopIteration:
            return ''

""" Called from processProjectFile """ 
def _processDotICompile(state, config):

    # Set the defaults from the default .icompile and ice.txt
    try:
      config.read_file(FakeFile(defaultDotICompile))
      config.read_file(FakeFile(defaultProjectFileContents))
    except AttributeError:
      config.readfp(FakeFile(defaultDotICompile))
      config.readfp(FakeFile(defaultProjectFileContents))

    # Process .icompile
    if os.path.exists(state.preferenceFile()):
        if verbosity >= TRACE: print('Processing ' + state.preferenceFile())
        config.read(state.preferenceFile())
    else:
        success = False

        HOME = os.environ['HOME']
        preferenceFile = HOME + '/.icompile'
        # Try to generate a default .icompile
        if os.path.exists(HOME):
            f = open(preferenceFile, 'wt+')
            if f != None:
                f.write(defaultDotICompile)
                f.close()
                success = True
                if verbosity >= TRACE:
                    colorPrint('Created a default preference file for ' +
                                    'you in ' + preferenceFile + '\n',
                                    SECTION_COLOR)
                
        # We don't need to read this new .icompile because
        # it matches the default, which we already read.
                           
        if not success and verbosity >= TRACE:
            print('No ' + preferenceFile + ' found and cannot write to '+ HOME)

""" Process the project file and .icompile so that we can use configGet.
    Sets a handful of variables."""
def processProjectFile(state, ignoreIceTxt = False):

    config = configparser.SafeConfigParser()
    _processDotICompile(state, config)

    if not ignoreIceTxt:
        # Process the project file
        projectFile = 'ice.txt'
        if verbosity >= TRACE: print('Processing ' + projectFile)
        config.read(projectFile)

    exclude = configGet(state, config, 'GLOBAL', 'exclude', True)
    state.excludeFromCompilation = re.compile(exclude)
 
    # Parses the "uses" line, if it exists
    L = ''
    try:
        L = configGet(state, config, 'GLOBAL', 'uses')
    except configparser.NoOptionError:
        # Old files have no 'uses' section
        pass

    for u in L.split(':'):
        if u.strip() != '':
            if os.path.exists(pathConcat(u, 'ice.txt')):
                # This is another iCompile project
                state.addUsesProject(u)
            else:
                state.addUsesLibrary(u)

    state.buildDir = addTrailingSlash(configGet(state, config, 'GLOBAL', 'builddir', True))
    state.workDir = addTrailingSlash(configGet(state, config, 'GLOBAL', 'workdir', True))

    # Libraries don't need a working directory so don't throw an erroneous warning for them
    if not isLibrary(state.binaryType) and not os.path.exists(state.workDir):
       maybeWarn("Working directory '" + state.workDir + "' does not exist; changing to '.'." +
          " Edit ice.txt to configure this permanently.", state) 
       state.workDir = '.'
    
    state.tempParentDir = addTrailingSlash(configGet(state, config, 'GLOBAL', 'tempdir', True))
    state.tempDir = addTrailingSlash(pathConcat(state.tempParentDir, state.projectName))

    state.beep = configGet(state, config, 'GLOBAL', 'beep')
    state.beep = (state.beep == True) or (state.beep.lower() == 'true')

    # Include Paths
    state.addIncludePath(makePathList(configGet(state, config, 'GLOBAL', 'include')))

    # Add our own include directories.
    if isLibrary(state.binaryType):
        extraInclude = [path for path in ['include', 'include/' + state.projectName] 
                        if os.path.exists(path)]
        state.addIncludePath(extraInclude)

    # Library Paths
    state.addLibraryPath(makePathList(configGet(state, config, 'GLOBAL', 'library')))

    state.compiler = configGet(state, config, 'GLOBAL', 'compiler')

    state.compilerOptions = configGet(state, config, state.target, 'compileoptions').split(' ')
    state.linkerOptions   = configGet(state, config, state.target, 'linkoptions').split(' ')


#########################################################################

    
# Loads configuration from the current directory, where args
# are the arguments preceding --run that were passed to iCompile
#
# Returns the configuration state
def getConfigurationState(args):
    
    state = State()

    state.args = args

    state.template = ''
    if '--template' in args:
        for i in range(0, len(args)):
            if args[i] == '--template':
                if i < len(args) - 1:
                    state.template = args[i + 1]

    state.noPrompt = '--noprompt' in args

    if state.template != '' and not state.noPrompt:
        colorPrint("ERROR: cannot specify --template without --noprompt", ERROR_COLOR)
        sys.exit(-208)
        
    if state.template != 'hello' and state.template != 'G3D'  and state.template != 'tinyG3D' and state.template != 'empty' and state.template != '':
        colorPrint("ERROR: 'hello', 'G3D', and 'empty' are the only legal template names (template='" +
                   state.template + "')", ERROR_COLOR)
        sys.exit(-209)

    # Root directory
    state.rootDir  = os.getcwd() + "/"

    # Project name
    state.projectName = state.rootDir.split('/')[-2]

    ext = extname(state.projectName).lower()
    state.projectName = rawfilename(state.projectName)

    # Binary type    
    if (ext == 'lib') or (ext == 'a'):
        state.binaryType = LIB
    elif (ext == 'dll') or (ext == 'so'):
        state.binaryType = DLL
    elif (ext == 'exe') or (ext == ''):
        state.binaryType = EXE
    else:
        state.binaryType = EXE
        maybeWarn("This project has unknown extension '" + ext +
                  "' and will be compiled as an executable.", state)

    # Choose target
    if ('--opt' in args) or ('-O' in args) or (('--deploy' in args) and not ('--debug' in args)):
        if ('--debug' in args):
            colorPrint("Cannot specify '--debug' and '--opt' at " +
                       "the same time.", WARNING_COLOR)
            sys.exit(-1)

        state.target          = RELEASE
        d                     = ''
    else:
        state.target          = DEBUG
        d                     = 'd'

    # Find an icompile project file.  If there isn't one, give the
    # user the opportunity to create one or abort.
    checkForProjectFile(state, args)

    discoverOS(state)

    # Load settings from the project file.
    processProjectFile(state)

    discoverCompiler(state)

    unix = not state.os.startswith('win')

    # On unix-like systems we prefix library names with 'lib'
    prefix = ''
    if unix and isLibrary(state.binaryType):
        prefix = 'lib'

    state.installDir = state.buildDir

    # Binary name
    if (state.binaryType == EXE):
        state.binaryDir  = state.installDir
        state.binaryName = state.projectName + d
    elif (state.binaryType == DLL):
        state.binaryDir  = state.installDir + 'lib/'
        state.binaryName = prefix + state.projectName + d + '.so'
    elif (state.binaryType == LIB):
        state.binaryDir  = state.installDir + 'lib/'
        state.binaryName = prefix + state.projectName + d + '.a'

    # Make separate directories for object files based on
    # debug/release
    state.objDir = state.tempDir + state.platform + '/' + state.target + '/'

    # Find out when icompile was itself modified
    state.icompileTime = getTimeStamp(sys.argv[0])
    if _IGNORE_ICOMPILE_DEPENDENCY:
        # Set the iCompile timestamp to the beginning of time, so that
        # it looks like icompile itself was never modified.
        state.icompileTime = 0

    # Rebuild if ice.txt or .icompile was modified
    # more recently than the source.
    if os.path.exists('ice.txt'):
        iceTime = getTimeStamp('ice.txt')
        if iceTime > state.icompileTime:
            state.icompileTime = iceTime

    if os.path.exists(state.preferenceFile()):
        configTime = getTimeStamp(state.preferenceFile())
        if configTime > state.icompileTime:
            state.icompileTime = configTime

    return state

##################################################################################

""" Checks for ice.txt and, if not found, prompts the user to create it
    and returns if they press Y, otherwise exits."""
def checkForProjectFile(state, args):
    # Assume default project file
    projectFile = 'ice.txt'
    if os.path.exists(projectFile): return

    # Everything below here executes only when there is no project file
    if not state.noPrompt:

        if '--clean' in args:
            print()
            colorPrint('Nothing to clean (you have never run iCompile in ' +
                   os.getcwd() + ')', WARNING_COLOR)
            print()
            # There's nothing to delete anyway, so just exit
            sys.exit(0)

        print()
        inHomeDir = (os.path.realpath(os.getenv('HOME')) == os.getcwd())

        if inHomeDir:
            colorPrint(' ******************************************************',
                       WARNING_COLOR)
            colorPrint(' * You are about run iCompile in your home directory! *',
                       'bold red')
            colorPrint(' ******************************************************',
                       WARNING_COLOR)
        else:        
            colorPrint('You have never run iCompile in this directory before.',
                       WARNING_COLOR)
        print()
        print('  Current Directory: ' + os.getcwd())
    
        # Don't show dot-files first if we can avoid it
        dirs = listDirs()
        dirs.reverse()
        num = len(dirs)
        sl = shortlist(dirs)
    
        if (num > 1):
            print('  Contains %d directories (%s)' % (num, sl))
        elif (num > 0):
            print('  Contains 1 directory (' + dirs[0] + ')')
        else:
            print('  Contains no subdirectories')

        cfiles = listCFiles()
        num = len(cfiles)
        sl = shortlist(cfiles)
    
        if (num > 1):
            print('  Subdirectories contain %d C++ files (%s)' % (num, sl))
        elif (num > 0):
            print('  Subdirectories contain 1 C++ file (' + cfiles[0] + ')')
        else:
            print('  Subdirectories contain no C++ files')    

        print()
    
        dir = os.getcwd().split('/')[-1]
        if inHomeDir:
            prompt = ('Are you sure you want to run iCompile '+
                      'in your home directory? (Y/N)')
        else:
            prompt = ("Are you sure you want to compile the '" +
                      dir + "' project? (Y/N)")
            
        colorPrint(prompt, 'bold')
        choice = getch().lower()
        while (choice != 'y' and choice != 'n'):
          choice = getch().lower()
        
        if choice != 'y':
            sys.exit(0)

        if (num == 0):
            prompt = ("Would you like to generate a set of starter files for the '" +
                      dir + "' project? (Y/N)")
            colorPrint(prompt, 'bold')
            choice = getch().lower()
            while (choice != 'y' and choice != 'n'):
                choice = getch().lower()
            if choice == 'y':
                if 'G3D10DATA' not in os.environ.keys():
                    templateHello.generateStarterFiles(state)
                else:
                    prompt = "Select a project template:\n  [H]ello World\n  [G]3D Starter\n  [T]iny G3D Starter\n"
                    colorPrint(prompt, 'bold')
                    c = getch().lower()
                    if c == 'h':
                        templateHello.generateStarterFiles(state)
                    elif c == 'g':
                        templateG3D.generateStarterFiles(state, 'starter')
                    else:
                        templateG3D.generateStarterFiles(state, 'tinyStarter')

    if state.noPrompt and state.template != '':
        if state.template == 'hello':
            templateHello.generateStarterFiles(state)
        elif state.template == 'G3D':
            templateG3D.generateStarterFiles(state, 'starter')
        elif state.template == 'tinyG3D':
            templateG3D.generateStarterFiles(state, 'tinyStarter')
        elif state.template == 'empty':
            # Intentionally do nothing
            ''
        else:
            print('ERROR: illegal template')
            
    writeFile(projectFile, defaultProjectFileContents);

    
