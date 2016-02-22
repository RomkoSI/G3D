/**
\file   DebugTextWidget.cpp
\maintainer Morgan McGuire
\edited 2016-02-06

G3D Library http://g3d.cs.williams.edu
Copyright 2000-2016, Morgan McGuire morgan@cs.williams.edu
All rights reserved.
Use permitted under the BSD license

*/
#include "G3D/platform.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/DefaultRenderer.h"
#include "GLG3D/DebugTextWidget.h"
#include "GLG3D/Draw.h"
#include "GLG3D/GFont.h"
#include "GLG3D/GApp.h"
#include "GLG3D/VideoRecordDialog.h"

namespace G3D {

void DebugTextWidget::render(RenderDevice *rd) const {
    if (m_app->debugFont && (m_app->showRenderingStats || (m_app->showDebugText && (m_app->debugText.length() > 0)))) {
        // Capture these values before we render debug output
        const int majGL = rd->stats().majorOpenGLStateChanges;
        const int majAll = rd->stats().majorStateChanges;
        const int minGL = rd->stats().minorOpenGLStateChanges;
        const int minAll = rd->stats().minorStateChanges;
        const int pushCalls = rd->stats().pushStates;

        rd->push2D(); {
            const static float size = 10.0f;
            if (m_app->showRenderingStats) {
                rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
                Draw::rect2D(Rect2D::xywh(2.0f, 2.0f, rd->width() - 4.0f, size * 5.8f + 2), rd, Color4(0, 0, 0, 0.3f));
            }

            static Array<GFont::CPUCharVertex> charVertexArray;
            static Array<int> indexArray;
            charVertexArray.fastClear();
            indexArray.fastClear();

            float x = 5;
            Vector2 pos(x, 5);

            if (m_app->showRenderingStats) {

                Color3 statColor = Color3::yellow();

                const char* build =
#               ifdef G3D_DEBUG
                    "";
#               else
                    " (Optimized)";
#               endif

                const String sysDescription = rd->getCardDescription() + "   " + System::version() + build;

                String description = sysDescription;
                const shared_ptr<DefaultRenderer>& renderer = dynamic_pointer_cast<DefaultRenderer>(m_app->renderer());
                if (notNull(renderer)) {
                    if (renderer->deferredShading()) {
                        description += "   Deferred Shading";
                    } else {
                        description += "   Forward Shading";
                    }

                    if (renderer->orderIndependentTransparency()) {
                        description += "   OIT";
                    } else {
                        description += "   Sorted Transparency";
                    }
                }

                m_app->debugFont->appendToCharVertexArray(charVertexArray, indexArray, rd, description, pos, size, Color3::white());
                pos.y += size * 1.5f;

                const float fps = rd->stats().smoothFrameRate;
                const String& s = format(
                    "% 4d fps (% 3d ms)  % 5.1fM tris  GL Calls: %d/%d Maj;  %d/%d Min;  %d push; %d Surfaces; %d Surface2Ds",
                    iRound(fps),
                    iRound(1000.0f / fps),
                    iRound(rd->stats().smoothTriangles / 1e5) * 0.1f,
                    /*iRound(rd->stats().smoothTriangleRate / 1e4) * 0.01f,*/
                    majGL, majAll, minGL, minAll, pushCalls,
                    m_app->m_posed3D.size(), m_app->m_posed2D.size());
                m_app->debugFont->appendToCharVertexArray(charVertexArray, indexArray, rd, s, pos, size, statColor);

                pos.x = x;
                pos.y += size * 1.5f;

                {
                    const int g = iRound(m_app->m_graphicsWatch.smoothElapsedTime() / units::milliseconds());
                    const int p = iRound(m_app->m_poseWatch.smoothElapsedTime() / units::milliseconds());
                    const int n = iRound(m_app->m_networkWatch.smoothElapsedTime() / units::milliseconds());
                    const int s = iRound(m_app->m_simulationWatch.smoothElapsedTime() / units::milliseconds());
                    const int L = iRound(m_app->m_logicWatch.smoothElapsedTime() / units::milliseconds());
                    const int u = iRound(m_app->m_userInputWatch.smoothElapsedTime() / units::milliseconds());
                    const int w = iRound(m_app->m_waitWatch.smoothElapsedTime() / units::milliseconds());

                    const int swapTime = iRound(rd->swapBufferTimer().smoothElapsedTime() / units::milliseconds());

                    const String& str =
                        format("Time:%4d ms Gfx,%4d ms Swap,%4d ms Sim,%4d ms Pose,%4d ms AI,%4d ms Net,%4d ms UI,%4d ms idle",
                        g, swapTime, s, p, L, n, u, w);
                    m_app->debugFont->appendToCharVertexArray(charVertexArray, indexArray, rd, str, pos, size, statColor);
                }

                pos.x = x;
                pos.y += size * 1.5f;

                const char* esc = NULL;
                switch (m_app->escapeKeyAction) {
                case GApp::ACTION_QUIT:
                    esc = "ESC: QUIT      ";
                    break;
                case GApp::ACTION_SHOW_CONSOLE:
                    esc = "ESC: CONSOLE   ";
                    break;
                case GApp::ACTION_NONE:
                    esc = "               ";
                    break;
                }

                const char* screenshot =
                    (m_app->developerWindow &&
                    m_app->developerWindow->videoRecordDialog &&
                    m_app->developerWindow->videoRecordDialog->enabled()) ?
                    "F4: SCREENSHOT  " :
                    "                ";

                const char* reload = "F5: RELOAD SHADERS ";

                const char* video =
                    (m_app->developerWindow &&
                    m_app->developerWindow->videoRecordDialog &&
                    m_app->developerWindow->videoRecordDialog->enabled()) ?
                    "F6: MOVIE     " :
                    "              ";

                const char* camera =
                    (m_app->cameraManipulator() &&
                    m_app->m_debugController) ?
                    "F2: DEBUG CAMERA  " :
                    "                  ";

                const char* cubemap = "F8: RENDER CUBEMAP";

                const char* time = notNull(m_app->developerWindow) && notNull(m_app->developerWindow->sceneEditorWindow) ?
                    "F9: START/STOP TIME " :
                    "                    ";

                const char* dev = m_app->developerWindow ?
                    "F11: DEV WINDOW" :
                    "               ";

                const String& Fstr = format("%s     %s     %s     %s     %s     %s     %s     %s", esc, camera, screenshot, reload, video, cubemap, time, dev);
                m_app->debugFont->appendToCharVertexArray(charVertexArray, indexArray, rd, Fstr, pos, 8, Color3::white());

                pos.x = x;
                pos.y += size;

            }

            m_app->m_debugTextMutex.lock();
            for (int i = 0; i < m_app->debugText.length(); ++i) {
                m_app->debugFont->appendToCharVertexArray(charVertexArray, indexArray, rd, m_app->debugText[i], pos, size, m_app->m_debugTextColor, m_app->m_debugTextOutlineColor);
                pos.y += size * 1.5f;
            }
            m_app->m_debugTextMutex.unlock();
            m_app->debugFont->renderCharVertexArray(rd, charVertexArray, indexArray);
        } rd->pop2D();
    }
}
} // G3D
