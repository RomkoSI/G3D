#ifndef App_h
#define App_h

#include <G3D/G3DAll.h>
#include "DeepGBufferRadiosity.h"
#include "DeepGBufferRadiositySettings.h"

class App : public GApp {
protected:

    class DemoSettings {
    public:
        G3D_DECLARE_ENUM_CLASS(DemoMode,			   AO, RADIOSITY, VARIATIONS);
        G3D_DECLARE_ENUM_CLASS(QualityPreset,          MAX_PERFORMANCE, BALANCED, MAX_QUALITY);
        G3D_DECLARE_ENUM_CLASS(GlobalIlluminationMode, RADIOSITY, SPLIT_SCREEN, STATIC_LIGHT_PROBE);
        G3D_DECLARE_ENUM_CLASS(CameraMode,			   STATIC, DYNAMIC, FREE);

        /** Allow direct editing of DeepGBufferRadiositySettings, when set to false, all changes will be lost. */
        bool                    advancedSettingsMode;

        /** Toggles all lights/entities that have names prefixed with "dynamic" on/off. */
        bool                    dynamicLights;

		DemoMode				demoMode;

		/** Used in AO-only mode */
		bool					twoLayerAO;

        QualityPreset          QualityPreset;
        
        GlobalIlluminationMode  globalIlluminationMode;

        CameraMode              cameraMode;

		/** Only used in VARIATIONS mode */
        bool                    aoEnabled;

        /** Only used in VARIATIONS mode */
        bool                    twoLayerRadiosity;

        /** Only used in VARIATIONS mode */
        bool                    lightProbeFallback;

        DemoSettings() : 
            advancedSettingsMode(false), 
            dynamicLights(true), 
			demoMode(DemoMode::AO),
			twoLayerAO(true),
            QualityPreset(QualityPreset::BALANCED), 
            globalIlluminationMode(GlobalIlluminationMode::SPLIT_SCREEN),
            cameraMode(CameraMode::STATIC),
            aoEnabled(true),
            twoLayerRadiosity(true),
            lightProbeFallback(true) {}

        /** Returns true if there may be significant changes to the radiosity solution between using these settings and other */
        bool significantRadiosityDifferences(const DemoSettings& other);
    };

    DemoSettings            m_demoSettings;

    /** Used to detect changed in demoSettings */
    DemoSettings            m_previousDemoSettings;

    shared_ptr<Framebuffer> m_lambertianDirectBuffer;
    shared_ptr<Framebuffer> m_peeledLambertianDirectBuffer;
    shared_ptr<DeepGBufferRadiosity> m_deepGBufferRadiosity;
    DeepGBufferRadiositySettings     m_deepGBufferRadiositySettings;

    /** Various presets for DeepGBufferRadiosity, along the perf-quality tradeoff spectrum */
    DeepGBufferRadiositySettings     m_maxPerformanceDeepGBufferRadiosityPresets;
    DeepGBufferRadiositySettings     m_BALANCEDDeepGBufferRadiosityPresets;
    DeepGBufferRadiositySettings     m_maxQualityDeepGBufferRadiosityPresets;

    shared_ptr<GBuffer>     m_peeledGBuffer;
    GBuffer::Specification  m_peeledGBufferSpecification;

    shared_ptr<Texture>     m_previousDepthBuffer;

    /** The demo user interface window */
    shared_ptr<GuiWindow>   m_gui;
    GuiButton*              m_drawerButton;

    GuiText                 m_rightIcon;
    GuiText                 m_leftIcon;
    shared_ptr<GFont>       m_titleFont;
    shared_ptr<GFont>       m_captionFont;

    GuiLabel*               m_controlLabel;

    GuiLabel*               m_resolutionLabel;

    GuiLabel*               m_radiosityTimeLabel;
    GuiLabel*               m_filteringTimeLabel;
	GuiPane*				m_performancePane;

    /** Builds the m_gui interface */
    void makeGUI();

    /** Builds the debugging/advanced interface in m_debugWindow */
    void makeAdvancedGUI();

    void computeGBuffers(RenderDevice* rd, Array<shared_ptr<Surface> >& all);
    void computeShadows(RenderDevice* rd, Array<shared_ptr<Surface> >& all, LightingEnvironment& environment);
    void deferredShade(RenderDevice* rd, const LightingEnvironment& environment);
    void forwardShade(RenderDevice* rd, Array<shared_ptr<Surface> >& all, const LightingEnvironment& environment);

    /** 
        Call to run the DeepGBufferRadiosity algorithm through a couple of iterations to converge when switching modes.
        Tuned to run in under 1/30 of a second on a Geforce GTX 770 
        */
    void convergeDeepGBufferRadiosity(RenderDevice* rd);

    void renderLambertianOnly
       (RenderDevice* rd, 
        const shared_ptr<Framebuffer>& fb, 
        const LightingEnvironment& environment, 
        const shared_ptr<GBuffer>& gbuffer, 
        const DeepGBufferRadiositySettings& radiositySettings, 
        const shared_ptr<Texture>& ssPositionChange,
        const shared_ptr<Texture>& indirectBuffer, 
        const shared_ptr<Texture>& oldDepth, 
        const shared_ptr<Texture>& peeledIndirectBuffer, 
        const shared_ptr<Texture>& peeledDepthBuffer);

    void initGBuffers();

    /** Called once per frame from onUserInput to translate the current demoSettings into the proper state throughout the system */
    void evaluateDemoSettings();

    /** Only called if split screen mode is active in demo settings. Assumes that the gbuffers have been 
        rendered and m_framebuffer contains a final image with screen-space radiosity 
    */
    void renderSplitScreen(RenderDevice* rd, Array<shared_ptr<Surface> >& all, const LightingEnvironment& environment);

public:
    
    App(const GApp::Settings& settings = GApp::Settings());
    virtual void onInit() override;
    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface) override;
    virtual void onGraphics2D(RenderDevice* rd, Array< shared_ptr< Surface2D > >& surface2D) override;
    virtual void onAfterLoadScene(const Any& any, const String& stringName) override;
    virtual void onUserInput(UserInput* ui) override;
    virtual bool onEvent(const GEvent& event) override;
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;
};

#endif
