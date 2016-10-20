/**
 @file Line2D.h
 
 2D-Line class
 
 @maintainer Michael Mara, mikemx7f@gmail.com
 
 @created 2012-08-27
 @edited  2012-08-27
 */

#include "G3D/Line2D.h"

namespace G3D {

Point2 Line2D::intersection(const Line2D& L) const {

    float denominator = (m_direction.x * L.m_direction.y) - 
                        (m_direction.y * L.m_direction.x);
    if(denominator == 0) return Point2((float)inf(), (float)inf());
    float leftTerm  = (m_point.x + m_direction.x) * m_point.y - (m_point.y + m_direction.y) * m_point.x;
    float rightTerm = (L.m_point.x + L.m_direction.x) * L.m_point.y - (L.m_point.y + L.m_direction.y) * L.m_point.x;
    Point2 numerator = leftTerm * L.m_direction - rightTerm * m_direction;
    return numerator / denominator;
}


Line2D::Line2D(class BinaryInput& b) {
    deserialize(b);
}


void Line2D::serialize(class BinaryOutput& b) const {
    m_point.serialize(b);
    m_direction.serialize(b);
}


void Line2D::deserialize(class BinaryInput& b) {
    m_point.deserialize(b);
    m_direction.deserialize(b);
}


Point2 Line2D::closestPoint(const Point2& pt) const {
    float t = m_direction.dot(pt - m_point);
    return m_point + m_direction * t;
}


Point2 Line2D::point() const {
    return m_point;
}


Vector2 Line2D::direction() const {
    return m_direction;
}

}

