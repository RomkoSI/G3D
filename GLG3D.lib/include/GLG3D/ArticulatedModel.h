/**
 \file GLG3D/ArticulatedModel.h

 \author Morgan McGuire, http://graphics.cs.williams.edu, Michael Mara, http://illuminationcodified.com
 \created 2011-07-19
 \edited  2016-02-10

  G3D Library http://g3d.codeplex.com
  Copyright 2000-2016, Morgan McGuire morgan@cs.williams.edu
  All rights reserved.
  Use permitted under the BSD license
*/
#ifndef GLG3D_ArticulatedModel_h
#define GLG3D_ArticulatedModel_h

#include "G3D/platform.h"

#define USE_ASSIMP

#include "G3D/ReferenceCount.h"
#include "G3D/Matrix4.h"
#include "G3D/AABox.h"
#include "G3D/Box.h"
#include "G3D/Sphere.h"
#include "G3D/Array.h"
#include "G3D/Table.h"
#include "G3D/constants.h"
#include "G3D/PhysicsFrameSpline.h"
#include "GLG3D/CPUVertexArray.h"
#include "GLG3D/UniversalMaterial.h"
#include "GLG3D/AttributeArray.h"
#include "GLG3D/UniversalSurface.h"
#include "GLG3D/Model.h"
#include "GLG3D/UniformTable.h"
#include "G3D/TextOutput.h"
#include "G3D/ParseOBJ.h"

namespace G3D {

#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable : 4396)
#endif

namespace _internal {
    class AssimpNodesToArticulatedModelParts;
}
class AMIntersector;
/**
 \brief A 3D object composed of multiple rigid triangle meshes connected by joints.

 Supports the following file formats:

 - <a href="http://www.martinreddy.net/gfx/3d/OBJ.spec">OBJ</a> + <a href="http://www.fileformat.info/format/material/">MTL</a>
 - <a href="http://paulbourke.net/dataformats/ply/">PLY</a>
 - <a href="http://g3d.svn.sourceforge.net/viewvc/g3d/data/ifs/fileformat.txt?revision=27&view=markup">IFS</a> 
 - <a href="http://www.geomview.org/docs/html/OFF.html">OFF</a>
 - <a href="http://www.the-labs.com/Blender/3DS-details.html">3DS</a>
 - <a href="http://www.riken.jp/brict/Yoshizawa/Research/PLYformat/PLYformat.html">PLY2</a>
 - Quake 3 <a href="http://www.mralligator.com/q3/">BSP</a>
 - <a href="http://orion.math.iastate.edu/burkardt/data/stl/stl.html">STL</a>
 - Any image format supported by G3D::Image can be loaded as a heightfield mesh; apply a scale factor and material to transform it as desired

 See ArticulatedModel::Specification for a complete description of the
 Any format that can be used with data files for modifying models on
 load.

 If you manually modify an index array or CPUVertexArray within an ArticulatedModel,
 then invoke either ArticulatedModel::clearAttributeArrays() or ArticulatedModel::

 ArticulatedModel::pose() checks each AttributeArray before creating a Surface.
 If any needed AttributeArray is not AttributeArray::valid(), then the interleaved
 AttributeArray for that ArticulatedModel::Geometry and the IndexStream for the
 ArticulatedModel::Mesh%s that reference that geometry are automatically 
 updated from the corresponding CPUVertexArray and CPU index array. 
 */
class ArticulatedModel : public Model {
public:
    friend class _internal::AssimpNodesToArticulatedModelParts;
//    friend shared_ptr<ArticulatedModel> std::make_shared<ArticulatedModel>();
    friend class AMIntersector;

    static void clearCache();

    /** Parameters for cleanGeometry(). Note that HAIR format models are never cleaned on load, as an optimization, because 
        they are always generated cleanly. */
    class CleanGeometrySettings {
    public:
        
        /** 
            Set to true to check for redundant vertices even if no
            normals or tangents need to be computed. This may increase
            rendering performance and decrease cleanGeometry()
            performance.  Default: true.
        */
        bool                        forceVertexMerging;

        /** 
            Set to false to prevent the (slow) operation of merging
            colocated vertices that have identical properties.
            Merging vertices speeds up rendering but slows down
            loading.  Setting to false overrides
            forceVertexMerging. */
        bool                        allowVertexMerging;

        /** Force recomputation of normals, ignoring what is already present */
        bool                        forceComputeNormals;

        /** Force recomputation of tangents, ignoring what is already present */
        bool                        forceComputeTangents;

        /**
           Maximum angle in radians that a normal can be bent through
           to merge two vertices.  Default: 8 degrees().
          */
        float                       maxNormalWeldAngle;

        /** 
            Maximum angle in radians between the normals of adjacent
            faces that will still create the appearance of a smooth
            surface between them.  Alternatively, the minimum angle
            between those normals required to create a sharp crease.

            Set to 0 to force faceting of a model.  Set to 2 * pif()
            to make completely smooth.

            Default: 65 degrees().
        */
        float                       maxSmoothAngle;

        /** 
            Maximum edge length in meters allowed for a triangle. 
            The loader subdivides the triangles until this requirement is met.
            \beta
        */
        float                       maxEdgeLength;

        CleanGeometrySettings() : 
            forceVertexMerging(true),
            allowVertexMerging(true),
            forceComputeNormals(false),
            forceComputeTangents(false),
            maxNormalWeldAngle(8 * units::degrees()),
            maxSmoothAngle(65 * units::degrees()),
            maxEdgeLength(finf()) {
        }

        CleanGeometrySettings(const Any& a);

        bool operator==(const CleanGeometrySettings& other) const {
            return 
                (forceVertexMerging == other.forceVertexMerging) &&
                (allowVertexMerging == other.allowVertexMerging) &&
                (forceComputeNormals == other.forceComputeNormals) &&
                (forceComputeTangents == other.forceComputeTangents) &&
                (maxNormalWeldAngle == other.maxNormalWeldAngle) &&
                (maxSmoothAngle == other.maxSmoothAngle) &&
                (maxEdgeLength == other.maxEdgeLength);
        }

        Any toAny() const;
    };


    /** AUTO is returned as -finf() */
    static float anyToMeshMergeRadius(const Any& a);

    static Any meshMergeRadiusToAny(const float r);

    /** \brief Preprocessing instruction. 
        \sa G3D::Specification::Specification
    */
    class Instruction {
    private:
        friend class ArticulatedModel;

        enum Type {SCALE, MOVE_CENTER_TO_ORIGIN, MOVE_BASE_TO_ORIGIN, SET_CFRAME, TRANSFORM_CFRAME, 
                   TRANSFORM_GEOMETRY, REMOVE_MESH, REMOVE_PART, SET_MATERIAL, SET_TWO_SIDED, 
                   MERGE_ALL, RENAME_PART, RENAME_MESH, ADD, REVERSE_WINDING, 
                   COPY_TEXCOORD0_TO_TEXCOORD1, OFFSET_AND_SCALE_TEXCOORD1, INTERSECT_BOX};

        /**
          An identifier is one of:

          - all(): all parts in a model, or all meshes in a part, depending on context
          - root(): all root parts
          - a string that is the name of a mesh or part at this point in preprocessing
         */
        class Identifier {
        public:
            String             name;

            Identifier() : name("") {}
            Identifier(const String& name) : name(name) {}
            Identifier(const Any& a);

            bool operator==(const Identifier& other) const {
                return name == other.name;
            }

            bool operator!=(const Identifier& other) const {
                return ! (*this == other);
            }

            static Identifier all() {
                return Identifier("all()");
            }

            static Identifier root() {
                return Identifier("root()");
            }

            static Identifier none() {
                return Identifier("none()");
            }

            bool isAll() const {
                return name == "all()";
            }

            bool isRoot() const {
                return name == "root()";
            }

            bool isNone() const {
                return name == "none()";
            }

            Any toAny() const;
        };

        Type                        type;
        Identifier                  part;
        Identifier                  mesh;
        Any                         arg;

        Any                         source;

    public:

        Instruction() : type(SCALE) {}

        Instruction(const Any& a);

        Any toAny() const;

        /** Does not check if sources are the same */
        bool operator==(const Instruction& other) const {
            return 
            ((type == other.type) &&
             (part == other.part) &&
             (mesh == other.mesh) &&
             (arg == other.arg));
        }

        bool operator!=(const Instruction& other) const {
            return ! (*this == other);
        }
    };


    /** \brief Parameters for constructing a new ArticulatedModel from
        a file on disk. 

Example:

<pre>
        ArticulatedModel::Specification {
            filename = "house.obj";
            
            // Can be AUTO, ALL, NONE, or a number
            meshMergeOpaqueClusterRadius = inf();
            meshMergeTransmissiveClusterRadius = 10;

            // true = Don't load any materials, thus speeding up load time
            // significantly if many textures are used.
            //
            // false = Load all materials as specified in the file (default)
            stripMaterials = false;

            // true = Don't load any vertex colors, thus speeding up load time (and eventual render time)
            // significantly if there are vertex colors
            //
            // false = Load all vertex colors as specified in the file (default)
            stripVertexColors = false;

            cleanGeometrySettings = ArticulatedModel::CleanGeometrySettings {
                forceVertexMerging = true;
                allowVertexMerging = true;
                forceComputeNormals = false;
                forceComputeTangents = false;
                maxNormalWeldAngleDegrees = 8;
                maxSmoothAngleDegrees = 65;
            };

            // Apply this uniform scale factor to the geometry and all
            // transformation nodes.  (default = 1.0)
            scale = 0.5;

            // A small programming language for modifying the scene graph during
            // loading.  This can contain zero or more instructions, which will
            // be processed in the order in which they appear.
            preprocess = (
                // Set the reference frame of a part, relative to its parent
                // All parts and meshes may be referred to by name string or ID integer
                // in any instruction.   Use partID = 0 when using a mesh ID.
                setCFrame("fence", CFrame::fromXYZYPRDegrees(0, 13, 0));

                // Scale the entire object, including pivots, by *another* factor of 0.1
                scale(0.1);

                // Add this model as a new root part
                add(ArticulatedModel::Specification {
                   filename = "dog.obj";
                   preprocess = ( renamePart(root(), "dog"); );
                });

                // Add this model as a new part, as a child of the root.
                // This feature is currently reserved and not implemented.
                add(root(), ArticulatedModel::Specification {
                   filename = "cat.obj";
                });

                copyTexCoord0ToTexCoord1("fence");
                offsetAndScaleTexCoord1("fence", offset, scale);

                transformCFrame(root(), CFrame::fromXYZYPRDegrees(0,0,0,90));

                // Remove all vertices and triangles touching them that lie outside
                // of the specified world-space box when in the default pose.
                intersectBox(all(), AABox(Point3(-10, 0, -10), Point3(10, 10, 10)));

                // Transform the root part translations and geometry
                // so that the center of the bounding box in the
                // default pose is at the origin.
                moveCenterToOrigin();

                moveBaseToOrigin();

                reverseWinding("tree");

                // Apply a transformation to the vertices of a geometry, within its reference frame
                transformGeometry("geom", Matrix4::scale(0, 1, 2));

                // Remove a mesh.
                removeMesh("gate");

                // Remove a geometry. This also removes all meshes that use it
                removePart("porch");

                // Replace the material of a Mesh.
                // If the last argument is true (the default), but keep the light maps that are currently on that mesh.
                setMaterial("woodLegs", UniversalMaterial::Specification { lambertian = Color3(0.5); }, true);

                // Change the two-sided flag
                setTwoSided("glass", true);
                
                // Merge all meshes that share materials. The first argument
                // is the opaque merge cluster radius. The second argument is
                // the transmissive/partial coverage merge cluster radius.
                mergeAll(ALL, NONE);

                renamePart("x17", "television");

                renameMesh("foo", "bar");

                renameGeometry("base_geom", "floor");
            );
        }
</pre>
         */
    class Specification {
    public:
        /** Materials will be loaded relative to this file.*/
        String                      filename;

        /** Ignore materials specified in the file, replacing them
            with UniversalMaterial::create().  Setting to true increases
            loading performance and may allow more aggressive
            optimization if mergeMeshesByMaterial is also true.
        */
        bool                        stripMaterials;

        /**
            Ignore vertex colors in the specified file.
        */
        bool                        stripVertexColors;

        bool                        stripLightMaps;

        bool                        stripLightMapCoords;

        /** Default alpha hint <b>for surfaces that have alpha maps at load time</b>. Default is AlphaHint::DETECT, which
            will use AlphaHint::BINARY for binary alpha channels and AlphaHint::BLEND for fractional alpha channels. Switching 
            this value to AlphaHint::BINARY will lead to faster rendering for models with lots of masking (such as trees), 
            at a cost of more aliasing. */
        AlphaHint                   alphaHint;

        /** Default refraction hint for surfaces that have refractive transmission and don't specify a value. 
            Default is RefractionHint::DYNAMIC_FLAT.
         */
        RefractionHint           refractionHint;

        /**
         Radius for clusters of meshes [that have the same material] that can be merged to reduce draw calls.

         <ul>
          <li>ALL = inf = merge all [default]
          <li>positive, finite = merge if the combined mesh's bounding box will have an inscribed sphere of this radius
          <li>NONE = 0 = merge no meshes
          <li>AUTO = -inf = choose a finite radius based on the bounding box of the entire model [not currently implemented]
         </ul>

         The radius is applied to <b>part-space geometry</b> bounds
         */
        float                       meshMergeOpaqueClusterRadius;

        /** The default value is 0.0. \sa meshMergeOpaqueClusterRadius */
        float                       meshMergeTransmissiveClusterRadius;

        /** Multiply all vertex positions and part translations by
            this factor after loading and before preprocessing.
            Default = 1.0. */
        float                       scale;

        CleanGeometrySettings       cleanGeometrySettings;

        /** A program to execute to preprocess the mesh before
            cleaning geometry. */
        Array<Instruction>          preprocess;

        /** If false, this articulated model may not be loaded from or stored in the global articulated model cache. Default: true*/
        bool                        cachable;
        ParseOBJ::Options           objOptions;


        class HeightfieldOptions {
        public:
			/** For texture coordinate generation. Set ArticulatedModel::Specification::scale to scale the model */
            Vector2         textureScale;
            bool            generateBackfaces;

            HeightfieldOptions() : textureScale(1,1), generateBackfaces(false) {}
            HeightfieldOptions(const Any& a);
            Any toAny() const;
            bool operator==(const HeightfieldOptions& other) const {
                return (textureScale == other.textureScale) && (generateBackfaces == other.generateBackfaces);
            }
        } heightfieldOptions;

        class HairOptions {
        public:
            /** How tesselated to make the cylinders approximating the hair strands. 6 is a hexagonal prism. */
            int sideCount;

            float strandRadiusMultiplier;

            /** If true, will make an independent surface for each strand. This will improve the quality of 
                sorted transparency at a potentially catastrophic performance penalty. */
            bool separateSurfacePerStrand;

            HairOptions() : sideCount(5), strandRadiusMultiplier(1.0f), separateSurfacePerStrand(false) {}
            HairOptions(const Any& a);
            Any toAny() const;
            bool operator==(const HairOptions& other) const {
                return (sideCount == other.sideCount) && (strandRadiusMultiplier == other.strandRadiusMultiplier) && (separateSurfacePerStrand == other.separateSurfacePerStrand);
            }

            size_t hashCode() const {
                return sideCount + int(separateSurfacePerStrand) + int(strandRadiusMultiplier * 10.0f);
            }
        } hairOptions;


        class ColladaOptions {
        public:
            /** When loading a transmissive material,
                G3D has the convention that black signifies fully transmissive, but for 
                some Collada models this convention is reversed. These options allow Collada models
                to be loaded with both conventions. The default value is MINIMIZE_TRANSMISSIVES, where 
                proper convention will be inferred while loading the model, 
            */
            G3D_DECLARE_ENUM_CLASS(TransmissiveOption,
                
                /** Load the model using G3D convention. Black is fully transmissive */
                NORMAL,
                
                /** Load the model using the inverse of the G3D convention. White is fully transmissive */
                INVERTED,
                
                /**
                    The convention that minimizes the number of fully transmissive materials will be
                    automatically chosen. This almost always will produce the desired result, but will
                    fail in edge cases (such as a scene made entirely of glass).
                */
                MINIMIZE_TRANSMISSIVES,
                
                /**
                    The exact inverse of MINIMIZE_TRANSMISSIVES.
                */
                MAXIMIZE_TRANSMISSIVES
            );

            TransmissiveOption transmissiveChoice;

            ColladaOptions() : transmissiveChoice(TransmissiveOption::MINIMIZE_TRANSMISSIVES) {}
            ColladaOptions(const Any& a);
            Any toAny() const;
            bool operator == (const ColladaOptions& other) const {
                return transmissiveChoice == other.transmissiveChoice;
            }
        } colladaOptions;


        Specification() : 
            stripMaterials(false), 
            stripVertexColors(false),
            stripLightMaps(false), 
            stripLightMapCoords(false),
            alphaHint(AlphaHint::DETECT),
            refractionHint(RefractionHint::DYNAMIC_FLAT),
            meshMergeOpaqueClusterRadius(finf()),
            meshMergeTransmissiveClusterRadius(0.0f),
            scale(1.0f), 
            cachable(true) {}

        Specification(const Any& a);

        size_t hashCode() const;

        bool operator==(const Specification& other) const;

        Any toAny() const;
    };

    class Part;
    class Mesh;

    /** Vertex information without an index array connecting them into triangles. \sa Mesh*/
    class Geometry {
    public:
        friend class ArticulatedModel;

        /** Used by cleanGeometry */
        class Face {
        public:
            /** Needed to extend CPUVertexArray::Vertex with texCoord1 and bone values. */
            class Vertex : public CPUVertexArray::Vertex {
                public:
                Point2unorm16   texCoord1;
                Color4          vertexColor;
                Vector4int32    boneIndices;
                Vector4         boneWeights;

                /** Index in the containing Face's Geometry's cpuVertexArray */
                int             indexInSourceGeometry;
                Vertex() : indexInSourceGeometry(-1) {}
                Vertex(const CPUVertexArray::Vertex& v, int i) : indexInSourceGeometry(i) {
                    normal      = v.normal;
                    position    = v.position;
                    tangent     = v.tangent;
                    texCoord0   = v.texCoord0;
                }
            };

            /** Tracks if position and texcoords match, but ignores normals and tangents */
            struct AMFaceVertexHash { 
                static size_t hashCode(const Vertex& vertex) {
                    // Likelihood of two vertices being identical except in bone properties is low, so don't bother using bones in hash        
                    return vertex.position.hashCode() ^ vertex.texCoord0.hashCode() ^ vertex.texCoord1.hashCode();
                }
                static bool equals(const Vertex& a, const Vertex& b) {
                    return  (a.position    == b.position)      && 
                            (a.texCoord0   == b.texCoord0)     && 
                            (a.texCoord1   == b.texCoord1)     && 
                            (a.vertexColor == b.vertexColor)   && 
                            (a.boneWeights == b.boneWeights)   && 
                            (a.boneIndices == b.boneIndices);
                }
            };

            /** Index of a Face in a temporary array*/
            typedef int                         Index;
            typedef SmallArray<Index, 7>        IndexArray;
            typedef Table<Point3, IndexArray>   AdjacentFaceTable;

            Vertex                              vertex[3];

            /** Mesh from which this face was originally created. Needed for reconstructing
              the index arrays after vertices are merged.*/
            Mesh*                               mesh;
            
            /** Non-unit face normal, used for weighted vertex normal computation */
            Vector3                             normal;
            Vector3                             unitNormal;

            Face(Mesh* m, const Vertex& v0, const Vertex& v1, const Vertex& v2) : mesh(m) {
                vertex[0] = v0; 
                vertex[1] = v1;
                vertex[2] = v2;
                normal = 
                (vertex[1].position - vertex[0].position).cross(
                    vertex[2].position - vertex[0].position);

                unitNormal = normal.directionOrZero();
            }

            Face() : mesh(NULL) {}
        };

        /** 
         Cleans the geometric data in response to changes, or after load.

        - Wipes out the GPU vertex attribute data.
        - Computes a vertex normal for every element whose normal.x is fnan() (or if the normal array is empty).
        - If there are texture coordinates, computes a tangent for every element whose tangent.x is fnan() (or if the tangent array is empty).
        - Merges all vertices with identical indices.
        - Updates all Mesh indices accordingly.
        - Recomputes the bounding sphere and box.
        
        Does not upload to the GPU. 

        Note that this invokes clearVertexRanges() and computeBounds().
        */
        void cleanGeometry(const CleanGeometrySettings& settings, const Array<Mesh*>& meshes);

        /** Subdivides all triangles using an ad-hoc algorithm to be described until each edge of every triangle is less than edgeLengthThreshold. Then merges vertices */
        void subdivideUntilThresholdEdgeLength(const Array<Mesh*>& affectedMeshes, const float edgeLengthThreshold = 0.1f, const float positionEpsilon = 0.001f, const float normalAngleEpsilon= 0.01f, const float texCoordEpsilon = 0.01f);

        void buildFaceArray(Array<Face>& faceArray, Face::AdjacentFaceTable& adjacentFaceTable, const Array<Mesh*>& meshes);

        void computeMissingVertexNormals(   
            Array<Face>&                      faceArray, 
            const Face::AdjacentFaceTable&    adjacentFaceTable, 
            const float                       maximumSmoothAngle);

        void computeMissingTangents(const Array<Mesh*> affectedMeshes);

        void mergeVertices(const Array<Face>& faceArray, float maxNormalWeldAngle, const Array<Mesh*> affectedMeshes);

        void getAffectedMeshes(const Array<Mesh*>& fullMeshArray, Array<Mesh*>& affectedMeshes);

        String                      name;

        /** \brief The CPU-side geometry.
            If you modify cpuVertexArray, invoke clearAttributeArrays() to force the GPU arrays 
            to update on the next ArticulatedMode::pose() */
        CPUVertexArray              cpuVertexArray;
        
        AttributeArray              gpuPositionArray;
        AttributeArray              gpuNormalArray;
        AttributeArray              gpuTexCoord0Array;
        AttributeArray              gpuTangentArray;
        AttributeArray              gpuTexCoord1Array;
        AttributeArray              gpuVertexColorArray;
        AttributeArray              gpuBoneIndicesArray;
        AttributeArray              gpuBoneWeightsArray;

        Sphere                      sphereBounds;

        AABox                       boxBounds;

        /** If you modify cpuVertexArray, invoke this method to force the GPU arrays to update on the next ArticulatedMode::pose() */
        void clearAttributeArrays();

        void determineCleaningNeeds(bool& computeSomeNormals, bool& computeSomeTangents);

        void computeBounds(const Array<Mesh*>& affectedMeshes);

    private:

        Geometry(const String& name) : name(name) {}

        void copyToGPU(ArticulatedModel* model);
    };


    /** 
        \brief A set of primitives (e.g., triangles) that share a material.
     */
    class Mesh {
    public:

        String                                  name;

        /** The containing logical part in the ArticulatedModel, NULL if not contained in a part.
            This will not be a bone. Used only for preprocessing */
        Part*                                   logicalPart;

        /** Written by copyToGPU */
        shared_ptr<UniversalSurface::GPUGeom>   gpuGeom;

        friend class ArticulatedModel;

        /** Joints that affect this mesh. If more than one, then use bone animation. */
        Array<Part*>                            contributingJoints;

        shared_ptr<UniversalMaterial>           material;

        /** The geometry used by this mesh. NULL if no geometry specified */
        Geometry*                               geometry;

        PrimitiveType                           primitive;
        
        /** If you modify cpuIndexArray, invoke clearIndexStream() to force gpuIndexArray to update on the next ArticulatedMode::pose() */
        Array<int>                              cpuIndexArray;

        /** \copydoc cpuIndexArray. Written by ArticulatedModel::Mesh::copyToGPU */
        IndexStream                             gpuIndexArray;

        bool                                    twoSided;

        /** Object Space */
        Sphere                                  sphereBounds;

        /** Object Space */
        AABox                                   boxBounds;

        /** Same as the containing model's */
        shared_ptr<Texture>                     boneTexture;   
        shared_ptr<Texture>                     prevBoneTexture;

        int                                     uniqueID;

        int triangleCount() const {
            alwaysAssertM(primitive == PrimitiveType::TRIANGLES, 
                    "Only implemented for PrimitiveType::TRIANGLES");
            return cpuIndexArray.size() / 3;
        }

        /** If you modify cpuIndexArray, invoke this method to force the GPU arrays to update on the next ArticulatedMode::pose() */
        void clearIndexStream();

    private:
        
        Mesh(const String& n, Part* p, Geometry* geom, int ID) : name(n), logicalPart(p), geometry(geom), primitive(PrimitiveType::TRIANGLES), twoSided(false), uniqueID(ID) {
            contributingJoints.append(p);
        }

        /** Copies the cpuIndexArray to gpuIndexArray. 
        
         \param indexBuffer If not NULL, append indices to this buffer.*/
        void copyToGPU(const shared_ptr<VertexBuffer>& indexBuffer = shared_ptr<VertexBuffer>());

        /** Called by copyToGPU and Geometry::copyToGPU */
        void updateGPUGeom();
    };


    /** Specifies the transformation that occurs at each node in the heirarchy. 
     */
    class Pose : public Model::Pose {
    private:
        static const PhysicsFrame identity;
    public:
        /** Mapping from part names to physics frames (relative to parent).
            If a name is not present, then its coordinate frame is assumed to
            be the identity. */
        Table<String, PhysicsFrame>                    frameTable;

        /** If material[meshName] exists and is notNull, then that material
            overrides the one specified in the model in this pose.  Allows the same model
            to be used with different materials when instancing.

            To find mesh names:
            - load a model into the G3D Viewer, click on the part and press F3,
            - or using SceneEditorWindow, unlock the scene, select the Entity, and open the info pane.
        */
        Table<String, shared_ptr<UniversalMaterial> >  materialTable;

        /** Additional uniform arguments passed to the Surface%s,
            useful for prototyping effects that need additional
            per-Entity state. */
        shared_ptr<UniformTable>                       uniformTable;

        /** For instanced rendering of a single model. Used as Args::numInstances */
        int                                            numInstances;

        Pose() : numInstances(1) {}

        /**
         Example:

        \code
          ArticulatedModel::Pose {
              numInstances = 10;
              frameTable = {
                  "part" = Point3(0, 10, 0);
              };
              uniformTable = {
                   FOO = "macro value";
                   count = 3;
              };
              materialTable = {
                   "mesh" = UniversalMaterial::Specification {
                       lambertian = Color3(1, 0, 0);
                   };
              };
          }
        \endcode

        For convenience when initializing a VisibleEntity from an .Any file,
        a single UniversalMaterial::Specification Any will also cast to an entire
        ArticulatedModel::Pose, where the materialTable key is "mesh"
        */
        explicit Pose(const Any& a);

        /** Returns the identity coordinate frame if there isn't one bound for partName */
        inline const PhysicsFrame& frame(const String& partName) const {
            if (frameTable.size() == 0) {
                // In the common case, there is nothing in cframe, so don't bother
                // even hashing the string.
                return identity;
            }

            const PhysicsFrame* ptr = frameTable.getPointer(partName);
            if (ptr != NULL) {
                return *ptr;
            } else {
                return identity;
            }
        }

        static void interpolate(const Pose& pose1, const Pose& pose2, float alpha, Pose& result);

    };

    
    class PoseSpline {
    public:
        typedef Table<String, PhysicsFrameSpline> SplineTable;
        SplineTable     partSpline;

        PoseSpline();

        /**
         The Any must be a table mapping part names to PhysicsFrameSpline%s.
         Note that a single PhysicsFrame (or any equivalent of it) can serve as
         to create a PhysicsFrameSpline.  

         Format example:
         \code
         ArticulatedModel::PoseSpline {
            "part1" = PhysicsFrameSpline {
                control = ( Vector3(0,0,0),
                            CFrame::fromXYZYPRDegrees(0,1,0,35)),
                cyclic = true
            };

            "part2" = Vector3(0,1,0);
            }
         \endcode
        */
        PoseSpline(const Any& any);
     
        /** Get the pose.cframeTable at time t, overriding values in \a pose that
            are specified by the spline table. */
        void get(float t, ArticulatedModel::Pose& pose);
    };


    /** A keyframe-based animation */
    class Animation {
    public:
        /** Duration of a single keyframe */
        SimTime duration;
        PoseSpline poseSpline;
        /** Returns the interpolated pose */
        void getCurrentPose(SimTime time, Pose& pose);
    };

    
    /** 
     \brief A set of meshes with a single reference frame, packed into
     a common vertex buffer.
    */
    class Part {
    private:
        friend class ArticulatedModel;
        friend class _internal::AssimpNodesToArticulatedModelParts;

    public:

        String                      name;

        int                         uniqueID;

    private:

        Part*                       m_parent;
        Array<Part*>                m_children;

    public:

        /** Transformation from this object to the parent's frame in
            the rest pose. Also known as the "pivot". */
        CoordinateFrame             cframe;

        CoordinateFrame             inverseBindPoseTransform;

    private:

        Part(const String& name, Part* parent, int ID) : name(name), uniqueID(ID), m_parent(parent) {}

    public:

        ~Part();

        /** NULL if this is a root of the model. */
        Part* parent() const {
            return m_parent;
        }

        const Array<Part*>& childArray() const {
            return m_children;
        }

        bool isRoot() const {
            return m_parent == NULL;
        }

        void transformGeometry(shared_ptr<ArticulatedModel> am, const Matrix4& xform);

        void intersectBox(shared_ptr<ArticulatedModel> am, const Box& box);

#ifndef DISABLE_cleanGeometry
        /** debugPrintf all of the geometry for this part. */
        void debugPrint() const;
#endif
    };


    /** Base class for defining operations to perform on each part, in hierarchy order.
    
    Example:
    \code

    class ExtractVertexCallback : public ArticulatedModel::PartCallback {
    public:
        Array<Point3>& vertexArray;

        ExtractVertexCallback(Array<Point3>& vertexArray) : vertexArray(vertexArray) {}

        void operator()(ArticulatedModel::Part *part, const CFrame &worldToPartFrame, shared_ptr<ArticulatedModel> model, const int treeDepth) {
            for (int i = 0; i < part->cpuVertexArray.size(); ++i) {
                vertexArray.append(worldToPartFrame.pointToObjectSpace(part->cpuVertexArray.vertex[i].position));
            }
        }
    } callback(vertexArray);

    model->forEachPart(callback);
    \endcode
    */
    class PartCallback {
    public:
        virtual ~PartCallback() {}

        /** \brief Override to implement processing of \a part. 
            
            \param worldToPartFrame The net transformation in this
            pose from world space to \a part's object space

            \param treeDepth depth in the hierarchy.  0 = a root
        */
        virtual void operator()(ArticulatedModel::Part* part, const CFrame& worldToPartFrame, 
                                shared_ptr<ArticulatedModel> model, const int treeDepth) = 0;
    };


    /** Computes the world-space bounds of this model. */
    class BoundsCallback : public PartCallback {
    public:
        
        AABox bounds;
        
        virtual void operator()(ArticulatedModel::Part* part, const CFrame& worldToPartFrame, 
                                shared_ptr<ArticulatedModel> m, const int treeDepth) override;
    };


    /** Merges meshes within the part based on their Materials. Does not update bounds or GPU VertexRanges.*/
    class MeshMergeCallback : public PartCallback {
    public:
        
        float opaqueRadius;
        float transmissiveRadius;
        MeshMergeCallback(float r, float t) : opaqueRadius(r), transmissiveRadius(t) {}
        
        virtual void operator()(ArticulatedModel::Part* part, const CFrame& worldToPartFrame, 
                                shared_ptr<ArticulatedModel> m, const int treeDepth) override;
    };

    /** Rescales each part (and the position of its cframe) by a constant factor. */
    class ScalePartTransformCallback : public PartCallback {
        float scaleFactor;
        
    public:
        ScalePartTransformCallback(float s) : scaleFactor(s) {}
        
        virtual void operator()(ArticulatedModel::Part* part, const CFrame& partFrame,
                                shared_ptr<ArticulatedModel> m, const int treeDepth) override;
    };

    /** \see forEachMesh */
    class MeshCallback {
    public:
        virtual ~MeshCallback() {}

        /** \brief Override to implement processing of \a mesh.
           
            The callback may not remove parts.  It may remove the mesh that it is operating on, but not other meshes.
            The callback may add new parts or meshes, but the callback will not be invoked on those parts or meshes.
        */
        virtual void operator()
           (shared_ptr<ArticulatedModel> model,
            ArticulatedModel::Mesh* mesh) = 0;
    };

    class RemoveMeshCallback : public MeshCallback {
    public:
        virtual void operator()
            (shared_ptr<ArticulatedModel> model,
            ArticulatedModel::Mesh* mesh) override;
    };

    class ReverseWindingCallback : public MeshCallback {
    public:
        virtual void operator()
            (shared_ptr<ArticulatedModel> model,
            ArticulatedModel::Mesh* mesh) override;
    };

    class SetTwoSidedCallback : public MeshCallback {
    public:
        bool twoSided;
        SetTwoSidedCallback(bool s) : twoSided(s) {}
        virtual void operator()
            (shared_ptr<ArticulatedModel> model,
            ArticulatedModel::Mesh* mesh) override;
    };

    /** \see forEachGeometry */
    class GeometryCallback {
    public:
        virtual ~GeometryCallback() {}

        /** \brief Override to implement processing of \a geometry.
        */
        virtual void operator()
           (shared_ptr<ArticulatedModel> model,
            ArticulatedModel::Geometry* geom) = 0;
    };

    /** Rescales each geometry by a constant factor. */
    class ScaleGeometryTransformCallback : public GeometryCallback {
        float scaleFactor;
        
    public:
        ScaleGeometryTransformCallback(float s) : scaleFactor(s) {}
        
        virtual void operator()(shared_ptr<ArticulatedModel> model,
            ArticulatedModel::Geometry* geom) override;
    };
    

    /** The rest pose.*/
    static const Pose& defaultPose();


protected:       
    
    String                     m_name;
    /* m_nextID is the ID of the next part or mesh to be added. Each mesh or part
       when added is assigned a unique int id. Inorder to make sure that it is unique
       every time a mesh or part is added it is given m_nextID value and m_nextID is 
       incremented by one. */
    int                             m_nextID;

    Array<Part*>                    m_rootArray;
    Array<Part*>                    m_partArray;
    Array<Part*>                    m_boneArray;
    Array<Geometry*>                m_geometryArray;
    Array<Mesh*>                    m_meshArray;
    Table<String, Animation>        m_animationTable;

    shared_ptr<Texture>             m_gpuBoneTransformations;
    shared_ptr<Texture>             m_gpuBonePrevTransformations;

    shared_ptr<Pose>                m_lastPose;

    Table<Part*, CFrame>            m_partTransformTable;
    Table<Part*, CFrame>            m_prevPartTransformTable;

    /**keeps track of the MTL files loaded from an OBJ
       only noneempty when loaded from an OBJ */
    Array<String>              m_mtlArray;

    int getID() {
        ++m_nextID;
        return m_nextID - 1;
    }

    void scaleAnimations(float scaleFactor);

    static shared_ptr<ArticulatedModel> loadArticulatedModel(const Specification& specification, const String& n);

    
    /** \brief Execute the program.  Called from load() */
    void preprocess(const Array<Instruction>& program);

    /** \brief Executes \a c for each part in the hierarchy.
     */
    void forEachPart(PartCallback& c, Part* part, const CFrame& parentFrame, const Pose& pose, const int treeDepth);

    /** Invoked by preprocess instructions to apply the callback to each mesh matching the specified identifiers. 
        \param source For error reporting*/
    void forEachMesh(Instruction::Identifier meshId, MeshCallback& c, const Any& source = Any());

    /** Invoked by preprocess instructions to apply the callback to each geometry matching the specified identifiers. 
        \param source For error reporting*/
    void forEachGeometry(Instruction::Identifier geomId, GeometryCallback& c, const Any& source = Any());


    /** After load, undefined normals have value NaN.  Undefined texcoords become (0,0).
        There are no tangents, the gpu arrays are empty, and the bounding spheres are
        undefined.*/
    void loadOBJ(const Specification& specification);


    void loadIFS(const Specification& specification);

    void loadPLY2(const Specification& specification);

    void loadOFF(const Specification& specification);

    void loadPLY(const Specification& specification);

    void load3DS(const Specification& specification);

    void loadBSP(const Specification& specification);

    void loadSTL(const Specification& specification);

    void loadHeightfield(const Specification& specification);

    /** The HAIR model format http://www.cemyuksel.com/research/hairmodels/ */
    void loadHAIR(const Specification& specification);

#   ifdef USE_ASSIMP
    void loadASSIMP(const Specification& specification);
#   endif

    void load(const Specification& specification);

    ArticulatedModel() : m_nextID(1) {}

    Mesh* mesh(const Instruction::Identifier& mesh);

    /** Appends all meshes specified by identifier to \param identifiedMeshes */
    void getIdentifiedMeshes(const Instruction::Identifier& identifier, Array<Mesh*>& identifiedMeshes);

    Part* part(const Instruction::Identifier& partIdent);

    Geometry* geometry(const Instruction::Identifier& geomIdent);

    /** Appends all geometry specified by identifier to \param identifiedGeometry */
    void getIdentifiedGeometry(const Instruction::Identifier& identifier, Array<Geometry*>& identifiedGeometry);

    /** Called from preprocess.  

        \param centerY If false, move the base to the origin instead
        of the center in the vertical direction. */
    void moveToOrigin(bool centerY);

    /** Called from preprocess */
    void setMaterial
       (Instruction::Identifier                 meshId,
        const UniversalMaterial::Specification& spec, 
        const bool                              keepLightMaps,
        const Any&                              source);

public:

    /** Call this if you change the underlying CPU data and have not manually invoked
        the corresponding clear calls on the exact meshes and geometry affected.
        Invokes Mesh::clearIndexStream and Geometry::clearAttributeArrays on all meshes and geometry.
      */
    void clearGPUArrays();
 
    /** Saves an OBJ with the given filename of this ArticulatedModel 
        materials currently only work if loaded from an OBJ*/
    void saveOBJ(const String& filename);

    /** Pairs of points representing each bone in this mode in the given pose are appended to skeleton */
    void getSkeletonLines(const Pose& pose, const CFrame& cframe, Array<Point3>& skeleton);

    /** Uses last pose */
    void getSkeletonLines(const CFrame& cframe, Array<Point3>& skeleton) {
        if (notNull(m_lastPose)) {
            getSkeletonLines(*(m_lastPose.get()), cframe, skeleton);
        }
    }

    void getAnimationNames(Array<String>& animationNames) {
        if(m_animationTable.size() > 0) {
            animationNames.append(m_animationTable.getKeys());
        }
    }

    void getAnimation(const String& name, Animation& animation) {
        m_animationTable.get(name, animation);
    }

    bool usesSkeletalAnimation() const { 
        return m_boneArray.size() > 0;
    }

    
    bool usesAnimation() const {
        return m_animationTable.size() != 0;
    }

    virtual const String& name() const override {
        return m_name;
    }
    
    /** Update the bounds on all meshes (without cleaning them) */
    void computeBounds();
    
    ~ArticulatedModel();

    /** Leaves empty filenames alone and resolves others */
    static String resolveRelativeFilename(const String& filename, const String& basePath);

    /** \sa createEmpty, fromFile 
      If the \a name is not the empty string, sets the name.
    */
    static shared_ptr<ArticulatedModel> create(const Specification& s, const String& name = "");

    /** \copydoc create */
    static lazy_ptr<Model> lazyCreate(const Specification& s, const String& name = "");

    static shared_ptr<ArticulatedModel> fromFile(const String& filename) {
        Specification s;
        s.filename = filename;
        return create(s, filename);
    }

    /** If this model's memory footprint is large, trim all of the internal CPU arrays to size. */
    void maybeCompactArrays();

    /** 
        \brief Creates an empty ArticulatedModel that you can then programmatically construct Part%s and Mesh%es within.

        Consider calling cleanGeometry() and maybeCompactArrays()
        after setting geometry during a preprocessing step.  If
        modifying geometry <i>after</i> the first call to pose(), invoke
        Part::clearVertexRanges() to wipe the out of date GPU data.


        Example of a procedurally generated model (run on load; too slow to execute in an animation loop):

        \code
        shared_ptr<ArticulatedModel>    model = ArticulatedModel::createEmpty("spiral");
        ArticulatedModel::Part*     part      = model->addPart("root");
        ArticulatedModel::Geometry* geometry  = model->addGeometry("geom");
        ArticulatedModel::Mesh*     mesh      = model->addMesh("mesh", part, geometry);

        // Create the vertices
        for (int i = 0; i < 100; ++i) {
            CPUVertexArray::Vertex& v = geometry->cpuVertexArray.vertex.next();
            v.position = Point3( ... );
        }
        
        // Create the indices
        for (int i = 0; i < 50; ++i) {
            mesh->cpuIndexArray.append( ... );
        }

        // Tell Articulated model to generate normals automatically
        model->cleanGeometry();
        \endcode


        Example of updating geometry on the CPU for vertex animation
        (consider using a custom Shader to perform this work on the
        GPU if possible):

        \code
        Array<CPUVertexArray::Vertex>& vertex = model->part(partID);
        for (int i = 0; i < 50; ++i) {
            vertex[i].position = ... ;
            vertex[i].normal   = ... ;
        }
        part->clearVertexArrays();
        \endcode

        Note that you can obtain the partID when a part is originally
        created, by explicitly iterating through the hierarchy from 
        rootArray(), or by iterating with forEachPart().


        Consider creating a UniversalSurface, your own Surface subclass,
        or using VertexBuffer directly if you need extensive dynamic
        geometry that doesn't fit the design of ArticulatedModel
        well.  G3D does not require the use of ArticulatedModel at all--
        it is just a helper class to jumpstart your projects.
        
        \sa create, fromFile, addMesh, addPart
       */
    static shared_ptr<ArticulatedModel> createEmpty(const String& name);

    /** Root parts.  There may be more than one. */
    const Array<Part*>& rootArray() const {
        return m_rootArray;
    }

    const Array<Mesh*>& meshArray() const {
        return m_meshArray;
    }

    const Array<Geometry*>& geometryArray() const {
      return m_geometryArray;
    }
    
    /** Get a Mesh by name.  Returns NULL if there is no such mesh. 
        will not neccesarily return the correct mesh if two meshes 
        have the same name */
    Mesh* mesh(const String& meshName);

    Mesh* mesh(int ID);

    /** Get a Part by name.  Returns NULL if there is no such part. */
    Part* part(const String& partName);

    /** Get a Geometry by name.  Returns NULL if there is no such geometry. */
    Geometry* geometry(const String& partName);

    /** \param part is set as both the logical part and the only part in the contributingJoint array.

        \sa addPart, addGeometry, createEmpty. */
    Mesh* addMesh(const String& name, Part* part, Geometry* geom);

    /** \sa addMesh, addGeometry, createEmpty */
    Part* addPart(const String& name, Part* parent = NULL);

    /** \sa addPart, addMesh, createEmpty */
    Geometry* addGeometry(const String& name);

    /** Walks the hierarchy and invokes PartCallback \a c on each Part,
        where each model is in \a pose and the entire model is relative to
        \a cframe.

        Remember to call cleanGeometry() if you change the geometry to force 
        it to re-upload to the GPU.

        Remember to set any normals and tangents you want recomputed to NaN.
    */
    void forEachPart(PartCallback& c, const CoordinateFrame& cframe = CoordinateFrame(), const Pose& pose = defaultPose());
    
    /** Applies the callback to all meshes in the model */
    void forEachMesh(MeshCallback& c, const Any& source = Any()) {
        forEachMesh(Instruction::Identifier::all(), c, source);
    }

    /** Applies the callback to all meshes in the model */
    void forEachGeometry(GeometryCallback& c, const Any& source = Any()) {
        forEachGeometry(Instruction::Identifier::all(), c, source);
    }

    /** Scales all the underlying geometry for the whole model, and all of the part transforms (including animations if they exist */
    void scaleWholeModel(float scaleFactor);

    /** 
      Invokes Part::cleanGeometry on all meshes.       
     */
    void cleanGeometry(const CleanGeometrySettings& settings = CleanGeometrySettings());

    /** Appends one posed model per sub-part with geometry.

        Poses an object with no motion (see the other overloaded
        version for expressing motion)
    */
    void pose
    (Array<shared_ptr<Surface> >&       surfaceArray,
     const CoordinateFrame&             cframe = CoordinateFrame(),
     const Pose&                        pose   = defaultPose(),
     const shared_ptr<class Entity>&    entity = shared_ptr<Entity>());

    void pose
    (Array<shared_ptr<Surface> >&       surfaceArray,
     const CoordinateFrame&             cframe,
     const Pose&                        pose,
     const CoordinateFrame&             prevCFrame,
     const Pose&                        prevPose,
     const shared_ptr<class Entity>&    entity = shared_ptr<Entity>());    

    /** Fills partTransforms with full joint-to-world transforms */
    void computePartTransforms(
        Table<Part*, CFrame>&           partTransforms,
        Table<Part*, CFrame>&           prevPartTransforms,
        const CoordinateFrame&          cframe, 
        const Pose&                     pose, 
        const CoordinateFrame&          prevCFrame,
        const Pose&                     prevPose);

    /**
      \brief Per-triangle ray-model intersection.

       Returns true if ray \a R intersects this model, when it has \a
       cframe and \a pose, at a distance less than \a maxDistance.  If
       so, sets maxDistance to the intersection distance and sets the
       pointers to the Part and Mesh, and the index in Mesh::cpuIndexArray of the 
       start of that triangle's indices.  \a u and \a v are the
       barycentric coordinates of vertices triStartIndex and triStartIndex + 1,
       The barycentric coordinate of vertex <code>triStartIndex + 2</code>
       is <code>1 - u - v</code>.

       This is primarily intended for mouse selection.  For ray tracing
       or physics, consider G3D::TriTree instead.

       Does not overwrite the arguments unless there is a hit closer than maxDistance.
     */
     // Not const because it returns non-const pointers to members
    bool intersect
       (const Ray&     R, 
        const CFrame&   cframe, 
        const Pose&     pose, 
        float&          maxDistance, 
        Model::HitInfo& info = Model::HitInfo::ignore,
        const shared_ptr<Entity>& entity = shared_ptr<Entity>());

    void countTrianglesAndVertices(int& tri, int& vert) const;

    /** Finds the bounding box of this articulated model */
    void getBoundingBox(AABox& box);

    virtual const String& className() const override;

};

}  // namespace G3D



template <> struct HashTrait<G3D::ArticulatedModel::Specification> {
    static size_t hashCode(const G3D::ArticulatedModel::Specification& key) { return key.hashCode(); }
};


#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // GLG3D_ArticulatedModel_h
