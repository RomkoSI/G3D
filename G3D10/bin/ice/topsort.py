# topsort.py
#
# General Topological Sort
from __future__ import print_function

import sys

##############################################################################
#                            Topological Sort                                #
##############################################################################

"Return a list of all edges (v0, w) in E starting at v0. Used by topsort."
def _allEdgesFrom(v0, E):
    resEdges = []
    for v, w in E:
        if v0 == v:
            resEdges.append((v, w))
    return resEdges


"Recursive topsort function."
def _topSort(v, E, visited, sorted, sortedIndices):

    # print "trying", v, visited, sorted, sortedIndices
    visited[v] = 1
    for v, w in _allEdgesFrom(v, E):
        # print "edge found", (v, w)
        if not visited[w]:
            _topSort(w, E, visited, sorted, sortedIndices)
        else:
            if not sorted[w]:
                print('Cyclic dependency in links.')
                sys.exit(0)
    sorted[v] = 1
    sortedIndices.insert(0, v)


"""
Topological sort.

V = list of vertex names
E = list of 0-indexed edge pairs between vertices

Result is a list of sorted vertex names

Based on Topsort by Nathan Wallace, Matthias Urlichs
Hans Nowak, Snippet 302, Dinu C. Gherman
http://www.faqts.com/knowledge_base/view.phtml/aid/4491

Example of use:

import topsort

pairs = [('a', 'b'), ('b', 'c'), ('a', 'd'), ('b', 'd')]
print pairs

(V, E) = topsort.pairsToVertexEdgeGraph(pairs)
print
print V
print E

result = topsort.topSort(V, E)
print
print result
"""
def topSort(V, E):
    n = len(V)
    visited = [0] * n   # Flags for (un-)visited elements.
    sorted  = [0] * n   # Flags for (un-)sorted elements.
    sortedIndices = []  # The list of sorted element indices.

    for v in range(n):
        if not visited[v]:
            _topSort(v, E, visited, sorted, sortedIndices)

    # Build and return a list of elements from the sort indices.
    sortedElements = [V[i] for i in sortedIndices]

    return sortedElements


""" For use in preparing input for topsort. 
Input is a dictionary whose values are the vertices connected to its keys.
Output is a list of pairs for pairsToVertexEdgeGraph
"""
def dictionaryToPairs(dict):
    pairs = [];
    for k in dict:
        for v in dict[k]:
            pairs.append( (k, v) )

    return pairs



"""Convert an element pairs list into (verticesList, edgeIndexList)
   for topSort.

   e.g. wrap( [('a','b'), ('b','c'), ('c','a')] )
         -> (['a','b','c'], [(0,1), (1,2), (2,0)])

   Returns Vertices, (indexed) Edges
"""
def pairsToVertexEdgeGraph(pairs):
    V = set()

    # Make a list of unique vertices.
    for x, y in pairs:
        V.add(x)
        V.add(y)

    # Convert the set to a list
    V = [v for v in V]
  
    # Make a fast index lookup table
    indexOf = {}
    i = 0
    for v in V:
        indexOf[v] = i
        i += 1

    # Convert original element pairs into index pairs.
    E = [(indexOf[x], indexOf[y]) for x, y in pairs]

    return V, E

