#ifndef Photon_h
#define Photon_h

#include <G3D/G3DAll.h>

/** A node on a light transport path describing incident power immediately before scattering. */
class Photon {
public:

    Point3        position;

    /** Unit direction FROM which the photon arrived. I.e., points 
        opposite the direction of propagation. */
    Vector3       wi;

    /** Power transported along this path. */
    Power3        power;

    /** Radius over which this photon can be gathered */
    float         effectRadius;

    /** For PointHashGrid */
    bool operator==(const Photon& other) const {
        return (position == other.position) && (wi == other.wi) && (power == other.power);
    }

    /** For PointHashGrid */
    static void getPosition(const Photon& P, Vector3& pos) {
        pos = P.position;
    }

    /** For PointHashGrid */
    static bool equals(const Photon& p, const Photon& q) {
        return (p == q);
    }
};

#endif
