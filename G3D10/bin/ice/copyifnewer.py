# copyifnewer.py
#
#
from __future__ import print_function

import re, string, glob
from .utils import *

_excludeDirPatterns = \
    ['^#',\
     '~$',\
     '^\.svn$',\
     '^\.git$',\
     '^CVS$', \
     '^Debug$', \
     '^Release$', \
     '^graveyard$', \
     '^tmp$', \
     '^temp$', \
     '^\.icompile-temp$', \
     '^\.ice-tmp$', \
     '^build$']
#'^\.',\

""" Regular expression patterns that will be excluded from copying by 
    copyIfNewer.
"""
_excludeFromCopyingPatterns =\
    ['\.ncb$', \
    '\.opt$', \
    '\.sdf$', \
    '\.ilk$', \
    '\.cvsignore$', \
    '^\.\#', \
    '\.pdb$', \
    '\.bsc$', \
    '^\.DS_store$', \
    '\.o$', \
    '\.pyc$', \
    '\.plg$', \
    '^#.*#$', \
    '^ice-stats\.csv$'\
    '~$', \
    '\.old$' \
    '^log.txt$', \
    '^stderr.txt$', \
    '^stdout.txt$', \
    '\.log$', \
    '\^.cvsignore$'] + _excludeDirPatterns

"""
  Regular expression patterns (i.e., directory and filename patterns) that are 
  excluded from the search for cpp files
"""
_cppExcludePatterns = ['^test$', '^tests$', '^#.*#$', '~$', '^old$'] + _excludeFromCopyingPatterns

"""
A regular expression matching files that should be excluded from copying.
"""
excludeFromCopying  = re.compile('|'.join(_excludeFromCopyingPatterns))


"""
Recursively copies all contents of source to dest 
(including source itself) that are out of date.  Does 
not copy files matching the excludeFromCopying patterns.

Returns a list of the files (if any were copied)

If actuallyCopy is false, doesn't actually copy the files, but still prints.

"""
def copyIfNewer(source, dest, echoCommands = True, echoFilenames = True, actuallyCopy = True):
    copiedFiles = []
    
    if source == dest:
        # Copying in place
        #print('copyIfNewer: Copying in place...nothing to do')
        1 # some statement

    elif ('*' in source) or ('?' in source):
        # expand wildcards
        for s in glob.glob(source):
            copiedFiles += copyIfNewer(s, dest, echoCommands, echoFilenames, actuallyCopy)

    else:
        if not os.path.exists(source):
            # Source does not exist
            print('copyIfNewer: Source (' + source + ') does not exist. Nothing to do')

        elif os.path.isdir(source):
            # Walk is a special iterator that visits all of the children recursively
            for dirpath, subdirs, filenames in os.walk(source):
                copiedFiles += _copyIfNewerVisit(len(source), dest, echoCommands, echoFilenames, actuallyCopy, dirpath, filenames, subdirs)

        else:
            # Source is a single file

            if (dest[-1] == '/') or (dest[-1] == '\\'):
                # Destination is a directory. Figure out the filename
                dest = pathConcat(dest, betterbasename(source))

            #dest = removeTrailingSlash(dest)
            
            if newer(source, dest):
                if echoCommands: 
                    colorPrint('cp ' + source + ' ' + dest, COMMAND_COLOR)
                elif echoFilenames:
                    print(source)
        
                if actuallyCopy:
                    shutil.copyfile(source, dest)
                    copiedFiles += [source]
            elif echoCommands:
                print(dest + ' is up to date with ' + source)

    return copiedFiles
    
#########################################################################
    
"""Helper for copyIfNewer."""
def _copyIfNewerVisit(prefixLen, rootDir, echoCommands, echoFilenames, actuallyCopy, sourceDirname, files, subdirs):

    if (excludeFromCopying.search(betterbasename(sourceDirname)) != None):
        # Don't recurse into subdirectories of excluded directories
        del subdirs[:]
        return []

    # Construct the destination directory name
    # by concatenating the root dir and source dir
    destDirname = pathConcat(rootDir, sourceDirname[prefixLen:])
    dirName     = betterbasename(destDirname)

    # Create the corresponding destination dir if necessary
    if actuallyCopy:
        mkdir(destDirname, echoCommands)

    copiedFiles = []
    # Iterate through the contents of this directory   
    for name in files:
        source = pathConcat(sourceDirname, name)

        if excludeFromCopying.search(name) == None:
            
            # Copy files if newer
            dest = pathConcat(destDirname, name)
            if (newer(source, dest)):
                if echoCommands:
                    colorPrint('cp ' + source + ' ' + dest, COMMAND_COLOR)
                elif echoFilenames: 
                    print(name)
                copiedFiles += [source]
                if actuallyCopy:
                    shutil.copy(source, dest)

    return copiedFiles
