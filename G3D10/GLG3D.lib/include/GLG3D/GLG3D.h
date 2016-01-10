/**
 \file GLG3D/GLG3D.h

 This header includes all of the GLG3D libraries in 
 appropriate namespaces.

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2002-08-07
 \edited  2015-09-15

 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/

#ifndef G3D_GLG3D_h
#define G3D_GLG3D_h

#include "G3D/G3D.h"
#include "GLG3D/Camera.h"
#include "GLG3D/glheaders.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/getOpenGLState.h"
#include "GLG3D/Texture.h"
#include "GLG3D/glFormat.h"
#include "GLG3D/Surfel.h"
#include "GLG3D/Milestone.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/VertexBuffer.h"
#include "GLG3D/AttributeArray.h"
#include "GLG3D/GFont.h"
#include "GLG3D/UserInput.h"
#include "GLG3D/FirstPersonManipulator.h"
#include "GLG3D/Draw.h"
#include "GLG3D/Light.h"
#include "GLG3D/tesselate.h"
#include "GLG3D/GApp.h"
#include "GLG3D/Surface.h"
#include "GLG3D/MD2Model.h"
#include "GLG3D/MD3Model.h"
#include "GLG3D/OSWindow.h"
#include "GLG3D/DepthOfFieldSettings.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/Shape.h"
#include "GLG3D/FilmSettings.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/Widget.h"
#include "GLG3D/ThirdPersonManipulator.h"
#include "GLG3D/GConsole.h"
#include "GLG3D/BSPMAP.h"
#include "GLG3D/GKey.h"
#include "GLG3D/UniversalMaterial.h"
#include "GLG3D/GaussianBlur.h"
#include "GLG3D/UniversalSurface.h"
#include "GLG3D/DirectionHistogram.h"
#include "GLG3D/UniversalBSDF.h"
#include "GLG3D/Component.h"
#include "GLG3D/Film.h"
#include "GLG3D/Tri.h"
#include "GLG3D/TriTree.h"
#include "GLG3D/Profiler.h"
#include "GLG3D/GuiTheme.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiCheckBox.h"
#include "GLG3D/GuiControl.h"
#include "GLG3D/GuiContainer.h"
#include "GLG3D/GuiLabel.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/GuiRadioButton.h"
#include "GLG3D/GuiSlider.h"
#include "GLG3D/GuiTextBox.h"
#include "GLG3D/GuiMenu.h"
#include "GLG3D/GuiDropDownList.h"
#include "GLG3D/GuiNumberBox.h"
#include "GLG3D/GuiFrameBox.h"
#include "GLG3D/GuiFunctionBox.h"
#include "GLG3D/GuiTextureBox.h"
#include "GLG3D/GuiTabPane.h"
#include "GLG3D/FileDialog.h"
#include "GLG3D/IconSet.h"
#include "GLG3D/MotionBlurSettings.h"
#include "GLG3D/LightingEnvironment.h"
#include "GLG3D/UprightSplineManipulator.h"
#include "GLG3D/CameraControlWindow.h"
#include "GLG3D/DeveloperWindow.h"
#include "GLG3D/VideoRecordDialog.h"
#include "GLG3D/SceneEditorWindow.h"
#include "GLG3D/ProfilerWindow.h"
#include "GLG3D/VideoInput.h"
#include "GLG3D/VideoOutput.h"
#include "GLG3D/ShadowMap.h"
#include "GLG3D/GBuffer.h"
#include "GLG3D/GLPixelTransferBuffer.h"
#include "GLG3D/SlowMesh.h"
#include "GLG3D/Discovery.h"
#include "GLG3D/Entity.h"
#include "GLG3D/ArticulatedModel.h"
#include "GLG3D/CPUVertexArray.h"
#include "GLG3D/PhysicsFrameSplineEditor.h"
#include "GLG3D/Scene.h"
#include "GLG3D/SceneVisualizationSettings.h"
#include "GLG3D/UniversalSurfel.h"
#include "GLG3D/MotionBlur.h"
#include "GLG3D/HeightfieldModel.h"
#include "GLG3D/Xbox360Controller.h"
#include "GLG3D/ArticulatedModelSpecificationEditorDialog.h"
#include "GLG3D/DebugTextWidget.h"
#include "GLG3D/Renderer.h"
#include "GLG3D/DefaultRenderer.h"
#include "GLG3D/ParticleSystem.h"
#include "GLG3D/ParticleSurface.h"

#ifdef G3D_OSX
#include "GLG3D/GLFWWindow.h"
#endif

#ifdef G3D_WINDOWS
#include "GLG3D/Win32Window.h"
#include "GLG3D/DXCaps.h"
#endif

#ifndef G3D_NO_FMOD
#include "GLG3D/AudioDevice.h"
#endif

#include "GLG3D/AmbientOcclusion.h"
#include "GLG3D/DepthOfField.h"
#include "GLG3D/Skybox.h"
#include "GLG3D/SkyboxSurface.h"
#include "GLG3D/Args.h"
#include "GLG3D/Shader.h"
#include "GLG3D/VisibleEntity.h"

#include "GLG3D/MarkerEntity.h"
#include "GLG3D/VisualizeCameraSurface.h"
#include "GLG3D/VisualizeLightSurface.h"
#include "GLG3D/GLPixelTransferBuffer.h"
#include "GLG3D/BufferTexture.h"
#include "GLG3D/SVO.h"
#include "GLG3D/Renderer.h"
#include "GLG3D/TemporalFilter.h"
#include "GLG3D/BindlessTextureHandle.h"

namespace G3D {

    /** 
      Call from main() to initialize the GLG3D library state and register
      shutdown memory managers.  This does not initialize OpenGL. 

      This automatically calls initG3D. It is safe to call this function 
      more than once--it simply ignores multiple calls.

      \see GLCaps, OSWindow, RenderDevice, initG3D
    */
    void initGLG3D(const G3DSpecification& spec = G3DSpecification());
}

// Set up the linker on Windows
#ifdef _MSC_VER

#   pragma comment(lib, "ole32")
#   pragma comment(lib, "opengl32")
#   pragma comment(lib, "glu32")
#   pragma comment(lib, "shell32") // for drag drop

#ifdef USE_ASSIMP
#   ifdef _DEBUG
#       pragma comment(lib, "assimp_x64d")
#   else
#       pragma comment(lib, "assimp_x64")
#   endif
#endif

#pragma comment(lib, "glew_x64")
#pragma comment(lib, "glfw_x64")
#pragma comment(lib, "nfd_x64.lib")
/** \def G3D_NO_FFMPEG If you #define this when building G3D and before including
 GLG3D.h or G3DAll.h, then G3D will not link in FFmpeg DLL's and not provide video
 support or enable the video dialog in the developer window.
*/
//#   define G3D_NO_FFMPEG

#ifndef G3D_NO_FFMPEG
#   pragma comment(lib, "avutil_x64.lib")
#   pragma comment(lib, "avcodec_x64.lib")
#   pragma comment(lib, "avformat_x64.lib")
#   pragma comment(lib, "swscale_x64.lib")
#   pragma comment(lib, "avfilter_x64.lib")
#   pragma comment(lib, "avdevice_x64.lib")
#   pragma comment(lib, "swresample_x64.lib")

#endif // G3D_NO_FFMPEG

#ifndef G3D_NO_FMOD
#	ifdef G3D_WINDOWS
#       pragma comment(lib, "fmod64_vc.lib")
#	endif
#endif

#   ifdef _DEBUG
#        pragma comment(lib, "GLG3D_x64d")
#   else
#       pragma comment(lib, "GLG3D_x64")
#   endif

#endif
#endif
