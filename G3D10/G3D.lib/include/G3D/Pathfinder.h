/**
 \file Pathfinder.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2014-10-25
 \edited  2014-10-25
 
 G3D Innovation Engine
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/
#ifndef G3D_Pathfinder_h
#define G3D_Pathfinder_h

#include "platform.h"
#include "HashTrait.h"
#include "SmallArray.h"
#include "Array.h"
#include "Table.h"

namespace G3D {

/** 
    \brief Finds good paths between nodes in an arbitrary directed graph.

    Subclass and override estimateCost(), costOfEdge(), and getNeighbors().

    \param Node must support hashCode (or provide a HashFunc) and
    operator== (see G3D::Table). Two Nodes must be == if and only if
    they describe the same location in the graph.  For a regular grid,
    use a class like G3D::Point2int32. For an arbitrary graph, Node
    may be a pointer or shared_ptr to a node, or an arbitrary unique
    ID. Node should be a relatively small object because it will be 
    copied a lot during Pathfinding.

    \cite Ported from Morgan McGuire's Javascript implementation
     at http://codeheartjs.com/examples/pathfinding/findPath.js

     Example:
     \code
#include <G3D/G3DAll.h>

class Map : public Pathfinder<Point2int32> {
protected:

    shared_ptr<Image> m_grid;

    bool isOpen(const Point2int32 P) const {
        return (m_grid->get<Color1>(P, WrapMode::CLAMP).value <= 0.5f);
    }

    void appendIfOpen(const Point2int32 P, NodeList& neighborArray) const {
        if (isOpen(P)) { neighborArray.append(P); }
    }

public:
    
    Map(const String& filename) {
        m_grid = Image::fromFile(filename);
    }

    virtual float estimateCost(const Point2int32& A, const Point2int32& B) const override {
        // Manhattan distance
        return abs(A.x - B.x) + abs(A.y - B.y);
    }


    virtual void getNeighbors(const Point2int32& A, NodeList& neighborArray) const override {
        neighborArray.clear();
        appendIfOpen(A + Point2int32(-1,  0), neighborArray);
        appendIfOpen(A + Point2int32(+1,  0), neighborArray);
        appendIfOpen(A + Point2int32( 0, -1), neighborArray);
        appendIfOpen(A + Point2int32( 0, +1), neighborArray);
    }


    void drawPath(const Point2int32& A, const Point2int32& B, const Path& path, const String& filename) const {
        // Draw the path
        const shared_ptr<Image>& image = m_grid->clone();
        printf("Path has length %d\n", path.length());
        for (int i = 0; i < path.length(); ++i) {
            image->set(path[i], Color3::yellow());
        }
        image->set(A, Color3::green());
        image->set(B, Color3::red());
        image->save(filename);
    }
};


int main(int argc, char** argv) {

    Map map("test.png"); Point2int32 A(10, 10), B(20, 4);
    //Map map("4x4.png"); Point2int32 A(1, 1), B(2, 2);
    //Map map("6x6.png"); Point2int32 A(1, 1), B(4, 4);


    Map::Path path;
    Map::StepTable bestPathTo;
    if (map.findPath(A, B, path, bestPathTo)) {
        map.drawPath(A, B, path, "result.png");
    } else {
        printf("No path found!\n");
    }
    
    debugPrintf("bestPathTo:\n");
    for (Map::StepTable::Iterator it = bestPathTo.begin(); it.hasMore(); ++it) {
        printf("  %s  <-  %s\n", it->value.to.toString().c_str(), it->value.from.isNull() ? "NULL" : it->value.from.node().toString().c_str());
    }

    return 0;
}
\endcode
    */
template<typename Node, class HashFunc = HashTrait<Node> >
class Pathfinder {
public:

    // Large enough to store a voxel grid's 1-ring without allocating heap memory
    typedef SmallArray<Node, 6> NodeList;
    typedef Array<Node>         Path;

    /** An inefficient implementation of a priority queue. A heap data
        structure would make a more asymptotically efficient
        implementation at the cost of some implementation
        complexity. For short queues the difference is not
        significant, but for long queues the performance difference is
        O(n) vs. O(log n) for the removeMin operation. The advantage
        of this implementation is that we avoid complexity in the
        update() call, which must be backed by a hash table in any
        case for efficiency but which requires a more complex tree
        traversal if a heap is used.
      */
    template<class Key, class Value>
    class PriorityQueue {
    private:

        class Entry {
        public:
            Value value;
            float cost;
            Entry() : cost(0.0f) {}
            Entry(const Value& v, float c) : value(v), cost(c) {}
        };

        Table<Key, Entry> m_table;

    public:

        void insert(const Key& k, const Value& v, float cost) {
            debugAssert(! m_table.containsKey(k));
            m_table.set(k, Entry(v, cost));
        }

        /** Update the cost of value N */
        void update(const Key& k, float cost) {
            m_table[k].cost = cost;
        }

        int length() const {
            return int(m_table.size());
        }

        /** Returns the minimum cost value in O(n) time in the length
            of the queue. */
        Value removeMin() {
            debugAssert(length() > 0);
            Value v;
            Key k;
            float minCost = finf();
            for (typename Table<Key, Entry>::Iterator it = m_table.begin(); it.hasMore(); ++it) {
                if (it->value.cost <= minCost) {
                    v = it->value.value;
                    k = it->key;
                    minCost = it->value.cost;
                }
            }

            m_table.remove(k);
            return v;
        }
    };

    class NodeOrNull {
    private:
        Node    m_node;
        bool    m_isNull;

    public:

        NodeOrNull() : m_isNull(true) {}

        NodeOrNull(const Node& n) : m_node(n), m_isNull(false) {}
        
        bool isNull() const { 
            return m_isNull; 
        }

        bool notNull() const {
            return ! m_isNull;
        }

        const Node& node() const {
            debugAssert(! m_isNull);
            return m_node;
        }

        void setNode(const Node& n) {
            m_isNull = false;
            m_node = n;
        }

        void setNull() {
            m_isNull = true;
            m_node = Node();
        }

        bool operator==(const NodeOrNull& other) const {
            if (m_isNull) {
                return other.m_isNull;
            } else {
                return ! other.m_isNull && (m_node == other.m_node);
            }
        }

        static size_t hashCode(const NodeOrNull& n) {
            if (n.m_isNull) {
                return 0;
            } else {
                return HashFunc::hashCode(n.m_node);
            }
        }
    };

    /** Used by findPath */
    class Step {
    public:
        /** The end of the step */
        Node        to;

        /** The beginning of the step, which may be NULL for the first step */
        NodeOrNull  from;

        /** Known exactly */
        float       costFromStart;

        /** Estimated */
        float       costToGoal;

        /** Is this step currently in the priority queue? */
        bool        inQueue;

        Step(const Node& to, float startCost, float goalCost, const NodeOrNull& from = NodeOrNull()) :
            to(to),
            from(from),
            costFromStart(startCost),
            costToGoal(goalCost),
            inQueue(true) {}

        Step() : costFromStart(finf()), costToGoal(finf()), inQueue(false) {}

        float totalCost() const {
            return costFromStart + costToGoal;
        }
    };

    typedef Table<Node, Step> StepTable;

    virtual ~Pathfinder() { }

    /** Returns an estimate of the cost of traversing from A to B. Return finf() if
        B is known to not be reachable from A. */
    virtual float estimateCost(const Node& A, const Node& B) const = 0;

    /** Returns the exact cost of traversing the directed edge from A to B, 
        which are two nodes known to be neighbors.
        The default implementation returns 1.0f for all pairs. */
    virtual float costOfEdge(const Node& A, const Node& B) const {
        return 1.0f;
    }
    
    /** Identifies all nodes (directionally) adjacent to N. First clears neighbors. */
    virtual void getNeighbors(const Node& N, NodeList& neighbors) const = 0;

    /**
       Finds a good path from start to goal, and
       returns it as a list of nodes to visit.  Returns null if there is
       no path.

       The default implementation uses the A* algorithm. 

       For visualization purposes, findPathBestPathTo contains information
       about the other explored paths when the function returns.

       \param bestPathTo Maps each Node to the Step on the best known
       path to that Node. Provided for visualization purposes. Cleared
       at the start. Note that the overloaded version of findPath()
       does not require a StepTable.

       \return True if a path was found, otherwise false
     */
    virtual bool findPath(const Node& start, const Node& goal, Path& path, StepTable& bestPathTo) const {
        bestPathTo.clear();
        path.fastClear();

        // Paths encoded by their last Step paired with expected shortest
        // distance
        PriorityQueue<Node, Step> queue;
    
        Step lastStepOnShortestPath(start, 0.0f, estimateCost(start, goal));
        bestPathTo.set(start, lastStepOnShortestPath);
        queue.insert(lastStepOnShortestPath.to, lastStepOnShortestPath, lastStepOnShortestPath.totalCost());

        while (queue.length() > 0) {
            lastStepOnShortestPath = queue.removeMin();
            lastStepOnShortestPath.inQueue = false;

            // Last node on the shortest path.
            const Node& P = lastStepOnShortestPath.to;
        
            // Test if we've reached the end point
            if (P == goal) {
                // We're done.  Generate the path to the goal by
                // retracing steps from the goal backwards
                path.append(goal);
                for (Step currentStep = lastStepOnShortestPath; currentStep.from.notNull(); ) {
                    // There are more steps. How did we reach this
                    // location?
                    currentStep = bestPathTo[currentStep.from.node()];

                    // Add the current step to the path
                    path.append(currentStep.to);
                }

                // Reorder so that the first location visited is actually the first
                // in the array.
                path.reverse();

                return true;
            }

            // Consider all neighbors of P (that are still in the queue
            // for consideration)
            NodeList neighbors;
            getNeighbors(P, neighbors);
            for (int i = 0; i < neighbors.size(); ++i) {
                const Node& N = neighbors[i];
                const float newCostFromStart = lastStepOnShortestPath.costFromStart + costOfEdge(P, N);
            
                // Find the current-best known way to neighbor N (or
                // create it, if there isn't one).  Keep a reference
                // so that we can mutate it based on new information.
                bool created = false;
                Step& bestKnownStepToN = bestPathTo.getCreate(N, created);
                if (created) {
                    // We've never seen this neighbor before
                    bestKnownStepToN = Step(N, newCostFromStart, estimateCost(N, goal), P);
                    queue.insert(N, bestKnownStepToN, bestKnownStepToN.totalCost());

                } else if (bestKnownStepToN.inQueue && (bestKnownStepToN.costFromStart > newCostFromStart)) {
                    // We have seen this neighbor before, but just discovered a better way to reach it
                    // Update the Step to N with the new, lower cost and new route through P
                    bestKnownStepToN.costFromStart = newCostFromStart;
                    bestKnownStepToN.from.setNode(P);

                    // Notify the priority queue of the new, lower cost
                    queue.update(N, bestKnownStepToN.totalCost());
                }
            
            } // for each neighbor
        
        } // while queue not empty

        // There was no path from start to goal
        return false;
    }


    bool findPath(const Node& start, const Node& goal, Path& path) const {
        StepTable bestPathTo;
        return findPath(start, goal, path, bestPathTo);
    }
};

} // namespace G3D

#endif
