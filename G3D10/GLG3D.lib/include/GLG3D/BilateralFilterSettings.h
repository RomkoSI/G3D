#pragma once
#include "GLG3D/GBuffer.h"
namespace G3D {

class GuiPane;
class BilateralFilterSettings {
public:
    /** Filter radius in pixels. This will be multiplied by stepSize.
        Each 1D filter will have 2*radius + 1 taps. If set to 0, the filter is turned off.
        */
    int radius;

    /**
        Default is to step in 2-pixel intervals. This constant can be
        increased while radius decreases to improve
        performance at the expense of some dithering artifacts.

        Must be at least 1.
    */
    int stepSize;

    /** If true, ensure that the "bilateral" weights are monotonically decreasing
        moving away from the current pixel. Default is true. */
    bool monotonicallyDecreasingBilateralWeights;

    /**
        How much depth difference is taken into account.
        Default is 1.
    */
    float depthWeight;

    /**
        How much normal difference is taken into account.
        Default is 1.
    */
    float normalWeight;

    /**
        How much plane difference is taken into account.
        Default is 1.
    */
    float planeWeight;

    /**
    How much glossy exponent is taken into account.
    Default is 1.
    */
    float glossyWeight;


    BilateralFilterSettings();

    /**
    Ensures the GBuffer specification has all the fields needed to render this effect
    \sa GApp::extendGBufferSpecification
    */
    void extendGBufferSpecification(GBuffer::Specification& spec) const;

    void makeGUI(GuiPane* pane);

};
}