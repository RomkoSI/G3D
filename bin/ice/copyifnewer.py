# copyifnewer.py
#
#
from __future__ import print_function

import re, string
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

""" Linked list of the source names that were copied """
_copyIfNewerCopiedAnything = None


"""
Recursively copies all contents of source to dest 
(including source itself) that are out of date.  Does 
not copy files matching the excludeFromCopying patterns.

Returns a list of the files (if any were copied)

If actuallyCopy is false, doesn't actually copy the files, but still prints.

"""
def copyIfNewer(source, dest, echoCommands = True, echoFilenames = True, actuallyCopy = True):
    global _copyIfNewerCopiedAnything
    _copyIfNewerCopiedAnything = []
    
    if source == dest:
        # Copying in place
        #print 'Copying in place: nothing to do'
        return []

    dest = removeTrailingSlash(dest)

    if (not os.path.exists(source)):
        # Source does not exist
        #print 'Source does not exist: nothing to do'
        return []

    if (not os.path.isdir(source) and newer(source, dest)):
        if echoCommands: 
            colorPrint('cp ' + source + ' ' + dest, COMMAND_COLOR)
        elif echoFilenames:
            print(source)
        
        if actuallyCopy:
            shutil.copyfile(source, dest)
                
        _copyIfNewerCopiedAnything += [source]
        
    else:
        # Walk is a special iterator that visits all of the
        # children and executes the 2nd argument on them.  
        for dirpath, subdirs, filenames in os.walk(source):
              _copyIfNewerVisit([len(source), dest, echoCommands, echoFilenames, actuallyCopy], dirpath, filenames, subdirs)

    if len(_copyIfNewerCopiedAnything) == 0 and echoCommands:
        print(dest + ' is up to date with ' + source)
        
    return _copyIfNewerCopiedAnything
    
#########################################################################
    
"""Helper for copyIfNewer.

args is a list of:
[length of the source prefix in sourceDirname,
 rootDir of the destination tree,
 echo commands
 echo filenames]
"""
def _copyIfNewerVisit(args, sourceDirname, files, subdirs):
    global _copyIfNewerCopiedAnything

    if (excludeFromCopying.search(betterbasename(sourceDirname)) != None):
        # Don't recurse into subdirectories of excluded directories
        del subdirs[:]
        return

    prefixLen   = args[0]
    # Construct the destination directory name
    # by concatenating the root dir and source dir
    destDirname = pathConcat(args[1], sourceDirname[prefixLen:])
    dirName     = betterbasename(destDirname)

    echoCommands  = args[2]
    echoFilenames = args[3]
    actuallyCopy  = args[4]
   

    # Create the corresponding destination dir if necessary
    if actuallyCopy:
        mkdir(destDirname, echoCommands)

    # Iterate through the contents of this directory   
    for name in files:
        source = pathConcat(sourceDirname, name)

        if (excludeFromCopying.search(name) == None) :
            
            # Copy files if newer
            dest = pathConcat(destDirname, name)
            if (newer(source, dest)):
                if echoCommands:
                    colorPrint('cp ' + source + ' ' + dest, COMMAND_COLOR)
                elif echoFilenames: 
                    print(name)
                _copyIfNewerCopiedAnything += [source]
                if actuallyCopy:
                    shutil.copy(source, dest)


                    
