/**
 \file    GLG3D/UniversalBSDF.h
 \author  Morgan McGuire, http://graphics.cs.williams.edu
 \created 2008-08-10
 \edited  2015-01-03

 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/
#ifndef GLG3D_SuperBSDF_h
#define GLG3D_SuperBSDF_h

#include "GLG3D/Component.h"
#include "G3D/SmallArray.h"

namespace G3D {

/**
   \brief G3D's default description of how a surface reflects light (photons).

   This is an analytic energy-conserving Bidirectional Scattering
   Distribution Function (BSDF) with phenomenonlogically meaningful
   parameters. It comprises Lambertian reflection, Schlick's Fresnel
   approximation for glossy and mirror reflection, Sloan, Hoffman, and Lafortune's
   normalization of the Blinn-Phong glossy lobe, and transmission
   (without exponential extinction) terms.  It is a extension of the isotropic
   version of Ashikhmin and Shirley's empirical BRDF http://www.cs.utah.edu/~shirley/papers/jgtbrdf.pdf .

   The methods of this class are primarily used for photon mapping,
   ray tracing, and software rasterization.  The G3D::UniversalMaterial class
   manages BSDFs for GPU rasterization.

   A surface is the 2D boundary between two 3D volumes.  BSDF works with single-sided surfaces,
   so it is assumed that for transparent materials there are <i>two</i> oppositely-oriented 
   surfaces, typically with different BSDFs, at every such boundary.  Thus there 
   are two indices of refraction at a surface: one for the inside (side opposite the normal)
   and one for the outside.

   The major routines are:

   <table border=0>
   <tr><td width=20></td><td width=100><b>scatter()</b></td><td>\copybrief scatter()</td></tr>
   <tr><td></td><td><b>getImpulses()</b></td><td>\copybrief getImpulses()</td></tr>
   <tr><td></td><td ><b>evaluate()</b></td><td>\copybrief evaluate()</td></tr>
   </table>

   The material is parameterized by:

   <table border=0>
   <tr valign=top><td width=20></td><td width=100>\f$\rho_{L0}\f$</td><td>[UniversalBSDF::lambertian.rgb, UniversalMaterial::Specification::setLambertian]. Peak Lambertian (a.k.a. "diffuse surface color") reflectance, on [0, 1]. The actual
        reflectance applied at normal incidence is \f$(1 - F_0) * \rho_{L0}\f$</td></tr>
   </td></tr>
   <tr valign=top><td></td><td>\f$T_0\f$</td><td>[UniversalBSDF::transmissive, UniversalMaterial::Specification::setTransmissive]. Transmission modulation factor ("transparent color") for the entire volume, on [0, 1]; 
     0 for opaque surfaces. The actual transmission at normal incidence will be 
     \f$(1 - F_0) * T_0\f$.  This is a fast approximation.  Use the extinction coefficients for true participating medium transmission.</td></tr>
   <tr valign=top><td></td><td>\f$F_0\f$</td><td>[UniversalBSDF::glossy.rgb, UniversalMaterial::Specification::setShininess]. Fresnel reflection at normal incidence (a.k.a. "glossy/glossy/reflection color") on [0, 1]</td></tr>
   <tr valign=top><td></td><td>\f$\sigma\f$</td><td>[UniversalBSDF::glossy.a, UniversalMaterial::Specification::setShininess] Surface shininess/smoothness (a.k.a. "shininess", "glossy exponent") 0 for purely Lambertian surfaces, 
     packedGlossyMirror() / UniversalMaterial::Specification::setMirrorShininess() for perfect reflection, and values between packGlossyExponent(1) and packGlossyExponent(128)
     for glossy reflection.  This is used to compute \f$s\f$, the exponent on the normalized Blinn-Phong lobe.</td></tr>
    <tr valign=top><td></td><td>\f$\eta_\mathrm{i}\f$</td><td>Index of refraction outside the material, i.e., on the same side as the normal (only used for surfaces with \f$\rho_t > 0\f$; 
      for computing refraction angle, not used for Fresnel factor).</td></tr>
    <tr valign=top><td></td><td>\f$\eta_\mathrm{o}\f$</td><td>Index of refraction inside the material, i.e., opposite the normal (only used for surfaces with \f$\rho_t > 0\f$; 
      for computing refraction angle, not used for Fresnel factor).</td></tr>
    </table>
 
   For energy conservation, choose \f$F_0 + (1 - F_0)T_0\leq 1\f$ and \f$\rho_{L0} + T_0 \leq 1\f$.

   The following terminology for photon scattering is used in the 
   G3D::UniversalMaterial::Settings and G3D::UniversalBSDF classes and 
   their documentation:
   \htmlonly 
   <center><img src="scatter-terms.png" width=80%/></center>
   \endhtmlonly

   (Departures from theory for artistic control: The direct shader always applies a glossy highlight with an exponent of 128 to mirror surfaces so that
   light sources produce highlights.  Setting the Glossy/Mirror coefficient to zero for a transmissive surface
   guarantees no reflection, although real transmissive surfaces should always be reflective at glancing angles.)

   The BSDF consists of four terms (at most three of which are
   non-zero): Lambertian, Glossy, Mirror, and Transmissive,

   \f[
   f(\vec{\omega}_\mathrm{i}, \vec{\omega}_\mathrm{o}) = f_L + f_g + f_m + f_t
   \f]

   where

   \f{eqnarray}
     \nonumber f_L &=& \rho_{L0} \cdot \frac{F_L(\vec{\omega}_\mathrm{i})}{\pi} \\
     \nonumber f_g &=& \left\{\begin{array}{ccc}     
 F_r(\vec{\omega}_\mathrm{i}) \cdot \frac{s + 8}{8 \pi} \cdot \max(0, \vec{n} \cdot \vec{\omega}_\mathrm{h})^{s} && \mbox{\texttt{packGlossyExponent}}(0) < \sigma <  \mbox{\texttt{packedGlossyMirror()}}\\
 \\
 0~\mbox{sr}^{-1} & &  \mbox{otherwise}  \\
\end{array}\right.\\
    \nonumber f_m &=& \left\{\begin{array}{ccc}
F_r(\vec{\omega}_i) ~ \delta(\vec{\omega}_o, \vec{\omega}_m) ~/ ~(\vec{\omega}_i \cdot \vec{n}) &&  \sigma =  \mbox{\texttt{packedGlossyMirror()}}\\
 \\
 0~\mbox{sr}^{-1} & &  \mbox{otherwise}  \\
\end{array}\right.\\
     \nonumber f_t &=& F_t(\vec{\omega}_i) ~ T_0 ~ \delta(\vec{\omega}_o, \vec{\omega}_t) ~ / ~(\vec{\omega}_i \cdot \vec{n})
   \f}

   All vectors point outward from the surface. Let

   \f{eqnarray}
       \nonumber \vec{\omega}_h &=& \langle \vec{\omega}_i + \vec{\omega}_o \rangle\\
       \nonumber s &=& \mbox{\texttt{unpackGlossyExponent}}(\sigma)\\
       \nonumber \vec{\omega}_m &=& 2 (\vec{\omega}_i \cdot \vec{n}) \vec{n} - \vec{\omega}_i\\
       \nonumber \vec{\omega}_t &=& -\frac{\eta_i}{\eta_t}(\vec{\omega}_i - (\vec{\omega}_i \cdot \vec{n}) \vec{n}) - \vec{n} \sqrt{1-\left( \frac{\eta_i}{\eta_t} \right)^2(1 - \vec{\omega}_i \cdot \vec{n})^2}\\
       \nonumber F_t(\vec{\omega}_i) &=& 1 - F_r(\vec{\omega}_i)\\
       \nonumber F_L(\vec{\omega}_i) &=& 1 - F_r(\vec{\omega}_i)\\
       \nonumber F_r(\vec{\omega}_i) &=& F_0 + (1 - F_0) (1 - \max(0, \vec{\omega}_i \cdot \vec{n}))^5
   \f}

  \f$F_r(\vec{\omega}_i)\f$ is the Fresnel mirror reflection coefficient, which is 
  approximated by Schlick's method as shown above.

  The \f$T_0\f$ factor is the only significant source of error in the
  BSDF. An accurate scatting function would transmit with probabilty
  \f$F_t\f$ and then attenuate the scattered photon based on the
  distance traveled through the translucent medium.  The concession to
  applying a constant attenuation is a typical one in rendering,
  however.  


  \beta UniversalBSDF is scheduled to be merged into G3D::UniversalMaterial in December 2012.

  \sa G3D::UniversalMaterial, G3D::Component, G3D::BumpMap, G3D::Texture, G3D::Surfel
*/
class UniversalBSDF : public ReferenceCountedObject {
public:

    /** Reference counted pointer alias.*/
    typedef shared_ptr<UniversalBSDF> Ref;

protected:

    /** @brief Packed factors affecting the lambertian term.

        - rgb = \f$\rho_L\f$ : lambertian scattering probability
        - a = coverage mask (mainly useful only for maps, not constants).
    */
    Component4          m_lambertian;

    /**
       @brief Packed factors affecting mirror and glossy reflection.

       - rgb = \f$F_0\f$ : glossy scattering probability/Fresnel
         reflectance at normal incidence. This is dependent on eta,
         although the interface allows them to be set independently.
       - a = \f$s/129\f$ : shininess (glossy exponent) divided by 129.
    */
    Component4          m_glossy;

    /** \f$T_0\f$ : transmissivity */
    Component3          m_transmissive;

    /** \f$\eta_t\f$ For the material on the inside.*/
    float               m_eta_t;

    /** \f$\kappa_t\f$ Extinction coefficient for the material on the inside;
        complex part of the index of refraction.
        http://en.wikipedia.org/wiki/Complex_index_of_refraction#Dispersion_and_absorption*/
    Color3              m_extinction_t;

    /** \f$\eta_r\f$ For the material on the outside.*/
    float               m_eta_r;

    Color3              m_extinction_r;

    inline UniversalBSDF() : 
        m_lambertian(Texture::white()), 
        m_glossy(Texture::opaqueBlack()),
        m_transmissive(Texture::zero()),
        m_eta_t(1.0f), 
        m_extinction_t(Color3::zero()),
        m_eta_r(1.0f), 
        m_extinction_r(Color3::zero()){}

public: 
    static float ignoreFloat;

    static shared_ptr<UniversalBSDF> create
    (const Component4&              lambertian,
     const Component4&              glossy               = Texture::zero(),
     const Component3&              transmissive         = Texture::opaqueBlack(),
     float                          eta_transmit         = 1.0f,
     const Color3&                  extinction_transmit  = Color3::zero(),
     float                          eta_reflect          = 1.0f,
     const Color3&                  extinction_reflect   = Color3::zero());


    /** \sa SpeedLoad */
    static Ref speedCreate(BinaryInput& b);

    /** \sa SpeedLoad */
    void speedSerialize(BinaryOutput& b) const;

    /** Computes F_r, given the cosine of the angle of incidence and 
       the reflectance at normal incidence. Uses smoothness as a masking term
       to keep rough surfaces from having too much Fresnel*/
    static inline Color3 schlickFresnel(const Color3& F0, float cos_i, float smoothness) {
        return (F0.r + F0.g + F0.b > 0.0f) ? lerp(F0, Color3(1.0f), (0.05f + smoothness * 0.95f) * pow5(1.0f - cos_i)) : F0;
    }

    /** @brief Packed factors affecting the lambertian term.

        - rgb = \f$\rho_L\f$ : lambertian scattering probability
        - a = coverage mask (mainly useful only for maps, not constants).
    */
    inline const Component4& lambertian() const {
        return m_lambertian;
    }

    /** \f$T_0\f$ : transmissivity */
    inline const Component3& transmissive() const {
        return m_transmissive;
    }

    /** \f$\eta_t\f$ for the material on the inside of this object (i.e. side opposite the normal).*/
    inline float etaTransmit() const {
        return m_eta_t;
    }

    /** \f$\kappa_t\f$ Extinction coefficient for the material on the inside;
        complex part of the index of refraction.
        http://en.wikipedia.org/wiki/Complex_index_of_refraction#Dispersion_and_absorption*/
    inline const Color3& extinctionTransmit() const {
        return m_extinction_t;
    }

    /** \f$\eta_r\f$ for the material on the outside of this object (i.e. side of the normal).*/
    float etaReflect() const {
        return m_eta_r;
    }

    /** \f$\kappa_r\f$ Extinction coefficient for the material on the outside;
        complex part of the index of refraction.
        http://en.wikipedia.org/wiki/Complex_index_of_refraction#Dispersion_and_absorption*/
    const Color3& extinctionReflect() const {
        return m_extinction_r;
    }

    /**
       @brief Packed factors affecting mirror and glossy reflection.

       - rgb = \f$F_0\f$ : glossy scattering probability/Fresnel
         reflectance at normal incidence. This is dependent on eta,
         although the interface allows them to be set independently.
       - a = \f$s\f$ : smoothness
    */
    const Component4& glossy() const {
        return m_glossy;
    }
    
    /** \brief Move or copy data to CPU or GPU.  
        Called from G3DMaterial::setStorage(). */
    virtual void setStorage(ImageStorage s) const;

    /** \brief Return true if there is any glossy (non-Lambertian, non-mirror) 
        reflection from this BSDF. */
    bool hasGlossy() const;

    /** \brief Return true if there is any mirror reflection from this BSDF. */
    bool hasMirror() const;

    /** \brief Return true if there is any Lambertian reflection from this BSDF. */
    bool hasLambertian() const;

    /** \brief Return true if there is any Lambertian, mirror, or glossy reflection from this BSDF (not just mirror!)*/
    inline bool hasReflection() const {
        return ! m_lambertian.isBlack() ||
               ! m_glossy.isBlack();
    }
    
    /** True if this absorbs all light */
    inline bool isZero() const {
        return m_lambertian.isBlack() && 
               m_glossy.isBlack() &&
               m_transmissive.isBlack();
    }



    /** The value that a mirror's glossy exponent (infinity) is packed as */
    inline static float packedSpecularMirror() {
        return 1.0f;
    }

    /** The value that a non-glossy surface is packed as */
    inline static float packedGlossyNone() {
        return 0.0f;
    }

    /** The glossy exponent is packed so that 0 = no glossy, 
        1 = mirror (infinity), and on the open interval \f$e \in (0, 1), ~ e \rightarrow 8192.0f e^2 + 1/2\f$.
        This function abstracts the unpacking, since it may change in future versions.        
        */
    static inline float unpackGlossyExponent(float e) {
        if (e >= 1.0) {
            return finf();
        } else {
            return square((clamp(e, 0.0f, 1.0f) * 255.0f - 1.0f) * (1.0f / 253.0f)) * 8192.0f + 0.5f;
        }
    }

    /** Packing is \f$\frac{ \sqrt{ \frac{x - 1/2}{8192} } * 253 + 1}{255} \f$ for \f$x < 8191 \f$, 1.0f/254.0 for
        \f$1024 \leq x < \infty\f$, and 1.0f for \f$x = \infty \f$ */
    static inline float packGlossyExponent(float x) {
        if (x == 0.0f) {
            return 0.0;
        } else {
            // Never let the exponent go above the max representable non-mirror value in a uint8
            return (clamp((float)(sqrt((x - 0.5f) * (1.0f / 8192.0f))), 0.0f, 1.0f) * 253.0f + 1.0f) * (1.0f / 255.0f);
        }
    }
};

}

#endif

