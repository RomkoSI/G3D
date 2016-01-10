#ifndef G3D_CompassDirection_h
#define G3D_CompassDirection_h

#include "G3D/platform.h"
#include "G3D/g3dmath.h"

namespace G3D {

class CompassDirection;
class CompassDelta;
class Vector3;
class Matrix3;
class Any;

/** Oriented angle measure on a compass; the difference of two CompassDirection%s.
    When relative to a heading, this is a bearing.
   \sa CompassDirection, \sa CompassBearing */
class CompassDelta {
protected:
    friend class CompassDirection;

    float m_angleDegrees;

public:

    explicit CompassDelta(float degrees = 0) : m_angleDegrees(degrees) {}

    CompassDelta(const Any& a);

    Any toAny() const;

    CompassDelta operator+(CompassDelta c) const {
        return CompassDelta(m_angleDegrees + c.m_angleDegrees);
    }

    CompassDelta operator-(CompassDelta c) const {
        return CompassDelta(m_angleDegrees - c.m_angleDegrees);
    }

    CompassDelta& operator+=(CompassDelta c) {
        m_angleDegrees += c.m_angleDegrees;
        return *this;
    }

    CompassDelta& operator-=(CompassDelta c) {
        m_angleDegrees -= c.m_angleDegrees;
        return *this;
    }

    CompassDelta operator-() const {
        return CompassDelta(-m_angleDegrees);
    }

    CompassDelta operator*(float s) const {
        return CompassDelta(m_angleDegrees * s);
    }

    CompassDelta operator/(float s) const {
        return CompassDelta(m_angleDegrees / s);
    }

    CompassDelta& operator*=(float s) {
        m_angleDegrees *= s;
        return *this;
    }

    CompassDelta& operator/=(float s) {
        m_angleDegrees /= s;
        return *this;
    }

    bool operator>(CompassDelta c) const {
        return m_angleDegrees > c.m_angleDegrees;
    }

    bool operator>=(CompassDelta c) const {
        return m_angleDegrees >= c.m_angleDegrees;
    }

    bool operator<(CompassDelta c) const {
        return m_angleDegrees < c.m_angleDegrees;
    }

    bool operator<=(CompassDelta c) const {
        return m_angleDegrees <= c.m_angleDegrees;
    }

    bool operator==(CompassDelta c) const {
        return m_angleDegrees == c.m_angleDegrees;
    }

    bool operator!=(CompassDelta c) const {
        return m_angleDegrees != c.m_angleDegrees;
    }

    /** The angle measure of this delta, in degrees on the compass. */
    float compassDegrees() const {
        return m_angleDegrees;
    }

    CompassDelta abs() const {
        return CompassDelta(::fabsf(m_angleDegrees));
    }

    /** The angle measure of this delta, in radians in the ZX plane (i.e., standard G3D yaw). */
    float zxRadians() const {
        return -toRadians(m_angleDegrees);
    }
};

typedef CompassDelta CompassBearing;

/** Azimuth measured in degrees from 0 = North = -z, increasing <b>clockwise</b> in the ZX plane. 
    Because the standard compass conventions are very different from G3D (and most 3D) conventions,
    this class helps avoid errors when modeling simulations that naturally use compass
    directions (e.g., boats and planes).
  
    This class does not model differences between true North and magnetic North, or between
    heading and course.

    To avoid ambiguity, no comparison operators (beyond equality) are provided.  Use a CompassDelta
    if you wish to compare directions.

    The internal storage is in floating point degrees, so all small integers are exactly represented.

    \sa CompassDelta, CompassBearing
  */
class CompassDirection {
protected:

    float m_angleDegrees;

public:

    CompassDirection(const Any& a);

    Any toAny() const;

    /** Initialize from a compass reading */
    explicit CompassDirection(float degrees = 0) : m_angleDegrees(degrees) {}

    static CompassDirection north() {
        return CompassDirection(0);
    }

    static CompassDirection east() {
        return CompassDirection(90);
    }

    static CompassDirection south() {
        return CompassDirection(180);
    }

    static CompassDirection west() {
        return CompassDirection(270);
    }

    CompassDirection right90Degrees() const {
        return CompassDirection(m_angleDegrees + 90);
    }

    /** Synonym for right90Degrees() */
    CompassDirection clockwise90Degrees() const {
        return right90Degrees();
    }

    /** Synonym for right90Degrees() */
    CompassDirection starboard90Degrees() const {
        return right90Degrees();
    }

    CompassDirection left90Degrees() const {
        return CompassDirection(m_angleDegrees - 90);
    }

    /** Synonym for left90Degrees() */
    CompassDirection counterclockwise90Degrees() const {
        return left90Degrees();
    }

    /** Synonym for left90Degrees() */
    CompassDirection port90Degrees() const {
        return left90Degrees();
    }

    /** Points in the opposite direction */
    CompassDirection operator-() const {
        return CompassDirection(-m_angleDegrees);
    }

    /** Returns the angle measure of the arc from this to other, going the 
        shorter way around the circle (e.g., -45 instead of +135). */
    CompassDelta operator-(CompassDirection other) const {
        const float a = (float)wrap(m_angleDegrees, 360.0f);
        const float b = (float)wrap(other.m_angleDegrees, 360.0f);
        if (a - b > 180) {
            return CompassDelta(a - b - 360.0f);
        } else if (a - b < -180) { 
            return CompassDelta(a - b + 360.0f);
        } else {
            return CompassDelta(a - b);
        }
    }

    CompassDirection& operator-=(CompassDelta d) {
        m_angleDegrees -= d.m_angleDegrees;
        return *this;
    }

    CompassDirection& operator+=(CompassDelta d) {
        m_angleDegrees += d.m_angleDegrees;
        return *this;
    }

    /** True if these are the same direction modulo 360 degrees */
    bool operator==(CompassDirection c) const {
        return value() == c.value();
    }

    /** True if these are not the same direction modulo 360 degrees */
    bool operator!=(CompassDirection c) const {
        return value() != c.value();
    }

    /** Returns the angle in radians in the ZX plane, measured counter-clockwise
       from the Z axis.  i.e., the canonical yaw angle in G3D. The result is not 
       bounded to any particular range; use wrap() to accomplish that. */
    float zxRadians() const {
 
        //         ^ x
        //         |
        //         E
        //       N * S ---> z  
        //         W  
        //
        // Compass  ZX
        //    0 ->-180
        //   90 ->  90
        //  180 ->   0
        //  -90 -> -90

        return toRadians(180.0f - m_angleDegrees);
    }

    /** Always returns a number on the interval [0, 360) */
    float value() const {
        return (float)wrap(m_angleDegrees, 360.0f);
    }

    /** Return a vector in the XZ plane pointing along this compass direction. */
    operator Vector3() const;

    /** Rotation matrix to produce this compass direction as a heading 
        for an object that normally faces along its -z axis.  
        
        \sa CoordinateFrame::rotation
      */
    Matrix3 toHeadingMatrix3() const;

    /** Compounded cardinal and ordinal description, e.g., "Southwest by South" 
        \cite http://en.wikipedia.org/wiki/Boxing_the_compass */
    const String& nearestCompassPointName() const;

    const String& nearestCompassPointAbbreviation() const;
};

} // namespace G3D

#endif
