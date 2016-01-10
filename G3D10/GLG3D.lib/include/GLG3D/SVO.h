#ifndef GLG3D_SVO_h
#define GLG3D_SVO_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "G3D/ImageFormat.h"
#include "G3D/Box.h"
#include "GLG3D/GBuffer.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/BufferTexture.h"
#include "GLG3D/Args.h"
#include "GLG3D/Profiler.h"

#define SVO_USE_CUDA							0 

#define SVO_OPTIMIZE_CACHING_BLOCKS				0		//Experimental

#define SVO_FORCE_NO_BRICK						0 

#define SVO_USE_TOP_DENSE						0		//Dense tree at the root.
#define SVO_USE_TOP_MIPMAP						0		//Use full mipmap as top octree head. Not working currently.
#define SVO_TOP_MIPMAP_NUM_LEVELS				6		//6
#define SVO_TOP_MIPMAP_SPARSE_TREE				0	


#define SVO_USE_NEIGHBOR_POINTERS				0
#define SVO_NUM_NEIGHBOR_POINTERS				4	//Round to immediately highest multiple of 4	(3->4)

#define SVO_MAX_NUM_LEVELS						16	

#define SVO_MAX_NUM_VOLUMES						2	 //Max number of volumes // Was 128


#define SVO_ACCUMULATE_VOXEL_FRAGMENTS			1 


//#define SVO_TEST_FBO_NOATTACHMENT				//Not supported anymore with latest NV drivers. Why ?

typedef unsigned int uint;


namespace G3D {

class Camera;

/** 
  Sparse Voxel Octree: (compressed) 3D Analog of a GBuffer.

  The algorithm operates in three passes:

   1. prepare (initialize data structures)
   2. render voxel fragments (Surface::renderIntoSVO)
   3. complete (build the tree)

   The octree always fills a cube (it is sparse, so very little space is lost if the aspect ratio is more extreme)
*/
class SVO : public ReferenceCountedObject {
public:

    typedef GBuffer::Field    Field;

    /** CS_POSITION is used for octree-space position, so */
    typedef GBuffer::Specification Specification;

	//TEMP//
	float	projectionScale;
	Vector2	projectionOffset;

	size_t		m_levelsNumNodes[SVO_MAX_NUM_LEVELS+1];

protected:
	const String                    m_name;

	bool							m_initOK;

    /** We pack all of the material data into 2D textures, which are really 
        just 1D wrapped at this width. */
    int                             BUFFER_WIDTH;
    
	uint							m_octreePoolNumNodes;		

	bool							m_useBricks;
	int								m_brickNumLevels;
	Vector3int16					m_brickRes;
	int								m_brickBorderSize;
	Vector3int16					m_brickResWithBorder;
	int								m_octreeBias;	//Bias due to brick resolution

	bool							m_useNeighborPointers;	//New

	int								m_numSurfaceLayers;

	size_t							m_svoVoxelMemSize;
	size_t							m_fragVoxelMemSize;

	bool							m_useTopMipMap;
	int								m_topMipMapNumLevels;
	int								m_topMipMapMaxLevel;
	int								m_topMipMapRes;

	int                             m_curSvoId;	//Current SVO ID used for building

	Sampler							m_textureSampler;


	Vector3int32                    m_maxComputeGridDims;
	int								m_max3DTextureSize;
	int								m_max2DTextureSize;

	

	///////////SVO DATA//////////
    /**        
       R32I pointers to children of nodes.  Each node is a 2^3 block.
    */
	shared_ptr<BufferTexture>       m_rootIndex;	//New: Index of the root block for each volume we store. Also used to optimize dense top tree (direct pointers to each dense levels.
	shared_ptr<BufferTexture>       m_childIndex;
	shared_ptr<BufferTexture>       m_parentIndex;
	shared_ptr<BufferTexture>       m_neighborsIndex;

	 /** m_levelIndexBuffer[0] is the number of allocated nodes.  
        m_levelIndexBuffer[i] is the offset into m_childIndex of the first node for level (i - 1) of the tree.*/
    shared_ptr<BufferTexture>       m_levelIndexBuffer;  //Current Index

	/** NEW: Offset for the allocation of each level  */
	shared_ptr<BufferTexture>       m_levelStartIndexBuffer;

	/** NEW: Allocated size for each mip level */
	shared_ptr<BufferTexture>       m_levelSizeBuffer;

	
	/** NEW */
	shared_ptr<BufferTexture>       m_prevNodeCountBuffer;



	/** The underlying output is stored in a gbuffer */
    shared_ptr<GBuffer>             m_gbuffer;

	/** GBuffer containing octree top mipMAp. */
	shared_ptr<GBuffer>             m_topMipMapGBuffer;


	/////////MANIPULATION DATA//////
	/** Storage for voxel fragments produced by Surface::renderIntoSVO before the tree is built.
        The size of this buffer is proportional to the surface area of the scene. */
    shared_ptr<GBuffer>             m_fragmentBuffer;

	/** Number of allocated elements in m_fragmentBuffer */
    shared_ptr<BufferTexture>       m_fragmentCount;


    /** The number of nodes already allocated in m_childIndex (NOT the index of the next node, which is this * 8) */
    shared_ptr<BufferTexture>       m_numberOfAllocatedNodes;


	/** The arguments for an indirect draw call to visualize the m_fragmentBuffer*/
    shared_ptr<BufferTexture>       m_fragmentsDrawIndirectBuffer;

    /** The arguments for an indirect draw call to visualize the octree*/
    shared_ptr<BufferTexture>       m_drawIndirectBuffer;

    /** The arguments for an indirect compute call to process the m_fragmentBuffer*/
    shared_ptr<BufferTexture>       m_dispatchIndirectBuffer;


    /** The arguments for an indirect compute call to process the next level of a tree.  The fourth element is the number of threads desired. */
    shared_ptr<BufferTexture>       m_dispatchIndirectLevelBuffer;


	shared_ptr<Shader>              m_visualizeNodes;



	/** Performance profiler **/
	shared_ptr<Profiler>			m_profiler;
	const Specification             m_specification;


	/** Used to force the fragment rasterization resolution.  Not actually written to. */
	shared_ptr<Framebuffer>         m_dummyFramebuffer;

	Box                             m_bounds;
	float                           m_timeOffset;
    
  
	String                     		m_writeDeclarationsFragmentBuffer;

	shared_ptr<Camera>              m_camera;

    int                             m_maxTreeDepth;
		

#if SVO_USE_CUDA
	//Temp
	shared_ptr<Texture>				m_renderTargetTexture;
#endif

	////////////////////////////////////

    SVO(const Specification& spec, const String& name, bool usebricks);

	uint64 getBufferGPUAddress(const shared_ptr<BufferTexture> &buff) const;

	void updateDispatchIndirectBuffer(RenderDevice* rd, shared_ptr<BufferTexture> &dispatchIndirectBuffer, 
										shared_ptr<BufferTexture> &startIndexBuffer, shared_ptr<BufferTexture> &endIndexBuffer,
										int startLevel, int endLevel, Vector2int32 workGroupSize);

	void copyScaleVal(RenderDevice* rd,
		shared_ptr<BufferTexture> &srcBuffer, shared_ptr<BufferTexture> &dstBuffer,
		int srcIndex, int dstIndex, 
		int mulFactor, int divFactor);
	
	/* param level Used during octree building. Leave as -1 (default) during rendering.
	*/
	void connectOctreeToShader(Args& args, Access access, int maxTreeDepth, int level=-1) const;

	void fillBuffer(shared_ptr<GLPixelTransferBuffer> buffer, uint numVal, uint val);

public:


    /** Requires a floating point WS_POSITION field */
    static shared_ptr<SVO> create(const Specification& spec = Specification(), const String& name = "SVO", bool usebricks=false);

    /** Draw the raw fragments as points for debugging purposes. */
    void visualizeFragments(RenderDevice* rd) const;

    const String& name() const {
        return m_name;
    }

    /** If null, there is no clipping frustum */
    const shared_ptr<Camera> camera() const {
        return m_camera;
    }

    const Specification& specification() const {
        return m_specification;
    }

    /** Bounds on the elements that were voxelized, not on the octtree itself */
    const Box& bounds() const {
        return m_bounds;
    }

	void setCurSvoId(int id){
		m_curSvoId = id;
	}

	void bindWriteUniformsFragmentBuffer(Args& args) const;
	void bindReadUniformsFragmentBuffer(Args& args) const;

	//level==-1 indicates maxLevel
	void connectToShader(Args& args, Access access, int maxTreeDepth, int level) const;

	const String& writeDeclarationsFragmentBuffer() const {
        return m_writeDeclarationsFragmentBuffer;
    }
	
	void init(RenderDevice* rd, size_t svoPoolSize, int maxTreeDepth, size_t fragmentPoolSize); //New for multi-SVO mode

    /** Bind and clear the data structure. Call before Surface::renderIntoSVO()

      \param camera The voxelization is clipped to the camera's frustum if the camera is non-null  
      \param wsBounds The world-space bounds of the voxelization.  For example, 
        \code
           camera->frustum(viewport)->boundingBox(1000)
        \endcode

      \param maxNodes Maximum number of nodes in the octree.  This is less than 1.25 * number of fine-resolution voxels.

      \param maxVoxelFragments Maximum number of fragments expected from Surface::renderIntoSVO. Proportional to the area of the scene. 
             There can be many fragments within a single voxel that will later be collapsed.
    */
    void prepare(RenderDevice* rd, const shared_ptr<Camera>& camera, const Box& wsBounds, float timeOffset, float velocityStartTimeOffset, size_t svoPoolSize, int maxTreeDepth, size_t fragmentPoolSize);
	void prepare(RenderDevice* rd, const shared_ptr<Camera>& camera, const Box& wsBounds, float timeOffset, float velocityStartTimeOffset); //New. Called per-SVO (volumes)

    /** Build the actual oct-tree.  Call after Surface::renderIntoSVO(). */
    void complete(RenderDevice*, const G3D::String &downSampleShader=G3D::String("SVO_downsampleValues.glc"));


	/** Build octree from fragment data. Multipass mode require a dummy pre-pass in order to allocate per-level memory.*/
    void build(RenderDevice*, bool multiPass=false, bool dummyPass=false, int curPass=0);
	void preBuild(RenderDevice*, bool multiPass=false, bool dummyPass=false);
	void postBuild(RenderDevice*);

	/** Filter octree data. */
    void filter(RenderDevice*, const G3D::String &downSampleShader=G3D::String("SVO_downsampleValues.glc"));

	void printDebugBuild();

    /** Number of voxels along each edge at the finest resolution */
    int fineVoxelResolution() const {
        return pow2(m_maxTreeDepth+m_octreeBias);
    }

	int maxDepth(){
		return m_maxTreeDepth;
	}

    /** Length of each side of a voxel */
    float voxelSideLength() const {
        return m_bounds.extent().max() / fineVoxelResolution();
    }

    /** The framebuffer bound during rendering by Surface::renderIntoSVO. It is not actually rendered into, however */
    shared_ptr<Framebuffer> framebuffer() const {
        return m_dummyFramebuffer;
    }

	CFrame svoToWorldMatrix() const;
	CFrame worldToSVOMatrix() const;

    /** Bind the camera and projection matrices for generating the SVO.  Used by Surface::renderIntoSVO */
    void setOrthogonalProjection(RenderDevice* rd) const;

    void visualizeNodes(RenderDevice* rd, int level) const;

	void renderRaycasting(RenderDevice* rd, shared_ptr<Texture> m_colorBuffer0, int level, float raycastingConeFactor);



	shared_ptr<BufferTexture> getChildIndexBuffer(){
		return m_childIndex;
	}
	shared_ptr<BufferTexture> getNeighborsIndexBuffer(){
		return m_neighborsIndex;
	}

	shared_ptr<GBuffer>  getGBuffer(){
		return m_gbuffer;
	}

	shared_ptr<GBuffer>  getTopMipMapGBuffer(){
		return m_topMipMapGBuffer;
	}

	size_t getTopDenseTreeNumNodes(int depth = (SVO_TOP_MIPMAP_NUM_LEVELS - 1)){
		assert(depth<=m_topMipMapMaxLevel);

		size_t size = 0;
		for (size_t i = 0; i < size_t(depth); ++i){
			size += size_t(1) << (i * 3);
		}

		return size;
	}

	int getNumSurfaceLayers(){
		return m_numSurfaceLayers; 
	}

	void clearFragmentCounter();

	void saveToDisk(const std::string &fileName);
	void loadFromDisk(const std::string &fileName);

	void debugPrintIndexBuffer();
	void debugPrintRootIndexBuffer();
};
}

#endif
