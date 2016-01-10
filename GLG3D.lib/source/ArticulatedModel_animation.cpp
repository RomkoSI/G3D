/**
 \file GLG3D/source/ArticulatedModel_animation.cpp

 \author Michael Mara, http://www.illuminationcodified.com
 \created 2013-01-18
 \edited  2014-03-03 
*/

#include "GLG3D/ArticulatedModel.h"

namespace G3D {

void ArticulatedModel::Animation::getCurrentPose(SimTime time, Pose& pose) {   
    poseSpline.get(float(time), pose);
}

}
