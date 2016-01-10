#ifndef supportclasses_h
#define supportclasses_h

#include "G3D/Color3.h"
#include <vector>

using G3D::Color3;

class Image {
private:
    int                 m_width;
    int                 m_height;
    std::vector<Color3> m_data;

    /** Radiance is scaled by this value, which should be chosen to
        scale the brightest values to about 1.0.*/
    float               m_exposureConstant;

    int PPMGammaCorrect(float radiance) const;

public:

    Image(int width, int height) :
        m_width(width), m_height(height), m_data(width * height),
        m_exposureConstant(1.0f) {}

    void setExposureConstant(float e) { m_exposureConstant = e; }

    float exposureConstant() const { return m_exposureConstant; }

    int width() const { return m_width; }

    int height() const { return m_height; }

    void set(int x, int y, const Color3& value) {
        m_data[x + y * m_width] = value;
    }

    const Color3& get(int x, int y) const {
        return m_data[x + y * m_width];
    }

    void save(const std::string& filename) const;

    /** Does not return */
    void display(float deviceGamma = 2.0f) const;
};

#endif
