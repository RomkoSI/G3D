/**
@file Intersect.cpp

@maintainer Morgan McGuire, http://graphics.cs.williams.edu

@created 2009-06-29
@edited  2009-06-29

Copyright 2000-2009, Morgan McGuire.
All rights reserved.

From the G3D Innovation Engine
http://g3d.cs.williams.edu
*/
#include "G3D/platform.h"
#include "G3D/Intersect.h"
#include "G3D/AABox.h"
#include "G3D/PrecomputedRay.h"

namespace G3D {

#ifdef _MSC_VER
    // Turn on fast floating-point optimizations
#pragma float_control( push )
#pragma fp_contract( on )
#pragma fenv_access( off )
#pragma float_control( except, off )
#pragma float_control( precise, off )
#endif

bool __fastcall Intersect::rayAABox(const PrecomputedRay& ray, const AABox& box) {
    switch (ray.classification) {
    case PrecomputedRay::MMM:

        if ((ray.m_origin.x < box.lo.x) || (ray.m_origin.y < box.lo.y) || (ray.m_origin.z < box.lo.z)
            || (ray.jbyi * box.lo.x - box.hi.y + ray.c_xy > 0)
            || (ray.ibyj * box.lo.y - box.hi.x + ray.c_yx > 0)
            || (ray.jbyk * box.lo.z - box.hi.y + ray.c_zy > 0)
            || (ray.kbyj * box.lo.y - box.hi.z + ray.c_yz > 0)
            || (ray.kbyi * box.lo.x - box.hi.z + ray.c_xz > 0)
            || (ray.ibyk * box.lo.z - box.hi.x + ray.c_zx > 0)
            )
            return false;
        else
        return true;

    case PrecomputedRay::MMP:

        if ((ray.m_origin.x < box.lo.x) || (ray.m_origin.y < box.lo.y) || (ray.m_origin.z > box.hi.z)
            || (ray.jbyi * box.lo.x - box.hi.y + ray.c_xy > 0)
            || (ray.ibyj * box.lo.y - box.hi.x + ray.c_yx > 0)
            || (ray.jbyk * box.hi.z - box.hi.y + ray.c_zy > 0)
            || (ray.kbyj * box.lo.y - box.lo.z + ray.c_yz < 0)
            || (ray.kbyi * box.lo.x - box.lo.z + ray.c_xz < 0)
            || (ray.ibyk * box.hi.z - box.hi.x + ray.c_zx > 0)
            )
            return false;
        else
            return true;

    case PrecomputedRay::MPM:

        if ((ray.m_origin.x < box.lo.x) || (ray.m_origin.y > box.hi.y) || (ray.m_origin.z < box.lo.z)
            || (ray.jbyi * box.lo.x - box.lo.y + ray.c_xy < 0) 
            || (ray.ibyj * box.hi.y - box.hi.x + ray.c_yx > 0)
            || (ray.jbyk * box.lo.z - box.lo.y + ray.c_zy < 0) 
            || (ray.kbyj * box.hi.y - box.hi.z + ray.c_yz > 0)
            || (ray.kbyi * box.lo.x - box.hi.z + ray.c_xz > 0)
            || (ray.ibyk * box.lo.z - box.hi.x + ray.c_zx > 0)
            )
            return false;
        else
        return true;

    case PrecomputedRay::MPP:

        if ((ray.m_origin.x < box.lo.x) || (ray.m_origin.y > box.hi.y) || (ray.m_origin.z > box.hi.z)
            || (ray.jbyi * box.lo.x - box.lo.y + ray.c_xy < 0) 
            || (ray.ibyj * box.hi.y - box.hi.x + ray.c_yx > 0)
            || (ray.jbyk * box.hi.z - box.lo.y + ray.c_zy < 0)
            || (ray.kbyj * box.hi.y - box.lo.z + ray.c_yz < 0)
            || (ray.kbyi * box.lo.x - box.lo.z + ray.c_xz < 0)
            || (ray.ibyk * box.hi.z - box.hi.x + ray.c_zx > 0)
            )
            return false;
        else
        return true;

    case PrecomputedRay::PMM:

        if ((ray.m_origin.x > box.hi.x) || (ray.m_origin.y < box.lo.y) || (ray.m_origin.z < box.lo.z)
            || (ray.jbyi * box.hi.x - box.hi.y + ray.c_xy > 0)
            || (ray.ibyj * box.lo.y - box.lo.x + ray.c_yx < 0)
            || (ray.jbyk * box.lo.z - box.hi.y + ray.c_zy > 0)
            || (ray.kbyj * box.lo.y - box.hi.z + ray.c_yz > 0)
            || (ray.kbyi * box.hi.x - box.hi.z + ray.c_xz > 0)
            || (ray.ibyk * box.lo.z - box.lo.x + ray.c_zx < 0)
            )
            return false;
        else
        return true;

    case PrecomputedRay::PMP:

        if ((ray.m_origin.x > box.hi.x) || (ray.m_origin.y < box.lo.y) || (ray.m_origin.z > box.hi.z)
            || (ray.jbyi * box.hi.x - box.hi.y + ray.c_xy > 0)
            || (ray.ibyj * box.lo.y - box.lo.x + ray.c_yx < 0)
            || (ray.jbyk * box.hi.z - box.hi.y + ray.c_zy > 0)
            || (ray.kbyj * box.lo.y - box.lo.z + ray.c_yz < 0)
            || (ray.kbyi * box.hi.x - box.lo.z + ray.c_xz < 0)
            || (ray.ibyk * box.hi.z - box.lo.x + ray.c_zx < 0)
            )
            return false;
        else
        return true;

    case PrecomputedRay::PPM:

        if ((ray.m_origin.x > box.hi.x) || (ray.m_origin.y > box.hi.y) || (ray.m_origin.z < box.lo.z)
            || (ray.jbyi * box.hi.x - box.lo.y + ray.c_xy < 0)
            || (ray.ibyj * box.hi.y - box.lo.x + ray.c_yx < 0)
            || (ray.jbyk * box.lo.z - box.lo.y + ray.c_zy < 0) 
            || (ray.kbyj * box.hi.y - box.hi.z + ray.c_yz > 0)
            || (ray.kbyi * box.hi.x - box.hi.z + ray.c_xz > 0)
            || (ray.ibyk * box.lo.z - box.lo.x + ray.c_zx < 0)
            )
            return false;
        else
            return true;

    case PrecomputedRay::PPP:

        if ((ray.m_origin.x > box.hi.x) || (ray.m_origin.y > box.hi.y) || (ray.m_origin.z > box.hi.z)
            || (ray.jbyi * box.hi.x - box.lo.y + ray.c_xy < 0)
            || (ray.ibyj * box.hi.y - box.lo.x + ray.c_yx < 0)
            || (ray.jbyk * box.hi.z - box.lo.y + ray.c_zy < 0)
            || (ray.kbyj * box.hi.y - box.lo.z + ray.c_yz < 0)
            || (ray.kbyi * box.hi.x - box.lo.z + ray.c_xz < 0)
            || (ray.ibyk * box.hi.z - box.lo.x + ray.c_zx < 0)) {
                return false;
        }   else
          return true;

    case PrecomputedRay::OMM:

        if((ray.m_origin.x < box.lo.x) || (ray.m_origin.x > box.hi.x)
            || (ray.m_origin.y < box.lo.y) || (ray.m_origin.z < box.lo.z)
            || (ray.jbyk * box.lo.z - box.hi.y + ray.c_zy > 0)
            || (ray.kbyj * box.lo.y - box.hi.z + ray.c_yz > 0)
            )
            return false;
        else
            return true;

    case PrecomputedRay::OMP:

        if((ray.m_origin.x < box.lo.x) || (ray.m_origin.x > box.hi.x)
            || (ray.m_origin.y < box.lo.y) || (ray.m_origin.z > box.hi.z)
            || (ray.jbyk * box.hi.z - box.hi.y + ray.c_zy > 0)
            || (ray.kbyj * box.lo.y - box.lo.z + ray.c_yz < 0)
            )
            return false;
        else
            return true;

    case PrecomputedRay::OPM:

        if((ray.m_origin.x < box.lo.x) || (ray.m_origin.x > box.hi.x)
            || (ray.m_origin.y > box.hi.y) || (ray.m_origin.z < box.lo.z)
            || (ray.jbyk * box.lo.z - box.lo.y + ray.c_zy < 0) 
            || (ray.kbyj * box.hi.y - box.hi.z + ray.c_yz > 0)
            )
            return false;
        else
            return true;

    case PrecomputedRay::OPP:

        if((ray.m_origin.x < box.lo.x) || (ray.m_origin.x > box.hi.x)
            || (ray.m_origin.y > box.hi.y) || (ray.m_origin.z > box.hi.z)
            || (ray.jbyk * box.hi.z - box.lo.y + ray.c_zy < 0)
            || (ray.kbyj * box.hi.y - box.lo.z + ray.c_yz < 0)
            )
            return false;
        else
            return true;

    case PrecomputedRay::MOM:

        if((ray.m_origin.y < box.lo.y) || (ray.m_origin.y > box.hi.y)
            || (ray.m_origin.x < box.lo.x) || (ray.m_origin.z < box.lo.z) 
            || (ray.kbyi * box.lo.x - box.hi.z + ray.c_xz > 0)
            || (ray.ibyk * box.lo.z - box.hi.x + ray.c_zx > 0)
            )
            return false;
        else
            return true;

    case PrecomputedRay::MOP:

        if((ray.m_origin.y < box.lo.y) || (ray.m_origin.y > box.hi.y)
            || (ray.m_origin.x < box.lo.x) || (ray.m_origin.z > box.hi.z) 
            || (ray.kbyi * box.lo.x - box.lo.z + ray.c_xz < 0)
            || (ray.ibyk * box.hi.z - box.hi.x + ray.c_zx > 0)
            )
            return false;
        else
            return true;

    case PrecomputedRay::POM:

        if((ray.m_origin.y < box.lo.y) || (ray.m_origin.y > box.hi.y)
            || (ray.m_origin.x > box.hi.x) || (ray.m_origin.z < box.lo.z)
            || (ray.kbyi * box.hi.x - box.hi.z + ray.c_xz > 0)
            || (ray.ibyk * box.lo.z - box.lo.x + ray.c_zx < 0)
            )
            return false;
        else
            return true;

    case PrecomputedRay::POP:

        if((ray.m_origin.y < box.lo.y) || (ray.m_origin.y > box.hi.y)
            || (ray.m_origin.x > box.hi.x) || (ray.m_origin.z > box.hi.z)
            || (ray.kbyi * box.hi.x - box.lo.z + ray.c_xz < 0)
            || (ray.ibyk * box.hi.z - box.lo.x + ray.c_zx < 0)
            )
            return false;
        else
            return true;

    case PrecomputedRay::MMO:

        if((ray.m_origin.z < box.lo.z) || (ray.m_origin.z > box.hi.z)
            || (ray.m_origin.x < box.lo.x) || (ray.m_origin.y < box.lo.y) 
            || (ray.jbyi * box.lo.x - box.hi.y + ray.c_xy > 0)
            || (ray.ibyj * box.lo.y - box.hi.x + ray.c_yx > 0)
            )
            return false;
        else
            return true;

    case PrecomputedRay::MPO:

        if((ray.m_origin.z < box.lo.z) || (ray.m_origin.z > box.hi.z)
            || (ray.m_origin.x < box.lo.x) || (ray.m_origin.y > box.hi.y) 
            || (ray.jbyi * box.lo.x - box.lo.y + ray.c_xy < 0) 
            || (ray.ibyj * box.hi.y - box.hi.x + ray.c_yx > 0)
            )
            return false;
        else
            return true;

    case PrecomputedRay::PMO:

        if((ray.m_origin.z < box.lo.z) || (ray.m_origin.z > box.hi.z)
            || (ray.m_origin.x > box.hi.x) || (ray.m_origin.y < box.lo.y) 
            || (ray.jbyi * box.hi.x - box.hi.y + ray.c_xy > 0)
            || (ray.ibyj * box.lo.y - box.lo.x + ray.c_yx < 0)  
            )
            return false;
        else
            return true;

    case PrecomputedRay::PPO:

        if((ray.m_origin.z < box.lo.z) || (ray.m_origin.z > box.hi.z)
            || (ray.m_origin.x > box.hi.x) || (ray.m_origin.y > box.hi.y)
            || (ray.jbyi * box.hi.x - box.lo.y + ray.c_xy < 0)
            || (ray.ibyj * box.hi.y - box.lo.x + ray.c_yx < 0)
            )
            return false;
        else
            return true;

    case PrecomputedRay::MOO:

        if((ray.m_origin.x < box.lo.x)
            || (ray.m_origin.y < box.lo.y) || (ray.m_origin.y > box.hi.y)
            || (ray.m_origin.z < box.lo.z) || (ray.m_origin.z > box.hi.z)
            )
            return false;
        else
            return true;

    case PrecomputedRay::POO:

        if((ray.m_origin.x > box.hi.x)
            || (ray.m_origin.y < box.lo.y) || (ray.m_origin.y > box.hi.y)
            || (ray.m_origin.z < box.lo.z) || (ray.m_origin.z > box.hi.z)
            )
            return false;
        else
            return true;

    case PrecomputedRay::OMO:

        if((ray.m_origin.y < box.lo.y)
            || (ray.m_origin.x < box.lo.x) || (ray.m_origin.x > box.hi.x)
            || (ray.m_origin.z < box.lo.z) || (ray.m_origin.z > box.hi.z)
            )
            return false;
        else
            return true;

    case PrecomputedRay::OPO:

        if((ray.m_origin.y > box.hi.y)
            || (ray.m_origin.x < box.lo.x) || (ray.m_origin.x > box.hi.x)
            || (ray.m_origin.z < box.lo.z) || (ray.m_origin.z > box.hi.z)
            )
            return false;
        else
            return true;

    case PrecomputedRay::OOM:

        if((ray.m_origin.z < box.lo.z)
            || (ray.m_origin.x < box.lo.x) || (ray.m_origin.x > box.hi.x)
            || (ray.m_origin.y < box.lo.y) || (ray.m_origin.y > box.hi.y)
            )
            return false;
        else
            return true;

    case PrecomputedRay::OOP:

        if((ray.m_origin.z > box.hi.z)
            || (ray.m_origin.x < box.lo.x) || (ray.m_origin.x > box.hi.x)
            || (ray.m_origin.y < box.lo.y) || (ray.m_origin.y > box.hi.y)
            )
            return false;
        else
            return true;

    }

    return false;
}


bool __fastcall Intersect::rayAABox(const PrecomputedRay& ray, const AABox& box, float& time) {

    switch (ray.classification) {
    case PrecomputedRay::MMM:
        {
            if ((ray.m_origin.x < box.lo.x) || (ray.m_origin.y < box.lo.y) || (ray.m_origin.z < box.lo.z)
                || (ray.jbyi * box.lo.x - box.hi.y + ray.c_xy > 0)
                || (ray.ibyj * box.lo.y - box.hi.x + ray.c_yx > 0)
                || (ray.jbyk * box.lo.z - box.hi.y + ray.c_zy > 0)
                || (ray.kbyj * box.lo.y - box.hi.z + ray.c_yz > 0)
                || (ray.kbyi * box.lo.x - box.hi.z + ray.c_xz > 0)
                || (ray.ibyk * box.lo.z - box.hi.x + ray.c_zx > 0)) {
                    return false;
            }

            // compute the intersection distance

            time = (box.hi.x - ray.m_origin.x) * ray.m_invDirection.x;
            float t1 = (box.hi.y - ray.m_origin.y) * ray.m_invDirection.y;
            if (t1 > time) {
                time = t1;
            }

            float t2 = (box.hi.z - ray.m_origin.z) * ray.m_invDirection.z;
            if (t2 > time) {
                time = t2;
            }

            return true;
        }

    case PrecomputedRay::MMP:
        {        
            if ((ray.m_origin.x < box.lo.x) || (ray.m_origin.y < box.lo.y) || (ray.m_origin.z > box.hi.z)
                || (ray.jbyi * box.lo.x - box.hi.y + ray.c_xy > 0)
                || (ray.ibyj * box.lo.y - box.hi.x + ray.c_yx > 0)
                || (ray.jbyk * box.hi.z - box.hi.y + ray.c_zy > 0)
                || (ray.kbyj * box.lo.y - box.lo.z + ray.c_yz < 0)
                || (ray.kbyi * box.lo.x - box.lo.z + ray.c_xz < 0)
                || (ray.ibyk * box.hi.z - box.hi.x + ray.c_zx > 0)) {
                    return false;
            }

            time = (box.hi.x - ray.m_origin.x) * ray.m_invDirection.x;
            float t1 = (box.hi.y - ray.m_origin.y) * ray.m_invDirection.y;
            if (t1 > time) {
                time = t1;
            }
            float t2 = (box.lo.z - ray.m_origin.z) * ray.m_invDirection.z;
            if (t2 > time) {
                time = t2;
            }

            return true;
        }

    case PrecomputedRay::MPM:
        {        
            if ((ray.m_origin.x < box.lo.x) || (ray.m_origin.y > box.hi.y) || (ray.m_origin.z < box.lo.z)
                || (ray.jbyi * box.lo.x - box.lo.y + ray.c_xy < 0) 
                || (ray.ibyj * box.hi.y - box.hi.x + ray.c_yx > 0)
                || (ray.jbyk * box.lo.z - box.lo.y + ray.c_zy < 0) 
                || (ray.kbyj * box.hi.y - box.hi.z + ray.c_yz > 0)
                || (ray.kbyi * box.lo.x - box.hi.z + ray.c_xz > 0)
                || (ray.ibyk * box.lo.z - box.hi.x + ray.c_zx > 0)) {
                    return false;
            }

            time = (box.hi.x - ray.m_origin.x) * ray.m_invDirection.x;
            float t1 = (box.lo.y - ray.m_origin.y) * ray.m_invDirection.y;
            if (t1 > time) {
                time = t1;
            }
            float t2 = (box.hi.z - ray.m_origin.z) * ray.m_invDirection.z;
            if (t2 > time) {
                time = t2;
            }

            return true;
        }

    case PrecomputedRay::MPP:
        {
            if ((ray.m_origin.x < box.lo.x) || (ray.m_origin.y > box.hi.y) || (ray.m_origin.z > box.hi.z)
                || (ray.jbyi * box.lo.x - box.lo.y + ray.c_xy < 0) 
                || (ray.ibyj * box.hi.y - box.hi.x + ray.c_yx > 0)
                || (ray.jbyk * box.hi.z - box.lo.y + ray.c_zy < 0)
                || (ray.kbyj * box.hi.y - box.lo.z + ray.c_yz < 0) 
                || (ray.kbyi * box.lo.x - box.lo.z + ray.c_xz < 0)
                || (ray.ibyk * box.hi.z - box.hi.x + ray.c_zx > 0)) {
                    return false;
            }

            time = (box.hi.x - ray.m_origin.x) * ray.m_invDirection.x;
            float t1 = (box.lo.y - ray.m_origin.y) * ray.m_invDirection.y;
            if (t1 > time) {
                time = t1;
            }
            float t2 = (box.lo.z - ray.m_origin.z) * ray.m_invDirection.z;
            if (t2 > time) {
                time = t2;
            }

            return true;
        }

    case PrecomputedRay::PMM:
        {
            if ((ray.m_origin.x > box.hi.x) || (ray.m_origin.y < box.lo.y) || (ray.m_origin.z < box.lo.z)
                || (ray.jbyi * box.hi.x - box.hi.y + ray.c_xy > 0)
                || (ray.ibyj * box.lo.y - box.lo.x + ray.c_yx < 0)
                || (ray.jbyk * box.lo.z - box.hi.y + ray.c_zy > 0)
                || (ray.kbyj * box.lo.y - box.hi.z + ray.c_yz > 0)
                || (ray.kbyi * box.hi.x - box.hi.z + ray.c_xz > 0)
                || (ray.ibyk * box.lo.z - box.lo.x + ray.c_zx < 0)) {
                    return false;
            }

            time = (box.lo.x - ray.m_origin.x) * ray.m_invDirection.x;
            float t1 = (box.hi.y - ray.m_origin.y) * ray.m_invDirection.y;
            if (t1 > time) {
                time = t1;
            }
            float t2 = (box.hi.z - ray.m_origin.z) * ray.m_invDirection.z;
            if (t2 > time) {
                time = t2;
            }

            return true;
        }


    case PrecomputedRay::PMP:
        {
            if ((ray.m_origin.x > box.hi.x) || (ray.m_origin.y < box.lo.y) || (ray.m_origin.z > box.hi.z)
                || (ray.jbyi * box.hi.x - box.hi.y + ray.c_xy > 0)
                || (ray.ibyj * box.lo.y - box.lo.x + ray.c_yx < 0)
                || (ray.jbyk * box.hi.z - box.hi.y + ray.c_zy > 0)
                || (ray.kbyj * box.lo.y - box.lo.z + ray.c_yz < 0)
                || (ray.kbyi * box.hi.x - box.lo.z + ray.c_xz < 0)
                || (ray.ibyk * box.hi.z - box.lo.x + ray.c_zx < 0)) {
                    return false;
            }

            time = (box.lo.x - ray.m_origin.x) * ray.m_invDirection.x;
            float t1 = (box.hi.y - ray.m_origin.y) * ray.m_invDirection.y;
            if (t1 > time) {
                time = t1;
            }
            float t2 = (box.lo.z - ray.m_origin.z) * ray.m_invDirection.z;
            if (t2 > time) {
                time = t2;
            }

            return true;
        }

    case PrecomputedRay::PPM:
        {
            if ((ray.m_origin.x > box.hi.x) || (ray.m_origin.y > box.hi.y) || (ray.m_origin.z < box.lo.z)
                || (ray.jbyi * box.hi.x - box.lo.y + ray.c_xy < 0)
                || (ray.ibyj * box.hi.y - box.lo.x + ray.c_yx < 0)
                || (ray.jbyk * box.lo.z - box.lo.y + ray.c_zy < 0) 
                || (ray.kbyj * box.hi.y - box.hi.z + ray.c_yz > 0)
                || (ray.kbyi * box.hi.x - box.hi.z + ray.c_xz > 0)
                || (ray.ibyk * box.lo.z - box.lo.x + ray.c_zx < 0))    {
                    return false;
            }

            time = (box.lo.x - ray.m_origin.x) * ray.m_invDirection.x;
            float t1 = (box.lo.y - ray.m_origin.y) * ray.m_invDirection.y;
            if (t1 > time) {
                time = t1;
            }
            float t2 = (box.hi.z - ray.m_origin.z) * ray.m_invDirection.z;
            if (t2 > time) {
                time = t2;
            }

            return true;
        }

    case PrecomputedRay::PPP:
        {
            if ((ray.m_origin.x > box.hi.x) || (ray.m_origin.y > box.hi.y) || (ray.m_origin.z > box.hi.z)
                || (ray.jbyi * box.hi.x - box.lo.y + ray.c_xy < 0)
                || (ray.ibyj * box.hi.y - box.lo.x + ray.c_yx < 0)
                || (ray.jbyk * box.hi.z - box.lo.y + ray.c_zy < 0)
                || (ray.kbyj * box.hi.y - box.lo.z + ray.c_yz < 0)
                || (ray.kbyi * box.hi.x - box.lo.z + ray.c_xz < 0)
                || (ray.ibyk * box.hi.z - box.lo.x + ray.c_zx < 0)) {
                    return false;
            }

            time = (box.lo.x - ray.m_origin.x) * ray.m_invDirection.x;
            float t1 = (box.lo.y - ray.m_origin.y) * ray.m_invDirection.y;
            if (t1 > time) {
                time = t1;
            }
            float t2 = (box.lo.z - ray.m_origin.z) * ray.m_invDirection.z;
            if (t2 > time) {
                time = t2;
            }

            return true;
        }

    case PrecomputedRay::OMM:
        {
            if((ray.m_origin.x < box.lo.x) || (ray.m_origin.x > box.hi.x)
                || (ray.m_origin.y < box.lo.y) || (ray.m_origin.z < box.lo.z)
                || (ray.jbyk * box.lo.z - box.hi.y + ray.c_zy > 0)
                || (ray.kbyj * box.lo.y - box.hi.z + ray.c_yz > 0)) {
                    return false;
            }

            time = (box.hi.y - ray.m_origin.y) * ray.m_invDirection.y;
            float t2 = (box.hi.z - ray.m_origin.z) * ray.m_invDirection.z;
            if (t2 > time) {
                time = t2;
            }

            return true;
        }

    case PrecomputedRay::OMP:
        {
            if((ray.m_origin.x < box.lo.x) || (ray.m_origin.x > box.hi.x)
                || (ray.m_origin.y < box.lo.y) || (ray.m_origin.z > box.hi.z)
                || (ray.jbyk * box.hi.z - box.hi.y + ray.c_zy > 0)
                || (ray.kbyj * box.lo.y - box.lo.z + ray.c_yz < 0)) {
                    return false;
            }

            time = (box.hi.y - ray.m_origin.y) * ray.m_invDirection.y;
            float t2 = (box.lo.z - ray.m_origin.z) * ray.m_invDirection.z;
            if (t2 > time) {
                time = t2;
            }

            return true;
        }

    case PrecomputedRay::OPM:
        {
            if((ray.m_origin.x < box.lo.x) || (ray.m_origin.x > box.hi.x)
                || (ray.m_origin.y > box.hi.y) || (ray.m_origin.z < box.lo.z)
                || (ray.jbyk * box.lo.z - box.lo.y + ray.c_zy < 0) 
                || (ray.kbyj * box.hi.y - box.hi.z + ray.c_yz > 0)) {
                    return false;
            }

            time = (box.lo.y - ray.m_origin.y) * ray.m_invDirection.y;        
            float t2 = (box.hi.z - ray.m_origin.z) * ray.m_invDirection.z;
            if (t2 > time) {
                time = t2;
            }

            return true;
        }

    case PrecomputedRay::OPP:
        {
            if((ray.m_origin.x < box.lo.x) || (ray.m_origin.x > box.hi.x)
                || (ray.m_origin.y > box.hi.y) || (ray.m_origin.z > box.hi.z)
                || (ray.jbyk * box.hi.z - box.lo.y + ray.c_zy < 0)
                || (ray.kbyj * box.hi.y - box.lo.z + ray.c_yz < 0)) {
                    return false;
            }

            time = (box.lo.y - ray.m_origin.y) * ray.m_invDirection.y;        
            float t2 = (box.lo.z - ray.m_origin.z) * ray.m_invDirection.z;
            if (t2 > time) {
                time = t2;
            }

            return true;
        }


    case PrecomputedRay::MOM:
        {
            if((ray.m_origin.y < box.lo.y) || (ray.m_origin.y > box.hi.y)
                || (ray.m_origin.x < box.lo.x) || (ray.m_origin.z < box.lo.z) 
                || (ray.kbyi * box.lo.x - box.hi.z + ray.c_xz > 0)
                || (ray.ibyk * box.lo.z - box.hi.x + ray.c_zx > 0)) {
                    return false;
            }

            time = (box.hi.x - ray.m_origin.x) * ray.m_invDirection.x;
            float t2 = (box.hi.z - ray.m_origin.z) * ray.m_invDirection.z;
            if (t2 > time) {
                time = t2;
            }

            return true;
        }


    case PrecomputedRay::MOP:
        {
            if((ray.m_origin.y < box.lo.y) || (ray.m_origin.y > box.hi.y)
                || (ray.m_origin.x < box.lo.x) || (ray.m_origin.z > box.hi.z) 
                || (ray.kbyi * box.lo.x - box.lo.z + ray.c_xz < 0)
                || (ray.ibyk * box.hi.z - box.hi.x + ray.c_zx > 0)) {
                    return false;
            }

            time = (box.hi.x - ray.m_origin.x) * ray.m_invDirection.x;
            float t2 = (box.lo.z - ray.m_origin.z) * ray.m_invDirection.z;
            if (t2 > time) {
                time = t2;
            }

            return true;
        }

    case PrecomputedRay::POM:
        {
            if((ray.m_origin.y < box.lo.y) || (ray.m_origin.y > box.hi.y)
                || (ray.m_origin.x > box.hi.x) || (ray.m_origin.z < box.lo.z)
                || (ray.kbyi * box.hi.x - box.hi.z + ray.c_xz > 0)
                || (ray.ibyk * box.lo.z - box.lo.x + ray.c_zx < 0)) {
                    return false;
            }

            time = (box.lo.x - ray.m_origin.x) * ray.m_invDirection.x;
            float t2 = (box.hi.z - ray.m_origin.z) * ray.m_invDirection.z;
            if (t2 > time) {
                time = t2;
            }

            return true;
        }


    case PrecomputedRay::POP:
        {
            if((ray.m_origin.y < box.lo.y) || (ray.m_origin.y > box.hi.y)
                || (ray.m_origin.x > box.hi.x) || (ray.m_origin.z > box.hi.z)
                || (ray.kbyi * box.hi.x - box.lo.z + ray.c_xz < 0)
                || (ray.ibyk * box.hi.z - box.lo.x + ray.c_zx < 0)) {
                    return false;
            }

            time = (box.lo.x - ray.m_origin.x) * ray.m_invDirection.x;
            float t2 = (box.lo.z - ray.m_origin.z) * ray.m_invDirection.z;
            if (t2 > time) {
                time = t2;
            }

            return true;
        }    

    case PrecomputedRay::MMO:
        {
            if((ray.m_origin.z < box.lo.z) || (ray.m_origin.z > box.hi.z)
                || (ray.m_origin.x < box.lo.x) || (ray.m_origin.y < box.lo.y)  
                || (ray.jbyi * box.lo.x - box.hi.y + ray.c_xy > 0)
                || (ray.ibyj * box.lo.y - box.hi.x + ray.c_yx > 0)) {
                    return false;
            }

            time = (box.hi.x - ray.m_origin.x) * ray.m_invDirection.x;
            float t1 = (box.hi.y - ray.m_origin.y) * ray.m_invDirection.y;
            if (t1 > time) {
                time = t1;
            }

            return true;
        }    

    case PrecomputedRay::MPO:
        {
            if((ray.m_origin.z < box.lo.z) || (ray.m_origin.z > box.hi.z)
                || (ray.m_origin.x < box.lo.x) || (ray.m_origin.y > box.hi.y) 
                || (ray.jbyi * box.lo.x - box.lo.y + ray.c_xy < 0) 
                || (ray.ibyj * box.hi.y - box.hi.x + ray.c_yx > 0))    {
                    return false;
            }

            time = (box.hi.x - ray.m_origin.x) * ray.m_invDirection.x;
            float t1 = (box.lo.y - ray.m_origin.y) * ray.m_invDirection.y;
            if (t1 > time) {
                time = t1;
            }

            return true;
        }


    case PrecomputedRay::PMO:
        {
            if((ray.m_origin.z < box.lo.z) || (ray.m_origin.z > box.hi.z)
                || (ray.m_origin.x > box.hi.x) || (ray.m_origin.y < box.lo.y) 
                || (ray.jbyi * box.hi.x - box.hi.y + ray.c_xy > 0)
                || (ray.ibyj * box.lo.y - box.lo.x + ray.c_yx < 0)) {
                    return false;
            }

            time = (box.lo.x - ray.m_origin.x) * ray.m_invDirection.x;
            float t1 = (box.hi.y - ray.m_origin.y) * ray.m_invDirection.y;
            if (t1 > time) {
                time = t1;
            }

            return true;
        }

    case PrecomputedRay::PPO:
        {
            if((ray.m_origin.z < box.lo.z) || (ray.m_origin.z > box.hi.z)
                || (ray.m_origin.x > box.hi.x) || (ray.m_origin.y > box.hi.y) 
                || (ray.jbyi * box.hi.x - box.lo.y + ray.c_xy < 0)
                || (ray.ibyj * box.hi.y - box.lo.x + ray.c_yx < 0)) {
                    return false;
            }

            time = (box.lo.x - ray.m_origin.x) * ray.m_invDirection.x;
            float t1 = (box.lo.y - ray.m_origin.y) * ray.m_invDirection.y;
            if (t1 > time) {
                time = t1;
            }

            return true;
        }


    case PrecomputedRay::MOO:
        {
            if((ray.m_origin.x < box.lo.x)
                || (ray.m_origin.y < box.lo.y) || (ray.m_origin.y > box.hi.y)
                || (ray.m_origin.z < box.lo.z) || (ray.m_origin.z > box.hi.z)) {
                    return false;
            }

            time = (box.hi.x - ray.m_origin.x) * ray.m_invDirection.x;
            return true;
        }

    case PrecomputedRay::POO:
        {
            if ((ray.m_origin.x > box.hi.x)
                || (ray.m_origin.y < box.lo.y) || (ray.m_origin.y > box.hi.y)
                || (ray.m_origin.z < box.lo.z) || (ray.m_origin.z > box.hi.z)) {
                    return false;
            }

            time = (box.lo.x - ray.m_origin.x) * ray.m_invDirection.x;
            return true;
        }

    case PrecomputedRay::OMO:
        {
            if ((ray.m_origin.y < box.lo.y)
                || (ray.m_origin.x < box.lo.x) || (ray.m_origin.x > box.hi.x)
                || (ray.m_origin.z < box.lo.z) || (ray.m_origin.z > box.hi.z)) {
                    return false;
            }

            time = (box.hi.y - ray.m_origin.y) * ray.m_invDirection.y;
            return true;
        }

    case PrecomputedRay::OPO:
        {
            if ((ray.m_origin.y > box.hi.y)
                || (ray.m_origin.x < box.lo.x) || (ray.m_origin.x > box.hi.x)
                || (ray.m_origin.z < box.lo.z) || (ray.m_origin.z > box.hi.z)) {
                    return false;
            }

            time = (box.lo.y - ray.m_origin.y) * ray.m_invDirection.y;
            return true;
        }


    case PrecomputedRay::OOM:
        {
            if ((ray.m_origin.z < box.lo.z)
                || (ray.m_origin.x < box.lo.x) || (ray.m_origin.x > box.hi.x)
                || (ray.m_origin.y < box.lo.y) || (ray.m_origin.y > box.hi.y)) {
                    return false;
            }

            time = (box.hi.z - ray.m_origin.z) * ray.m_invDirection.z;
            return true;
        }

    case PrecomputedRay::OOP:
        {
            if ((ray.m_origin.z > box.hi.z)
                || (ray.m_origin.x < box.lo.x) || (ray.m_origin.x > box.hi.x)
                || (ray.m_origin.y < box.lo.y) || (ray.m_origin.y > box.hi.y)) {
                    return false;
            }

            time = (box.lo.z - ray.m_origin.z) * ray.m_invDirection.z;
            return true;
        }    
    }

    return false;
}

#ifdef _MSC_VER
    // Turn off fast floating-point optimizations
#pragma float_control( pop )
#endif

}
