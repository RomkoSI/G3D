/** \file App.cpp */
#include "App.h"
#include "testassert.h"

static const String RENDER_TEST_DIRECTORY      = "../data-files/RenderTest/";
static const String RESULT_DIRECTORY           = RENDER_TEST_DIRECTORY + "Results/";
static const String GOLD_STANDARD_DIRECTORY    = RENDER_TEST_DIRECTORY + "GoldStandard/";
static const String DIFF_DIRECTORY             = RENDER_TEST_DIRECTORY + "Diffs/";

static const Array<String> TEST_SCENE_LIST("G3D Cornell Box", "G3D Sponza", "G3D Feature Test");

App::App(const GApp::Settings& settings) : GApp(settings) {
}


static void clearDirectory(const String& path) {
    FileSystem::removeFile(path + "*");
}


bool App::setupGoldStandardMode() const {
    // Assumptions abound
    return settings().screenshotDirectory.find("GoldStandard") != String::npos;
}

static bool testHarnessFailureHook(
    const char* _expression,
    const String& message,
    const char* filename,
    int lineNumber,
    bool useGuiPrompt) {
    fprintf(stderr, "%s:%d\n", _expression, lineNumber);
    fprintf(stderr, "%s\n", _expression);
    fprintf(stderr, "%s\n", message.c_str());
    return true;
}


void App::onInit() {
    GApp::onInit();
    Shader::setFailureBehavior(Shader::EXCEPTION);
    setFailureHook(testHarnessFailureHook);
    m_frameCount = 0;
    m_success = true;
    showRenderingStats    = false;

    createDeveloperHUD();
    // For higher-quality screenshots:
    developerWindow->videoRecordDialog->setScreenShotFormat("PNG");

    developerWindow->cameraControlWindow->moveTo(Point2(developerWindow->cameraControlWindow->rect().x0(), 0));

    if (setupGoldStandardMode()) {
        clearDirectory(GOLD_STANDARD_DIRECTORY);
    } 

    clearDirectory(RESULT_DIRECTORY);
    clearDirectory(DIFF_DIRECTORY);


    m_sceneIndex = 0;
    loadScene(TEST_SCENE_LIST[m_sceneIndex]);
}


static Vector3 maxDifference(const shared_ptr<Image>& im0, const shared_ptr<Image>& im1) {
    testAssertM(notNull(im0) && notNull(im1), "Either comparison or gold standard image was not loaded");
    testAssertM(im0->bounds() == im1->bounds(), "Gold Standard and comparison image are not the same size!");
    Vector3 maxDiff = Vector3::zero();
    Point2int32 p(0,0);
    for (p.y = 0; p.y < im0->height(); ++p.y) {
        for (p.x = 0; p.x < im0->width(); ++p.x) {
            Color3 c0, c1;
            im0->get(p, c0);
            im1->get(p, c1);
            Color3 diff = c1-c0;
            for(int i = 0; i < 3; ++i) {
	      maxDiff[i] = max(maxDiff[i], (float)abs(diff[i]));
            }
        }
    }
    return maxDiff;
}

static shared_ptr<Image> diffImage(const shared_ptr<Image>& im0, const shared_ptr<Image>& im1, float scale) {
    testAssertM(notNull(im0) && notNull(im1), "Either comparison or gold standard image was not loaded");
    testAssertM(im0->bounds() == im1->bounds(), "Gold Standard and comparison image are not the same size!");
    shared_ptr<Image> diffIm = Image::create(im0->width(), im0->height(), im0->format());
    Point2int32 p(0,0);
    for (p.y = 0; p.y < im0->height(); ++p.y) {
        for (p.x = 0; p.x < im0->width(); ++p.x) {
            Color3 c0, c1;
            im0->get(p, c0);
            im1->get(p, c1);
            Color3 diff = c1-c0;
            Color3 absDiff;
            for(int i = 0; i < 3; ++i) {
                absDiff[i] = abs(diff[i]);
            }
            diffIm->set(p, absDiff * scale);
        }
    }
    return diffIm;
}

static bool compareToGoldStandard(const String& name) {
    String filename = name + ".exr";
    shared_ptr<Image> comparisonImage   = Image::fromFile(RESULT_DIRECTORY + filename);
    shared_ptr<Image> goldStandard      = Image::fromFile(GOLD_STANDARD_DIRECTORY + filename);
    Vector3 maxDiff = maxDifference(comparisonImage, goldStandard);

    if (maxDiff.max() > 0.0f) {
        float multiplier = 1.0f / maxDiff.max();
        shared_ptr<Image> differenceImage = diffImage(comparisonImage, goldStandard, multiplier);
        differenceImage->convert(ImageFormat::RGB8()); 
        // TODO: Save multiplier somewhere...
        differenceImage->save(format("%s%s-diff_X%4.2f.png", DIFF_DIRECTORY.c_str(), name.c_str(), multiplier));
        
        return false;
    }
    return true;
}

static String canonicalizeSceneName(const String& name) {
    return replace(name, " ", "_");
}

void App::saveAndPossiblyCompareTextureToGoldStandard(const String& name, const shared_ptr<Texture>& texture) {
    shared_ptr<Image> im = Image::fromPixelTransferBuffer(texture->toPixelTransferBuffer());
    im->save(settings().screenshotDirectory + name + ".exr");
    if ( !setupGoldStandardMode() ) {
	bool result = compareToGoldStandard(name);
        m_success = m_success && result;
    }
}

void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& allSurfaces) {

    GApp::onGraphics3D(rd, allSurfaces);

    static shared_ptr<Texture> postRenderTexture;
    static shared_ptr<Texture> postRenderAndDoFTexture;
    static shared_ptr<Texture> postRenderAndDoFAndMotionBlurTexture;
    static shared_ptr<Texture> aoTexture = Texture::createEmpty("AO Save Texture", 2, 2, ImageFormat::RGB32F());

    ++m_frameCount;
    if (m_frameCount > 3) {
        shared_ptr<Texture> resultTexture;

        m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_framebuffer->texture(0), resultTexture);

        const String& name = canonicalizeSceneName(scene()->name());
        saveAndPossiblyCompareTextureToGoldStandard(name, resultTexture);

        Texture::copy(m_ambientOcclusion->texture(), aoTexture);

        const String& aoName = name + "_AOBuffer";
        saveAndPossiblyCompareTextureToGoldStandard(aoName, aoTexture);
        #if 0 //TODO: Implement
        const String& prePostProcessName = name + "_prePostProcess";
        saveAndPossiblyCompareTextureToGoldStandard(prePostProcessName, postRenderTexture);

        const String& postDoFName = name + "_postDoF";
        saveAndPossiblyCompareTextureToGoldStandard(postDoFName, postRenderAndDoFTexture);

        const String& postDoFAndMotionBlur = name + "_postDoFAndMotionBlur";
        saveAndPossiblyCompareTextureToGoldStandard(postDoFAndMotionBlur, postRenderAndDoFAndMotionBlurTexture);
        #endif
        ++m_sceneIndex;
        if (m_sceneIndex < TEST_SCENE_LIST.size()) {
            m_frameCount = 0; // Reset frame count
            loadScene(TEST_SCENE_LIST[m_sceneIndex]); // load next test scene
        } else {
            m_endProgram = true;
        }
    }
}

