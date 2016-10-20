#include "G3D/HaltonSequence.h"
#ifndef INT_MAX
#include <climits>
#endif
namespace G3D {

    static float vanDerCorput(int index, int base) {
       float result = 0.0f;
       float f = 1.0f / (float)base;
       int i = index;
       while (i > 0) { 
           result = result + f * (i % base);
           i = i / base;
           f = f / (float)base;
       }
       return result;
    }

#if 0
    static void advanceSequence(int base, Point2int32& current, int currentStride) {
        ++current.x;
        if (current.x == current.y) {
            debugAssertM(current.x < INT_MAX / base, "Halton Sequence wrapping around");
            current.y *= base;
            current.x = 1;
        } else if (current.x % base == 0) {
            ++current.x;
        }
    }
#endif

    void HaltonSequence::next(Point2& p) {
        p.x = vanDerCorput(m_currentIndex, m_base.x);
        p.y = vanDerCorput(m_currentIndex, m_base.y);
        ++m_currentIndex;
#if 0
        p.x = ((float)m_xCurrent.x) / m_xCurrent.y;
        p.y = ((float)m_yCurrent.x) / m_yCurrent.y;
        advanceSequence(m_base.x, m_xCurrent, m_xCurrentStride);
        advanceSequence(m_base.y, m_yCurrent, m_yCurrentStride);
#endif
    }
}
