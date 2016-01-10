#
# icedepend.py
#
# Determines if files are out of date. Routines:
# 
#  getOutOfDateFiles
#  getObjectFilename
#  getDependencies
#  getDependencyInformation
#  getLibrarySiblingDirs
#  identifySiblingLibraryDependencies
#  anyRelativeFilenameIn
from __future__ import print_function

from .utils import *
from .variables import *
from .library import *
from .doticompile import *
from .help import *

###############################################################

"""
  cfiles - list of fully qualified source files
  dependences - table(cfile, list of files on which it depends)
  files - list of all fully qualified files that are the dependencies of anything
  timeStamp - table(filename, timestamp) time stamp cache for use with getTimeStampCached

  Returns a list of all files that are older than their dependencies.
  
  The dependences have already been recursively expanded; if A depends on B and
  B depends on C, then dependencies[A] = [B, C]
"""
def getOutOfDateFiles(state, cfiles, dependencies, files, timeStamp):
    
    buildList = []
    
    # Need to rebuild all if iCompile itself was modified more
    # recently than a given file.

    icompileTime = state.icompileTime

    # Generate a list of all files newer than their targets
    for cfile in cfiles:
        targetFile = getObjectFilename(state, cfile)
        targetTime = getTimeStampCached(targetFile, timeStamp)

        # See if the object file is out of date because it is older than iCompile
        rebuild = (targetTime < icompileTime)
        if rebuild and (verbosity >= TRACE):
            print('iCompile is newer than ' + targetFile)

        # See if the object file is out of date because it is older than a dependency
        if not rebuild:
            # Check dependencies
            for dependencyFile in dependencies[cfile]:
                dependencyTime = getTimeStampCached(dependencyFile, timeStamp)
                if targetTime < dependencyTime:
                    if verbosity >= TRACE:
                        print(d + " is newer than " + targetFile)
                    rebuild = 1
                    break

        if rebuild:
            # This file needs to be rebuilt
            buildList.append(cfile)

    return buildList

#########################################################################

"""Given a source file (relative to rootDir) returns an object file
  (relative to rootDir).
"""
def getObjectFilename(state, sourceFile):
    # strip the extension and replace it with .o
    i = sourceFile.rfind('.')
    return state.objDir + sourceFile[len(state.rootDir):i] + '.o'

#########################################################################

# Returns a new list that has any -arch flags removed from oldList.
def _removeArch(oldList):
    if not ('-arch' in oldList):
        return oldList
    
    newList = []
    skipNext = False
    for arg in oldList:
        if skipNext:
            # explicitly skip this element, but disable skipping from now on
            skipNext = False
        elif (arg == '-arch'):
            skipNext = True
            # and implicitly skip this element
        else:
            newList += [arg]

    return newList


"""
   Returns a tuple.  The first element is True if dependencies need to
   be recomputed after sibling libraries are added.  The second
   element is a list of *all* files on which file depends (including
   itself).  Returned filenames will either be fully qualified or
   relative to state.rootDir.

   May modify the default compiler and linker options as a result of
   discovering common library dependencies.
 
   timeStamp = table(filename, timestamp) that is updated as getDependencies runs   
"""
def getDependencies(state, file, verbosity, timeStamp, iteration = 1):
    if verbosity >= VERBOSE: print('  ' + file)
    
    # dependencyCache: table(file, (timestamp at which dependencies were computed, dependency list))
    targetCache = state.getTargetCache()    
    dependencyCache = targetCache.dependencies
    
    # Assume that we can trust the cache, and try to prove otherwise
    trustCache = file in dependencyCache
    if trustCache:
        entry = dependencyCache[file]
        timeComputed = entry[0]
        if getTimeStampCached(file, timeStamp) > timeComputed:
            if verbosity >= SUPERTRACE:
                print(('Cannot use cached dependency information for ' + file +
                       ' because it has changed.'))
            # The file has changed since we checked dependencies
            trustCache = False
        else:
            dependencies = entry[1]
            for d in dependencies:
                if not os.path.exists(d):
                    raise Exception('Internal error: dependency ' + d +
                                    ' of ' + file + ' does not exist.')
                
                if getTimeStampCached(d, timeStamp) > timeComputed:
                    if verbosity >= TRACE:
                        print(('Cannot use cached dependency information for ' + file +
                        ' because ' + d + ' has changed.'))
                    # One of the dependencies has itself changed since
                    # we checked dependencies, which means that there might
                    # be new dependencies for file
                    trustCache = False
                    break
    else:
        if verbosity >= SUPERTRACE:
            print('There is no cached dependency information for ' + file + '.')
                    
    if trustCache:
        # The entry for this file is up to date.  If it is up to date,
        # it must also not need resolution.
        if verbosity >= SUPERTRACE: print('Using cached dependency information for ' + file)        
        return (False, dependencies)
    
    # ...otherwise, dependency information for this file is out of date, so recompute it

    # We need to use the -msse2 flag during dependency checking because of the way
    # xmmintrin.h is set up
    #
    # -MG means "don't give errors for header files that don't exist"
    # -MM means "don't tell me about system header files". 
    # -w  means "shut off warnings", which would otherwise confuse our dependency checker
    #
    # We have to remove -arch flags, which are not compatible with the -M flags
    argsWithoutArchitecture = _removeArch(getCompilerOptions(state, []))
    
    if 'clang' in state.compiler and extname(file).lower() == 'c':
        argsWithoutArchitecture += ['-x', 'c']

    argsWithoutArchitecture = ' '.join(argsWithoutArchitecture)

    argsWithoutArchitecture = maybeRemoveCPPOptions(file, argsWithoutArchitecture)

    raw = shell(state.compiler + ' -M -msse2 -MG -w ' + argsWithoutArchitecture + ' ' + file, verbosity >= TRACE)
    
    if verbosity >= SUPERTRACE:
        print('Raw output of dependency determination:')
        print(raw)

    if (' error:' in raw) or ('invalid argument' in raw):
        print('Error while computing dependencies for ' + file)
        print(raw)
        sys.exit(-1)

    if raw.startswith('cc1plus: warning:'):
        # Remove the first line
        raw = raw.split('\n', 1)[1]
        
    if raw.startswith('In file included from'):
        # If there was an error, the output will have the form
        # "In file included from _____:" and a list of files
        
        if iteration == 3:
            # Give up; we can't resolve the problem
            print(raw)
            sys.exit(-1)
        
        if verbosity >= VERBOSE:
            print(('\nThere were some errors computing dependencies.  ' +
                   'Attempting to recover.('), iteration,')')
        
        # Generate a list of files and see if they are something we
        # know how to fix.
        noSuchFile = []
        for line in raw.split('\n'):
            if line.endswith(': No such file or directory'):
                x = line[:-len(': No such file or directory')]
                j = x.rfind(': ')
                if (j >= 0): x = x[(j+2):]

                # x now has the filename
                noSuchFile.append(betterbasename(x))

        if verbosity >= NORMAL: print('Files not found:' + str(noSuchFile))
        
        # Look for specific header files that we know how to handle
        for f in noSuchFile:
            if f == 'wx.h':
                if verbosity >= NORMAL: print('wxWindows detected.')
                copt = shell('wx-config --cxxflags', verbosity >= VERBOSE)
                lopt = shell('wx-config --gl-libs --libs', verbosity >= VERBOSE)
                targetCache.compilerOptions += copt
                targetCache.linkerOptions += lopt
                state.compilerOptions.append(copt)
                state.linkerOptions.append(lopt)
                
        return getDependencies(state, file, verbosity, timeStamp, iteration + 1)

    else:

        # Normal case, no error
        result = []
        # Turn end-of-line continuation characters into spaces
        raw = raw.replace('\\', ' ')
        
        for line in raw.splitlines():
            # gcc 3.4.2 likes to print the name of the file first, as in
            # """# 1 "/home/faculty/morgan/Projects/ice/tests/meta/helper.lib//""""
            if not line.startswith('# '):
                result += line.split(' ')
        
        # There is always at least one file in the raw file list,
        # since every file depends on itself.
        # Remove empty entries arising from string split
        result = [f for f in result if f != '']
            
        # The first element of result will be "file:", so remove that, and then strip the remaining files
        result = [f.strip() for f in result[1:]]

        # Make all paths absolute.  We can't just call abspath
        # since some files are in sibling directories.
        result = [makeAbsolute(f, state.includePaths()) for f in result]
        
        result.append(os.environ["HOME"] + "/.icompile")
        needResolution = anyRelativeFilenameIn(result)
        if needResolution:
            # Wipe the cache entry, since we're going to have to re-process
            # this file's dependencies
            if file in dependencyCache: del dependencyCache[file]
        else:
            # Update the cache entry.
            dependencyCache[file] = (time.time(), result)

        return (needResolution, result)

#########################################################################

"""
 Returns true if any file in this list has a relative (instead of absolute)
 name.
"""
def anyRelativeFilenameIn(fileList):
    for file in fileList:
        if not os.path.isabs(file):
            return True
    return False

#########################################################################

"""
 Finds file in searchPath if it is not already fully qualified.
 If the file cannot be found anywhere, it is returned unchanged.
"""
def makeAbsolute(file, searchPath):
    if os.path.isabs(file):
        return file

    f = os.path.abspath(file)
    if os.path.exists(f):
        return f

    for path in searchPath:
        f = os.path.join(path, file)
        if os.path.exists(f):
            return f

    # Could not find this file; just return it
    return file

#########################################################################

"""
 Returns (files that need to be rechecked, headers that were not found)

 allCFiles is the list of all source files to compile,

 dependencies[f] is the list of all files on which
 f depends,

 allDependencyFiles is the list of all files
 on which *any* file depends, and

 parents[f] is the list of all files that depend on f

 rerun is a list of files that need their dependencies recomputed after
 sibling libraries are processed.

 While getting dependencies, this may change the include/library
 list and restart the process if it appears that some include
 directory is missing.

 allCFiles is a list of source files (.c, .cpp)
 
 timeStamp is a dict of cached time stamps for use with
 getTimeStampCached. It is updated by the function.
 
 dependencySet is a Set of files on which something depends. It is
 updated by the function.

 dependencies is a dict mapping source files to the list of all files
 on which they depend.

 parents is a dict. parents[file] is a list of all files that depend on file.
"""
def getDependencyInformation(allCFiles, dependencySet, dependencies, parents, state, verbosity, timeStamp):
    if verbosity >= TRACE: print('Source files found:' + str(allCFiles))

    rerun = []
    missingHeaders = []
 
    for cfile in allCFiles:

        # Do not use warning or verbose options for this; they
        # would corrupt the output.
        (needResolution, dlist) = getDependencies(state, cfile, verbosity, timeStamp)

        if needResolution:
            # Add this file to the list of those that need to be recomputed after
            # checking for sibling libraries
            if verbosity >= TRACE:
                for d in dlist:
                    if not os.path.isabs(d): print('    Header not found (yet): ' + d)
                    
            rerun += [cfile]
        
        # Update the dependency set
        for d in dlist:
            dependencySet.add(d)
            if d in parents:
                parents[d] += cfile
            else:
                parents[d] = [cfile]
                
            if not os.path.isabs(d):
                missingHeaders += [d]
 
        dependencies[cfile] = dlist
    
    return (rerun, missingHeaders)

###############################################################################


""" Returns sibling directories that are also iCompile libraries as names
    relative to the current directory."""
def getLibrarySiblingDirs(howFarBack = 1):
    libsibs = []
    for dir in getSiblingDirs(howFarBack):

        # See if it contains an ice.txt and ends in a library extension
        ext = extname(dir).lower()
       
        if ((ext == 'lib') or (ext == 'a') or 
            (ext == 'so') or  (ext == 'dll')):
            libsibs.append(dir)

    return libsibs

#############################################################################

"""

For any header in files that is not found in the current include paths,
try to find it in a sibling directory that is an icompile library
and extend the state.includePaths and state.usesLibraryList as needed.

Called from buildBinary.

"""
def identifySiblingLibraryDependencies(files, parents, state):

    # See if any of the header files are not found; that might imply
    # sibling project is an implicit dependency.
    # Include the empty path on this list so that we'll find files
    # that are fully qualified as well as files in the current directory.
    # This list will be extended as we go forward.
    incPaths = [''] + state.includePaths()
    librarySiblingDirs = getLibrarySiblingDirs(3)

    for header in files:
        if len(header) == 1:
            raise Exception('Unexpected format from dependency checker: ' + str(files))

        # Try to locate the file using the existing include path
        found = False
        i = 0
        while not found and (i < len(incPaths)):
            found = os.path.exists(pathConcat(incPaths[i], header))
            i += 1

        # Try to locate the file in a sibling directory
        if not found:
            # This header doesn't exist in any of the standard include locations.
            # See if it is in a sibling directory that is a library.
            i = 0

            while not found and (i < len(librarySiblingDirs)):
                dirname = librarySiblingDirs[i]
                found = os.path.exists(dirname + '/include/' + header)

                if found:
                    if verbosity >= TRACE: print("Found '" + header + "' in '" + dirname + "/include'.")
                    # We have identified a sibling library on which this project appears
                    # to depend.  

                    type = EXE
                    libname = rawfilename(dirname)
                    ext = extname(dirname).lower()
                    if ext == 'dll' or ext == 'so':
                        type = DLL
                    elif ext == 'lib' or ext == 'a':
                        type = LIB

                    if isLibrary(type):
                        if verbosity >= TRACE: print('Need library "' + libname + '"')
                        if libname not in libraryTable:
                            newLib = Library(libname, type, libname, libname + 'd',  
                                             None,  None, [betterbasename(header)], [], [])
                            
                            # Define in the runtime structure
                            defineLibrary(newLib)

                            # Add to the cache
                            state.cache.customLibraryList.append(newLib)

                        if not libname in state.usesLibrariesList:
                            state.usesLibrariesList.append(libname)

                        if not dirname in state.usesProjectsList:
                            print(('Detected dependency on ' + dirname + ' from inclusion of ' + 
                                   header + ' by'), shortname(state.rootDir, parents[header][0]))

                            state.addUsesProject(dirname)

                            # Load the configuration state for the dependency project
                            curDir = os.getcwd()
                            os.chdir(dirname)
                            other = getConfigurationState(state.args)
                            os.chdir(curDir)

                            includepath = pathConcat(dirname, 'include')
                            libpath = maybePathConcat(dirname, other.binaryDir)

                            state.addIncludePath(includepath)
                            state.addLibraryPath(libpath)

                            # Update incPaths, which also includes the
                            # empty directory (that will not appear as
                            # a -I argument to the compiler because
                            # doing so generates an error)
                            incPaths = [''] + state.includePaths()

                            # TODO: recursively add all dependencies
                            # that come from this new header/library
                i += 1

        if not found:
            maybeWarn("Header not found: '" + header + "'.", state)


""" Remove any C++ specific options """
def maybeRemoveCPPOptions(cfile, options):
    if (extname(cfile).lower() == 'c') or isObjectiveC(cfile):
        # This is not a C++ file. Remove any -std lines
        i = options.find('-std=')
        while i >= 0:
            j = options.find(' ', i)
            if j == -1: j = len(options)
            options = options[:i] + options[j:]
            i = options.find('-std=')

    return options

