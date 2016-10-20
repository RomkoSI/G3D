/**
   \file HaltonSequence.h
 
    \maintainer Michael Mara, http://www.illuminationcodified.com
 
    \created 2013-02-19
    \edited  2013-02-19
    Copyright 2000-2015, Morgan McGuire.
    All rights reserved.
 */
#ifndef G3D_HaltonSequence_h
#define G3D_HaltonSequence_h

#include "G3D/Vector2int32.h"
#include "G3D/Vector2.h"


namespace G3D {

class HaltonSequence {

private:
    Point2int32 m_base;
    Point2int32 m_xCurrent;
    Point2int32 m_yCurrent;
    int m_currentIndex;

public:
    /** The next point in the Halton Sequence */
    void next(Point2& p);

    Point2 next() {
        Point2 p;
        next(p);
        return p;
    }

    /** Throw out the next N terms in the sequence */
    void trash(int n) {
        m_currentIndex += n;
        /*for (int i = 0; i < n; ++n) {
            next();
        }*/
    }

    void reset() {
        m_xCurrent = Point2int32(1, m_base.x);
        m_yCurrent = Point2int32(1, m_base.y);
    }

    /** To be a true Halton sequence, xBase and yBase must be prime, and not equal */
    HaltonSequence(int xBase, int yBase) : m_base(Point2int32(xBase,yBase)), m_xCurrent(Point2int32(1,xBase)), m_yCurrent(Point2int32(1,yBase)), m_currentIndex(1) {}

};

}

#endif
