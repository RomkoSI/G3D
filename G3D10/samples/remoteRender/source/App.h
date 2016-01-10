/**
  \file App.h
 */
#ifndef App_h
#define App_h
#include <G3D/G3DAll.h>

#define WEB_PORT (8080)

// Implemented in qr.cpp
shared_ptr<Texture> qrEncodeHTTPAddress(const NetAddress& addr);

struct mg_context;

/** 
  Simple example of sending G3D events from a web browser and injecting them
  into the GApp event system and sending images in real-time to a web browser.

  Connects G3D to codeheart.js.

  Connect to the displayed URL from any browser or use the displayed 
  QR code to automatically connect from a mobile device. */
class App : public GApp {
protected:    
    bool                    m_showWireframe;

    mg_context*             m_webServer;

    shared_ptr<GFont>       m_font;
    String                  m_addressString;

    /** QR code for letting clients automatically connect */
    shared_ptr<Texture>     m_qrTexture;
    shared_ptr<Texture>     m_background;

    /** The image sent across the network */
    shared_ptr<Framebuffer> m_finalFramebuffer;

    /** Called from onInit */
    void makeGUI();

    void startWebServer();
    void stopWebServer();
    void handleRemoteEvents();

public:
    
    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit() override;

    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D) override;
    virtual void onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& surface2D) override;

    virtual void onNetwork() override;
    virtual bool onEvent(const GEvent& e) override;
    virtual void onCleanup() override;
    
    /** Sets m_endProgram to true. */
    virtual void endProgram();
};

#endif
