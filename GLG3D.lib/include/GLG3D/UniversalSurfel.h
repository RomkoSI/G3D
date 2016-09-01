/** \file G3D/Surfel.h
    \maintainer Morgan McGuire, http://graphics.cs.williams.edu

    \created 2011-07-01
    \edited  2016-07-16
    
 G3D Innovation Engine
 Copyright 2000-2016, Morgan McGuire.
 All rights reserved.
 */
#pragma once

#include "G3D/platform.h"
#include "GLG3D/Surfel.h"

namespace G3D {

class Tri;
class CPUVertexArray;

/** 
 \brief A Surfel for a surface patch described by a UniversalMaterial.

 Computes the Surfel::ExpressiveParameters::boost solely from the lambertianReflectivity coefficient.

 \sa UniversalMaterial
*/
class UniversalSurfel : public Surfel {
public:

    /** \f$ \rho_\mathrm{L} \f$
    */
    Color3          lambertianReflectivity;

    Color3          glossyReflectionCoefficient;
    
    /** \deprecated Use smoothness */
    float           glossyReflectionExponent;
    
    Color3          transmissionCoefficient;

    /* 
        Post-normal-mapped normal in the coordinate frame determined
        by the pre-normal mapped interpolated normal and tangents
        (i.e. tangent space).

        Always (0,0,1) when there is no normal map. 

        This is useful for sampling Source-style Radiosity Normal
        Maps" http://www2.ati.com/developer/gdc/D3DTutorial10_Half-Life2_Shading.pdf
     */
    Vector3         tangentSpaceNormal;

    Radiance3       emission;

    /** ``alpha'' */
    float           coverage;

    // TODO: Make this a field of the base class instead of a method,
    // and then protect all fields with accessors to allow precomputation
    bool            isTransmissive;

    /** Zero = very rough, 1.0 = perfectly smooth (). Equivalent the
      the \f$\alpha\f$ parameter of the Generalized Trowbridge-Reitz
      microfacet BSDF (GTR/GGX).


      \beta
      G3D 10.0 release version will move this method to a member variable
      that will replace

      \sa blinnPhongExponent
    */
    float smoothness() const;

    void setSmoothness(float a);

    /** The glossy exponent in the Blinn-Phong BSDF.

        \beta This will
        become approximate in the future when UniversalSurfel
        moves to the GTR2/GGX microfacet model.
     */
    float blinnPhongExponent() const {
        return glossyReflectionExponent;
    }

    /** Allocates with System::malloc to avoid the performance
        overhead of creating lots of small heap objects using
        the standard library %<code>malloc</code>. 
     */
    static void* operator new(size_t size) {
        return System::malloc(size);
    }

    static void operator delete(void* p) {
        System::free(p);
    }

    UniversalSurfel() : glossyReflectionExponent(0.0f), coverage(1.0f), isTransmissive(false) {}

    static shared_ptr<UniversalSurfel> create() {
        return std::make_shared<UniversalSurfel>();
    }

    UniversalSurfel(const Tri& tri, float u, float v, int triIndex, const CPUVertexArray& vertexArray, bool backside);

    virtual Radiance3 emittedRadiance(const Vector3& wo) const override;
    
    virtual bool transmissive() const override;

    virtual Color3 finiteScatteringDensity
    (const Vector3&    wi, 
     const Vector3&    wo,
     const ExpressiveParameters& expressiveParameters = ExpressiveParameters()) const override;
    
    virtual void getImpulses
    (PathDirection     direction,
     const Vector3&    wi,
     ImpulseArray&     impulseArray,
     const ExpressiveParameters& expressiveParameters = ExpressiveParameters()) const override;

    virtual Color3 reflectivity(Random& rng,
     const ExpressiveParameters& expressiveParameters = ExpressiveParameters()) const override;

    virtual bool nonZeroFiniteScattering() const override {
        return (isFinite(glossyReflectionExponent) && glossyReflectionCoefficient.nonZero()) 
            || lambertianReflectivity.nonZero();
    }

    /** 
        Optimized to avoid numerical integration for lambertian-only and perfectly black surfaces.
     */
    virtual Color3 probabilityOfScattering
    (PathDirection pathDirection, const Vector3& w, Random& rng,
     const ExpressiveParameters& expressiveParameters = ExpressiveParameters()) const override;
    
};

} // namespace G3D
