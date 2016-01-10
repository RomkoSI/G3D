/**
 \file GLG3D/SVO.cpp

 \maintainer Morgan McGuire http://graphics.cs.williams.edu

 \created 2013-06-07
 \edited  2013-06-07

 */
#include "GLG3D/SVO.h"
#include "GLG3D/GLPixelTransferBuffer.h"
#include "GLG3D/Shader.h"
#include "GLG3D/RenderDevice.h"
#include <math.h>
#include <time.h>

#include <fstream>



#if SVO_USE_CUDA
extern "C" void svoInitCuda(G3D::SVO *svo );
extern "C" void svoRenderRaycasting_kernel( G3D::SVO *svo, shared_ptr<G3D::Texture> m_colorBuffer0, G3D::Vector3 rayOrigin, G3D::Matrix4 modelViewMat, float focalLength,
											int level, float raycastingConeFactor);
#endif

namespace G3D {
SVO::SVO(const Specification& spec, const String& name, bool usebricks) :
    m_name(name),
    m_useBricks(usebricks),
    m_initOK(false),
    m_specification(spec),
    m_curSvoId(-1),
	m_octreePoolNumNodes(0){

    BUFFER_WIDTH = 16384;
    //BUFFER_WIDTH = 8192;
    //BUFFER_WIDTH = 32768;
    //BUFFER_WIDTH = 4096;

#if SVO_FORCE_NO_BRICK
    m_useBricks			= false;
#endif
	m_brickNumLevels	= 1;
	m_brickBorderSize	= 1;
    m_brickRes				= Vector3int16(1<<m_brickNumLevels, 1<<m_brickNumLevels, 1<<m_brickNumLevels);
    m_brickResWithBorder	= m_brickRes + Vector3int16(m_brickBorderSize, m_brickBorderSize, m_brickBorderSize);
    m_octreeBias			= m_brickNumLevels-1;

    m_numSurfaceLayers		= 1;	//Layers of surfaces used for voxelization.

    m_useTopMipMap			= SVO_USE_TOP_MIPMAP;
    m_topMipMapNumLevels	= SVO_TOP_MIPMAP_NUM_LEVELS;
    m_topMipMapMaxLevel		= m_topMipMapNumLevels-1;
    m_topMipMapRes			= 1<<m_topMipMapMaxLevel;

    m_useNeighborPointers	= SVO_USE_NEIGHBOR_POINTERS;


    // Query OpenGL for the maximum grid dimensions
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT,  0,  &(m_maxComputeGridDims.x) );
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT,  1,  &(m_maxComputeGridDims.y) );
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT,  2,  &(m_maxComputeGridDims.z) );

    //int maxTexture3DSize = 0;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &(m_max3DTextureSize) );
    //m_max3DTextureSize=m_max3DTextureSize/2;


    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &(m_max2DTextureSize) );

    alwaysAssertM(BUFFER_WIDTH <= m_maxComputeGridDims.x,
                  "BUFFER_WIDTH must be less than or equal to the maximum compute grid dimension for this implementation");
    debugAssertGLOk();

    /////////FRAG BUFFER////////
    Specification fragmentSpec = m_specification;
    fragmentSpec.dimension = Texture::DIM_2D;

    if (isNull(fragmentSpec.encoding[GBuffer::Field::SVO_POSITION].format)) {
        fragmentSpec.encoding[GBuffer::Field::SVO_POSITION] = ImageFormat::RGBA16F();
    }

    m_fragmentBuffer = GBuffer::create(fragmentSpec, m_name + "->m_fragmentBuffer");
    m_fragmentBuffer->setImageStore(true);
    m_fragVoxelMemSize = fragmentSpec.memorySize();

    /////////////////////////////

    //TODO: write declaration for the SVO !!!
    m_writeDeclarationsFragmentBuffer += "\n";
    for (int f = 0; f < Field::COUNT; ++f) {
        const ImageFormat* format = fragmentSpec.encoding[f].format;
        if ((format != NULL) && (f != Field::DEPTH_AND_STENCIL)) {
            m_writeDeclarationsFragmentBuffer += String("#define ") + String(Field(f).toString()) + " " + String(Field(f).toString()) + "\n";

            alwaysAssertM(format->numComponents != 3,
                          "SVO requires that all fields have 1, 2, or 4 components due to restrictions from OpenGL glBindImageTexture");
        }
    }

	Specification voxelSpec = m_specification;
	if(m_useBricks){
		voxelSpec.dimension = Texture::DIM_3D;

		m_textureSampler = Sampler::video();
		//m_textureSampler = Sampler::buffer();
	}else{
		//voxelSpec.dimension = Texture::DIM_2D;
		m_textureSampler = Sampler::buffer();
		//m_textureSampler = Sampler::video();
	}

	//Uses original spec
    m_gbuffer = GBuffer::create(voxelSpec, m_name + "->m_gbuffer");
	m_gbuffer->setImageStore(true);	//Enable image store API
	m_svoVoxelMemSize = voxelSpec.memorySize();
	debugAssertGLOk();

	if(m_useTopMipMap){
		Specification mipMapSpec = m_specification;
		mipMapSpec.dimension = Texture::DIM_3D;
		mipMapSpec.genMipMaps =  true;

		m_topMipMapGBuffer = GBuffer::create(mipMapSpec, m_name + "->m_topMipMapGBuffer");
		m_topMipMapGBuffer->setImageStore(true);

		debugAssertGLOk();
	}

    m_fragmentCount                 = BufferTexture::create("SVO::m_fragmentCount",                 GLPixelTransferBuffer::create(1, 1, ImageFormat::R32UI()));
	m_fragmentsDrawIndirectBuffer	= BufferTexture::create("SVO::m_fragmentsDrawIndirectBuffer",   GLPixelTransferBuffer::create(4, 1, ImageFormat::R32UI()));
    m_drawIndirectBuffer            = BufferTexture::create("SVO::m_drawIndirectBuffer",            GLPixelTransferBuffer::create(4, 1, ImageFormat::R32UI()));
    m_dispatchIndirectBuffer        = BufferTexture::create("SVO::m_dispatchIndirectBuffer",        GLPixelTransferBuffer::create(4, 1, ImageFormat::R32UI()));
    m_dispatchIndirectLevelBuffer   = BufferTexture::create("SVO::m_dispatchIndirectLevelBuffer",   GLPixelTransferBuffer::create(4, 1, ImageFormat::R32UI()));
    m_numberOfAllocatedNodes        = BufferTexture::create("SVO::m_numberOfAllocatedNodes",        GLPixelTransferBuffer::create(1, 1, ImageFormat::R32UI()));
	debugAssertGLOk();

    // Allocate the smallest possible dummy framebuffer
#ifndef SVO_TEST_FBO_NOATTACHMENT
    m_dummyFramebuffer              = Framebuffer::create(Texture::createEmpty(name + ".m_dummyFramebuffer->texture", 1024, 1024, ImageFormat::R8(), Texture::DIM_2D, false, 1, 1));
#else
	m_dummyFramebuffer              = Framebuffer::createWithoutAttachments(name + ".m_dummyFramebuffer", Vector2(32, 32), 1, 4, false);	//No attachment FBO, 8xMSAA
#endif
	debugAssertGLOk();


    m_visualizeNodes                = Shader::fromFiles(System::findDataFile("SVO/SVO_visualizeNodes.vrt"),
                                                        System::findDataFile("SVO/SVO_visualizeNodes.geo"),
                                                        System::findDataFile("SVO/SVO_visualizeNodes.pix"));


#if SVO_USE_CUDA
	m_renderTargetTexture = Texture::createEmpty("m_renderTargetTexture", 1280, 720, ImageFormat::RGBA32F() /*ImageFormat::R11G11B10F()*/, Texture::DIM_2D, false);
#endif


#if 0 //SVO_USE_CUDA
	svoInitCuda( this );
#endif
}

shared_ptr<SVO> SVO::create(const Specification& spec, const String& name, bool usebricks) {
    alwaysAssertM
       (isNull(spec.encoding[GBuffer::Field::CS_NORMAL].format) &&
        isNull(spec.encoding[GBuffer::Field::CS_Z].format) &&
        isNull(spec.encoding[GBuffer::Field::CS_POSITION].format) &&
        isNull(spec.encoding[GBuffer::Field::CS_POSITION_CHANGE].format) &&
        isNull(spec.encoding[GBuffer::Field::CS_FACE_NORMAL].format),
        "Camera space properties not supported for SVO");

    return shared_ptr<SVO>(new SVO(spec, name, usebricks));
}


void SVO::init(RenderDevice* rd, size_t svoPoolSize, int maxTreeDepth, size_t fragmentPoolSize){
	m_maxTreeDepth = maxTreeDepth-m_octreeBias;


	size_t maxNodes = int(svoPoolSize/m_svoVoxelMemSize);
	size_t maxVoxelFragments = int(fragmentPoolSize/m_fragVoxelMemSize);

	size_t totalSVOSize=0;

	// Round maxNodes to the nearest multiple of 8 (for the 2^3 leaves)
	maxNodes = (maxNodes/8)*8;

	// Allocate GBuffer (Pool)
	int width;
	int height;
	int depth;

	if( m_gbuffer->specification().dimension == Texture::DIM_3D ){


		if(m_useBricks){
			size_t numBricks = maxNodes/(m_brickRes.x*m_brickRes.y*m_brickRes.z);
			int max3dTexResInBrick=m_max3DTextureSize/m_brickResWithBorder.x;

#if 0
			int widthInBrick	= max3dTexResInBrick;
			width = ceilPow2(widthInBrick * m_brickResWithBorder.x);
			widthInBrick = width/m_brickResWithBorder.x;

			int heightInBrick	= min( iCeil(numBricks / float(widthInBrick)), max3dTexResInBrick);
			height = ceilPow2(heightInBrick * m_brickResWithBorder.y);
			heightInBrick = height/m_brickResWithBorder.y;


			int depthInBrick	= /*iCeil*/ int(numBricks / float(widthInBrick*heightInBrick));
			depth	= depthInBrick * m_brickResWithBorder.z;
#else
			int curtResBrick = int(ceilPow2(powf(float(numBricks), 1.0f / 3.0f)));

			int widthInBrick = min(curtResBrick, max3dTexResInBrick);
			width = ceilPow2(widthInBrick * m_brickResWithBorder.x);
			widthInBrick = width / m_brickResWithBorder.x;

			int numRows = iCeil(float(numBricks) / float(widthInBrick));
			int sqrtResBrick = ceilPow2(powf(float(numRows), 1.0f / 2.0f));

			int heightInBrick = min(sqrtResBrick, max3dTexResInBrick);
			height = ceilPow2(heightInBrick * m_brickResWithBorder.y);
			heightInBrick = height / m_brickResWithBorder.y;


			int depthInBrick = iCeil(numBricks / float(widthInBrick * heightInBrick));
			depth = depthInBrick * m_brickResWithBorder.z;
#endif

			m_octreePoolNumNodes = (widthInBrick * m_brickRes.x) * (heightInBrick * m_brickRes.y) * (depthInBrick * m_brickRes.z);
		} else {

#if 0
			width	= m_max3DTextureSize;
			height	= min( iCeil(maxNodes / float(width)), m_max3DTextureSize);
			depth	= iCeil(maxNodes / float(width*height));
#else
			int curtRes = int(ceilPow2(iCeil(powf(float(maxNodes), 1.0f/3.0f))));
			width	= min( curtRes, m_max3DTextureSize );
			height	= min( curtRes, m_max3DTextureSize);
			depth	= iCeil(maxNodes / float(width*height));
#endif

			m_octreePoolNumNodes	=	width*height*depth;

		}

		//Todo: use NV_deep_texture3D

		alwaysAssertM(depth <= m_max3DTextureSize, "");

	}else{

		width = min( ceilPow2( powf(float(maxNodes), 1.0f / 2.0f) ), int(m_max2DTextureSize) );
		//width  = BUFFER_WIDTH;
		height = iCeil(maxNodes / float(width));
		depth = 1;


		m_octreePoolNumNodes	=	width*height*depth;


		alwaysAssertM(height <= m_max2DTextureSize, "");
	}




	std::cout<<"SVO texture :"<<width<<" x "<<height<<" x "<<depth<<" !!\n";
	std::cout<<"SVO num nodes: "<<maxNodes<<"->"<<m_octreePoolNumNodes<<"\n";
	std::cout<<"SVO pool size: "<<(width*height*depth*m_svoVoxelMemSize)/1024.0f/1024.0f<<"MB\n";
	totalSVOSize+=size_t(width*height*depth)*m_svoVoxelMemSize;


	if ((m_gbuffer->width() != width) || (m_gbuffer->height() != height) || (m_gbuffer->depth() != depth) ) {
		m_gbuffer->resize(width, height, depth);
	}


	//Clear G-Buffer
	m_gbuffer->prepare(rd, shared_ptr<Camera>(), 0.0f, 0.0f, Vector2int16(0, 0), Vector2int16(0, 0));

	if(m_useTopMipMap){
		m_topMipMapGBuffer->resize(m_topMipMapRes, m_topMipMapRes, m_topMipMapRes);

		totalSVOSize+=size_t(m_topMipMapRes*m_topMipMapRes*m_topMipMapRes)*m_svoVoxelMemSize;

		//Clear
		m_topMipMapGBuffer->prepare(rd, shared_ptr<Camera>(), 0.0f, 0.0f, Vector2int16(0, 0), Vector2int16(0, 0));
	}


	////////////FRAG BUFFER//////////////
	const int fragmentBufferWidth  = BUFFER_WIDTH;
	const int fragmentBufferHeight = /*iCeil*/ int(maxVoxelFragments / float(fragmentBufferWidth));
	if ((m_fragmentBuffer->width() != fragmentBufferWidth) || (m_fragmentBuffer->height() < fragmentBufferHeight)) {
		m_fragmentBuffer->resize(fragmentBufferWidth, fragmentBufferHeight);
	}
	//Clear
	m_fragmentBuffer->prepare(rd, shared_ptr<Camera>(), 0.0f, 0.0f, Vector2int16(0, 0), Vector2int16(0, 0));

	std::cout<<"Fragment texture :"<<fragmentBufferWidth<<" x "<<fragmentBufferHeight<<" !!\n";
	std::cout<<"Fragment buffer mem size: "<<(fragmentBufferWidth*fragmentBufferHeight*m_fragVoxelMemSize)/1024.0f/1024.0f<<"MB\n";

	//Zero fragment counter
	//GLPixelTransferBuffer::copy(m_zero->buffer(), m_fragmentCount->buffer(), 1);
	fillBuffer(m_fragmentCount->buffer(), 1, 0);

	/////////////////////////////


#ifndef SVO_TEST_FBO_NOATTACHMENT
	///shared_ptr<Texture> texture = m_dummyFramebuffer->get(Framebuffer::COLOR0)->texture();
	///texture->resize(fineVoxelResolution(), fineVoxelResolution());
	///texture->clear();

	m_dummyFramebuffer->resize(fineVoxelResolution(), fineVoxelResolution());
#else
	m_dummyFramebuffer->resize(Framebuffer::NO_ATTACHMENT, fineVoxelResolution(), fineVoxelResolution());
#endif

	// NEW Allocate root buffer
	if (isNull(m_rootIndex) || (m_rootIndex->size() < SVO_MAX_NUM_VOLUMES)) {
		// Allocate to the worst case size, which is the number of fragments
		m_rootIndex = BufferTexture::create("SVO::m_rootIndex", GLPixelTransferBuffer::create(SVO_MAX_NUM_VOLUMES * SVO_TOP_MIPMAP_NUM_LEVELS, 1, ImageFormat::R32UI(), NULL, 1));
	}

	// Allocate Child pointer buffer
	if (isNull(m_childIndex) || (m_childIndex->size() < int(m_octreePoolNumNodes))) {
		// Allocate to the worst case size, which is the number of fragments
		m_childIndex = BufferTexture::create("SVO::childIndexBufferTexture", GLPixelTransferBuffer::create(m_octreePoolNumNodes, 1, ImageFormat::R32UI(), NULL, 1));

		std::cout<<"SVO childBuff size: "<<(width*height*depth*sizeof(uint))/1024.0f/1024.0f<<"MB\n";
		totalSVOSize+=size_t(width*height*depth)*sizeof(uint);
	}

	// Allocate parent pointer buffer
	const int pnumnodes = iCeil( m_octreePoolNumNodes / 8.0f );
	if ( isNull(m_parentIndex) || (m_parentIndex->size() < pnumnodes) ) {
		// Allocate to the worst case size, which is the number of fragments

		//const int pheight = iCeil( height / 8.0f );


		m_parentIndex = BufferTexture::create("SVO::parentIndexBufferTexture", GLPixelTransferBuffer::create(pnumnodes, 1, ImageFormat::R32UI(), NULL, 1));
		std::cout<<"SVO parentBuff size: "<<(pnumnodes*sizeof(uint))/1024.0f/1024.0f<<"MB\n";

		totalSVOSize+=size_t(pnumnodes)*sizeof(uint);
	}

	if(m_useNeighborPointers && isNull(m_neighborsIndex)){

		// Allocate to the worst case size, which is the number of fragments
		const int pheight = iCeil( height / 8.0f )*SVO_NUM_NEIGHBOR_POINTERS;
		m_neighborsIndex = BufferTexture::create("SVO::m_neighborsIndex", GLPixelTransferBuffer::create(width, pheight, ImageFormat::R32UI(), NULL, depth));
		std::cout<<"SVO neighborBuff size: "<<(width*pheight*depth*sizeof(uint))/1024.0f/1024.0f<<"MB\n";

		totalSVOSize+=size_t(width*pheight*depth)*sizeof(uint);
	}


	if (isNull(m_levelIndexBuffer) || (m_levelIndexBuffer->size() < m_maxTreeDepth + 1)) {
		shared_ptr<GLPixelTransferBuffer> buffer = GLPixelTransferBuffer::create( m_maxTreeDepth + 1, 1, ImageFormat::R32UI());  //m_maxTreeDepth + 1
		m_levelIndexBuffer = BufferTexture::create("SVO::m_levelIndexBuffer", buffer);
	}

	// Zero the node counter
	///GLPixelTransferBuffer::copy(m_zero->buffer(), m_levelIndexBuffer->buffer(), 1);


	//New//
	if (isNull(m_levelSizeBuffer) || (m_levelSizeBuffer->size() < m_maxTreeDepth + 1)) {
		shared_ptr<GLPixelTransferBuffer> buffer = GLPixelTransferBuffer::create( m_maxTreeDepth + 1, 1, ImageFormat::R32UI());
		m_levelSizeBuffer = BufferTexture::create("SVO::m_levelSizeBuffer", buffer);
	}
	///GLPixelTransferBuffer::copy(m_zero->buffer(), m_levelSizeBuffer->buffer(), 16);

	//Needed ?
	if (isNull(m_levelStartIndexBuffer) || (m_levelStartIndexBuffer->size() < m_maxTreeDepth + 1)) {
		shared_ptr<GLPixelTransferBuffer> buffer = GLPixelTransferBuffer::create( m_maxTreeDepth + 1, 1, ImageFormat::R32UI());
		m_levelStartIndexBuffer = BufferTexture::create("SVO::m_levelStartIndexBuffer", buffer);
	}
	///GLPixelTransferBuffer::copy(m_zero->buffer(), m_levelStartIndexBuffer->buffer(), 16);

	//
	if (isNull(m_prevNodeCountBuffer) || (m_prevNodeCountBuffer->size() <  m_maxTreeDepth + 1)) {  //16
		shared_ptr<GLPixelTransferBuffer> buffer = GLPixelTransferBuffer::create( m_maxTreeDepth + 1, 1, ImageFormat::R32UI());
		m_prevNodeCountBuffer = BufferTexture::create("SVO::m_prevNodeCountBuffer", buffer);
	}
	///GLPixelTransferBuffer::copy(m_zero->buffer(), m_prevNodeCountBuffer->buffer(), 16);

	////////

	std::cout<<"\n[TOTAL SVO Size: "<<totalSVOSize/1024.0f/1024.0f<<"MB]\n\n";

	m_curSvoId = -1;


	m_initOK = true;
}


void SVO::prepare(RenderDevice* rd, const shared_ptr<Camera>& camera, const Box& wsBounds, float timeOffset, float velocityStartTimeOffset) {

	alwaysAssertM(m_initOK, "SVO::init() must be called before calling SVO::prepare()");

	//Increment SVO ID
	m_curSvoId++;
	alwaysAssertM( m_curSvoId<SVO_MAX_NUM_VOLUMES, "Too many SVOs voxelized !");

    m_camera = camera;
    m_bounds = wsBounds;



	//Clear G-Buffer: REMOVED for multi-SVO !
#if 0
    m_gbuffer->prepare(rd, camera, timeOffset, velocityStartTimeOffset, Vector2int16(0, 0), Vector2int16(0, 0));
	//m_gbuffer->setFiltering(Texture::BILINEAR_NO_MIPMAP);

	if(m_useTopMipMap){
		m_topMipMapGBuffer->prepare(rd, camera, timeOffset, velocityStartTimeOffset, Vector2int16(0, 0), Vector2int16(0, 0));
	}


#else
	m_gbuffer->setCamera(camera);
	m_gbuffer->setTimeOffsets(timeOffset, velocityStartTimeOffset);

	if(m_useTopMipMap){
		m_topMipMapGBuffer->setCamera(camera);
		m_topMipMapGBuffer->setTimeOffsets(timeOffset, velocityStartTimeOffset);
	}

	////////////FRAG BUFFER//////////////
	//m_fragmentBuffer->setCamera(camera);
	//m_fragmentBuffer->setTimeOffsets(timeOffset, velocityStartTimeOffset);
	/////////////////////////////

#endif

	////////////FRAG BUFFER//////////////
	//Always reset fragment buffer
	m_fragmentBuffer->prepare(rd, camera, timeOffset, velocityStartTimeOffset, Vector2int16(0, 0), Vector2int16(0, 0));
	//Zero fragment counter
	///GLPixelTransferBuffer::copy(m_zero->buffer(), m_fragmentCount->buffer(), 1);
	fillBuffer(m_fragmentCount->buffer(), 1, 0);
	////////////////////////////////////
}


//Deprecated
void SVO::prepare(RenderDevice* rd, const shared_ptr<Camera>& camera, const Box& wsBounds, float timeOffset, float velocityStartTimeOffset, size_t svoPoolSize, int maxTreeDepth, size_t fragmentPoolSize) {

	this->init(rd, svoPoolSize, maxTreeDepth, fragmentPoolSize);
	this->prepare(rd, camera, wsBounds, timeOffset, velocityStartTimeOffset);

}


void SVO::bindWriteUniformsFragmentBuffer(Args& args) const {
    args.setMacro("USE_IMAGE_STORE", 1);

	args.setMacro("USE_SVO", 1);

    args.setMacro("BUFFER_WIDTH",       BUFFER_WIDTH);
    args.setMacro("BUFFER_WIDTH_MASK",  BUFFER_WIDTH - 1);
    args.setMacro("BUFFER_WIDTH_SHIFT", iRound(log2(BUFFER_WIDTH)));
    args.setMacro("USE_DEPTH_PEEL", 0);

	args.appendToPreamble(m_writeDeclarationsFragmentBuffer);

	args.setUniform("invResolution", 1.0f / fineVoxelResolution() );

    m_fragmentBuffer->setShaderArgsWrite(args);

    // Bind the images for output [fragmentBuffer]
    for (int f = 0; f < Field::COUNT; ++f) {
        const ImageFormat* format = m_fragmentBuffer->specification().encoding[f].format;
        if ((format != NULL) && (f != Field::DEPTH_AND_STENCIL)) {
            //const Encoding& encoding = m_fragmentBuffer->specification().encoding[f];
            args.setImageUniform(String(Field(f).toString()) + "_buffer", m_fragmentBuffer->texture(Field(f)), Access::WRITE, 0, true);
        }
    }

    // Bind the fragment counter
    args.setImageUniform("fragmentCounter_buffer", m_fragmentCount, Access::READ_WRITE);


	m_fragmentBuffer->connectToShader( "gbuffer", args, Access::READ_WRITE, Sampler::buffer() );
}

void SVO::bindReadUniformsFragmentBuffer(Args& args) const {
    args.setMacro("USE_IMAGE_STORE", 1);

	args.setMacro("USE_SVO", 1);

    args.setMacro("BUFFER_WIDTH",       BUFFER_WIDTH);
    args.setMacro("BUFFER_WIDTH_MASK",  BUFFER_WIDTH - 1);
    args.setMacro("BUFFER_WIDTH_SHIFT", iRound(log2(BUFFER_WIDTH)));

	args.appendToPreamble(m_writeDeclarationsFragmentBuffer);

    m_fragmentBuffer->setShaderArgsRead(args);

#if 0
    // Bind the images for output [fragmentBuffer]
    for (int f = 0; f < Field::COUNT; ++f) {
        const ImageFormat* format = m_fragmentBuffer->specification().format[f];
        if ((format != NULL) && (f != Field::DEPTH_AND_STENCIL)) {
            //const Encoding& encoding = m_fragmentBuffer->specification().encoding[f];
            args.setImageUniform(String(Field(f).toString()) + "_buffer", m_fragmentBuffer->texture(Field(f)), Access::READ, 0, true);

			String texName = String(Field(f).toString()) + "_tex";
			args.setUniform( texName, m_gbuffer->texture(Field(f)) );

        }
    }
#endif

    // Bind the fragment counter
    args.setImageUniform("fragmentCounter_buffer",	m_fragmentCount, Access::READ);

	m_fragmentBuffer->connectToShader("fragBuffer", args, Access::READ, Sampler::buffer());
}

void SVO::connectOctreeToShader(Args& args, Access access, int maxTreeDepth, int level) const{
	if (level == -1) {
		// Default
		level = maxTreeDepth;
	}

	float svoMinVoxelSize = 1.0f/float(1<<(maxTreeDepth+m_octreeBias));

	args.setMacro("SVO_OCTREE_BIAS", m_octreeBias);
	args.setMacro("USE_SVO", 1);

	args.setMacro("SVO_MAX_NUM_VOLUMES",		int(SVO_MAX_NUM_VOLUMES));
	args.setMacro("SVO_MAX_NUM_LEVELS",			int(SVO_MAX_NUM_LEVELS));

	args.setMacro("SVO_MAX_LEVEL",				int(maxTreeDepth));
	args.setMacro("SVO_NUM_LEVELS",				int(maxTreeDepth + 1));
	args.setMacro("SVO_MIN_VOXEL_SIZE",			svoMinVoxelSize);

	args.setMacro("SVO_USE_TOP_DENSE", int(SVO_USE_TOP_DENSE));

	args.setMacro("SVO_USE_TOP_MIPMAP", int(m_useTopMipMap));
	args.setMacro("SVO_TOP_MIPMAP_NUM_LEVELS", int(m_topMipMapNumLevels));
	args.setMacro("SVO_TOP_MIPMAP_MAX_LEVEL", int(m_topMipMapMaxLevel));
	args.setMacro("SVO_TOP_MIPMAP_RES", int(m_topMipMapRes));


	args.setMacro("SVO_USE_NEIGHBOR_POINTERS",  int(m_useNeighborPointers));
	args.setMacro("SVO_NUM_NEIGHBOR_POINTERS",  SVO_NUM_NEIGHBOR_POINTERS);



	args.setMacro("SVO_CUR_SVO_ID",  m_curSvoId);



	args.setUniform("svoMaxLevel", min(level, m_maxTreeDepth) );
	args.setUniform("svoMinVoxelSize", svoMinVoxelSize);

	args.setImageUniform("levelIndexBuffer",            m_levelIndexBuffer,   access);

	// Octree

	args.setUniform(		"octreePoolNumNodes",	m_octreePoolNumNodes );

	args.setUniform(		"d_rootIndexBuffer",	getBufferGPUAddress(m_rootIndex) );
	args.setUniform(		"rootIndexBufferTex",	m_rootIndex );

	args.setUniform(		"d_childIndexBuffer",	m_childIndex->buffer()->getGPUAddress());
	args.setImageUniform(	"childIndexBuffer",		m_childIndex,		access);
	args.setUniform(		"childIndexBufferTex",	m_childIndex );


	args.setImageUniform(	"parentIndexBuffer",    m_parentIndex,		access);
	args.setUniform(		"d_parentIndexBuffer",	getBufferGPUAddress(m_parentIndex) );


	if(m_useNeighborPointers && !isNull(m_neighborsIndex)){
		args.setUniform(		"d_neighborsIndexBuffer",	getBufferGPUAddress(m_neighborsIndex) );
	}

}


void SVO::connectToShader(Args& args, Access access, int maxTreeDepth, int level) const {
    if ((maxTreeDepth < 0) || (maxTreeDepth>m_maxTreeDepth)) {
        maxTreeDepth = m_maxTreeDepth;
    }

    // const float svoMinVoxelSize = 1.0f / float(1<<(maxTreeDepth+m_octreeBias));

    args.setMacro("USE_IMAGE_STORE", 1);
    args.setMacro("SVO_USE_CUDA",	int(SVO_USE_CUDA));
    args.setMacro("SVO_WRITE_ENABLED", 1);
    args.setMacro("SVO_USE_BRICKS",				int(m_useBricks));
    args.setMacro("SVO_BRICK_NUM_LEVELS",		m_brickNumLevels);
    args.setMacro("SVO_BRICK_RES",				m_brickRes.x);
    args.setMacro("SVO_BRICK_BORDER",			m_brickBorderSize);
    args.setMacro("SVO_BRICK_BORDER_OFFSET",	m_brickBorderSize/2);
    args.setMacro("SVO_BRICK_RES_WITH_BORDER",	m_brickResWithBorder.x);




    {
        int maxLevelIndex = 8;
        for(int l = 1; l < m_topMipMapMaxLevel; ++l){
            const int lres = 1 << l;
            maxLevelIndex += lres * lres * lres;
        }
        args.setMacro("SVO_TOP_MIPMAP_MAX_LEVEL_INDEX",	maxLevelIndex);
    }

    connectOctreeToShader(args, access, maxTreeDepth, level);

	int depth = m_gbuffer->depth();

    args.setMacro("SVO_VOXELPOOL_RES_X", int(m_gbuffer->width()) );
    args.setMacro("SVO_VOXELPOOL_RES_Y", int(m_gbuffer->height()) );
    args.setMacro("SVO_VOXELPOOL_RES_Z", int(m_gbuffer->depth()) );

    args.setMacro("SVO_VOXELPOOL_RES_BRICK_X", int(m_gbuffer->width()/m_brickResWithBorder.x) );
    args.setMacro("SVO_VOXELPOOL_RES_BRICK_Y", int(m_gbuffer->height()/m_brickResWithBorder.y) );
    args.setMacro("SVO_VOXELPOOL_RES_BRICK_Z", int(m_gbuffer->depth()/m_brickResWithBorder.z) );

    args.setMacro("SVO_OPTIMIZE_CACHING_BLOCKS",  SVO_OPTIMIZE_CACHING_BLOCKS);	//Optim



    args.setUniform("svoWorldToSVOMat", worldToSVOMatrix().toMatrix4() );

    m_gbuffer->connectToShader("svo", args, access, m_textureSampler);

    args.setUniform("projectionScale", projectionScale );
    args.setUniform("projectionOffset", projectionOffset );

    if (m_useTopMipMap) {
        int topMipLevel = 0;
        if ((level >= 0) && (level < m_topMipMapNumLevels)) {
            topMipLevel = m_topMipMapNumLevels - level - 1;
        }

        m_topMipMapGBuffer->connectToShader("svoTopMipMap", args, access, m_textureSampler, topMipLevel);
    }

    // Bind the fragment counter
    //args.setImageUniform("fragmentCounter_buffer", m_fragmentCount, Access::READ);
}


void SVO::setOrthogonalProjection(RenderDevice* rd) const {
    // Orthographic projection (center of the bounds is at the origin)
    CFrame ortho;
   // const Vector3& extent = m_bounds.extent();
   // (void)extent;
    m_bounds.getLocalFrame(ortho);


    // Move the origin to the corner
    //ortho.translation -= extent / 2;

    // Map to the unit cube
    //rd->setProjectionMatrix(Matrix4::scale(2.0f / m_bounds.extent().max()));
	rd->setProjectionMatrix( Matrix4::scale( Vector3(2.0f, 2.0f, 2.0f) / m_bounds.extent() ) );	//Non-cubical bounds


    rd->setCameraToWorldMatrix(ortho);
}

static Vector2int32 workGroupSize(32, 1);

void SVO::updateDispatchIndirectBuffer(RenderDevice* rd, shared_ptr<BufferTexture> &dispatchIndirectBuffer,
									   shared_ptr<BufferTexture> &startIndexBuffer, shared_ptr<BufferTexture> &endIndexBuffer,
									   int startLevel, int endLevel, Vector2int32 workGroupSize) {

	// Set the grid dimensions to iterate over the previous level's nodes, which contain
	// either 0 [no children] or all 1 bits [subtree] in their child pointers.
	Args args;
	args.setMacro("COMPUTE_DISPATCH_INDIRECT_BUFFER", "1");	//Should be set ?

	args.setUniform("startIndex",                   startLevel);  //-1
	args.setUniform("endIndex",                     endLevel);
	args.setUniform("dispatchMaxWidth",             m_maxComputeGridDims.x);
	args.setUniform("dispatchWorkGroupSize",        workGroupSize);
	//args.setImageUniform("countBuffer",             m_levelIndexBuffer,              Access::READ);
	args.setImageUniform("startIndexBuffer",        startIndexBuffer,            Access::READ);
	args.setImageUniform("endIndexBuffer",          endIndexBuffer,              Access::READ);

	args.setImageUniform("dispatchIndirectBuffer",  m_dispatchIndirectLevelBuffer,   Access::WRITE);

	args.computeGridDim = Vector3int32(1, 1, 1);

	LAUNCH_SHADER_WITH_HINT("SVO/SVO_countToIndirectArgument.glc", args, name());

	//glFinish();

	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}


void SVO::copyScaleVal(RenderDevice* rd,
						shared_ptr<BufferTexture> &srcBuffer, shared_ptr<BufferTexture> &dstBuffer,
						int srcIndex, int dstIndex,
						int mulFactor, int divFactor){

	Args args;
	args.setUniform("srcIndex", srcIndex);
	args.setUniform("dstIndex", dstIndex);
	args.setUniform("mulFactor", mulFactor);
	args.setUniform("divFactor", divFactor);

	args.setImageUniform("srcBuffer", srcBuffer, Access::READ);
	args.setImageUniform("dstBuffer", dstBuffer, Access::WRITE);

	args.computeGridDim = Vector3int32(1, 1, 1);

	LAUNCH_SHADER_WITH_HINT("SVO/SVO_updateLevelIndexBuffer.glc", args, name());

	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	//glFinish();
}


void SVO::complete(RenderDevice* rd, const G3D::String &downSampleShader) {


	preBuild(rd);


	build(rd);
	postBuild(rd);


	//Filter and copy borders
	filter(rd, downSampleShader);

}

void SVO::preBuild(RenderDevice* rd, bool multiPass, bool dummyPass){
	// Ensure that m_fragmentCount is completely written
    glMemoryBarrier(GL_ALL_BARRIER_BITS);


#if 0
	// Set the number of allocated nodes to 2 (reserve spot 0, and then the node allocated for the root)
    GLPixelTransferBuffer::copy(m_eightOneOneTwo->buffer(), m_numberOfAllocatedNodes->buffer(), 1, 3, 0);

    // Set the initial level to start at location 8 (we reserved location 0 to act as the NULL pointer, and allocate in blocks of 8)
	GLPixelTransferBuffer::copy(m_zero->buffer(), m_levelIndexBuffer->buffer(), 16);
    GLPixelTransferBuffer::copy(m_eightOneOneTwo->buffer(), m_levelIndexBuffer->buffer(), 1);
	GLPixelTransferBuffer::copy(m_eightOneOneTwo->buffer(), m_levelIndexBuffer->buffer(), 1, 4, 1);	//16 copied as end offset for level 1 (8 empty nodes + 8 level 1 nodes)
	GLPixelTransferBuffer::copy(m_eightOneOneTwo->buffer(), m_levelIndexBuffer->buffer(), 1, 4, 2);

	// NEW
	GLPixelTransferBuffer::copy(m_zero->buffer(), m_levelStartIndexBuffer->buffer(), 16);
	GLPixelTransferBuffer::copy(m_zero->buffer(), m_levelStartIndexBuffer->buffer(), 1);					//0
	GLPixelTransferBuffer::copy(m_eightOneOneTwo->buffer(), m_levelStartIndexBuffer->buffer(), 1, 0, 1);	//8
	GLPixelTransferBuffer::copy(m_eightOneOneTwo->buffer(), m_levelStartIndexBuffer->buffer(), 1, 4, 2);	//16


    // Zero the root's eight node pointers + the first 8 NULL pointers
    GLPixelTransferBuffer::copy(m_zero->buffer(), m_childIndex->buffer(), 16);

	// Zero the root's + level 1 parent pointers
    GLPixelTransferBuffer::copy(m_zero->buffer(), m_parentIndex->buffer(), 16); //2


#else
	{
		//Init building

		Args args;
		args.setUniform("curSvoId", m_curSvoId);

		bool resetPool =  (m_curSvoId==0);
		args.setUniform("resetPoolAllocation", resetPool);

		//connectOctreeToShader(args, Access::READ_WRITE, m_maxTreeDepth, 0);
		connectToShader(args, Access::READ_WRITE, m_maxTreeDepth, 0);

		args.setUniform("d_numberOfAllocatedNodes",			getBufferGPUAddress(m_numberOfAllocatedNodes));
		args.setUniform("d_levelStartIndexBuffer",			getBufferGPUAddress(m_levelStartIndexBuffer));
		args.setUniform("d_levelIndexBuffer",				getBufferGPUAddress(m_levelIndexBuffer));

		args.setUniform("d_dispatchIndirectLevelBuffer",	getBufferGPUAddress(m_dispatchIndirectLevelBuffer));


		args.setUniform("topDenseTreeNumNodes",			uint(this->getTopDenseTreeNumNodes()) );

		args.computeGridDim = Vector3int32(1, 1, 1);

		LAUNCH_SHADER_WITH_HINT("SVO/SVO_buildInit.glc", args, name());

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}
#endif

	debugPrintIndexBuffer();
	debugPrintRootIndexBuffer();

#if 1
	{
		//Clear full octree pool: Warning disable with multi-svo !

		Args args;

		args.setMacro("WORK_GROUP_SIZE_X", workGroupSize.x);
		args.setMacro("WORK_GROUP_SIZE_Y", workGroupSize.y);


		connectToShader(args, Access::READ_WRITE, m_maxTreeDepth, 0);


		Vector3int32 gridDim = Vector3int32(iCeil(m_octreePoolNumNodes / float(workGroupSize.x*workGroupSize.y)), 1, 1);
		args.computeGridDim = gridDim;

		LAUNCH_SHADER_WITH_HINT("SVO/SVO_clearTree.glc", args, name());

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}
#endif

#if SVO_USE_TOP_DENSE
	{
		//Clear dense tree

		Args args;
		args.setMacro("WORK_GROUP_SIZE_X", workGroupSize.x);
		args.setMacro("WORK_GROUP_SIZE_Y", workGroupSize.y);

		args.setUniform("curSvoId", m_curSvoId);

		bool resetPool = (m_curSvoId == 0);
		args.setUniform("resetPoolAllocation", resetPool);

		args.setUniform("topDenseTreeNumNodes", uint(this->getTopDenseTreeNumNodes()));

		//connectOctreeToShader(args, Access::READ_WRITE, m_maxTreeDepth, 0);
		connectToShader(args, Access::READ_WRITE, m_maxTreeDepth, 0);

		Vector3int32 gridDim = Vector3int32( iCeil(this->getTopDenseTreeNumNodes()/float(workGroupSize.x*workGroupSize.y)), 1, 1);
		args.computeGridDim = gridDim;

		LAUNCH_SHADER_WITH_HINT("SVO/SVO_clearTopDenseTree.glc", args, name());

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}
#endif




	// Set the initial grid to 8x1x1 to process the root's 8 pointers
	///GLPixelTransferBuffer::copy(m_eightOneOneTwo->buffer(), m_dispatchIndirectLevelBuffer->buffer(), 3);
	///GLPixelTransferBuffer::copy(m_eightOneOneTwo->buffer(), m_dispatchIndirectLevelBuffer->buffer(), 1, 0, 3);	//Set 4th element to 8

	//
	glFinish();
	//


	debugPrintIndexBuffer();

	//
	//updateDispatchIndirectBuffer(rd, m_dispatchIndirectLevelBuffer, 0, 0, workGroupSize);
	//

	//New
	if(multiPass){


		if(dummyPass) {

			//GLPixelTransferBuffer::copy(m_zero->buffer(), m_levelSizeBuffer->buffer(), 16);
			fillBuffer(m_levelSizeBuffer->buffer(), 16, 0);

		}else{
			//Convert allocations size from dummy pass to allocation offsets per level, and initialize m_levelIndexBuffer

			Args args;
			//args.setUniform("level", level+1);

			args.setImageUniform("sizeBuffer", m_levelSizeBuffer, Access::READ);

			args.setImageUniform("levelStartIndexBuffer", m_levelStartIndexBuffer, Access::READ_WRITE);
			args.setImageUniform("levelIndexBuffer", m_levelIndexBuffer, Access::READ_WRITE);

			args.computeGridDim = Vector3int32(1, 1, 1);

			LAUNCH_SHADER_WITH_HINT("SVO_levelAllocToIndex.glc", args, name());

			glMemoryBarrier(GL_ALL_BARRIER_BITS);
		}

		//GLPixelTransferBuffer::copy(m_levelStartIndexBuffer->buffer(), m_levelPrevIndexBuffer->buffer(), 16);


		//GLPixelTransferBuffer::copy(m_eightOneOneTwo->buffer(), m_levelIndexBuffer->buffer(), 1, 4, 1);	//16 copied as end offset for level 1 (8 empty nodes + 8 level 1 nodes)

	}

    // Ensure that m_drawIndirectBuffer and m_dispatchIndirectLevelBuffer are completely written
    glMemoryBarrier(GL_ALL_BARRIER_BITS);



}

void SVO::postBuild(RenderDevice* rd){

#if 1
	glFinish();
	{

		uint32* ptrStart = (uint32*)m_levelStartIndexBuffer->buffer()->mapRead();
		uint32* ptr = (uint32*)m_levelIndexBuffer->buffer()->mapRead();


		std::cout<<"m_levelIndexBuffer:\n";

		for(int i=0; i<=m_maxTreeDepth; i++){

			int prevLevelEndIdx = ptrStart[i];

			m_levelsNumNodes[i] = ptr[i]-prevLevelEndIdx;

			//prevLevelEndIdx = ptr[i];

			//std::cout<<m_levelsNumNodes[i]<<" ";
			std::cout<<"("<<ptrStart[i]<<", "<<ptr[i]<<") ";
		}
		std::cout<<"\n";
		m_levelIndexBuffer->buffer()->unmap();
		m_levelStartIndexBuffer->buffer()->unmap();
	}

	glFinish();
#endif
}

/** Build octree from fragment data. */
void SVO::build(RenderDevice* rd, bool multiPass, bool dummyPass, int curPass){

	//std::cout<<"PRE PASS "<<curPass<<"\n";
	//printDebugBuild();


	// Ensure that m_drawIndirectBuffer and m_dispatchIndirectLevelBuffer are completely written
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

    // Fragment Buffer: Create the visualization and compute-indirect arguments
    {
        Args args;
        args.computeGridDim = Vector3int32(1, 1, 1);
        args.setMacro("COMPUTE_DRAW_INDIRECT_BUFFER", "1");
        args.setMacro("COMPUTE_DISPATCH_INDIRECT_BUFFER", "1");

        // Make the grid correspond to the buffer pixels
        alwaysAssertM(BUFFER_WIDTH % 32 == 0 && BUFFER_WIDTH >= 32, "BUFFER_WIDTH must be a multiple of 32");
        args.setUniform("dispatchMaxWidth",                BUFFER_WIDTH / 32);
        args.setUniform("dispatchWorkGroupSize",           workGroupSize);

        args.setUniform("startIndex",                      -1);
        args.setUniform("endIndex",                        0);

        // The buffer to read from
        //args.setImageUniform("countBuffer",                m_fragmentCount,                 Access::READ);
		args.setImageUniform("startIndexBuffer",        m_fragmentCount,            Access::READ);
		args.setImageUniform("endIndexBuffer",          m_fragmentCount,              Access::READ);

        args.setImageUniform("drawIndirectBuffer",         m_fragmentsDrawIndirectBuffer,   Access::WRITE);
        args.setImageUniform("dispatchIndirectBuffer",     m_dispatchIndirectBuffer,        Access::WRITE);

        LAUNCH_SHADER_WITH_HINT("SVO/SVO_countToIndirectArgument.glc", args, name());
    }

    // Ensure that m_drawIndirectBuffer and m_dispatchIndirectLevelBuffer are completely written
    glMemoryBarrier(GL_ALL_BARRIER_BITS);


    debugAssertGLOk();
    // The root has already been inserted
    for (int level = 1; level < m_maxTreeDepth; ++level) {		//<=m_maxTreeDepth

		//FLAG//
		//Finest level flagged as allocated//
#if 1
        if( (SVO_TOP_MIPMAP_SPARSE_TREE || !m_useTopMipMap || level>=m_topMipMapMaxLevel)  /*&& curPass==0*/ ) {

            // Launch one thread per fragment, flagging all nodes at this level that contain
            // that fragment.  This is then used to determine which nodes must be recursed into.
            Args args;
            args.setMacro("WORK_GROUP_SIZE_X",          workGroupSize.x);
            args.setMacro("WORK_GROUP_SIZE_Y",          workGroupSize.y);

			//if(dummyPass)
				args.setMacro("DUMMY_PASS", 1);


			bindReadUniformsFragmentBuffer(args);
			connectToShader(args, Access::READ_WRITE, m_maxTreeDepth, level);

            args.setUniform("level",                    level);

            // Process each fragment
            args.setIndirectBuffer(m_dispatchIndirectBuffer->buffer());

            debugAssertGLOk();
            //rd->apply(m_flagPopulatedChildren, args);
			LAUNCH_SHADER_WITH_HINT("SVO/SVO_flagPopulatedChildren.glc", args, name());


			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			debugAssertGLOk();
		}
#endif

		//ALLOCATE//
		if(level < m_maxTreeDepth){

			if(!m_useTopMipMap || level>=m_topMipMapMaxLevel) {

				if(!multiPass){

					// Set the grid dimensions to iterate over the previous level's nodes, which contain
					// either 0 [no children] or all 1 bits [subtree] in their child pointers.

					///updateDispatchIndirectBuffer(rd, m_dispatchIndirectLevelBuffer, m_levelIndexBuffer, m_levelIndexBuffer, level-1, level, workGroupSize);
					updateDispatchIndirectBuffer(rd, m_dispatchIndirectLevelBuffer, m_levelStartIndexBuffer, m_levelIndexBuffer, level, level, workGroupSize); //New

				}else{

					if(dummyPass){

						//Iterate over ALL previous nodes
						updateDispatchIndirectBuffer(rd, m_dispatchIndirectLevelBuffer, m_levelStartIndexBuffer, m_levelIndexBuffer, -1, level, workGroupSize);

						///copyScaleVal(rd, m_numberOfAllocatedNodes, m_prevNodeCountBuffer, 0, 0, 8, 1);

					}else{
						// Set the grid dimensions to iterate over the previous level's nodes, which contain
						// either 0 [no children] or all 1 bits [subtree] in their child pointers.
						updateDispatchIndirectBuffer(rd, m_dispatchIndirectLevelBuffer, m_levelStartIndexBuffer, m_levelIndexBuffer, level, level, workGroupSize);


						//m_levelIndexBuffer[l+1] -> m_numberOfAllocatedNodes
						copyScaleVal(rd, m_levelIndexBuffer, m_numberOfAllocatedNodes, level+1, 0, 1, 8);
						//copyScaleVal(rd, m_levelStartIndexBuffer, m_numberOfAllocatedNodes, level+1, 0, 1, 8); //test
					}
				}

				{
					// Allocate new nodes and clear *their* child pointers to zero
					Args args;
					args.setMacro("WORK_GROUP_SIZE_X",                  workGroupSize.x);
					args.setMacro("WORK_GROUP_SIZE_Y",                  workGroupSize.y);


					connectToShader(args, Access::READ_WRITE, m_maxTreeDepth, level);

					args.setUniform("level",                            level);

					if(dummyPass){
						args.setMacro("DUMMY_PASS", 1);
						args.setImageUniform("levelSizeBuffer", m_levelSizeBuffer, Access::READ_WRITE);
					}

					// Used for accessing the number of threads
					args.setImageUniform("dispatchIndirectLevelBuffer", m_dispatchIndirectLevelBuffer,      Access::READ);

					/*if(!multiPass || dummyPass)
						args.setImageUniform("levelIndexBuffer",            m_levelIndexBuffer,                 Access::READ);
					else*/
						args.setImageUniform("levelStartIndexBuffer",       m_levelStartIndexBuffer,            Access::READ);

					args.setImageUniform("numberOfAllocatedNodes",      m_numberOfAllocatedNodes,           Access::READ_WRITE);

					args.setIndirectBuffer(m_dispatchIndirectLevelBuffer->buffer(), 0);


					if( (SVO_USE_TOP_DENSE==0) || level>=m_topMipMapMaxLevel){
						LAUNCH_SHADER_WITH_HINT("SVO/SVO_allocateNodes.glc", args, name());
					}else{
						LAUNCH_SHADER_WITH_HINT("SVO/SVO_allocateNodesDenseTree.glc", args, name());
					}

					glMemoryBarrier(GL_ALL_BARRIER_BITS);
				}

			}else{	//Top  MipMap allocation

#if 0  //!!! Broken by levelStartIndexBuffer change
				//SVO_allocateNodesTopMipMap.glc

				//int allocLevel=level+1;

				int levelRes = 1<<level;

				Vector3int32 workGroupSize(4, 4, 4);
				Args args;
				args.setMacro("WORK_GROUP_SIZE_X", workGroupSize.x);
				args.setMacro("WORK_GROUP_SIZE_Y", workGroupSize.y);
				args.setMacro("WORK_GROUP_SIZE_Z", workGroupSize.z);
				args.setMacro("SVO_TOP_MIPMAP_SPARSE_TREE", SVO_TOP_MIPMAP_SPARSE_TREE);

				Vector3int32 gridDim = Vector3int32( iCeil(levelRes/float(workGroupSize.x)), iCeil(levelRes/float(workGroupSize.y)), iCeil(levelRes/float(workGroupSize.z)));
				args.computeGridDim = gridDim;

				args.setUniform("level", level);

				connectToShader(args, Access::READ_WRITE, m_maxTreeDepth, level);
				// Used for accessing the number of threads
				args.setImageUniform("dispatchIndirectLevelBuffer", m_dispatchIndirectLevelBuffer,      Access::READ);
				args.setImageUniform("levelIndexBuffer",            m_levelIndexBuffer,                 Access::READ);
				args.setImageUniform("numberOfAllocatedNodes",      m_numberOfAllocatedNodes,           Access::READ_WRITE);

				LAUNCH_SHADER_WITH_HINT("SVO_allocateNodesTopMipMap.glc", args, name());

				glMemoryBarrier(GL_ALL_BARRIER_BITS);

				debugAssertGLOk();
#endif

			}


			//Set the Start Index value for the next level //??
			if(!multiPass || dummyPass){

				if( (SVO_USE_TOP_DENSE==0) || level>=m_topMipMapMaxLevel)
				{
					copyScaleVal(rd, m_levelIndexBuffer, m_levelStartIndexBuffer, level, level+1, 1, 1);
				}

			}


			if(multiPass){


				if(dummyPass){
# if 0
					// Add the newly allocated nodes to the number of nodes of the next level
					Args args;
					args.setUniform("level", level+1);
					args.setImageUniform("numberOfAllocatedNodes", m_numberOfAllocatedNodes, Access::READ);
					args.setImageUniform("levelIndexBuffer", m_levelIndexBuffer, Access::READ);
					args.setImageUniform("levelSizeBuffer", m_levelSizeBuffer, Access::READ_WRITE);

					args.setImageUniform("prevNodeCountBuffer", m_prevNodeCountBuffer, Access::READ);

					args.computeGridDim = Vector3int32(1, 1, 1);

					LAUNCH_SHADER("SVO_updateLevelSizeBuffer.glc", args);

					glMemoryBarrier(GL_ALL_BARRIER_BITS);
# endif

				}else{

					//GLPixelTransferBuffer::copy(m_levelIndexBuffer->buffer(), m_levelPrevIndexBuffer->buffer(), 1, level+1, level);
					//glMemoryBarrier(GL_ALL_BARRIER_BITS);
				}
			}



			if( (SVO_USE_TOP_DENSE==0) || (level>=m_topMipMapMaxLevel) ){
				// Set the end of levelIndexBuffer = nextFreeNodeNumber * 8
				copyScaleVal(rd, m_numberOfAllocatedNodes, m_levelIndexBuffer, 0, level+1, 8, 1);
			}

			//if(level<4){
			//	std::cout<<"[LOD "<<level<<"]\n";
			//	printDebugBuild();
			//}

		}
    }


    ///Store values and downsample///

	//Write values//
	if(!dummyPass)
	{
        Args args;
        args.setMacro("WORK_GROUP_SIZE_X",          workGroupSize.x);
        args.setMacro("WORK_GROUP_SIZE_Y",          workGroupSize.y);
        args.setMacro("BUFFER_WIDTH",               BUFFER_WIDTH);

        args.setImageUniform("fragmentCount",       m_fragmentCount,   Access::READ);

		bindReadUniformsFragmentBuffer(args);
		connectToShader(args, Access::READ_WRITE, m_maxTreeDepth, m_maxTreeDepth);

		//m_gbuffer->connectToShader("svo", args, Access::READ_WRITE, m_textureSampler);

        //args.setUniform("SVO_POSITION_tex",     m_fragmentBuffer->texture(GBuffer::Field::SVO_POSITION));
		//args.setUniform("WS_NORMAL_tex",		m_fragmentBuffer->texture(GBuffer::Field::WS_NORMAL));

        args.setUniform("level",                m_maxTreeDepth);

        // Process each fragment
        args.setIndirectBuffer(m_dispatchIndirectBuffer->buffer());

        debugAssertGLOk();

		LAUNCH_SHADER_WITH_HINT("SVO_writeVoxelFragValues.glc", args, name());

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		debugAssertGLOk();
    }
}

/** Filter octree data. */
void SVO::filter(RenderDevice* rd, const G3D::String &downSampleShader){


#if SVO_ACCUMULATE_VOXEL_FRAGMENTS
	//Normalize accumulated values
	{
		int level = m_maxTreeDepth;

		//Build dispatch indirect buffer//
		updateDispatchIndirectBuffer(rd, m_dispatchIndirectLevelBuffer, m_levelStartIndexBuffer, m_levelIndexBuffer, level, level, workGroupSize);

		//Launch//

		// Allocate new nodes and clear *their* child pointers to zero
		Args args;
		args.setMacro("WORK_GROUP_SIZE_X", workGroupSize.x);
		args.setMacro("WORK_GROUP_SIZE_Y", workGroupSize.y);

		connectToShader(args, Access::READ_WRITE, m_maxTreeDepth, level);

		args.setUniform("level", level);
		args.setImageUniform("dispatchIndirectLevelBuffer", m_dispatchIndirectLevelBuffer, Access::READ);	// Used for accessing the number of threads
		//args.setImageUniform("numberOfAllocatedNodes",      m_numberOfAllocatedNodes,           Access::READ_WRITE);

		args.setIndirectBuffer(m_dispatchIndirectLevelBuffer->buffer(), 0);

		LAUNCH_SHADER_WITH_HINT("SVO_normalizeValues.glc", args, name());	


		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

#endif

#if 1
    //Downsample//
	for (int level = m_maxTreeDepth-1; level > 0; level--) {

		//Build dispatch indirect buffer//
		updateDispatchIndirectBuffer(rd, m_dispatchIndirectLevelBuffer, m_levelStartIndexBuffer, m_levelIndexBuffer, level, level, workGroupSize);

		//Launch//

		// Allocate new nodes and clear *their* child pointers to zero
		Args args;
		args.setMacro("WORK_GROUP_SIZE_X",                  workGroupSize.x);
		args.setMacro("WORK_GROUP_SIZE_Y",                  workGroupSize.y);

		connectToShader(args, Access::READ_WRITE, m_maxTreeDepth, level);
		//m_gbuffer->connectToShader("svo", args, Access::READ_WRITE);

		args.setUniform("level",                            level);
		args.setImageUniform("dispatchIndirectLevelBuffer", m_dispatchIndirectLevelBuffer,      Access::READ);	// Used for accessing the number of threads
		//args.setImageUniform("numberOfAllocatedNodes",      m_numberOfAllocatedNodes,           Access::READ_WRITE);

		args.setIndirectBuffer(m_dispatchIndirectLevelBuffer->buffer(), 0);

		LAUNCH_SHADER_WITH_HINT(downSampleShader, args, name());
		//LAUNCH_SHADER("SVO_downsampleValues.glc", args);	//Test


		glMemoryBarrier(GL_ALL_BARRIER_BITS);

	}
#endif

#if 1	//Copy borders
	if(m_useBricks && m_brickBorderSize>0){
		for (int level = 1; level<=m_maxTreeDepth; level++) {

			//Build dispatch indirect buffer//
			updateDispatchIndirectBuffer(rd, m_dispatchIndirectLevelBuffer, m_levelStartIndexBuffer, m_levelIndexBuffer, level, level, workGroupSize);

			//Launch//

			// Allocate new nodes and clear *their* child pointers to zero
			Args args;
			args.setMacro("WORK_GROUP_SIZE_X",                  workGroupSize.x);
			args.setMacro("WORK_GROUP_SIZE_Y",                  workGroupSize.y);

			connectToShader(args, Access::READ_WRITE, m_maxTreeDepth, level);

			args.setUniform("level",                            level);
			args.setImageUniform("dispatchIndirectLevelBuffer", m_dispatchIndirectLevelBuffer,      Access::READ);	// Used for accessing the number of threads

			args.setIndirectBuffer(m_dispatchIndirectLevelBuffer->buffer(), 0);

			LAUNCH_SHADER_WITH_HINT("SVO_copyBrickBorder.glc", args, name());

			glMemoryBarrier(GL_ALL_BARRIER_BITS);

		}
	}
#endif

#if 1 //Downsample Top MipMap
	if(m_useTopMipMap){
		for (int level = m_topMipMapNumLevels-1; level > 0; level--) {

			int levelRes = 1<<level;

			Vector3int32 workGroupSize(4, 4, 4);

			/*workGroupSize.x=min(workGroupSize.x, levelRes);
			workGroupSize.y=min(workGroupSize.y, levelRes);
			workGroupSize.z=min(workGroupSize.z, levelRes);*/

			Args args;
			args.setMacro("WORK_GROUP_SIZE_X", workGroupSize.x);
			args.setMacro("WORK_GROUP_SIZE_Y", workGroupSize.y);
			args.setMacro("WORK_GROUP_SIZE_Z", workGroupSize.z);

			Vector3int32 gridDim = Vector3int32( iCeil(levelRes/float(workGroupSize.x)), iCeil(levelRes/float(workGroupSize.y)), iCeil(levelRes/float(workGroupSize.z)));
			args.computeGridDim = gridDim;

			args.setUniform("level", level);

			connectToShader(args, Access::READ_WRITE, m_maxTreeDepth, level);

			LAUNCH_SHADER_WITH_HINT("SVO_downsampleTopMipMap.glc", args, name());

			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			debugAssertGLOk();
		}

	}

#endif

#if 1	//Create neighbors pointers
	if(m_useNeighborPointers){
		for (int level = 1; level<m_maxTreeDepth; level++) {

			//Build dispatch indirect buffer//
			updateDispatchIndirectBuffer(rd, m_dispatchIndirectLevelBuffer, m_levelStartIndexBuffer, m_levelIndexBuffer, level, level, workGroupSize);

			//Launch//
			Args args;
			args.setMacro("WORK_GROUP_SIZE_X",                  workGroupSize.x);
			args.setMacro("WORK_GROUP_SIZE_Y",                  workGroupSize.y);

			connectToShader(args, Access::READ_WRITE, m_maxTreeDepth, level);

			args.setUniform("level",                            level);
			args.setImageUniform("dispatchIndirectLevelBuffer", m_dispatchIndirectLevelBuffer,      Access::READ);	// Used for accessing the number of threads

			args.setIndirectBuffer(m_dispatchIndirectLevelBuffer->buffer(), 0);

			LAUNCH_SHADER_WITH_HINT("SVO_writeNeighborPointers.glc", args, name());

			glMemoryBarrier(GL_ALL_BARRIER_BITS);

		}
	}
#endif

	///////////////////////////
	glFinish();
	///////////////////////////
}




CFrame SVO::svoToWorldMatrix() const {
	//Using svo-space coordinates
	CFrame frame;
    m_bounds.getLocalFrame(frame);

	const Vector3& extent = m_bounds.extent();

	// Put the origin in the min corner of the box
    frame.translation -= frame.vectorToWorldSpace(m_bounds.extent() / 2);

	//Scale svo-space to the right BBox size
	frame.rotation *= Matrix3::fromRows(Vector3(extent.x, 0, 0), Vector3(0, extent.y, 0), Vector3(0, 0, extent.z));

	return frame;

}

CFrame SVO::worldToSVOMatrix() const {
	//Using svo-space coordinates
	CFrame frame;
    m_bounds.getLocalFrame(frame);

	frame = frame.inverse();

	const Vector3& extent = m_bounds.extent();

	//Scale svo-space to the right BBox size
	frame.rotation *= Matrix3::fromRows(Vector3(1.0f/extent.x, 0, 0), Vector3(0, 1.0f/extent.y, 0), Vector3(0, 0, 1.0f/extent.z));

	// Put the origin in the min corner of the box
    frame.translation += frame.vectorToWorldSpace(m_bounds.extent() / 2);

	//std::cout<<extent.x<<"\n";



	return frame;

}

void SVO::visualizeNodes(RenderDevice* rd, int treeLevel) const {

	treeLevel=min(treeLevel, m_maxTreeDepth);

    rd->pushState();

	//Fill the indirect structure to visualize current level
	{
		Args args;
		args.setMacro("COMPUTE_DRAW_INDIRECT_BUFFER", "1");	//Should be set ?

		args.setUniform("startIndex",                   treeLevel);
		args.setUniform("endIndex",                     treeLevel);
		args.setUniform("dispatchMaxWidth",             m_maxComputeGridDims.x);
		args.setUniform("dispatchWorkGroupSize",        workGroupSize);
		//args.setImageUniform("countBuffer",             m_levelIndexBuffer,             Access::READ);
		args.setImageUniform("startIndexBuffer",        m_levelStartIndexBuffer,		Access::READ);
		args.setImageUniform("endIndexBuffer",          m_levelIndexBuffer,				Access::READ);

		args.setImageUniform("drawIndirectBuffer",      m_drawIndirectBuffer,			Access::WRITE);

		args.computeGridDim = Vector3int32(1, 1, 1);
		LAUNCH_SHADER_WITH_HINT("SVO/SVO_countToIndirectArgument.glc", args, name());

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	{
        Args args;

        args.setUniform("level",                            treeLevel);
		connectToShader(args, Access::READ, m_maxTreeDepth, treeLevel);


        // The bounds are a cube
        args.setUniform("scale",                            1.0f /*m_bounds.extent().x*/);
        args.setUniform("color",                            Color4(Color3::white(), 1.0));


        rd->setObjectToWorldMatrix( svoToWorldMatrix() );

        args.setPrimitiveType(PrimitiveType::POINTS);
#if 0
		int n= (1<<(treeLevel*3));
		args.setNumIndices(n);
#else
		//Indirect
		args.setIndirectBuffer(m_drawIndirectBuffer->buffer(), 0);
#endif
        rd->apply(m_visualizeNodes, args);

    } rd->popState();
}


void SVO::visualizeFragments(RenderDevice* rd) const {
    static shared_ptr<Shader> pointShader = Shader::fromFiles(System::findDataFile("SVO/SVO_visualizeFragments.vrt"), System::findDataFile("SVO/SVO_visualizeFragments.pix"));
    Args args;
    args.setIndirectBuffer(m_fragmentsDrawIndirectBuffer->buffer());
    args.setPrimitiveType(PrimitiveType::POINTS);
    //args.setUniform("WS_POSITION_buffer",	m_fragmentBuffer->texture(GBuffer::Field::WS_POSITION), Sampler::buffer());
    //args.setUniform("LAMBERTIAN_buffer",	m_fragmentBuffer->texture(GBuffer::Field::LAMBERTIAN), Sampler::buffer());
	args.setUniform("SVO_POSITION_buffer",	m_fragmentBuffer->texture(GBuffer::Field::SVO_POSITION), Sampler::buffer());
	args.setMacro("BUFFER_WIDTH", BUFFER_WIDTH);


    rd->setPointSize(5);

#if 0
	//When using world space coordinates
    rd->setObjectToWorldMatrix( CFrame() );
#else
    rd->setObjectToWorldMatrix( svoToWorldMatrix() );
#endif

    rd->apply(pointShader, args);
}

void SVO::renderRaycasting(RenderDevice* rd, shared_ptr<Texture> m_colorBuffer0, int level, float raycastingConeFactor)  {
	rd->pushState(); {

		Matrix4 proj = rd->projectionMatrix();
		float focalLength=proj[0][0];
	/*
	Args args;
#if 1
	args.setAttributeArray("g3d_Vertex", m_fullscreenQuadVertices);
#else
	args.setAttributeArray("g3d_Vertex", m_cubeVertices);
#endif
	args.setPrimitiveType(PrimitiveType::TRIANGLES);
	*/

		rd->setObjectToWorldMatrix( svoToWorldMatrix() );	//CC

		Matrix4 modelViewMat=Matrix4::identity();
		CoordinateFrame modelViewFrame=rd->modelViewMatrix();
		modelViewMat=modelViewFrame.toMatrix4();
		modelViewMat = modelViewMat.inverse();

		Vector4 rayOrigin4=Vector4(0.0,0.0,0.0,1.0);
		rayOrigin4 = modelViewMat * rayOrigin4;
		rayOrigin4.xyz() = rayOrigin4.xyz()/rayOrigin4.w;
		Vector3 rayOrigin=rayOrigin4.xyz();

#if SVO_USE_CUDA==0
		Args args;

		args.setUniform("focalLength", focalLength);
		args.setUniform("renderRes", rd->viewport().extent() );
		args.setUniform("screenRatio", float(rd->viewport().extent().y)/float(rd->viewport().extent().x) );

		args.setUniform("level", level);

		//SVO//
		connectToShader(args, Access::READ, m_maxTreeDepth, level);

		//m_profiler->endGFX();

		//m_profiler->beginGFX("renderRaycasting");

        rd->setColorWrite(true);
		rd->setDepthWrite(false);
		rd->setDepthTest(RenderDevice::DEPTH_ALWAYS_PASS);
		rd->setStencilTest(RenderDevice::STENCIL_ALWAYS_PASS);

		rd->setCullFace(CullFace(CullFace::FRONT));

		//Set svo-space local coordinates


		//////////////////////////////

		args.setUniform("rayOrigin", rayOrigin );
		args.setUniform("modelViewMat", modelViewMat );

		args.setUniform("raycastingConeFactor", raycastingConeFactor );

		args.setImageUniform("outColorBuffer", m_colorBuffer0, Access::READ_WRITE);


		const Vector2int32 blockSize=Vector2int32(16,16);
		args.setMacro("WORK_GROUP_SIZE_X", blockSize.x);
		args.setMacro("WORK_GROUP_SIZE_Y", blockSize.y);

		//args.setMacro("SVO_TRACING_BLOCK_SIZE_X", blockSize.x);
		//args.setMacro("SVO_TRACING_BLOCK_SIZE_Y", blockSize.y);
		Vector2int32 extent(rd->viewport().extent());
		args.computeGridDim = Vector3int32(extent.x / blockSize.x, extent.y / blockSize.y, 1);

		/////////////////////////////

		LAUNCH_SHADER_WITH_HINT("SVO_renderRaycasting_comp.glc", args, name());

#endif

#if SVO_USE_CUDA

		svoRenderRaycasting_kernel( this, m_colorBuffer0, rayOrigin, modelViewMat, focalLength, level, raycastingConeFactor );

#if 0
		glCopyImageSubDataNV(	m_renderTargetTexture->openGLID(), GL_TEXTURE_2D, 0,
								0, 0, 0,
								m_colorBuffer0->openGLID(), GL_TEXTURE_2D, 0,
								0, 0, 0,
								m_colorBuffer0->width(), m_colorBuffer0->height(), 1);

		std::cout<<m_renderTargetTexture->width()<<" "<<m_renderTargetTexture->height()<<" !!\n";

#else
	//std::cout<<m_colorBuffer0->format()->name()<<" !!\n";
	//Texture::copy(m_renderTargetTexture, m_colorBuffer0, 0, 0, Vector2int16(0,0), CubeFace::POS_X, CubeFace::POS_X, rd, true);
#endif

		debugAssertGLOk();
#endif

	} rd->popState();
	//m_profiler->endGFX();
}

uint64 SVO::getBufferGPUAddress(const shared_ptr<BufferTexture> &buff) const{
	if(!glIsNamedBufferResidentNV(buff->buffer()->glBufferID()))
		glMakeNamedBufferResidentNV( buff->buffer()->glBufferID(), GL_READ_WRITE );

	uint64 gpuAddress=0;
	glGetNamedBufferParameterui64vNV(buff->buffer()->glBufferID(), GL_BUFFER_GPU_ADDRESS_NV, &gpuAddress);

	return gpuAddress;
}


void SVO::saveToDisk(const std::string &fileName){
	std::ofstream dataOut( fileName, std::ios::out | std::ios::binary);

	//dataOut.write((const char *)&(viewPos), sizeof(viewPos));




	dataOut.close();

	std::cout<<"SVO saved\n";
}

void SVO::loadFromDisk(const std::string &fileName){
	std::ifstream dataIn( fileName, std::ios::in | std::ios::binary);

	//dataIn.read((char *)&(viewPos), sizeof(viewPos));



	dataIn.close();

	std::cout<<"SVO loaded\n";
}

void SVO::clearFragmentCounter(){

	//GLPixelTransferBuffer::copy(m_zero->buffer(), m_fragmentCount->buffer(), 1);
	fillBuffer(m_fragmentCount->buffer(), 1, 0);
}

void SVO::fillBuffer(shared_ptr<GLPixelTransferBuffer> buffer, uint numVal, uint val){

	//GLPixelTransferBuffer::copy(m_zero->buffer(), m_fragmentCount->buffer(), 1);
	{

		Args args;

		args.setMacro("WORK_GROUP_SIZE_X", workGroupSize.x);
		args.setMacro("WORK_GROUP_SIZE_Y", workGroupSize.y);

		args.setUniform("size", numVal);
		args.setUniform("value", val);

		args.setUniform("d_dataBuffer", buffer->getGPUAddress());

		Vector3int32 gridDim = Vector3int32(iCeil(numVal / float(workGroupSize.x*workGroupSize.y)), 1, 1);
		args.computeGridDim = gridDim;

		LAUNCH_SHADER_WITH_HINT("SVO/SVO_fillBuffer.glc", args, name());

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

}


void SVO::debugPrintIndexBuffer(){
#if 0
	glFinish();
	{

		uint32* ptrStart = (uint32*)m_levelStartIndexBuffer->buffer()->mapRead();
		uint32* ptr = (uint32*)m_levelIndexBuffer->buffer()->mapRead();


		std::cout << "levelIndexBuffer:\n";

		for (int i = 0; i <= m_maxTreeDepth; i++){

			int prevLevelEndIdx = ptrStart[i];

			//m_levelsNumNodes[i] = ptr[i] - prevLevelEndIdx;

			std::cout << "(" << ptrStart[i] << ", " << ptr[i] << ") ";
		}
		std::cout << "\n";
		m_levelIndexBuffer->buffer()->unmap();
		m_levelStartIndexBuffer->buffer()->unmap();
	}
#endif
}

void SVO::debugPrintRootIndexBuffer(){
#if 0
	glFinish();
	{

		uint32* ptr = (uint32*)m_rootIndex->buffer()->mapRead();

		std::cout << "rootIndexBuffer:\n( ";

		for (int i = 0; i <= (SVO_MAX_NUM_VOLUMES * SVO_TOP_MIPMAP_NUM_LEVELS); i++){

			uint rootIdx = ptr[i];


			std::cout << rootIdx << ", ";
		}
		std::cout << " )\n";
		m_rootIndex->buffer()->unmap();

	}
#endif
}

void SVO::printDebugBuild(){
#if 0
	glFinish();
	{
		uint32* ptr = (uint32*)m_levelStartIndexBuffer->buffer()->mapRead();
		std::cout<<"m_levelStartIndexBuffer:\n";
		for(int i=0; i<=m_maxTreeDepth; i++){
			std::cout<<ptr[i]<<" ";
		}
		std::cout<<"\n";
		m_levelStartIndexBuffer->buffer()->unmap();
	}

	{
		uint32* ptr = (uint32*)m_levelIndexBuffer->buffer()->mapRead();
		std::cout<<"m_levelIndexBuffer:\n";
		for(int i=0; i<=m_maxTreeDepth; i++){
			std::cout<<ptr[i]<<" ";
		}
		std::cout<<"\n";
		m_levelIndexBuffer->buffer()->unmap();
	}

	{
		uint32* ptr = (uint32*)m_levelSizeBuffer->buffer()->mapRead();
		std::cout<<"m_levelSizeBuffer:\n";
		for(int i=0; i<=m_maxTreeDepth; i++){
			std::cout<<ptr[i]<<" ";
		}
		std::cout<<"\n";
		m_levelSizeBuffer->buffer()->unmap();
	}

	{
		uint32* ptr = (uint32*)m_numberOfAllocatedNodes->buffer()->mapRead();
		std::cout<<"m_numberOfAllocatedNodes:\n";
		std::cout<<ptr[0]*8<<"\n";
		m_numberOfAllocatedNodes->buffer()->unmap();
	}

	glFinish();
#endif
}

} // namespace G3D

