/** \file App.cpp 

*/
#include "App.h"
#include <civetweb.h>
#include <time.h>

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

/** Events coming in from the remote machine */
static ThreadsafeQueue<GEvent>      remoteEventQueue;

static GMutex                       clientSetMutex;
static Set<struct mg_connection*>   clientSet;

/** Set to 1 when the client requests a full-screen image and back to 0 after the frame is sent. */
static AtomicInt32                  clientWantsImage(0);

int main(int argc, const char* argv[]) {
    initGLG3D();

    GApp::Settings settings(argc, argv);

    settings.window.caption      = "Remote Rendering Demo";
    settings.window.width        = 640;    settings.window.height       = 400;

#   ifdef G3D_OSX
        debugPrintf("You may need to disable your firewall. See http://support.apple.com/kb/PH11309\n\n");
#   endif

    alwaysAssertM(FileSystem::exists("www"), "Not running from the contents of the data-files directory");
    
    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings), m_webServer(NULL) {
}


void App::onInit() {
    GApp::onInit();
	renderDevice->setSwapBuffersAutomatically(true);
    
    showRenderingStats    = false;
    m_showWireframe       = false;

    // May be using a web browser on the same machine in the foreground
    setLowerFrameRateInBackground(false);
    setFrameDuration(1.0f / 30);

    makeGUI();

    developerWindow->videoRecordDialog->setCaptureGui(false);
    developerWindow->setVisible(false);
    developerWindow->sceneEditorWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);

    developerWindow->cameraControlWindow->moveTo(Point2(developerWindow->cameraControlWindow->rect().x0(), 0));
    m_finalFramebuffer = Framebuffer::create(Texture::createEmpty("App::m_finalFramebuffer[0]", renderDevice->width(), renderDevice->height(), ImageFormat::RGB8(), Texture::DIM_2D));

    //loadScene(developerWindow->sceneEditorWindow->selectedSceneName());
    loadScene("G3D Sponza");
    
    setActiveCamera(m_debugCamera);

    startWebServer();

    m_font = GFont::fromFile(System::findDataFile("arial.fnt"));
    const NetAddress serverAddress(NetAddress::localHostname(), WEB_PORT);
    m_addressString = serverAddress.toString();
    m_qrTexture = qrEncodeHTTPAddress(serverAddress);
    debugPrintf("Server Address: %s\n", serverAddress.toString().c_str());
}


void printRequest(const mg_request_info* request_info) {
    NetAddress client(request_info->remote_ip);

    printf("Client at %s performed %s on URI \"%s\", which contains data \"%s\"\n",
                client.ipString().c_str(), request_info->request_method, request_info->uri,
                request_info->query_string ? request_info->query_string : "(NULL)");
    for (int i = 0; i < request_info->num_headers; ++i) {
        printf("   %s: %s\n", request_info->http_headers[i].name, request_info->http_headers[i].value);
    }
}


/** C++ wrapper for mg_websocket_write */
int mg_websocket_write(struct mg_connection* conn, int opcode, const String& str) {
    return mg_websocket_write(conn, opcode, str.c_str(), str.length());
}


static void websocket_ready_handler(struct mg_connection* conn) {
    clientSetMutex.lock();
    clientSet.insert(conn);
    clientSetMutex.unlock();

    mg_websocket_write(conn, 0x1, "{\"type\": 0, \"value\":\"server ready\"}");
    debugPrintf("Connection 0x%x: Opened for websocket\n",  (unsigned int)(uintptr_t)conn);
    clientWantsImage = 1;
}


/**
   \param flags first byte of websocket frame, see websocket RFC, http://tools.ietf.org/html/rfc6455#section-5.2
   \param data, data_len payload data. Mask, if any, is already applied. */
static int websocket_data_handler(struct mg_connection* conn, int flags, char* data, size_t data_len) {
    // Lower 4 bits are the opcode
    const int opcode = flags & 0xF;

    switch (opcode) {
    case 0x0: // Continuation
        debugPrintf("Connection 0x%x: Received continuation, ignoring\n", (unsigned int)(uintptr_t)conn);
        return 1;

    case 0x1: // Text
        break;

    case 0x2: // Binary
        debugPrintf("Connection 0x%x: Received binary data, ignoring\n", (unsigned int)(uintptr_t)conn);
        return 1;

    case 0x8: // Close connection
        debugPrintf("Connection 0x%x: Received close connection\n", (unsigned int)(uintptr_t)conn);
        return 0;

    case 0x9: // Ping
        debugPrintf("Connection 0x%x: Received ping\n", (unsigned int)(uintptr_t)conn);
        // TODO: Send pong
        return 1;

    case 0xA: // Pong
        // Don't have to do anything
        debugPrintf("Connection 0x%x: Received pong\n", (unsigned int)(uintptr_t)conn);
        return 1;

    default: // Reserved
        debugPrintf("Connection 0x%x: Received reserved opcode 0x%x, ignoring\n", (unsigned int)(uintptr_t)conn, opcode);
        return 1;
   }
                                      
    // debugPrintf("rcv: '%.*s'\n", (int) data_len, data);

    if ((data_len == 6) && (memcmp(data, "\"ping\"", 6) == 0)) {
        // This is our application protocol ping message; ignore it
        return 1;
    }

    if ((data_len < 2) || (data[0] != '{')) {
        // Some corrupt message
        debugPrintf("Message makes no sense\n");
        return 1;
    }


    try {
        TextInput t(TextInput::FROM_STRING, data, data_len);
        const Any msg(t);

        const int UNKNOWN = 0;
        const int SEND_IMAGE = 1000;
        const int type = msg.get("type", UNKNOWN);

        switch (type) {
        case UNKNOWN:
            debugPrintf("Cannot identify message type\n");
            break;

        case SEND_IMAGE:
            clientWantsImage = 1;
            break;

        case GEventType::KEY_DOWN:
        case GEventType::KEY_UP:
            {
                GEvent event;
                memset(&event, 0, sizeof(event));
                event.type = type;
                Any key = msg.get("key", Any());
                Any keysym = key.get("keysym", Any());
                event.key.keysym.sym = GKey::Value((int)keysym.get("sym", 0));
                event.key.state = (type == GEventType::KEY_DOWN) ? GButtonState::PRESSED : GButtonState::RELEASED;
                remoteEventQueue.pushBack(event);
            }
            break;

        default:
            debugPrintf("Unrecognized type\n");
            break;
        };

    } catch (...) {
        debugPrintf("Message makes no sense\n");
    }

    // Returning zero means stoping websocket conversation.
    return 1;
}


int error_handler(mg_connection* conn, int status) {
    if (status == 304) {
        // 304 is a request for a value that has not changed since last sent to the client.
        // This is a normal condition for a correctly operating program.

        // debugPrintf("Connection 0x%x: HTTP error %d\n", (unsigned int)(uintptr_t)conn, status);

        const String& content = "Not modified";
        mg_printf(conn, 
                  "HTTP/1.1 304 NOT MODIFIED\r\n"
                  "Content-Type: text/html\r\n"
                  "Content-Length: %d\r\n"
                  "\r\n"
                  "%s", (int)content.size(), content.c_str());
    } else {
        debugPrintf("Connection 0x%x: HTTP error %d\n", (unsigned int)(uintptr_t)conn, status);
        const struct mg_request_info* ri = mg_get_request_info(conn);
        printRequest(ri);

        if (status == 500) {
            // The client already closed the connection
            return 0;
        }

        const String& content =
            format("<html><head><title>Illegal URL</title></head><body>Illegal URL (error %d)</body></html>\n", status);

        mg_printf(conn, 
                  "HTTP/1.1 200 OK\r\n"
                  "Content-Type: text/html\r\n"
                  "Content-Length: %d\r\n"
                  "\r\n"
                  "%s", (int)content.size(), content.c_str());

    }

    // Returning non-zero tells civetweb that our function has replied to
    // the client, and civetweb should not send client any more data.
    return 1;
}


static void connection_close_handler(struct mg_connection* conn) {
    clientSetMutex.lock();
    clientSet.remove(conn);
    clientSetMutex.unlock();
}


String futureTime(int secondsIntoFuture) {
    const time_t seconds = ::time(NULL) + secondsIntoFuture;
    tm* t = gmtime(&seconds);

    static const char* day[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char* month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    return format("%s, %02d %s %d %02d:%02d:%02d GMT", day[t->tm_wday], t->tm_mday, month[t->tm_mon], t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
}


static const int IMAGE = 1;
void mg_websocket_write_image(mg_connection* conn, const shared_ptr<Image>& buf, Image::ImageFileFormat ff) {
    BinaryOutput bo("<memory>", G3D_BIG_ENDIAN);

    alwaysAssertM((ff == Image::PNG) || (ff == Image::JPEG), "Only PNG and JPEG are supported right now");
    const char* mimeType = (ff == Image::PNG) ? "image/png" : "image/jpeg";
    const String& msg = 
        format("{\"type\":%d,\"width\":%d,\"height\":%d,\"mimeType\":\"%s\"}",IMAGE, buf->width(), buf->height(), mimeType);

    // JSON header length (in network byte order)
    bo.writeInt32((int32)msg.length());

    // JSON header
    bo.writeString(msg, (int32)msg.length());

    // Binary data
    buf->serialize(bo, ff);
    
    const size_t bytes = mg_websocket_write(conn, 0x2, (const char*)bo.getCArray(), bo.length());
    (void)bytes;
    // debugPrintf("Sent %d bytes\n", (unsigned int)bytes);
}


void mg_http_write_image(mg_connection* conn, const shared_ptr<Image>& image, Image::ImageFileFormat ff, int maxAge) {
    mg_printf(conn, "HTTP/1.1 200 OK\r\n");

    debugAssert(ff == Image::JPEG || ff == Image::PNG);

    BinaryOutput b("<memory>", G3D_LITTLE_ENDIAN);
    image->serialize(b, ff);

    mg_printf(conn,
        "Expires: %s\r\n"
        "Cache-Control: max-age=%d, public\r\n"
        "Content-Type: image/%s\r\n\r\n", 
        futureTime(maxAge).c_str(),
        maxAge, 
        (ff == Image::JPEG) ? "jpeg" : "png");

    mg_write(conn, b.getCArray(), b.size());
}


void App::startWebServer() {
    debugAssert(isNull(m_webServer));
    mg_callbacks callbacks;

    // List of options. Last element must be NULL.
    static const String port = format("%d", WEB_PORT);
    static const String root = FilePath::concat(FileSystem::currentDirectory(), "www");
    const char* options[] = 
    {"listening_ports", port.c_str(), 
     "document_root",   root.c_str(),
     NULL};

    // Prepare callbacks structure. We have only one callback, the rest are NULL.
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.http_error       = error_handler;
    callbacks.websocket_ready  = websocket_ready_handler;
    callbacks.websocket_data   = websocket_data_handler;
    callbacks.connection_close = connection_close_handler;
    //callbacks.begin_request    = begin_request_handler;

    // Start the web server.
    m_webServer = mg_start(&callbacks, NULL, options);
    debugAssert(notNull(m_webServer));
}


void App::stopWebServer() {
    if (notNull(m_webServer)) {
        mg_stop(m_webServer);
        m_webServer = NULL;
    }
}


void App::makeGUI() {
    // Initialize the developer HUD
    createDeveloperHUD();
    debugWindow->setVisible(false);
    developerWindow->videoRecordDialog->setEnabled(true);

    debugWindow->pack();
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
}


void App::onNetwork() {
    handleRemoteEvents();
}


void App::handleRemoteEvents() {
    userInput->beginEvents();
    GEvent event;

    // Inject these events as if they occured locally
    while (remoteEventQueue.popFront(event)) {
        if (! WidgetManager::onEvent(event, m_widgetManager)) {
            if (! onEvent(event)) {
                userInput->processEvent(event);
            }
        }
    }

    userInput->endEvents();
}


bool App::onEvent(const GEvent& event) {
    // Handle super-class events
    if (GApp::onEvent(event)) { return true; }

    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == 'p')) {
        // Send a message to the clients
        clientSetMutex.lock();
        for (Set<struct mg_connection*>::Iterator it = clientSet.begin(); it.isValid(); ++it) {
            mg_connection* conn = *it;
            mg_websocket_write(conn, 0x1, "{\"type\": 0, \"value\": \"how are you?\"}");
        }
        clientSetMutex.unlock();
        return true;
    }

    return false;
}


void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& allSurfaces) {
    // Perform gamma correction, bloom, and SSAA, and write to the native window frame buffer
    rd->pushState(m_finalFramebuffer); {
        GApp::onGraphics3D(rd, allSurfaces);
    } rd->popState();

    // Copy the final buffer to the server screen
    rd->push2D(); {
        Draw::rect2D(m_finalFramebuffer->texture(0)->rect2DBounds(), rd, Color3::white(), m_finalFramebuffer->texture(0));
    } rd->pop2D();

    clientSetMutex.lock();
    screenPrintf("Number of clients: %d\n", clientSet.size());
    //screenPrintf("clientWantsImage: %d\n", clientWantsImage.value());

    if ((clientWantsImage.value() != 0) && (clientSet.size() > 0)) {
        // Send the image to the first client
        mg_connection* conn = *clientSet.begin();

        // JPEG encoding/decoding takes more time but substantially less bandwidth than PNG
        mg_websocket_write_image(conn, m_finalFramebuffer->texture(0)->toImage(ImageFormat::RGB8()), Image::JPEG);
        clientWantsImage = 0;
    }
    clientSetMutex.unlock();
}


void App::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& posed2D) {
    if (notNull(m_qrTexture)) {
        Rect2D rect = Rect2D::xywh(Point2(20, 20), m_qrTexture->vector2Bounds() * 10);
        Draw::rect2D(rect, rd, Color3::white(), m_qrTexture, Sampler::buffer());
        m_font->draw2D(rd, m_addressString, Point2(rect.center().x, rect.y1() + 20), 24, Color3::white(), Color3::black(), GFont::XALIGN_CENTER);
    }

    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction.
    Surface2D::sortAndRender(rd, posed2D);
}


void App::onCleanup() {
    stopWebServer();
}


void App::endProgram() {
    m_endProgram = true;
}
