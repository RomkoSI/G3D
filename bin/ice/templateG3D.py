# templateG3D.py
#
from __future__ import print_function

import os
from .utils import *
from .variables import *
from . import copyifnewer
import copy

def findG3DStarter(state, name):
    from .doticompile import processProjectFile
    # load the default .icompile
    fakeState = copy.copy(state)
    processProjectFile(fakeState, True)

    locations = [pathConcat(x, '../') for x in fakeState.includePaths()]
    locations += [pathConcat(x, '../') for x in os.environ['G3D10DATA'].split(':')]
    locations += [
        pathConcat(os.environ['HOME'], 'Projects/G3D/'),
        '/usr/local/G3D/']
    
    for path in locations:
        f = pathConcat(path, 'samples/' + name)
        if os.path.exists(f):
            return f

    raise Exception('Could not find G3D installation in ' + '\n'.join(locations))

""" Generates an empty project. """
def generateStarterFiles(state, name):
    if isLibrary(state.binaryType):
        colorPrint("ERROR: G3D template cannot be used with a library", ERROR_COLOR)
        sys.exit(-232)
                
    starterPath = findG3DStarter(state, name)
    
    print('\nCopying G3D starter files from ' + starterPath)

    mkdir('doc-files')
    copyifnewer.copyIfNewer(pathConcat(starterPath, '.'), '.')
    #for d in ['data-files', 'source', 'journal', '.']:
    #    copyifnewer.copyIfNewer(pathConcat(starterPath, d), d)
     

