/** \file G3D/Surfel.h
    \maintainer Morgan McGuire, http://graphics.cs.williams.edu

    \created 2011-07-01
    \edited  2016-09-01
    
 G3D Innovation Engine
 Copyright 2000-2016, Morgan McGuire.
 All rights reserved.
 */
#pragma once

#include "G3D/platform.h"
#include "GLG3D/Tri.h"
#include "GLG3D/Surfel.h"

namespace G3D {

class CPUVertexArray;

/** 
 \brief A Surfel for a surface patch described by a UniversalMaterial.

 Computes the Surfel::ExpressiveParameters::boost solely from the lambertianReflectivity coefficient.

 \sa UniversalMaterial
*/
class UniversalSurfel : public Surfel {
protected:

    virtual void sampleFiniteDirectionPDF
    (PathDirection      pathDirection,
     const Vector3&     w_o,
     Random&            rng,
     const ExpressiveParameters& expressiveParameters,
     Vector3&           w_i,
     float&             pdfValue) const override;

public:

    /** \f$ \rho_\mathrm{L} \f$
    */
    Color3          lambertianReflectivity;

    /** F0, the Fresnel reflection coefficient at normal incidence */
    Color3          glossyReflectionCoefficient;
        
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

    /** Zero = very rough, 1.0 = perfectly smooth (mirror). Equivalent the
      the \f$1 - \alpha\f$ parameter of the Generalized Trowbridge-Reitz
      microfacet BSDF (GTR/GGX).

      Transmission is always perfectly smooth 

      \sa blinnPhongExponent
    */
    float           smoothness;

    /** An approximate glossy exponent in the Blinn-Phong BSDF for this BSDF. */
    float blinnPhongExponent() const;

    /** Allocates with System::malloc to avoid the performance
        overhead of creating lots of small heap objects using
        the standard library %<code>malloc</code>. 

        This is actually not used by make_shared, which is the common case.
      */
    static void* operator new(size_t size) {
        return System::malloc(size);
    }

    static void operator delete(void* p) {
        System::free(p);
    }

    UniversalSurfel() : coverage(1.0f), isTransmissive(false), smoothness(0.0f) {}

    static shared_ptr<UniversalSurfel> create() {
        return std::make_shared<UniversalSurfel>();
    }

    void sample(const Tri& tri, float u, float v, int triIndex, const CPUVertexArray& vertexArray, bool backside, const class UniversalMaterial* universalMaterial);

    UniversalSurfel(const Tri& tri, float u, float v, int triIndex, const CPUVertexArray& vertexArray, bool backside) {
        sample(tri, u, v, triIndex, vertexArray, backside, dynamic_pointer_cast<UniversalMaterial>(tri.material()).get());
    }

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
        return ((smoothness < 1.0f) && glossyReflectionCoefficient.nonZero()) || lambertianReflectivity.nonZero();
    }

    /** 
        Optimized to avoid numerical integration for lambertian-only and perfectly black surfaces.
     */
    virtual Color3 probabilityOfScattering
    (PathDirection pathDirection, const Vector3& w, Random& rng,
     const ExpressiveParameters& expressiveParameters = ExpressiveParameters()) const override;
    
};

} // namespace G3D
