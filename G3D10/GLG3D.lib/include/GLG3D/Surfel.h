/** \file G3D/Surfel.h
    \maintainer Morgan McGuire, http://graphics.cs.williams.edu

    \created 2011-07-01
    \edited  2013-03-16
    
    Copyright 2012 Morgan McGuire

    */
#ifndef G3D_Surfel_h
#define G3D_Surfel_h


#include "G3D/platform.h"
#include "G3D/Vector3.h"
#include "G3D/Color3.h"
#include "G3D/Array.h"
#include "G3D/SmallArray.h"
#include "G3D/PathDirection.h"

namespace G3D {

class Random;
class Any;
class CoordinateFrame;
class Material;
class Surface;

/** 
\brief Local surface geometry + BSDF + emission function.

The Surfel models the interface between two homogeneous media at a small patch
on a surface.  It is oriented, and the normal is used to give
orientation to distinguish the two sides.  This combines the
mathematical models of a BSDF, and emission function, and a
surface patch.

Scene geometric sampling functions (e.g., ray intersection) return
Surfel subclasses that have been initialized with the appropriate
coefficients for that location in the scene (i.e., they sample
the texture map).

All vectors and positions are in world space.  In this documentation,

- \f$X\f$ is a point at which scattering (or absorption) occurs
- \f$\hat{n}\f$ is a unit surface normal at \f$X\f$
- \f$\hat{\omega}_\mathrm{i}\f$ is a unit vector pointing backwards along the direction that a real photon arrived a surface before scattering
- \f$\hat{\omega}_\mathrm{o}\f$ is a unit vector in the direction that a real photon leaves a surface after scattering
- \f$ f_X \f$ is the Bidirectional Scattering Distribution Function (BSDF) at point \f$ X \f$, which is proportional to the probability density function describing how a photon scatters
- \f$ g \f$ is proportional to a probability density function that is used for sampling; it is ideally close to \f$ f_X \f$ to give good efficiency, but achieving that is impractical for complex BSDFs

   \htmlonly 
   <center><img src="surfel-variables.png"/></center>
   \endhtmlonly

<hr>

The BSDF (\f$ f_X \f$) must obey the following constraints:

- Normalized: \f$ \int_{\mathbf{S}^2} f_X (\hat{\omega}_\mathrm{i}, \hat{\omega}_\mathrm{o}) |\hat{\omega}_\mathrm{i} \cdot \hat{n}| d\hat{\omega}_\mathrm{i} \leq 1 \f$ for all \f$ \hat{\omega_\mathrm{o}} \f$
- Real and non-negative everywhere
- Obey the reciprocity principle \f$ \frac{ f_X(\hat{\omega}_\mathrm{i}, \hat{\omega}_\mathrm{o}) }{ \eta_\mathrm{o}^2} = \frac{ f_X(\hat{\omega}_\mathrm{o}, \hat{\omega}_\mathrm{i}) }{ \eta_\mathrm{i}^2 } \f$

<hr>

Most methods require a PathDirection because transmissive BSDFs
are different depending on whether the path denotes a photon
moving along its physical direction of propagation or a virtual
path backwards from an eye.  For non-transmissive BSDFs the
result is the same either way.

<hr>

scatter() returns a weight that compensates for
distortion of the sampling distribution within the functions.  When
the scattering is non refractive, the caller must adjust the power or 
radiance scattered by this weight:

\f{eqnarray*}
    \Phi_\mathrm{o} = \Phi_\mathrm{i} \cdot \mathrm{weight}\\
    L_\mathrm{o} = L_\mathrm{i} \cdot \mathrm{weight}
\f}


The refractive case requires more care.  If you're are tracing paths
that will be connected as rays as in ray tracing or path tracing,
then at a refractive interface you must adjust the radiance by:

\f[
    L_\mathrm{o} = \frac{\eta_\mathrm{i} }{ \eta_\mathrm{o}} \cdot L_\mathrm{i} \cdot \mathrm{weight}
\f]

after invoking scatterIn or scatterOut when the path refracts.  You
can detect the refractive case by:

\code
  bool refracted = (sign(wo.dot(surfel->shadingNormal)) != sign(wi.dot(surfel->shadingNormal)));
\endcode

However, if you're tracing paths that represent photons that will be
gathered to estimate radiance by:

\f[
    L_\mathrm{o} = (\Phi_\mathrm{i} / \mathrm{gatherArea}) \cdot f_X(\hat{\omega_\mathrm{i}}, \hat{\omega_\mathrm{o}}) \cdot |\hat{\omega_\mathrm{o}} \cdot \hat{n}|
\f]

then adjust the power after scatter() by:

\f[
    \Phi_\mathrm{o} = \Phi_\mathrm{i} \cdot \mathrm{weight}
\f]
 
In this case, the divergence (or convergence) of the photons at a
refractive interface will naturally be captured by the number of
photons within gatherArea and no explicit correction is required.

<hr>

For subclass implementers:

If it is computationally harder to importance sample than to trace
rays, make the scatter functions use weights and select directions
naively (e.g., uniformly).  In this case, more rays will be needed
for convergence because the amount of energy they contribute to the
final signal will have high variance.

If it is computationally harder to trace rays than to importance
sample, then it is worth spending a longer time on importance
sampling to drive the weights close to 1.0, meaning that each ray
contributes approximately the same energy towards convergence of
the image.

Of course, the Surfel implementor cannot make these decisions 
in the context of a single class--they are end-to-end decisions
for an entire system.    

\cite Thanks to John F. Hughes, Matt Pharr, and Michael Mara for help designing this API
 */
class Surfel : public ReferenceCountedObject {
public:

    /** Non-physical manipulations of the BSDF commonly employed for expressive rendering effects. \beta*/
    class ExpressiveParameters {
    public:

        /** Scale the diffuse (i.e., non-impulse) reflectivity of surfaces with saturated 
            diffuse spectra by this amount.

            In <i>Mirror's Edge</i>, values as high as 10 were used for this term.

            Note that many BSDFs will not be energy conserving if this value is greater than 1.0f.
         */
        float               saturatedMaterialBoost;

        /** Scale the diffuse (i.e., non-impulse) reflectivity of surfaces with <b>un</b>saturated 
            diffuse spectra by this amount.
         */
        float               unsaturatedMaterialBoost;

        /** Return the amount to boost reflectivity for a surface with a priori reflectivity diffuseReflectivity. */
        float boost(const Color3& diffuseReflectivity) const;

        ExpressiveParameters() : saturatedMaterialBoost(1.0f), unsaturatedMaterialBoost(1.0f) {}

        ExpressiveParameters(const Any& a);
        Any toAny() const;
    };
    
    /** 
        \brief A BSDF impulse ("delta function").
     */
    class Impulse {
    public:
        /** Unit length facing away from the scattering point. This
            may either be an incoming or outgoing direction depending
            on how it was sampled. */
        Vector3       direction;

        /** Probability of scattering along this impulse; the integral
            of the non-finite portion of the BSDF over a small area
            about direction.  The magnitude must be on the range [0,
            1] for each channel because it is a probability.

            The factor by which incoming radiance is scaled to get
            outgoing radiance.
        */
        Color3        magnitude;

        Impulse(const Vector3& d, const Color3& m) : direction(d), magnitude(m) {}

        // For SmallArray's default constructor
        Impulse() {}
    };

    /** 
        \brief Impulses in the BSDF.

        This contains three inline-allocated elements to support reflection,
        refraction, and retro-reflection without heap allocation. */
    typedef SmallArray<Impulse, 3> ImpulseArray;

    /** For debugging purposes only. */
    String       name;
    
    /** Point in world space at the geometric center of
        this surfel. */ 
    Point3            position;

    /** Point in world space at the geometric center of
        this surfel in the previously rendered frame of animation.
        Useful for computing velocity. */ 
    Point3            prevPosition;

    /** \deprecated Use position.*/
    Point3&           location;

    /** The normal to the true underlying geometry of the patch that
        was sampled (e.g., the face normal).  This is often useful for ray bumping. 

        Always a unit vector.
    */
    Vector3           geometricNormal;

    /** The normal to the patch for shading purposes, i.e., after
        normal mapping.  e.g., the interpolated vertex normal
        or normal-mapped normal.

        Always a unit vector.*/
    Vector3           shadingNormal;

    /** Primary tangent vector for use in shading anisotropic surfaces.  
       This is the tangent space after normal mapping has been applied.*/
    Vector3           shadingTangent1;

    /** Secondary shading tangent. \see shadingTangent1 */
    Vector3           shadingTangent2;
    
    /** Complex refractive index for each side of the interface.  

        eta (\f$\eta\f$) is the real part, which determines the angle of refraction.
        kappa (\f$\kappa\f$) is the imaginary part, which determines absorption within
        the medium.

        For a non-transmissive medium, eta should be NaN() and kappa
        should be Color3::inf().

        The "Pos" versions are for the material on the side of
        the surface that the geometricNormal faces towards.  The "Neg"
        versions are for the material on the side opposite the
        geometricNormal.

        We ignore the frequency-variation in eta, since three color
        samples isn't enough to produce meaningful chromatic
        dispersion anyway.
    */
    float             etaPos;

    /** \copydoc etaPos */
    Color3            kappaPos;

    /** \copydoc etaPos */
    float             etaNeg;

    /** \copydoc etaPos */
    Color3            kappaNeg;

    /** The material that generated this Surfel. May be NULL */
    shared_ptr<Material>    material;

    /** The surface that generated this Surfel. May be NULL */
    shared_ptr<Surface>     surface;

    /** Mostly for debugging */
    struct Source {
        /** Index of this primitive in the source object (interpretation depends on the object type). 
         For TriTree, use this with TriTree:operator[] as a primitive index.*/
        int      index;

        /** Barycentric coordinate corresponding to vertex 1 (NOT vertex 0) */
        float    u;

        /** Barycentric coordinate corresponding to vertex 2 */
        float    v;

        Source() : index(-1), u(0), v(0) {}
        Source(int i, float u, float v) : index(i), u(u), v(v) {}
    } source;

protected:

    Surfel
    (const String& name,
     const Point3&     position,
     const Point3&     prevPosition,
     const Vector3&    geometricNormal,
     const Vector3&    shadingNormal,
     const Vector3&    shadingTangent1,
     const Vector3&    shadingTangent2,
     const float       etaPos,
     const Color3&     kappaPos,
     const float       etaNeg,
     const Color3&     kappaNeg,
     const Source&     source,
     const shared_ptr<Material>& material,
     const shared_ptr<Surface>& surface);

public:

    Surfel() : location(position) {}

    virtual ~Surfel() {}

    /** 
        \brief Returns the radiance emitted by this surface in
        direction \a wo.

        This models an emission function.

        Defaults to zero. N.B. A "Lambertian emitter", which is often
        considered the ideal area light source, would return a
        constant value on the side

        Note that this interface is intentionally insufficient for
        representing a point light, which requires a biradiance
        function.

        If this function returns a non-zero value, then emissive()
        must return true.
     */
    virtual Radiance3 emittedRadiance(const Vector3& wo) const {
        return Radiance3::zero();
    }

    /** Transform this to world space using the provided \a xform. */
    virtual void transformToWorldSpace(const CoordinateFrame& xform);

    /** Must return true if a ray is ever scattered to the opposite side
        of the surface with respect to the Surfel::shadingNormal.  
        
        This may conservatively always return true (which is the 
        default implementation). Setting this correctly doubles the 
        efficiency of the default implementation of other methods.

        There is no equivalent reflective() method because all physical
        transmissive surfaces are also reflective, so it would rarely be
        false in practice.
       */
    virtual bool transmissive() const {
        return true;
    }

    /** 
        True if this surfel's finiteScatteringDensity function ever
        returns a non-zero value.  (May also be true even if it does
        only ever return zero, however).
     */
    virtual bool nonZeroFiniteScattering() const {
        return true;
    }

    /** 
        \brief Evaluates the finite portion of \f$f_X(\hat{\omega_\mathrm{i}}, \hat{\omega_\mathrm{o}})\f$.
        
        This is the straightforward abstraction of the BSDF, i.e.,
        "forward BSDF evaluation".  It is useful for direct illumination 
        and in the gather kernel of a ray tracer.  Note that this does <i>not</i>
        scale the result by \f$|\hat{n} \cdot \hat{\omega}_\mathrm{i}|\f$.

        It omits the impulses because they would force the return
        value to infinity.

        \param wi Unit vector pointing backwards along the direction from which
        light came ("to the light source")
        
        \param wi Unit vector pointing out in the direction that light will go
        ("to the eye")
     */
    virtual Color3 finiteScatteringDensity
    (const Vector3&    wi, 
     const Vector3&    wo,
     const ExpressiveParameters& expressiveParameters = ExpressiveParameters()) const = 0;


    /**
       Provided as a convenience helper method for implementers of scatter().
       Allows programmatically swapping the directions.

       - <code>finiteScatteringDensity(PathDirection::SOURCE_TO_EYE, wi, wo)</code> = \f$f_X(\hat{\omega_\mathrm{i}}, \hat{\omega_\mathrm{o}})\f$
       - <code>finiteScatteringDensity(PathDirection::EYE_TO_SOURCE, wo, wi)</code>= \f$f_X(\hat{\omega_\mathrm{o}}, \hat{\omega_\mathrm{i}})\f$
     */
    virtual Color3 finiteScatteringDensity
    (PathDirection pathDirection,
     const Vector3&    wFrom, 
     const Vector3&    wTo,
     const ExpressiveParameters& expressiveParameters = ExpressiveParameters()) const;


    /**
     \brief Given \a wi, returns all wo directions that yield
      impulses in \f$f_X(\hat{\omega_\mathrm{i}}, \hat{\omega_\mathrm{o}})\f$ for PathDirection::SOURCE_TO_EYE
      paths, and vice versa for PathDirection::EYE_TO_SOURCE paths.

      Overwrites the impulseArray.

      For example, <code>getImpulses(PathDirection::SOURCE_TO_EYE, wi,
      impulseArray)</code> returns the impulses for scattering a
      photon emitted from a light.

      <code>getImpulses(PathDirection::EYE_TO_SOURCE, wo,
      impulseArray)</code> returns the impulses for scattering a
      backwards-traced ray from the ey.
     */
    virtual void getImpulses
    (PathDirection     direction,
     const Vector3&    wi,
     ImpulseArray&     impulseArray,
     const ExpressiveParameters& expressiveParameters = ExpressiveParameters()) const = 0;


    static float ignore;
    /** 
\brief Computes the direction of a scattered photon and a \a weight
that compensates for the way that the the sampling process is
performed.

<ul>
<li> For photons, call: <code>scatter(PathDirection::SOURCE_TO_EYE, wi, ..., wo)</code></li>
<li> For backwards rays, call: <code>scatter(PathDirection::EYE_TO_SOURCE, wo, ..., wi)</code></li>
</ul>

The following description is for the PathDirection::SOURCE_TO_EYE
case.  The description of the backwards case is the same, but with \a
wi and \a wo swapped.  Note that for a non-transmissive surface the
function is symmetric and gives the same result regardless of \a
pathDirection.
           
Sets \a wo to the outgoing photon direction and \a
weight to a statistical energy compensation factor on the
scattered radiance.  

If the photon was absorbed, then \a wo will be Vector3::nan().

This method is useful for photon and bidirectional path tracing.

<b>Details:</b><br/>
Given \a wi, samples \a wo from a PDF proportional to 

\f[ 
\hat{\omega_\mathrm{i}} \to g( \hat{\omega_\mathrm{i}},  \hat{\omega_\mathrm{o}}) \cdot |\hat{\omega_\mathrm{o}} \cdot \hat{n}|,
\f]

where the shape of \f$g\f$ is ideally close to that of BSDF \f$f\f$.
Note that this includes impulse scattering, i.e., the whole BSDF is
considered for scattering and not just the finite portion.

The \a weight is

\f[
\mathrm{weight} = \lim_{|\Gamma| \to 0~\mathrm{sr}, |\hat{\omega}_\mathrm{o} \in \Gamma|}
                      \frac{~~~~\frac{\int_\Gamma f(\hat{\omega_\mathrm{i}}, \hat{h}) ~|\hat{h} \cdot \hat{n}| ~ d\hat{h}}{\int_{\mathbf{S}^2} f(\hat{\hat{\omega}_\mathrm{i}}, \hat{v})~ | \hat{v} \cdot \hat{n}|~ d\hat{v}}~~~~}                 
                           {~~~~\frac{\int_\Gamma g(\hat{\omega_\mathrm{i}}, \hat{h})~|\hat{h}\cdot\hat{n}|~ d\hat{h}}{\int_{\mathbf{S}^2} g(\hat{\omega_\mathrm{i}}, \hat{v}) ~|\hat{v}\cdot\hat{n}|~d\hat{v}}~~~~} 
\f]

which is evaluated independently for each frequency (where frequency
samples are modeled by color channels).  That is, the weight is the
ratio of the normalized versions of the probability distribution
functions at the location sampled, with the limit accomodating
impulses.  By way of analogy, consider sampling with respect to the 1D PDF
\f$g(w)\f$ below, when desiring samples distributed according to \f$f(w)\f$:

\htmlonly 
<center><img src="Surfel-weight.png"/></center>
\endhtmlonly

The sample taken was selected uniformly at random, when it should have been
sampled half as frequently as the average sample.  We therefore would weigh its
contribution by \f$f(w)/g(w) = 0.5 / 1.0 = 0.5\f$.

Note that the weight might be zero even if the photon scatters, for
example, a perfectly "red" photon striking a perfectly "green" surface
would have a weight of zero.

The \a weight is needed to allow efficient computation of scattering
from BSDFs that have no computationally efficient method for exact
analytic sampling.  The caller should scale the radiance or power
transported by the \a weight.  The caller should discontinue tracing when
that product is nearly zero.

The \a probabilityHint is for use by photon mappers to decrease
convergence time.  If the a priori probability density of scattering
in this direction was infinity, it returns 1.0.  If the density was
0.0, it returns 0.  For finite densities it scales between them.

Note that because they are driven by the finite portion of the BSDF,
weights returned must be non-negative and finite, but are not bounded.
However, an efficient importance-sampling implementation will return
weights close to 1.0.  In the best case, \f$f\f$ and \f$g\f$ differ by
only a constant, so \f$ f() = k \cdot g()\f$ and \a weight = 1.
  
        
The default implementation relies on getIncoming() and inefficient
uniform (vs. importance) sampling against
evaluateFiniteScatteringDensity().  For example, for a white
Lambertian BSDF it will return \a weight = 0 half of the time and \a
weight = 2 the rest of the time.  That is because it is randomly
sampling the entire sphere, returning no \a weight for samples inside
the material, and then increasing the weight of the samples that do
reflect to compensate.  Thus it is desirable to optimize the
implementation in subclasses where possible.
    */
    virtual void scatter
    (PathDirection    pathDirection,
     const Vector3&   wi,
     bool             russianRoulette,
     Random&          rng,
     Color3&          weight,
     Vector3&         wo,
     float&           probabilityHint = ignore,
     const ExpressiveParameters& expressiveParameters = ExpressiveParameters()) const;


    /**
\brief Given \a wi, returns the a priori probability of  scattering in any direction (vs absorption), 
\f[
\int_{\mathbf{S}^2} f_X(\hat{\omega_\mathrm{i}}, \hat{\omega_\mathrm{o}}) |\hat{\omega_\mathrm{o}} \cdot \hat{n}| d\hat{\omega_\mathrm{o}}.\f]
        
This is called by scatter().
        
By default, the probability computed by sampling
evaluateFiniteScatteringDensity() and getImpulses()
because analytic integrals of the BSDF do not exist for many
scattering models.

The return value is necessarily on [0, 1] for each frequency unless non-default expressive parameters are used.
    */
    virtual Color3 probabilityOfScattering(PathDirection pathDirection, const Vector3& w, Random& rng,
     const ExpressiveParameters& expressiveParameters = ExpressiveParameters()) const;

    /** Approximate reflectivity of this surface, primarily used for ambient terms. 
        The default implementation invokes probabilityOfScattering many times 
        and is not very efficient.
    */
    virtual Color3 reflectivity(Random& rng,
     const ExpressiveParameters& expressiveParameters = ExpressiveParameters()) const;
};

} // namespace G3D

#endif
