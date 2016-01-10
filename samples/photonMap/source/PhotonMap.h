#ifndef PhotonMap_h
#define PhotonMap_h

#include <G3D/G3DAll.h>
#include "Photon.h"

class PhotonMap {
private:

    // No significant performance difference from  PointHashGrid<Photon, Photon, Photon> 
    // for this program
    typedef FastPointHashGrid<Photon, Photon> PhotonGrid;

    PhotonGrid   m_grid;

public:

    PhotonMap() : m_grid(0.5f) {}

    void clear(float gatherRadius, int expectedCells = 64) {
        m_grid.clear(gatherRadius);
    }
    
    void insert(const Photon& photon) {
        m_grid.insert(photon);
    }

    void insert(const Array<Photon>& photonArray) {
        m_grid.insert(photonArray);
    }

    /** Number of photons */
    int size() const {
        return m_grid.size();
    }

    typedef PhotonGrid::SphereIterator SphereIterator;    
    SphereIterator begin(const Sphere& s) const {
        return m_grid.begin(s);
    }

    typedef PhotonGrid::Iterator Iterator;
    Iterator begin() const {
        return m_grid.begin();
    }

    void debugPrintStatistics() {
        m_grid.debugPrintStatistics();
    }
};

#endif
