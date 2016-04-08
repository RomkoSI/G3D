#include <G3D/G3DAll.h>

/** Chat example: p2p chat in which every node is both a client and a server */
class ChatApp : public GApp {
protected:

    enum {PORT = 18821};
    enum MessageType {TEXT = 1, CHANGE_NAME};

    class Conversation : public ReferenceCountedObject {
    public:
        String                 name;
        shared_ptr<NetConnection>   connection;
        shared_ptr<GuiWindow>       window;
        GuiTextBox*                 textBox;
        GuiLabel*                   lastTextReceived;
        String                 textToSend;

        Conversation(ChatApp* app, const shared_ptr<NetConnection>& c) : connection(c) {
            window = GuiWindow::create("New Connection", shared_ptr<GuiTheme>(), Rect2D::xywh(100, 100, 100, 100), GuiTheme::NORMAL_WINDOW_STYLE, GuiWindow::REMOVE_ON_CLOSE);
            // One line of history!
            lastTextReceived = window->pane()->addLabel("");
            textBox = window->pane()->addTextBox("", &textToSend);
            app->addWidget(window);
        }

        ~Conversation() {
            connection->disconnect(false);
        }
    };

    String                                  m_name;

    /** For allowing others to connect to me */
    shared_ptr<NetServer>                   m_server;

    /** All of my connections, regardless of who created them */
    Array< shared_ptr<Conversation> >       m_conversationArray;

    String                                  m_connectToAddress;

    GuiTextBox*                             m_connectToAddressBox;

    /** Returns a null pointer if there is no matching conversation. */
    shared_ptr<Conversation> findConversation(const GuiControl* c) {
        for (int i = 0; i < m_conversationArray.size(); ++i) {
            if (m_conversationArray[i]->textBox == c) {
                return m_conversationArray[i];
            }
        }
        return shared_ptr<Conversation>();
    }
    
    void sendMyName(shared_ptr<NetSendConnection> connection) const {
        BinaryOutput bo;
        bo.writeString32(m_name);
        connection->send(CHANGE_NAME, bo);
    }


    void makeGUI() {
        m_connectToAddressBox = debugPane->addTextBox("Connect to IP:", &m_connectToAddress);
    }


    void connectToServer(const String& address) {
        NetAddress serverAddress(address + format(":%d", PORT));
        m_conversationArray.append(shared_ptr<Conversation>(new Conversation(this, NetConnection::connectToServer(serverAddress))));
    }

public:

    ChatApp(const GApp::Settings& s) : GApp(s), m_connectToAddress("?.?.?.?") {}

    void onInit() override {
        renderDevice->setColorClearValue(Color3::white());
        m_name = NetAddress::localHostname();
        window()->setCaption(m_name + " (" + NetAddress(m_name, 0).ipString() + ":" + format("%d", PORT) + ")");

        m_server = NetServer::create(NetAddress(NetAddress::DEFAULT_ADAPTER_HOST, PORT));
        createDeveloperHUD();
        developerWindow->setVisible(false);
        developerWindow->cameraControlWindow->setVisible(false);
        makeGUI();
        showRenderingStats = false;
        debugWindow->setVisible(true);
    }


    virtual bool onEvent(const GEvent& e) override {
        if (GApp::onEvent(e)) {
            return true;
        }

        switch (e.type) {
        case GEventType::GUI_ACTION:
            if (e.gui.control == m_connectToAddressBox) {
                connectToServer(m_connectToAddress);
                m_connectToAddress = "";
                return true;
            } else {
                // Send a text message to the other machine
                shared_ptr<Conversation> conversation = findConversation(e.gui.control);
                if (notNull(conversation)) {
                    BinaryOutput bo;
                    bo.writeString32(conversation->textToSend);
                    conversation->connection->send(TEXT, bo);
                    conversation->textToSend = "";
                    return true;
                }
            }
            break;

        case GEventType::GUI_CLOSE:
            {
                // Shut down the associated network connection by letting 
                // the Conversation's destructor execute
                for (int i = 0; i < m_conversationArray.size(); ++i) {
                    if (m_conversationArray[i]->window.get() == e.guiClose.window) {
                        m_conversationArray.fastRemove(i);
                        break; // Leave the FOR loop
                    }
                } // for
            }
            break;

        case GEventType::QUIT:
            m_conversationArray.clear();
            m_server->stop();
            m_server.reset();
            break;
        } //switch

        return false;
    } // onEvent


    void onNetwork() override {
        // If the app is shutting down, don't service network connections
        if (isNull(m_server)) {
            return;
        }

        // See if there are any new clients
        for (NetConnectionIterator& client = m_server->newConnectionIterator(); client.isValid(); ++client) {
            m_conversationArray.append(shared_ptr<Conversation>(new Conversation(this, client.connection())));

            // Tell this client who I am
            sendMyName(client.connection());
        }

        for (int c = 0; c < m_conversationArray.size(); ++c) {
            shared_ptr<Conversation> conversation = m_conversationArray[c];

            switch (conversation->connection->status()) {
            case NetConnection::WAITING_TO_CONNECT:
                // Still waiting for the server to accept us.
                break;

            case NetConnection::JUST_CONNECTED:
                // We've just connected to the server but never invoked send() or incomingMessageIterator().
                // Tell the server our name.
                sendMyName(conversation->connection);

                // Intentionally fall through

            case NetConnection::CONNECTED:
                // Read all incoming messages from all connections, regardless of who created them
                for (NetMessageIterator& msg = conversation->connection->incomingMessageIterator(); msg.isValid(); ++msg) {

                    BinaryInput& bi = msg.binaryInput();

                    switch (msg.type()) {
                    case TEXT:
                        conversation->lastTextReceived->setCaption(bi.readString32());
                        break;

                    case CHANGE_NAME:
                        conversation->name = bi.readString32();
                        conversation->window->setCaption(conversation->name);
                        break;
                    } // Dispatch on message type

                } // For each message
                break;

            case NetConnection::DISCONNECTED:
                // Remove this conversation from my list
                removeWidget(m_conversationArray[c]->window);
                m_conversationArray.fastRemove(c);
                --c;
                break;
            } // Switch status
        } // For each conversation
    }
}; // class ChatApp


void runChat(const GApp::Settings& settings) {
}

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
    initGLG3D();

    GApp::Settings settings(argc, argv);
    
    // Has to be small to avoid overloading the network
    settings.window.caption      = argv[0];
    settings.window.width        = 1280; 
    settings.window.height       = 720;
    settings.hdrFramebuffer.colorGuardBandThickness  = Vector2int16(0, 0);
    
    return ChatApp(settings).run();
}
