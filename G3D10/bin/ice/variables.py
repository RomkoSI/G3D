# variables.py
#
# Global variables for iCompile modules
from __future__ import print_function

import os
from .utils import *
from platform import machine
from .library import *
import multiprocessing

##############################################################################
#                                 Constants                                  #
##############################################################################

DEBUG                     = 'DEBUG'
RELEASE                   = 'RELEASE'
EXE                       = 'EXE'
DLL                       = 'DLL'
LIB                       = 'LIB'
YES                       = True
NO                        = False


# Adds a new path or list of paths to an existing path list
# and then returns the mutated plist.  If plist was None
# when the function was invoked, a new list is returned.
def _pathAppend(plist, newPath, checkForExist = False):
    if plist == None: plist = []

    if isinstance(newPath, list):
        # This is a list of paths.  Append all of them
        for p in newPath:
            _pathAppend(plist, p, checkForExist)

    elif not newPath in plist:
        # This is a single path that does not already appear in 
        # the output list
        if not checkForExist or os.path.exists(newPath):
            plist.insert(0, newPath)
        elif not ('/SDL11/' in newPath) and not ('/SDL/' in newPath) and not ('/X11R6/' in newPath):
            # Don't print warnings about SDL/X11; we need those to make
            # lots of different unix installs all work
            print('Path ' + newPath + ' specified in configuration file does not exist.')

    return plist


class Compiler:
    # Full path to compiler executable
    filename                    = None

    # Short name for the compiler, e.g., 'vc8'
    nickname                    = None

    # Strings will be elided with their arguments, lists will have
    # their arugments appended.
    
    # Flag to append to compiler options to specify that a
    # is in the C language (instead of C++)
    cLangFlag                   = None

    outputFilenameFlag          = None

    useCommandFile              = None

    declareMacroFlag            = None
    
    commandfileFlag             = None

    # Warning level 2
    warning2Flag                = None

    # 64-bit compatibility warnings
    warning64Flag               = None

    debugSymbolsFlag            = None

    dependencyFlag              = None
    
    def initGcc(self):
        cLangFlag = ['-x', 'c']
        outputFilenameFlag = ['-o']
        declareMacroFlag = '-D'
        warning2Flag = ['-W2']
        warning64Flag = []
        debugSymbolsFlag = ['-g']
        
        # We need to use the -msse2 flad during dependency checking because of the way
        # xmmintrin.h is set up
        #
        # -MG means "don't give errors for header files that don't exist"
        # -MM means "don't tell me about system header files"
        dependencyFlag = ['-M', '-MG', '-msse2']
        
        useCommandFile = os.uname()[0] == 'Darwin'
        if useCommandFile:
            commandFileFlag = []

    def initVC8():
        cLangFlag = ['']
        outputFilenameFlag = '/Fo'
        commandFileFlag = '@'
        declareMacroFlag = '/D'
        warning2Flag = ['/W2']
        warning64Flag = ['/Wp64']
        exceptionFlag = ['/EHs']
        rttiFlag = ['/GR']
        nologoFlag = ['/nologo']
        debugSymbolsFlag = ['/Zi']

        # Outputs to stderr in the format:  Note: including file: <filename>
        # during compilation

        # Note:
        # /E       Copies preprocessor output to standard output
        # /P       Writes preprocessor output to a file
        # /EP      Suppress compilation and write preprocessor output without #line directives        
        dependencyFlag = ['/showIncludes', '/EP']



"""
  Items marked with * are written to cache directly from state
  during save and are read from cache to state directly after load.
"""
class Depend:
    """ dict used by in getDependencies in depend.py"""
    dependencies = None
            
    """ * list of extra include paths triggered by dependencies """
    includePaths = None
    
    """ * list of extra link paths triggered by dependencies """
    libraryPaths = None

    """ * list of extra libraries to link against, triggered by dependencies """
    usesProjectsList = None
    
    """ * list of extra compiler options """
    compilerOptions = None
    
    """ * list of extra linker options """
    linkerOptions = None

    def __init__(self):
        self.dependencies = {}
                        
    def __str__(self):
        s = ''
        for k in self.__dict__:
            if k != 'dependencies':
                s += '\n    ' + str(k) + ' = ' + str(self.__dict__[k])
        return s

    # For pickle module
    def __getinitargs__(self):
        return ()
            
    def __getnewargs__(self):
        return self.__getinitargs__()

    def setPropertiesOn(self, state):
        if self.includePaths == None:
            # This is an empty cache
            return
        
        for p in self.includePaths:
            state.addIncludePath(p)
            if state.includePaths().count(p) > 1:
                raise Exception('Duplicate instance of "' + p + '" in: ' + str(state.includePaths()))

        for p in self.libraryPaths:
            state.addLibraryPath(p)

        for opt in self.compilerOptions:
            if not opt in state.compilerOptions:
                state.compilerOptions.append(opt)

        for opt in self.linkerOptions:
            if not opt in state.linkerOptions:
                state.linkerOptions.append(opt)

        for p in self.usesProjectsList:
            if not p in state.usesProjectsList:
                state.addUsesProject(p)

        for p in self.usesLibrariesList:
            if not p in state.usesLibrariesList:
                state.addUsesLibrary(p)

  

    """ Copy over whatever state was using in order to preserve those
    values for the next invokation.  Some of these values are coming
    from the configuration files, however if the configuration files
    change the cache will be thrown away so that does not matter."""
    def getPropertiesFrom(self, state):
        self.includePaths = state.includePaths()
        self.libraryPaths = state.libraryPaths()
        self.compilerOptions = state.compilerOptions
        self.linkerOptions = state.linkerOptions
        self.usesProjectsList = state.usesProjectsList
        self.usesLibrariesList = state.usesLibrariesList


class Cache:
    # Definition of Cache
    warnings = None

    # Table mapping targets (e.g. RELEASE, DEBUG) to Depend instances
    depend = None

    """ Copy of the library table values """
    customLibraryList = None
    
    # For pickle module
    def __getinitargs__(self):
        return ()
    
    def __getnewargs__(self):
        return self.__getinitargs__()

    def __init__(self):
        self.warnings = {}
        self.depend = {}
        self.customLibraryList = []

    def __str__(self):
        s = 'Cache:'
        s += '\n  warnings = ' + str(self.warnings)
        s += '\n  customLibraryList = ' + str([str(x) for x in self.customLibraryList])
        for k in self.depend:
            s += '\n  depend[\'' + str(k) + '\'] = ' + str(self.depend[k])
            
        return s

    # Called by State.loadCache to update libraries
    def setPropertiesOn(self, state):
        # Add libraries that are not already in the table
        for lib in self.customLibraryList:
            print('iterating through custom libraries')
            if not lib.name in libraryTable:
                defineLibrary(lib)


# Use doticompile.getConfigurationState to load
#
class State:
    # e.g., linux-gcc4.1
    platform                 	= None

    # The arguments that were supplied to iCompile preceeding --run
    args                        = None

    # e.g., 'osx', 'freebsd', 'linux'
    os                          = None

    # Directory in which the tempDir is stored; tempDir is 
    tempParentDir               = None

    # Temp directory where scratch and .o files are stored, relative to rootDir
    # this is always tempParentDir/<projname> so that multiple projects can
    # share a temp directory
    tempDir                     = None

    # Absolute location of the project root directory.  Ends in '/'.
    rootDir                     = None

    # RELEASE or DEBUG
    target                      = None

    # Name of the project (without .lib/.dll extension)
    projectName                 = None

    # Instance of Cache
    cache                       = None

    # Filename of the compiler. Set by doticompile.processProjectfile
    compiler                    = None
    
    # Detected architecture
    detectedArchitecture        = None
    
    # Detected cpu tuning
    detectedTuning              = None

    # A list of options to be passed to the compiler.  Does not include
    # verbose or warnings options.
    compilerOptions             = None

    # Options regarding warnings
    compilerWarningOptions      = None

    # Options regarding verbose
    compilerVerboseOptions      = None

    # A list of options to be passed to the linker.  Does not include
    # options specified in the configuration file.
    linkerOptions               = None
 
    # List of all library canonical names that are known to be
    # used by this project.
    usesLibraries               = None

    # List of all projects that this one depends on, determined by the
    # uses: line of 
    usesProjectsList            = None
    usesLibrariesList           = None

    # Location to which all build files are written relative to 
    # rootDir
    buildDir                    = None

    # Location used as the CWD when running the program
    workDir                     = None

    # Location to which distribution files are written relative to rootDir
    installDir                  = None
     
    # Location to which generated binaries are written
    binaryDir                   = None

    # Binary name not including directory.  Set by setVariables.
    binaryName                  = None

    # EXE, LIB, or DLL. Set by setVariables.
    binaryType                  = None

    # Location to which object files are written (target-specific).
    # Set by setVariables
    objDir                      = None
  
    # If true, the user is never prompted
    noPrompt                    = False

    # Name of the template or empty string if none was specified
    template                    = None
  
    # Compiled regular expression for files to ignore during compilation
    excludeFromCompilation      = None

    # All paths for #include; updated as libraries are detected
    _includePaths               = None
    _libraryPaths               = None

    # All libraries (by name) that are used
    _libList                    = None

    # On OS X, should we build universal binaries?
    # Set in 
    universalBinary             = False

    numProcessors               = None

    # Time at which icompile or the project file
    # was most recently modified
    icompileTime                = None

    def __init__(self):
        self.usesProjectsList = []
        self.usesLibrariesList = []
        self.compilerVerboseOptions = []

        # includes HT virtual cores, unlike the cpuCount() function
        self.numProcessors = multiprocessing.cpu_count()
        self.args = []

    # path is either a string or a list of paths
    # Paths are only added if they exist.
    def addIncludePath(self, path):
        self._includePaths = _pathAppend(self._includePaths, path, True)

    # Returns a list of all include paths
    def includePaths(self):
        return self._includePaths

    # Adds path (which can be a list or a single path) to the end
    # of the library path list.
    def addLibraryPath(self, path):
        self._libraryPaths = _pathAppend(self._libraryPaths, path, True)

    def libraryPaths(self):
        return self._libraryPaths

    def setLibList(self, L):
        self._libList = L

    def libList(self):
        return self._libList

    def addUsesProject(self, dirname):
        self.usesProjectsList.append(dirname)

    def addUsesLibrary(self, dirname):
        self.usesLibrariesList.append(dirname)

    # Location of the user configuration (.icompile) file, including the filename
    # Defaults to $HOME/.icompile
    def preferenceFile(self):
    
        if ('--config' in self.args):
            for i in range(0, len(self.args)):
                if self.args[i] == '--config':
                    if i < len(self.args) - 1:
                        pref = self.args[i + 1]
                        if not os.path.exists(pref):
                            colorPrint("ERROR: Config file '" + pref + "' does not exist.", ERROR_COLOR)
                            sys.exit(-200)
                        else:
                            return pref
                        
        # Otherwise, fall through

        HOME = os.environ['HOME']
        return pathConcat(HOME, '.icompile')

    """ Returns the cache containing information about this compilation target. """
    def getTargetCache(self):
        if self.cache.depend == None:
            raise Exception('Dependency cache has not been initialized')
        
        if not self.target in self.cache.depend:
            self.cache.depend[self.target] = Depend()
            
        return self.cache.depend[self.target]

        
    def __str__(self):
        s = 'State:'
        for k in self.__dict__:
            if k != 'cache':
                s += '\n  ' + str(k) + ' = ' + str(self.__dict__[k])
        return s

    def saveCache(self, filename):
        self.getTargetCache().getPropertiesFrom(self)

        # Libraries were automatically updated inside
        # identifySiblingLibraryDependencies()
        
        file = open(filename, 'wb')
        pickle.dump(self.cache, file)
        file.close()
        if verbosity >= TRACE:
            print('Saved cache:')
            print(self.cache)

    def loadCache(self, filename):
        if verbosity >= TRACE: print("Loading cache from " + filename + "\n")

        self.cache = Cache()

        if os.path.exists(filename) and (getTimeStamp(filename) >= self.icompileTime):
            # Cache exists and is newer than the project file
        
            file = open(filename, 'rb')
            try:
                self.cache = pickle.load(file);
                if not isinstance(self.cache, Cache):
                    raise Exception('Cache format changed in version 0.5.5')
            
            except Exception as e:
                # The cache is corrupt; ignore (and delete) it
                print(e)
                self.cache = Cache()
                if verbosity >= NORMAL: 
                    colorPrint("Warning: Internal iCompile cache at '" + os.path.abspath(filename) +
                               "' is corrupt.", WARNING_COLOR)
                os.remove(filename)
 
            file.close()
        elif verbosity >= TRACE: print('No cache found, or cache is out of date')

        if verbosity >= TRACE:
            print('Loaded Cache:')
            print(self.cache)
            print()

        self.cache.setPropertiesOn(self)
        self.getTargetCache().setPropertiesOn(self)

###############################################

def isLibrary(L):
    return L == LIB or L == DLL

##################################################

"""allFiles is a list of all files on which something
   depends for the project.

   Returns a list of strings

   extraOpts are options that are needed for compilation
   but not dependency checking:
     state.compilerWarningOptions + state.compilerVerboseOptions
   """
def getCompilerOptions(state, allFiles, extraOpts = []):
    opt = state.compilerOptions + extraOpts + ['-c']
    
    for i in state.includePaths():
        if (' ' in i):
            # Surround the path in quotes and escape slashes (still doesn't seem to work with spawnv)
            i = '\'' + i + '\''
            i = i.replace(' ', '\\ ')
            
        opt += ['-I', i]

    # See if the xmm intrinsics are being used
    # This was disabled because -msse2 allows code generation,
    # not just explicit use of intrinsics
    #for f in allFiles:
    #    if f[-11:] == 'xmmintrin.h':
    #        opt += ['-msse2']
    #        break
        
    return opt

##############################################################################
#                            Discover Platform                               #
##############################################################################

def discoverOS(state):
    state.os = ''
    # Figure out the os
    if (os.name == 'nt'):
        state.os = 'win'
    elif (os.uname()[0] == 'Linux'):
        state.os = 'linux'
    elif (os.uname()[0] == 'FreeBSD'):
        state.os = 'freebsd'
    elif sys.platform.startswith('darwin'):
        state.os = 'osx'
    else:
        raise Exception('iCompile only supports FreeBSD, Linux, ' + 
          'OS X, and Windows')


def discoverCompiler(state):
    state.universalBinary = False#(state.os == 'osx') and (machine() == 'i386')

    nickname = getCompilerNickname(state.compiler)

    state.platform = state.os + '-' + machine() + '-' + nickname
