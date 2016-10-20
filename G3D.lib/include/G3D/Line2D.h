/**
 @file Line2D.h
 
 2D-Line class
 
 @maintainer Michael Mara, mikemx7f@gmail.com
 
 @created 2012-08-27
 @edited  2012-08-27
 */

#ifndef G3D_LINE2D_H
#define G3D_LINE2D_H

#include "G3D/platform.h"
#include "G3D/Vector2.h"

namespace G3D {

/**
 An infinite 2D line.
 */
class Line2D {
protected:

    Point2 m_point;
    Vector2 m_direction;

    Line2D(const Point2& point, const Vector2& direction) {
        m_point     = point;
        m_direction = direction.direction();
    }

public:

    /** Undefined (provided for creating Array<Line2D> only) */
    inline Line2D() {}

    Line2D(class BinaryInput& b);

    void serialize(class BinaryOutput& b) const;

    void deserialize(class BinaryInput& b);

    virtual ~Line2D() {}

    /**
      Constructs a line from two (not equal) points.
     */
    static Line2D fromTwoPoints(const Point2 &point1, const Point2 &point2) {
        return Line2D(point1, point2 - point1);
    }

    /**
      Creates a line from a point and a (nonzero) direction.
     */
    static Line2D fromPointAndDirection(const Point2& point, const Vector2& direction) {
        return Line2D(point, direction);
    }

    /**
      Returns the closest point on the line to point.
     */
    Point2 closestPoint(const Point2& pt) const;

    /**
      Returns the distance between point and the line
     */
    double distance(const Point2& point) const {
        return (closestPoint(point) - point).length();
    }

    /** Returns a point on the line */
    Point2 point() const;

    /** Returns the direction (or negative direction) of the line */
    Vector2 direction() const;

    /**
     Returns the point where the lines intersect.  If there
     is no intersection, returns a point at infinity.
     */
    Point2 intersection(const Line2D &otherLine) const;

};

};// namespace


#endif
