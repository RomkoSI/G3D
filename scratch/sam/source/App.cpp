/** \file App.cpp */
#include "App.h"
#define WINDOWS 1
extern "C" {
#   include <GLFW/glfw3.h>
#   ifdef WINDOWS
#   define GLFW_EXPOSE_NATIVE_WIN32
#   define GLFW_EXPOSE_NATIVE_WGL
#   include <GLFW/glfw3native.h>
#   endif
}
#include "GLG3D/GLFWWindow.h"
#include <GL/glew.h>

#ifdef WINDOWS
#   include "GL/wglew.h"
#endif
// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();
unsigned int fbo = 0, fb_tex = 0, fb_depth = 0;
unsigned int chess_tex = 0;
int fb_width = 0, fb_height = 0;
int fb_tex_width = 0, fb_tex_height = 0;

ovrHmd                  HMD;
ovrSizei                eyeres[2];
ovrEyeRenderDesc        eye_rdesc[2];
ovrGLTexture            fb_ovr_tex[2];

static unsigned int next_pow2(unsigned int x) {
    x -= 1;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}
void draw_box(float xsz, float ysz, float zsz, float norm_sign) {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glScalef(xsz * 0.5, ysz * 0.5, zsz * 0.5);

    if (norm_sign < 0.0) {
        glFrontFace(GL_CW);
    }

    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1 * norm_sign);
    glTexCoord2f(0, 0); glVertex3f(-1, -1, 1);
    glTexCoord2f(1, 0); glVertex3f(1, -1, 1);
    glTexCoord2f(1, 1); glVertex3f(1, 1, 1);
    glTexCoord2f(0, 1); glVertex3f(-1, 1, 1);
    glNormal3f(1 * norm_sign, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(1, -1, 1);
    glTexCoord2f(1, 0); glVertex3f(1, -1, -1);
    glTexCoord2f(1, 1); glVertex3f(1, 1, -1);
    glTexCoord2f(0, 1); glVertex3f(1, 1, 1);
    glNormal3f(0, 0, -1 * norm_sign);
    glTexCoord2f(0, 0); glVertex3f(1, -1, -1);
    glTexCoord2f(1, 0); glVertex3f(-1, -1, -1);
    glTexCoord2f(1, 1); glVertex3f(-1, 1, -1);
    glTexCoord2f(0, 1); glVertex3f(1, 1, -1);
    glNormal3f(-1 * norm_sign, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(-1, -1, -1);
    glTexCoord2f(1, 0); glVertex3f(-1, -1, 1);
    glTexCoord2f(1, 1); glVertex3f(-1, 1, 1);
    glTexCoord2f(0, 1); glVertex3f(-1, 1, -1);
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1 * norm_sign, 0);
    glTexCoord2f(0.5, 0.5); glVertex3f(0, 1, 0);
    glTexCoord2f(0, 0); glVertex3f(-1, 1, 1);
    glTexCoord2f(1, 0); glVertex3f(1, 1, 1);
    glTexCoord2f(1, 1); glVertex3f(1, 1, -1);
    glTexCoord2f(0, 1); glVertex3f(-1, 1, -1);
    glTexCoord2f(0, 0); glVertex3f(-1, 1, 1);
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, -1 * norm_sign, 0);
    glTexCoord2f(0.5, 0.5); glVertex3f(0, -1, 0);
    glTexCoord2f(0, 0); glVertex3f(-1, -1, -1);
    glTexCoord2f(1, 0); glVertex3f(1, -1, -1);
    glTexCoord2f(1, 1); glVertex3f(1, -1, 1);
    glTexCoord2f(0, 1); glVertex3f(-1, -1, 1);
    glTexCoord2f(0, 0); glVertex3f(-1, -1, -1);
    glEnd();

    glFrontFace(GL_CCW);
    glPopMatrix();
}
void draw_scene(void) {
    int i;
    float grey[] = { 0.8, 0.8, 0.8, 1 };
    float col[] = { 0, 0, 0, 1 };
    float lpos[][4] = {
        { -8, 2, 10, 1 },
        { 0, 15, 0, 1 }
    };

    float lcol[][4] = {
        { 0.8, 0.8, 0.8, 1 },
        { 0.4, 0.3, 0.3, 1 }
    };

    for (i = 0; i < 2; ++i) {
        glLightfv(GL_LIGHT0 + i, GL_POSITION, lpos[i]);
        glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, lcol[i]);
    }

    glMatrixMode(GL_MODELVIEW);

    glPushMatrix();
    glTranslatef(0, 10, 0);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, grey);
    glBindTexture(GL_TEXTURE_2D, chess_tex);
    glEnable(GL_TEXTURE_2D);
    draw_box(30, 20, 30, -1.0);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();

    for (i = 0; i < 4; ++i) {
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, grey);
        glPushMatrix();
        glTranslatef(i & 1 ? 5 : -5, 1, i & 2 ? -5 : 5);
        draw_box(0.5, 2, 0.5, 1.0);
        glPopMatrix();

        col[0] = i & 1 ? 1.0 : 0.3;
        col[1] = i == 0 ? 1.0 : 0.3;
        col[2] = i & 2 ? 1.0 : 0.3;
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, col);

        glPushMatrix();
        if (i & 1) {
            glTranslatef(0, 0.25, i & 2 ? 2 : -2);
        }
        else {
            glTranslatef(i & 2 ? 2 : -2, 0.25, 0);
        }
        draw_box(0.5, 0.5, 0.5, 1.0);
        glPopMatrix();
    }

    col[0] = 1;
    col[1] = 1;
    col[2] = 0.4;
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, col);
    draw_box(0.05, 1.2, 6, 1.0);
    draw_box(6, 1.2, 0.05, 1.0);
}
unsigned int gen_chess_tex(float r0, float g0, float b0, float r1, float g1, float b1) {
    int i, j;
    unsigned int tex;
    unsigned char img[8 * 8 * 3];
    unsigned char *pix = img;

    for (i = 0; i<8; i++) {
        for (j = 0; j<8; j++) {
            int black = (i & 1) == (j & 1);
            pix[0] = (black ? r0 : r1) * 255;
            pix[1] = (black ? g0 : g1) * 255;
            pix[2] = (black ? b0 : b1) * 255;
            pix += 3;
        }
    }

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8, 8, 0, GL_RGB, GL_UNSIGNED_BYTE, img);

    return tex;
}
void quat_to_matrix(const float *quat, float *mat) {
    mat[0] = 1.0 - 2.0 * quat[1] * quat[1] - 2.0 * quat[2] * quat[2];
    mat[4] = 2.0 * quat[0] * quat[1] + 2.0 * quat[3] * quat[2];
    mat[8] = 2.0 * quat[2] * quat[0] - 2.0 * quat[3] * quat[1];
    mat[12] = 0.0f;

    mat[1] = 2.0 * quat[0] * quat[1] - 2.0 * quat[3] * quat[2];
    mat[5] = 1.0 - 2.0 * quat[0] * quat[0] - 2.0 * quat[2] * quat[2];
    mat[9] = 2.0 * quat[1] * quat[2] + 2.0 * quat[3] * quat[0];
    mat[13] = 0.0f;

    mat[2] = 2.0 * quat[2] * quat[0] + 2.0 * quat[3] * quat[1];
    mat[6] = 2.0 * quat[1] * quat[2] - 2.0 * quat[3] * quat[0];
    mat[10] = 1.0 - 2.0 * quat[0] * quat[0] - 2.0 * quat[1] * quat[1];
    mat[14] = 0.0f;

    mat[3] = mat[7] = mat[11] = 0.0f;
    mat[15] = 1.0f;
}

void update_rtarg(int width, int height) {
    if (!fbo) {
        /* if fbo does not exist, then nothing does... create every opengl object */
        glGenFramebuffers(1, &fbo);


        glGenTextures(1, &fb_tex);
        glGenRenderbuffers(1, &fb_depth);

        glBindTexture(GL_TEXTURE_2D, fb_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    /* calculate the next power of two in both dimensions and use that as a texture size */
    fb_tex_width = next_pow2(width);
    fb_tex_height = next_pow2(height);

    /* create and attach the texture that will be used as a color buffer */
    glBindTexture(GL_TEXTURE_2D, fb_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fb_tex_width, fb_tex_height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb_tex, 0);

    /* create and attach the renderbuffer that will serve as our z-buffer */
    glBindRenderbuffer(GL_RENDERBUFFER, fb_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fb_tex_width, fb_tex_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fb_depth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "incomplete framebuffer!\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    printf("created render target: %dx%d (texture size: %dx%d)\n", width, height, fb_tex_width, fb_tex_height);
}

int main(int argc, const char* argv[]) {
    {
        G3DSpecification g3dSpec;
        g3dSpec.audio = false;
        initGLG3D(g3dSpec);
    }

    GApp::Settings settings(argc, argv);

    // Change the window and other startup parameters by modifying the
    // settings class.  For example:
    settings.window.caption = argv[0];

    // Some popular resolutions:
    //settings.window.width        = 640;  settings.window.height       = 400;
    // settings.window.width		= 1024; settings.window.height       = 768;
    settings.window.width = 1280; settings.window.height = 720;
    // settings.window.width        = 1920; settings.window.height       = 1080;
    // settings.window.width		= OSWindow::primaryDisplayWindowSize().x; settings.window.height = OSWindow::primaryDisplayWindowSize().y;

    // Set to true for a significant performance boost if your app can't render at 60fps,
    // or if you *want* to render faster than the display.
    settings.window.asynchronous = true;
    settings.depthGuardBandThickness = Vector2int16(0, 0);
    settings.colorGuardBandThickness = Vector2int16(0, 0);
    settings.dataDir = FileSystem::currentDirectory();
    //settings.screenshotDirectory	    = "../journal/";
    if (!ovr_Initialize()) { MessageBoxA(NULL, "Unable to initialize libOVR.", "", MB_OK); exit(0); }

    HMD = ovrHmd_Create(0);
    if (HMD == NULL)
    {
        HMD = ovrHmd_CreateDebug(ovrHmd_DK2);
    }
    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
}



void App::onInit() {
    GApp::onInit();



    // setFrameDuration(1.0f / 60.0f);
    if (!HMD) { MessageBoxA(NULL, "Oculus Rift not detected.", "", MB_OK); ovr_Shutdown(); exit(0); }
    if (HMD->ProductName[0] == '\0') MessageBoxA(NULL, "Rift detected, display not enabled.", "", MB_OK);


    //   GLFWwindow* window = glfwCreateWindow(640, 480, "", NULL, NULL);
    //  glfwMakeContextCurrent(window);
    //  glfwSetWindowSize(window, HMD->Resolution.w, HMD->Resolution.h);
    //  showRenderingStats      = false;
    // m_showWireframe         = false;

    //  makeGUI();
    // m_lastLightingChangeTime = 0.0;

    //    developerWindow->cameraControlWindow->moveTo(Point2(developerWindow->cameraControlWindow->rect().x0(), 0));

    bool windowed = (HMD->HmdCaps & ovrHmdCap_ExtendDesktop) ? false : true;
    ((GLFWWindow*)window())->setSize(HMD->Resolution.w, HMD->Resolution.h);
    ((GLFWWindow*)window())->reallyMakeCurrent();

    ovrHmd_ConfigureTracking(HMD, ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection |
        ovrTrackingCap_Position, 0);


    eyeres[0] = ovrHmd_GetFovTextureSize(HMD, ovrEye_Left, HMD->DefaultEyeFov[0], 1.0);
    eyeres[1] = ovrHmd_GetFovTextureSize(HMD, ovrEye_Right, HMD->DefaultEyeFov[1], 1.0);

    fb_width = eyeres[0].w + eyeres[1].w;
    fb_height = eyeres[0].h > eyeres[1].h ? eyeres[0].h : eyeres[1].h;
    update_rtarg(fb_width, fb_height);


    for (int i = 0; i < 2; ++i) {
        fb_ovr_tex[i].OGL.Header.API = ovrRenderAPI_OpenGL;
        fb_ovr_tex[i].OGL.Header.TextureSize.w = fb_tex_width;
        fb_ovr_tex[i].OGL.Header.TextureSize.h = fb_tex_height;
        /* this next field is the only one that differs between the two eyes */
        fb_ovr_tex[i].OGL.Header.RenderViewport.Pos.x = (i == 0) ? 0 : (fb_width / 2);
        fb_ovr_tex[i].OGL.Header.RenderViewport.Pos.y = 0;
        fb_ovr_tex[i].OGL.Header.RenderViewport.Size.w = fb_width / 2;
        fb_ovr_tex[i].OGL.Header.RenderViewport.Size.h = fb_height;
        fb_ovr_tex[i].OGL.TexId = fb_tex;	/* both eyes will use the same texture id */
    }

    ovrGLConfig config;
    config.OGL.Header.API = ovrRenderAPI_OpenGL;
    config.OGL.Header.BackBufferSize = HMD->Resolution;
    config.OGL.Header.Multisample = 0;
    config.OGL.Window = GetActiveWindow();
    config.OGL.DC = wglGetCurrentDC();

    ovrHmd_AttachToWindow(HMD, config.OGL.Window, 0, 0);
    ovrHmd_SetEnabledCaps(HMD, ovrHmdCap_LowPersistence | ovrHmdCap_DynamicPrediction);

    ovrEyeRenderDesc EyeRenderDesc[2];
    ovrHmd_ConfigureRendering(HMD, &config.Config, ovrDistortionCap_Vignette | ovrDistortionCap_TimeWarp | ovrDistortionCap_Overdrive, HMD->DefaultEyeFov, EyeRenderDesc);


    if (!ovrHmd_ConfigureRendering(HMD, &config.Config, ovrDistortionCap_Vignette | ovrDistortionCap_TimeWarp | ovrDistortionCap_Overdrive, HMD->DefaultEyeFov, eye_rdesc)) {
        fprintf(stderr, "failed to configure distortion renderer\n");
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_NORMALIZE);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    chess_tex = gen_chess_tex(1.0, 0.7, 0.4, 0.4, 0.7, 1.0);
    Array<shared_ptr<Surface> > a;
    while (true) {
        onGraphics3D(renderDevice, a);
        ovrHSWDisplayState hsw;
        ovrHmd_GetHSWDisplayState(HMD, &hsw);
        if (hsw.Displayed) {
            ovrHmd_DismissHSWDisplay(HMD);
        }
    }
    /*loadScene(
    "G3D Cornell Box" // Load something simple
    //    developerWindow->sceneEditorWindow->selectedSceneName()  // Load the first scene encountered
    );
    dynamic_pointer_cast<DefaultRenderer>(m_renderer)->setOrderIndependentTransparency(false);*/
}


void App::makeGUI() {
    // Initialize the developer HUD (using the existing scene)
    createDeveloperHUD();
    debugWindow->setVisible(true);
    developerWindow->videoRecordDialog->setEnabled(true);

    GuiPane* infoPane = debugPane->addPane("Info", GuiTheme::ORNATE_PANE_STYLE);
    infoPane->addCheckBox("Show wireframe", &m_showWireframe);

    // Example of how to add debugging controls
    infoPane->addLabel("You can add more GUI controls");
    infoPane->addLabel("in App::onInit().");
    infoPane->addButton("Exit", this, &App::endProgram);
    infoPane->pack();



    debugWindow->pack();
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
}

static int id = 0;
void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& allSurfaces) {


    int i;
    ovrMatrix4f proj;
    ovrPosef pose[2];
    float rot_mat[16];

    /* the drawing starts with a call to ovrHmd_BeginFrame */
    ovrHmd_BeginFrame(HMD, 0);

    /* start drawing onto our texture render target */
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* for each eye ... */
    for (i = 0; i<2; i++) {
        ovrEyeType eye = HMD->EyeRenderOrder[i];

        /* -- viewport transformation --
        * setup the viewport to draw in the left half of the framebuffer when we're
        * rendering the left eye's view (0, 0, width/2, height), and in the right half
        * of the framebuffer for the right eye's view (width/2, 0, width/2, height)
        */
        glViewport(eye == ovrEye_Left ? 0 : fb_width / 2, 0, fb_width / 2, fb_height);

        /* -- projection transformation --
        * we'll just have to use the projection matrix supplied by the oculus SDK for this eye
        * note that libovr matrices are the transpose of what OpenGL expects, so we have to
        * use glLoadTransposeMatrixf instead of glLoadMatrixf to load it.
        */
        proj = ovrMatrix4f_Projection(HMD->DefaultEyeFov[eye], 0.5, 500.0, 1);
        glMatrixMode(GL_PROJECTION);
        glLoadTransposeMatrixf(proj.M[0]);

        /* -- view/camera transformation --
        * we need to construct a view matrix by combining all the information provided by the oculus
        * SDK, about the position and orientation of the user's head in the world.
        */
        /* TODO: use ovrHmd_GetEyePoses out of the loop instead */

        pose[eye] = ovrHmd_GetHmdPosePerEye(HMD, eye);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(eye_rdesc[eye].HmdToEyeViewOffset.x,
            eye_rdesc[eye].HmdToEyeViewOffset.y,
            eye_rdesc[eye].HmdToEyeViewOffset.z);
        /* retrieve the orientation quaternion and convert it to a rotation matrix */
        quat_to_matrix(&pose[eye].Orientation.x, rot_mat);
        glMultMatrixf(rot_mat);
        /* translate the view matrix with the positional tracking */
        glTranslatef(-pose[eye].Position.x, -pose[eye].Position.y, -pose[eye].Position.z);
        /* move the camera to the eye level of the user */
        glTranslatef(0, -ovrHmd_GetFloat(HMD, OVR_KEY_EYE_HEIGHT, 1.65), 0);

        /* finally draw the scene for this eye */
        draw_scene();

    }

    /* after drawing both eyes into the texture render target, revert to drawing directly to the
    * display, and we call ovrHmd_EndFrame, to let the Oculus SDK draw both images properly
    * compensated for lens distortion and chromatic abberation onto the HMD screen.
    */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ovrHmd_EndFrame(HMD, pose, &fb_ovr_tex[0].Texture);

    /* workaround for the oculus sdk distortion renderer bug, which uses a shader
    * program, and doesn't restore the original binding when it's done.
    */
    glUseProgram(0);

    assert(glGetError() == GL_NO_ERROR);
#if 0
    m_gbuffer->setSpecification(m_gbufferSpecification);
    m_gbuffer->resize(fb_tex_width, fb_tex_height);
    m_gbuffer->prepare(rd, activeCamera(), 0, -(float)previousSimTimeStep(), m_settings.depthGuardBandThickness, m_settings.colorGuardBandThickness);

    ovrPosef pose[2];
    m_framebuffer->resize(fb_tex_width, fb_tex_height);


    ovrHmd_BeginFrame(HMD, 0);

    for (int i = 0; i < 2; ++i) {
        ovrEyeType eye = HMD->EyeRenderOrder[i];
        pose[eye] = ovrHmd_GetHmdPosePerEye(HMD, eye);
        // fb_ovr_tex[i].OGL.TexId = m_framebuffer->openGLID();
    }
    /*
    m_renderer->render(rd, m_framebuffer, m_depthPeelFramebuffer, scene()->lightingEnvironment(), m_gbuffer, allSurfaces);

    // Debug visualizations and post-process effects
    rd->pushState(m_framebuffer); {
    // Call to make the App show the output of debugDraw(...)
    drawDebugShapes();
    const shared_ptr<Entity>& selectedEntity = (notNull(developerWindow) && notNull(developerWindow->sceneEditorWindow)) ? developerWindow->sceneEditorWindow->selectedEntity() : shared_ptr<Entity>();
    scene()->visualize(rd, selectedEntity, sceneVisualizationSettings());

    // Post-process special effects
    m_depthOfField->apply(rd, m_framebuffer->texture(0), m_framebuffer->texture(Framebuffer::DEPTH), activeCamera(), m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness);

    m_motionBlur->apply(rd, m_framebuffer->texture(0), m_gbuffer->texture(GBuffer::Field::SS_EXPRESSIVE_MOTION),
    m_framebuffer->texture(Framebuffer::DEPTH), activeCamera(),
    m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness);
    } rd->popState();

    if ((submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) && (!renderDevice->swapBuffersAutomatically())) {
    // We're about to render to the actual back buffer, so swap the buffers now.
    // This call also allows the screenshot and video recording to capture the
    // previous frame just before it is displayed.
    swapBuffers();
    }

    // Clear the entire screen (needed even though we'll render over it, since
    // AFR uses clear() to detect that the buffer is not re-used.)
    rd->clear();

    // Perform gamma correction, bloom, and SSAA, and write to the native window frame buffer
    m_film->exposeAndRender(rd,  activeCamera()->filmSettings(), m_framebuffer->texture(0));*/
    ovrHmd_EndFrame(HMD, pose, &fb_ovr_tex[0].Texture);


    //   assert(glGetError() == GL_NO_ERROR);
#endif
}


void App::onAI() {
    GApp::onAI();
    // Add non-simulation game logic and AI code here
}


void App::onNetwork() {
    GApp::onNetwork();
    // Poll net messages here
}


void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    GApp::onSimulation(rdt, sdt, idt);

    // Example GUI dynamic layout code.  Resize the debugWindow to fill
    // the screen horizontally.
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
}


bool App::onEvent(const GEvent& event) {
    // Handle super-class events
    if (GApp::onEvent(event)) { return true; }

    // If you need to track individual UI events, manage them here.
    // Return true if you want to prevent other parts of the system
    // from observing this specific event.
    //
    // For example,
    // if ((event.type == GEventType::GUI_ACTION) && (event.gui.control == m_button)) { ... return true; }
    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::TAB)) { ++id; }

    return false;
}


void App::onUserInput(UserInput* ui) {
    GApp::onUserInput(ui);
    (void)ui;
    // Add key handling here based on the keys currently held or
    // ones that changed in the last frame.
}


void App::onPose(Array<shared_ptr<Surface> >& surface, Array<shared_ptr<Surface2D> >& surface2D) {
    GApp::onPose(surface, surface2D);

    // Append any models to the arrays that you want to later be rendered by onGraphics()
}


void App::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& posed2D) {
    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction.
    // Surface2D::sortAndRender(rd, posed2D);
}


void App::onCleanup() {
    // Called after the application loop ends.  Place a majority of cleanup code
    // here instead of in the constructor so that exceptions can be caught.
}


void App::endProgram() {
    m_endProgram = true;
    ovrHmd_Destroy(HMD);
    ovr_Shutdown();
}
