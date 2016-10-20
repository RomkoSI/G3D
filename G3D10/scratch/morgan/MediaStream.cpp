#include <G3D/G3DAll.h>

enum {MEDIA_PORT = 8080};


/** Sends a rendered image to a single client (like Splashtop Streamer or VNC).

    Sending 640 * 400 * 3 bytes on a 10 megabit connection yields about 1.7 fps
    on the client.
*/
class MediaServer : public GApp {
    
    enum {MAX_PACKET_BACKLOG_ALLOWED = 150};

    shared_ptr<NetConnection>               m_connection;

    shared_ptr<NetServer>                   m_server;

    /** Waiting for OpenGL to finish so that it can be mapped */
    Queue<shared_ptr<PixelTransferBuffer> > m_sendQueue;

public:

    MediaServer(const GApp::Settings& s) : GApp(s) { }


    virtual void onInit() override {
        setLowerFrameRateInBackground(false);

        // Use a relatively low framerate to avoid overloading the network. 
        // We could do better by compressing frames into JPG using Image or
        // H.264 using VideoOutput.
        setFrameDuration(1.0 / 15.0f);
        window()->setCaption(String("MediaServer on ") + NetAddress::localHostname() + " (" + NetAddress(NetAddress::localHostname(), 0).ipString() + ":" + format("%d", MEDIA_PORT) + ")");
        m_server = NetServer::create(NetAddress(NetAddress::DEFAULT_ADAPTER_HOST, MEDIA_PORT));
        createDeveloperHUD();
    }


    virtual void onCleanup() override {
        m_server->stop();
    }


    virtual void onNetwork() override {
        // See if there are any new clients
        for (NetConnectionIterator& client = m_server->newConnectionIterator(); client.isValid(); ++client) {
            // Override our previous client with the new one
            m_connection = client.connection();
        }

        while (m_sendQueue.size() > 0) {
            shared_ptr<PixelTransferBuffer> buffer = m_sendQueue[0];

            if (buffer->readyToMap()) {
                m_sendQueue.popFront();

                if (notNull(m_connection)) {
                    // Copy directly out of OpenGL memory to minimize latency for the case where we have high bandwidth.
                    // For a lower-bandwidth connection, we could perform JPG or MPG encoding here and then
                    // stream the result.
                    if (m_connection->status() == NetConnection::DISCONNECTED) {
                        // Drop the pointer
                        m_connection.reset();
                    } else {
                        m_connection->send(0, buffer->mapRead(), buffer->size(), 0, BufferUnmapper::create(buffer));
                    }
                }
            } else {
                // We can't process the top of the queue, so keep waiting
                break;
            }
        }
    }


    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& posed3D) override {
        // Bind the main framebuffer
        rd->pushState(m_framebuffer); {
            rd->setColorClearValue(Color3::white());
            rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
            rd->clear();

            Draw::box(AABox(Point3(-1, -1, -1), Point3(1, 1, 1)), rd);
        } rd->popState();

        // Show the result on screen for debugging purposes
        rd->push2D(); {
            rd->setTexture(0, m_framebuffer->texture(0));
            Draw::fastRect2D(m_framebuffer->rect2DBounds(), rd);
        } rd->pop2D();

        // Don't bother putting anything in the queue unless there are 
        // clients and the queue isn't too backed up. 
        if (notNull(m_connection) && (m_sendQueue.size() < 3) && (networkSendBacklog() < MAX_PACKET_BACKLOG_ALLOWED)) {
//            m_connection->debugPrintPeerAndHost();
            //if(sent < 10)
            m_sendQueue.pushBack(m_framebuffer->texture(0)->toPixelTransferBuffer());
            //++sent;
        }

        screenPrintf("Send Queue Size:      % 4d\n",    m_sendQueue.size());
        screenPrintf("Send Queue Bytes:     % 4d\n",    m_sendQueue.size() * m_framebuffer->texture(0)->sizeInMemory());
        screenPrintf("networkSendBacklog(): % 4d\n",    networkSendBacklog());
        if (notNull(m_connection)) {
            screenPrintf("latency:              % 4dms\n",  iRound(m_connection->latency() / units::milliseconds()));
        }
    } // onGraphics3D
}; // MediaServer


class MediaClient : public GApp {
protected:

    shared_ptr<NetConnection>           m_connection;

    String                              m_connectToAddress;

    GuiTextBox*                         m_connectToAddressBox;

public:

    MediaClient(const GApp::Settings& s) : GApp(s), m_connectToAddress("Octahedron.cs.williams.edu") {}


    virtual void onInit() override {
        setFrameDuration(1.0f/30.0f);
        setLowerFrameRateInBackground(false);
        window()->setCaption("MediaClient");
        createDeveloperHUD();
        m_connectToAddressBox = debugPane->addTextBox("Connect to IP:", &m_connectToAddress);
        showRenderingStats = false;
        debugWindow->setVisible(true);
        m_framebuffer->texture(0)->clear();
    }

    virtual void onCleanup() override {
        if (notNull(m_connection) && (m_connection->status() != NetConnection::DISCONNECTED)) {
            m_connection->disconnect(false);
        }
    }

    virtual bool onEvent(const GEvent& e) {
        if (GApp::onEvent(e)) { return true; }

        if ((e.type == GEventType::GUI_ACTION) &&
            (e.gui.control == m_connectToAddressBox)) {
            NetAddress serverAddress(m_connectToAddress + format(":%d", MEDIA_PORT));
            m_connection = NetConnection::connectToServer(serverAddress);
            return true;
        }

        return false;
    }


    virtual void onNetwork() override {
        if (isNull(m_connection)) { return; }

        for (NetMessageIterator msg = m_connection->incomingMessageIterator(); msg.isValid(); ++msg) {
            if(notNull(m_connection)) {
//                m_connection->debugPrintPeerAndHost();
            }
            // A properly optimized OpenGL driver copies the memory itself and then schedules the update;
            // we could use GLPixelTransferBuffer and perform our own copy if we didn't trust the driver.
            shared_ptr<GLPixelTransferBuffer> buffer = GLPixelTransferBuffer::create(m_framebuffer->texture(0)->width(), m_framebuffer->texture(0)->height(), m_framebuffer->texture(0)->format(), msg.binaryInput().getCArray());
            m_framebuffer->texture(0)->update(buffer);
        } // for each message
    }

    
    virtual void onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& posed2D) override {
        // Perform gamma correction, bloom, and SSAA, and write to the native window frame buffer
        m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_framebuffer->texture(0));

        String statusString = "never connected";

        if (notNull(m_connection)) {
            switch (m_connection->status()) {
            case NetConnection::CONNECTED:          statusString = "CONNECTED"; break;
            case NetConnection::DISCONNECTED:       statusString = "DISCONNECTED"; break;
            case NetConnection::JUST_CONNECTED:     statusString = "JUST_CONNECTED"; break;
            case NetConnection::WAITING_TO_CONNECT: statusString = "WAITING_TO_CONNECT"; break;
            }
        }
        debugFont->draw2D(rd, statusString, rd->viewport().center(), 40, Color3::black(), Color3::white(), GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
        
        GApp::onGraphics2D(rd, posed2D);
    }

};


// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();


int main(int argc, const char* argv[]) {
    initGLG3D();

    GApp::Settings settings(argc, argv);
//    setNetworkCommunicationInterval(1 * units::milliseconds());
    
    // Has to be small to avoid overloading the network
    settings.window.caption      = argv[0];
    settings.window.width        = 640; 
    settings.window.height       = 400;
    settings.window.resizable    = false;
    
    settings.film.preferredColorFormats.clear();
    settings.film.preferredColorFormats.append(ImageFormat::RGB8());
    static const char* choice[] = {"Server", "Client"};
    if (prompt("MediaStream", "Run as:", choice, 2, true) == 0) {
        MediaServer(settings).run();
    } else {
        MediaClient(settings).run();
    }

    return 0;
}
