/**
  \file GLG3D/UniversalSurface.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2008-11-12
  \edited  2015-01-02
 
 G3D Innovation Engine
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/
#ifndef GLG3D_UniversalSurface_h
#define GLG3D_UniversalSurface_h

#include "G3D/platform.h"
#include "G3D/System.h"
#include "G3D/Array.h"
#include "G3D/Vector3.h"
#include "G3D/Vector2.h"
#include "G3D/MeshAlg.h"
#include "G3D/Sphere.h"
#include "G3D/AABox.h"
#include "G3D/constants.h"
#include "GLG3D/AttributeArray.h"
#include "GLG3D/Surface.h"
#include "GLG3D/UniformTable.h"

namespace G3D {

class CPUVertexArray;
class UniversalMaterial;
class UniversalSurface;
class Args;

/**
   \brief An optimized implementation G3D::Surface for
   G3D::Shader / G3D::UniversalMaterial classes.

   Used by G3D::ArticulatedModel, G3D::MD2Model, G3D::MD3Model
 */
class UniversalSurface : public Surface {
public:

    static void bindDepthPeelArgs
        (Args& args,
        RenderDevice* rd,
        const shared_ptr<Texture>& depthPeelTexture,
        const float minZSeparation);

    /** Allocates with System::malloc to avoid the performance
        overhead of creating lots of small heap objects using
        <code>::malloc</code>. */
    static void* operator new(size_t size) {
        return System::malloc(size);
    }

    static void operator delete(void* p) {
        System::free(p);
    }

    /** \brief A GPU mesh utility class that works with G3D::UniversalSurface.
        
        A set of lines, points, quads, or triangles that have a
        single UniversalMaterial and can be rendered as a single OpenGL
        primitive using RenderDevice::sendIndices inside a
        RenderDevice::beginIndexedPrimitives() block.
        
        \sa G3D::MeshAlg, G3D::ArticulatedModel, G3D::Surface, G3D::CPUVertexArray
    */
    class GPUGeom : public ReferenceCountedObject {
    public:
        MeshAlg::Primitive              primitive;
    
        /** Indices into the VARs. */
        IndexStream                     index;
        AttributeArray                  vertex;
        AttributeArray                  normal;
        AttributeArray                  packedTangent;
        AttributeArray                  texCoord0;
        AttributeArray                  texCoord1;
        AttributeArray                  vertexColor;

        // Either all three are defined or none are
        AttributeArray                  boneIndices;
        AttributeArray                  boneWeights;
        shared_ptr<Texture>             boneTexture;
        shared_ptr<Texture>             prevBoneTexture;
        
        /** When true, this primitive should be rendered with
            two-sided lighting and texturing and not cull
            back faces. */
        bool                            twoSided;
        
        /** Object space bounds */
        AABox                           boxBounds;

        /** Object space bounds */
        Sphere                          sphereBounds;
        
    protected:

        GPUGeom(const shared_ptr<GPUGeom>& other);

        inline GPUGeom(PrimitiveType p, bool t) : 
            primitive(p), twoSided(t) {}
        
    public:

        static void* operator new(size_t size) {
            return System::malloc(size);
        }

        static void operator delete(void* p) {
            System::free(p);
        }

        inline static shared_ptr<GPUGeom> create(const shared_ptr<GPUGeom>& other) {
            return shared_ptr<GPUGeom>(new GPUGeom(other));
        }


        inline static shared_ptr<GPUGeom> create(PrimitiveType p = PrimitiveType::TRIANGLES) {
            return shared_ptr<GPUGeom>(new GPUGeom(p, false));
        }

        /** True if this part has some geometry */
        bool hasGeometry() const {
            return index.size() > 0;
        }

        bool hasBones() const {
            return notNull(boneTexture) && boneIndices.valid() && boneIndices.size() > 0 
                    && boneWeights.valid() && boneWeights.size() > 0;
        }



        /** 
            Sets \param args index array, and the following vertex attributes:
            vec4 g3d_Vertex;
            vec3 g3d_Normal;
            [vec2  g3d_TexCoord0;]
            [vec2  g3d_TexCoord1;]
            [vec4  g3d_PackedTangent;]
            [vec4  g3d_VertexColor;]
            [ivec4 g3d_BoneIndices;]
            [vec4  g3d_BoneWeights;]

            and uniform:
            [sampler2D boneMatrixTexture]

            and macros:
            [HAS_BONES]

            where square brackets denotes optional attributes, dependent on whether the geom contains valid values for them.

            This binds attribute arrays, so it cannot accept a UniformTable argument.
          */
        void setShaderArgs(Args& args) const;
    };

    class CPUGeom {
    public:
        const Array<int>*        index;

        /** If non-NULL, this superceeds geometry, packedTangent, and texCoord0.*/
        const CPUVertexArray*    vertexArray;

        const MeshAlg::Geometry* geometry;

        /**  Packs two tangents, T1 and T2 that form a reference frame with the normal such that 
            
            - \f$ \vec{x} = \vec{T}_1 = \vec{t}_{xyz}\f$ 
            - \f$ \vec{y} = \vec{T}_2 = \vec{t}_w * (\vec{n} \times \vec{t}_{xyz})  \f$
            - \f$ \vec{z} = \vec{n} \f$ */
        const Array<Vector4>*    packedTangent;
        const Array<Vector2>*    texCoord0;

        /** May be NULL */
        const Array<Vector2unorm16>*    texCoord1;
        const Array<Color4>*            vertexColors;
        
        CPUGeom
           (const Array<int>*           index,
            const MeshAlg::Geometry*    geometry,
            const Array<Vector2>*       texCoord0,
            const Array<Vector2unorm16>* texCoord1 = NULL,
            const Array<Color4>*        vertexColors = NULL,
            const Array<Vector4>*       packedTangent = NULL) : 
            index(index), 
            vertexArray(NULL),
            geometry(geometry), 
            packedTangent(packedTangent), 
            texCoord0(texCoord0), 
            texCoord1(texCoord1),
            vertexColors(vertexColors) {}

        CPUGeom
           (const Array<int>*           index,
            const CPUVertexArray*       vertexArray) : 
            index(index), 
            vertexArray(vertexArray),
            geometry(NULL),
            packedTangent(NULL), 
            texCoord0(NULL),
            texCoord1(NULL),
            vertexColors(NULL) {}

        CPUGeom() : index(NULL), vertexArray(NULL), geometry(NULL), packedTangent(NULL), texCoord0(NULL), texCoord1(NULL), vertexColors(NULL) {}
         
        /** Updates the interleaved vertex arrays.  If they are not
            big enough, allocates a new vertex buffer and reallocates
            the vertex arrays inside them.  This is often used as a
            helper to convert a CPUGeom to a GPUGeom.
         */
        void copyVertexDataToGPU
            (AttributeArray& vertex, 
             AttributeArray& normal, 
             AttributeArray& packedTangents, 
             AttributeArray& texCoord0, 
             AttributeArray& texCoord1, 
             AttributeArray& vertexColors, 
             VertexBuffer::UsageHint hint);
    };
    
protected:

    

    /** Used in renderDepthOnlyHomogeneous to store the last pass type. Alpha testing turns of 
        depth-only optimizations on GPUs, so we need to avoid using alpha testing when unneccessary.
        When a surface is both parallax mapped and has alpha, we need a shader to render the depth pass.
        Otherwise the depth pass renders the flat version of the surface, which leads to background color
        bleeding in around the edges where the surface is perturbed .
    */
    enum DepthPassType {
        FIXED_FUNCTION_NO_ALPHA,
        FIXED_FUNCTION_ALPHA,
        PARALLAX_AND_ALPHA
    };

    virtual void defaultRender(RenderDevice* rd) const override {
        alwaysAssertM(false, "Not implemented");
    }

    String                  m_name;

    /** A string used in indentifing profiler events*/
    String                  m_profilerHint;

    /** Object to world space transformation. */
    CoordinateFrame         m_frame;

    /** Object to world transformation from the previous time step. */
    CoordinateFrame         m_previousFrame;
    
    shared_ptr<UniversalMaterial> m_material;

    shared_ptr<GPUGeom>     m_gpuGeom;

    CPUGeom                 m_cpuGeom;

    int                     m_numInstances;

    /** For use by classes that want the m_cpuGeom to point at
     geometry that is deallocated with the surface.*/
    MeshAlg::Geometry       m_internalGeometry;

    shared_ptr<UniformTable> m_uniformTable;

    shared_ptr<ReferenceCountedObject> m_source;

    UniversalSurface
        (const String&                          name,
            const CoordinateFrame&                 frame,
            const CoordinateFrame&                 previousFrame,
            const shared_ptr<UniversalMaterial>&   material,
            const shared_ptr<GPUGeom>&             gpuGeom,
            const CPUGeom&                         cpuGeom,
            const shared_ptr<ReferenceCountedObject>& source,
            const ExpressiveLightScatteringProperties& expressive,
            const shared_ptr<class Model>&         model,
            const shared_ptr<class Entity>&        entity,
            const shared_ptr<class UniformTable>&  uniformTable,
            int                                    numInstances);

    /** Launch the bone or non-bone shader as needed. Called from render() and  renderNonRefractiveTransparency() */
    void launchForwardShader(Args& args) const;

    /** Using the current cull face, modulate the background by the transmission, if there is any transmission.
        Also darkens based on coverage.
        Called from render() for RenderPassType::MULTIPASS_BLENDED_SAMPLES. */
    void modulateBackgroundByTransmission(RenderDevice* rd) const;

    void bindScreenSpaceTexture
       (Args&                                       args, 
        const LightingEnvironment&                  lightingEnvironment,
        RenderDevice*                               rd, 
        const Vector2int16                          colorGuardBandSize,
        const Vector2int16                          depthGuardBandSize) const;
            
public:

    int numInstances() const {
        return m_numInstances;
    }

    /** Bind material and geometry arguments, including setting args.numInstances() */
    void setShaderArgs(Args& args, bool useStructFormat = false) const;

    /** For use by classes that pose objects on the CPU and need a
        place to store the geometry.  See MD2Model::pose
        implementation for an example of how to use this.  */
    const MeshAlg::Geometry& internalGeometry() const {
        return m_internalGeometry;
    }

    MeshAlg::Geometry& internalGeometry() {
        return m_internalGeometry;
    }

    const shared_ptr<UniversalMaterial>& material() const {
        return m_material;
    }

    virtual bool anyUnblended() const override;

    shared_ptr<GPUGeom>& gpuGeom() {
        return m_gpuGeom;
    }

    virtual CPUGeom& cpuGeom() {
        return m_cpuGeom;
    }

    const shared_ptr<GPUGeom>& gpuGeom() const {
        return m_gpuGeom;
    }

    virtual void render
    (RenderDevice*                      rd,
     const LightingEnvironment&         environment,
     RenderPassType                     passType, 
     const String&                      singlePassBlendedOutputMacro) const override;

    virtual const CPUGeom& cpuGeom() const {
        return m_cpuGeom;
    }
    
    virtual bool canBeFullyRepresentedInGBuffer(const GBuffer::Specification& specification) const override;

    virtual bool requiresBlending() const override;
    
    /** Called by Surface.
     
        Removes the SuperSurfaces from array @a all and appends
        them to the \a super array.
        */
    static void extract(Array<shared_ptr<Surface> >& all, Array<shared_ptr<Surface> >& super);

    /** \param source An object to hold a strong pointer to to prevent it from being
        garbage collected.  This is useful because m_cpuGeom often
        contains pointers into an object that may not be held by
        anything else. Note that any shared_ptr will automatically
        upcast to this type.*/
    static shared_ptr<UniversalSurface> create
    (const String&                          name,
     const CFrame&                          frame, 
     const CFrame&                          previousFrame,
     const shared_ptr<UniversalMaterial>&   material,
     const shared_ptr<GPUGeom>&             gpuGeom,
     const CPUGeom&                         cpuGeom = CPUGeom(),
     const shared_ptr<ReferenceCountedObject>& source = shared_ptr<ReferenceCountedObject>(),
     const ExpressiveLightScatteringProperties& expressiveProperties = ExpressiveLightScatteringProperties(),
     const shared_ptr<Model>&               model = shared_ptr<Model>(),
     const shared_ptr<Entity>&              entity = shared_ptr<Entity>(),
     const shared_ptr<UniformTable>&        uniformTable = shared_ptr<UniformTable>(),
     int                                    numInstances = 1);


    virtual String name() const override;

    virtual bool hasTransmission() const override;

    /** Has transmission that can't be modeled in the opaque pass (since refraction is modeled
        by painting the surface via screen-space refraction by UniversalSurface) */
    virtual bool hasNonRefractiveTransmission() const;

    virtual bool hasRefractiveTransmission() const;

    virtual void getCoordinateFrame(CoordinateFrame& c, bool previous = false) const override;

    virtual void getObjectSpaceBoundingSphere(Sphere&, bool previous = false) const override;

    virtual void getObjectSpaceBoundingBox(AABox&, bool previous = false) const override;  

    virtual void getObjectSpaceGeometry
    (Array<int>&                  index, 
     Array<Point3>&               vertex, 
     Array<Vector3>&              normal, 
     Array<Vector4>&              packedTangent, 
     Array<Point2>&               texCoord, 
     bool                         previous = false) const override;
        
    static void sortFrontToBack(Array<shared_ptr<UniversalSurface> >& a, const Vector3& v);

    virtual void renderIntoSVOHomogeneous(RenderDevice* rd, Array<shared_ptr<Surface> >& surfaceArray, const shared_ptr<SVO>& svo, const CFrame& previousCameraFrame) const override;

    virtual bool canRenderIntoSVO() const override {
        return true;
    }    

    virtual void renderIntoGBufferHomogeneous
    (RenderDevice*                          rd, 
     const Array<shared_ptr<Surface> >&     surfaceArray,
     const shared_ptr<GBuffer>&             gbuffer,
     const CoordinateFrame&                 previousCameraFrame,
     const CoordinateFrame&                 expressivePreviousCameraFrame,
     const shared_ptr<Texture>&             depthPeelTexture,
     const float                            minZSeparation,
     const LightingEnvironment&             lightingEnvironment) const override;

    virtual void renderDepthOnlyHomogeneous
    (RenderDevice*                          rd, 
     const Array<shared_ptr<Surface> >&     surfaceArray,
     const shared_ptr<Texture>&             depthPeelTexture,
     const float                            minZSeparation,
     bool                                   requireBinaryAlpha,
     const Color3&                          transmissionWeight) const override;
    
    virtual void getTrisHomogeneous
    (const Array<shared_ptr<Surface> >&     surfaceArray, 
     CPUVertexArray&                        cpuVertexArray, 
     Array<Tri>&                            triArray,
     bool                                   computePrevPosition = false) const override;

    virtual void renderWireframeHomogeneous
    (RenderDevice*                          rd, 
     const Array<shared_ptr<Surface> >&     surfaceArray, 
     const Color4&                          color,
     bool                                   previous) const override;
};

} // G3D

#endif // G3D_SuperSurface_h
