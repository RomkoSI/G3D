#if 0


class ImageLoader : public GThread {
public:
    typedef shared_ptr<ImageLoader> Ref;

    const std::string filename;
    GLuint  pbo;
    GLvoid* ptr;
    GImage  im;
    int     sidePixels;

    /** Must be invoked on the OpenGL thread */
    ImageLoader(const std::string& filename, int sidePixels) : GThread(filename), filename(filename), sidePixels(sidePixels) {
        glGenBuffers(1, &pbo);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
        size_t size = sidePixels * sidePixels * 3;
        glBufferData(GL_PIXEL_UNPACK_BUFFER, size, NULL, GL_STREAM_DRAW);
        ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);
    }

    virtual void threadMain() {
        im.load(filename);
        debugAssert(im.width() == sidePixels);
        debugAssert(im.height() == sidePixels);
        debugAssert(im.channels() == 3);
        System::memcpy(ptr, im.byte(), im.width() * im.height() * im.channels());
    }

    /** Call on the GL thread after threadMain is done */
    void unmap() {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);
        ptr = NULL;
    }

    /** Must be invoked on the OpenGL thread */
    virtual ~ImageLoader() {
        glDeleteBuffers(1, &pbo);
        pbo = GL_NONE;
    }
};

shared_ptr<Texture> sky;
shared_ptr<Texture> tex;

void loadt2D() {    
    Stopwatch stopwatch;

    // Load an image from disk.  It is 2048^2 x 3
    GImage image("D:/morgan/g3d/data/cubemap/sky_skylab_01/sky_skylab_01bk.png");
    stopwatch.after("Load from disk");

    // Create an empty texture in bilinear-nearest-nearest mode, with MIP-map generation disabled
    tex = Texture::createEmpty("tex", image.width(), image.height(), ImageFormat::RGB8(), Texture::DIM_2D, Texture::Settings::buffer());
    stopwatch.after("Create GL texture");

    // Create a pbo
    GLuint pbo;
    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    stopwatch.after("Create GL PBO");

    // Allocate memory on the GPU
    const size_t size = image.width() * image.height() * 3;
    glBufferData(GL_PIXEL_UNPACK_BUFFER, size, NULL, GL_STREAM_DRAW);
    stopwatch.after("Allocate PBO space");

    // Map the memory to the CPU
    GLvoid* ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    stopwatch.after("Map PBO");

    // Copy data to the mapped memory (using SIMD wide loads and stores)
    System::memcpy(ptr, image.byte(), size);
    stopwatch.after("Memcpy");

    // Unmap the memory
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    ptr = NULL;
    stopwatch.after("Unmap PBO");
    // Even if we sleep after unmapping the PBO, the following glTexImage2D still takes 0.14 s
    // System::sleep(1);
    // stopwatch.after("sleep");

    // Copy the PBO to the texture
    glBindTexture(tex->openGLTextureTarget(), tex->openGLID());
//    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image.width(), image.height(), GL_RGB, GL_BYTE, (GLvoid*)0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, image.width(), image.height(), 0, GL_RGB, GL_BYTE, (GLvoid*)0);
    glBindTexture(tex->openGLTextureTarget(), GL_NONE);
    stopwatch.after("glTexImage2D");

    // Unbind PBO
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);
    stopwatch.after("Unbind PBO");

    // Delete the PBO
    glDeleteBuffers(1, &pbo);
    pbo = GL_NONE;
    stopwatch.after("Delete PBO");
}


void test() {

    stopwatch.tick();

    Texture::Settings settings = Texture::Settings::cubeMap();
    settings.interpolateMode = Texture::BILINEAR_NO_MIPMAP;
    sky = Texture::createEmpty("cubemap", 0, 0, ImageFormat::RGB8(), Texture::DIM_CUBE_MAP, settings);


    // Using multiple threads cuts the load time from disk from 0.9s to 0.6s.
    // Using PBO and multiple threads for DMA has no measurable impact on performance over single-threaded glTexImage2D from
    // the CPU memory--both take about 1.5s.
    {
        ThreadSet threadSet;
        int w = 2048;
        Array<ImageLoader::Ref > image;
        image.append(new ImageLoader("D:/morgan/g3d/data/cubemap/sky_skylab_01/sky_skylab_01bk.png", w));
        image.append(new ImageLoader("D:/morgan/g3d/data/cubemap/sky_skylab_01/sky_skylab_01dn.png", w));
        image.append(new ImageLoader("D:/morgan/g3d/data/cubemap/sky_skylab_01/sky_skylab_01lf.png", w));
        image.append(new ImageLoader("D:/morgan/g3d/data/cubemap/sky_skylab_01/sky_skylab_01ft.png", w));
        image.append(new ImageLoader("D:/morgan/g3d/data/cubemap/sky_skylab_01/sky_skylab_01rt.png", w));
        image.append(new ImageLoader("D:/morgan/g3d/data/cubemap/sky_skylab_01/sky_skylab_01up.png", w));
        for (int i = 0; i < 6; ++i) {
            threadSet.insert(image[i]);
        }

        // Run the threads, reusing the current thread and blocking until
        // all complete
        threadSet.start();

        // On the main thread, resize the textures while uploading happens on the other threads
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);
        glBindTexture(sky->openGLTextureTarget(), sky->openGLID());
        for (int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i, 0, GL_RGB8, w, w, 0, GL_RGB, GL_BYTE, (GLvoid*)0);
        }
        glBindTexture(sky->openGLTextureTarget(), GL_NONE);

        threadSet.waitForCompletion();

        for (int i = 0; i < 6; ++i) {
            image[i]->unmap();
        }

        // Copy PBOs to GL texture faces
        glBindTexture(sky->openGLTextureTarget(), sky->openGLID());
        for (int i = 0; i < 6; ++i) {
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, image[i]->pbo);
            glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i, 0, 0, 0, w, w, GL_RGB, GL_BYTE, (GLvoid*)0);
        }
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);
        glBindTexture(sky->openGLTextureTarget(), GL_NONE);
    }


    /*
    Texture::Settings settings  = Texture::Settings::cubeMap();
    settings.interpolateMode = Texture::BILINEAR_NO_MIPMAP;
    shared_ptr<Texture> t = Texture::fromFile("D:/morgan/g3d/data/cubemap/sky_skylab_01/sky_skylab_01*.png", ImageFormat::AUTO(), Texture::DIM_CUBE_MAP, settings, skyprocess);
    */
    stopwatch.tock();
    debugPrintf("PBO thread: %fs\n", stopwatch.elapsedTime());
    /*
    stopwatch.tick();
    sky = Texture::fromFile("D:/morgan/g3d/data/cubemap/sky_skylab_01/sky_skylab_01*.png", ImageFormat::RGB8(), Texture::DIM_CUBE_MAP, settings);
    stopwatch.tock();
    debugPrintf("Texture: %fs\n", stopwatch.elapsedTime());
    */
}
#endif
