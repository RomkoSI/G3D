/**
 @file Discovery2.cpp
  
 @maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
 @created 2008-11-20
 @edited  2008-11-22
 */

#include <climits>
#include "G3D/platform.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "G3D/Log.h"
#include "GLG3D/Draw.h"
#include "GLG3D/Discovery.h"
#include "GLG3D/RenderDevice.h"


namespace G3D {
namespace Discovery {

ServerDescription::ServerDescription(BinaryInput& b) {
    deserialize(b);
}


void ServerDescription::serialize(BinaryOutput& b) const {
    b.writeString(serverName);
    applicationAddress.serialize(b);
    b.writeString(applicationName);
    b.writeInt32(maxClients);
    b.writeInt32(currentClients);
    b.writeString(data);
}


void ServerDescription::deserialize(BinaryInput& b) {
    serverName      = b.readString();
    applicationAddress.deserialize(b);
    applicationName = b.readString();
    maxClients      = b.readInt32();
    debugAssert(maxClients >= 0);
    currentClients  = b.readInt32();
    data            = b.readString();
    lastUpdateTime  = System::time();
}


String ServerDescription::displayText() const {
    if (maxClients == INT_MAX) {
        // Infinite number of clients
        return 
            format("%24s (%6d ) %s:%-5d", 
                   serverName.c_str(),
                   currentClients,
                   NetworkDevice::formatIP(applicationAddress.ip()).c_str(),
                   applicationAddress.port());
    } else {
        // Finite number of clients
        return 
            format("%24s (%3d/%-3d) %s:%-5d", 
                   serverName.c_str(),
                   currentClients,
                   maxClients,
                   NetworkDevice::formatIP(applicationAddress.ip()).c_str(),
                   applicationAddress.port());
    }
}

/////////////////////////////////////////////////////////////

Rect2D Client::Display::bounds() const {
    return client->m_osWindow->clientRect();
}

float Client::Display::depth () const {
    return 0;
}

void Client::Display::render(RenderDevice* rd) const {
    client->render(rd);
}

/////////////////////////////////////////////////////////////

void Client::initNetwork() {
    // Construct broadcast array
    const Array<uint32> ipArray = NetworkDevice::instance()->broadcastAddressArray();
    m_broadcastAddressArray.resize(ipArray.size());
    for (int i = 0; i < m_broadcastAddressArray.size(); ++i) {
        m_broadcastAddressArray[i] = NetAddress(ipArray[i], m_settings.clientBroadcastPort);
    }

    m_net = LightweightConduit::create(m_settings.serverBroadcastPort, true, true);
}


Client::Client(const String& applicationName, const Settings& settings) : 
    m_settings(settings), 
    m_osWindow(NULL), 
    m_applicationName(applicationName), 
    m_index(0), 
    m_connectPushed(false) {

    initNetwork();    
}


Client::Client(const String& applicationName, const Settings& settings,
               OSWindow* osWindow, shared_ptr<GuiTheme> theme) :
    GuiWindow("Server Browser", theme, 
              Rect2D::xywh(100, 100, 500, 500), GuiTheme::NO_WINDOW_STYLE, NO_CLOSE),
    m_settings(settings),
    m_osWindow(osWindow),
    m_applicationName(applicationName) {

    initNetwork();

    if (theme) { 
        if (isNull(m_settings.displayStyle.font)) {
            m_settings.displayStyle.font = theme->defaultStyle().font;
        }
        if (m_settings.displayStyle.color.r == -1) {
            m_settings.displayStyle.color = theme->defaultStyle().color;
        }
        if (m_settings.displayStyle.outlineColor.r == -1) {
            m_settings.displayStyle.outlineColor = theme->defaultStyle().outlineColor;
        }
        if (m_settings.displayStyle.size == -1) {
            m_settings.displayStyle.size = theme->defaultStyle().size;
        }
    }

   
    m_display.reset(new Display());
    m_display->client = this;
    m_connectPushed = false;
    m_index = 0;
    if (osWindow != NULL) {
        // Fill the screen
        setRect(osWindow->clientRect());
    }
}


ClientRef Client::create
(const String& applicationName, 
 OSWindow* osWindow,
 shared_ptr<GuiTheme> theme,
 const Settings& settings) {

    return ClientRef(new Client(applicationName, settings, osWindow, theme));
}


ClientRef Client::createNoGui
(const String& applicationName, 
 const Settings& settings) {

    return ClientRef(new Client(applicationName, settings));
}


void Client::onPose(Array<shared_ptr<Surface> >& posedArray, Array<Surface2DRef>& posed2DArray) {
    GuiWindow::onPose(posedArray, posed2DArray);
    if ((m_osWindow != NULL) && visible()) {
        posed2DArray.append(m_display);
    }
}


void Client::onNetwork() {
    // Check for server announcements
    switch (m_net->waitingMessageType()) {
    case 0:
        // No incoming message
        break;

    case Settings::SERVER_DESCRIPTION_TYPE:
        receiveDescription();
        break;

    default:
        // Some unknown message
        logPrintf("Discovery::Client ignored an unexpected "
                  "packet of type %d on port %d\n",
                  m_net->waitingMessageType(),
                  m_settings.serverBroadcastPort);
        m_net->receive();
        break;
    }

    // Remove old servers    
    const RealTime tooOld = System::time() - 2.0 * m_settings.serverAdvertisementPeriod;
    for (int i = 0; i < m_serverArray.size(); ++i) {
        if (tooOld > m_serverArray[i].lastUpdateTime) {
            // This server was old; remove it
            logPrintf("Discovery2::Client: Server %s went offline\n", 
                m_serverArray[i].applicationAddress.toString().c_str());
            m_serverArray.remove(i);
            m_serverDisplayArray.remove(i);
            if (m_clickBox.size() > 0) {
                m_clickBox.remove(i);
            }

            // Process index i again since we just shortened the array
            --i;
        }
    }
}


void Client::receiveDescription() {
    NetAddress sender;
    ServerDescription s;
    m_net->receive(sender, s);

    if (s.applicationName != m_applicationName) {
        // Different application
        return;
    }
    
    // See if this server is already known to us
    int i = 0;
    for (i = 0; i < m_serverArray.size(); ++i) {
        if (m_serverArray[i].applicationAddress == s.applicationAddress) {
            // This is the server we're looking for 
            break;
        }
    }

    if (i == m_serverArray.size()) {
        // Not found-- append to the end of the list
        m_serverArray.append(s);
        m_serverDisplayArray.append("");
        logPrintf("Discovery2::Client: Found new server at %s\n", s.applicationAddress.toString().c_str());
    } else {
        m_serverArray[i] = s;
    }

    // Update the time in this entry
    m_serverArray[i].lastUpdateTime = System::time();

    // Update the display entry
    m_serverDisplayArray[i] = m_serverArray[i].displayText();

    // Wipe the click bounds since they no longer match the data
    m_clickBox.clear();
}


void Client::render(RenderDevice* rd) const {
    const GuiTheme::TextStyle& style = m_settings.displayStyle;
    const shared_ptr<GFont>& font = style.font;

    const Rect2D& box = Rect2D::xywh(20, 20 + style.size, 
        m_osWindow->width() - 40.0f, m_osWindow->height() - 200.0f - style.size);
    Draw::rect2DBorder(box, rd, style.color, 0, max(1.0f, style.size / 20.0f));

    // Show title
    font->draw2D(rd, m_settings.prompt, Vector2(box.center().x, 10), style.size, style.color, 
        style.outlineColor, GFont::XALIGN_CENTER); 

    // Show server list
    rd->setClip2D(box);
    m_clickBox.resize(m_serverDisplayArray.size());
    for (int i = 0; i < m_serverDisplayArray.size(); ++i) {
        Vector2 pos(box.x0() + 10, box.y0() + 10 + style.size * 1.5f * i);
        Vector2 bounds = font->draw2D(rd, m_serverDisplayArray[i], pos, style.size, style.color, 
            style.outlineColor, GFont::XALIGN_LEFT, GFont::YALIGN_TOP, GFont::FIXED_SPACING);
        m_clickBox[i] = Rect2D::xywh(pos, bounds);
    }
    rd->setClip2D(Rect2D::inf());

}


bool Client::onEvent(const GEvent& event) {
    if (GuiWindow::onEvent(event)) {
        return true;
    }

    if (! visible()) {
        return false;
    }

    if (event.type == GEventType::KEY_DOWN) {
        if (event.key.keysym.sym == GKey::ESCAPE) {
            // Cancelled
            m_connectPushed = false;
            setVisible(false);
            return true;
        }
    } else if (event.type == GEventType::MOUSE_BUTTON_DOWN) {
        Vector2 mouse(event.button.x, event.button.y);
        for (int i = 0; i < m_clickBox.size(); ++i) {
            if (m_clickBox[i].contains(mouse)) {
                // Clicked on this server
                m_index = i;
                m_connectPushed = true;
                setVisible(false);
                return true;
            }
        }
    }

    return false;
}


bool Client::browseImpl(ServerDescription& d) {
    m_connectPushed = false;
    m_index = 0;

    showModal(m_osWindow);

    if (m_connectPushed) {
        d = m_serverArray[m_index];
    }

    return m_connectPushed;
}


ReliableConduitRef Client::browseAndConnect
    (const String& applicationName,
     OSWindow* osWindow, 
     shared_ptr<GuiTheme> theme,
     const Settings& settings) {

    ClientRef c(new Client(applicationName, settings, osWindow, theme));
     
    ServerDescription server;

    while (c->browseImpl(server)) {
        
        // Try to connect to the selected server
        ReliableConduitRef connection = ReliableConduit::create(server.applicationAddress);
        
        if (connection && connection->ok()) {
            // Successful connection; return
            return connection;
        } else {
            // Display failure message and repeat
            //TODO
        }
    }
    
    // Cancelled
    return ReliableConduitRef();
}


bool Client::browse(const String applicationName, OSWindow* osWindow, 
                    shared_ptr<GuiTheme> theme, ServerDescription& d, const Settings& settings) {
     ClientRef c(new Client(applicationName, settings, osWindow, theme));     
     return c->browseImpl(d);
}



//////////////////////////////////////////////////////////////////////////////////////////////

ServerRef Server::create(const ServerDescription& description, 
                         const Settings& settings) {
    return ServerRef(new Server(description, settings));
}


Server::Server(const ServerDescription& description, const Settings& settings) :
    m_settings(settings),
    m_description(description) {

    const Array<uint32> ipArray = NetworkDevice::instance()->broadcastAddressArray();
    m_broadcastAddressArray.resize(ipArray.size());
    for (int i = 0; i < m_broadcastAddressArray.size(); ++i) {
        m_broadcastAddressArray[i] = NetAddress(ipArray[i], m_settings.serverBroadcastPort);
    }

    m_net = LightweightConduit::create(m_settings.clientBroadcastPort, true, true);
    
    debugAssert(m_settings.serverAdvertisementPeriod > 0);
    
    sendAdvertisement();
}


void Server::setDescription(const ServerDescription& d) {
    m_description = d;
    sendAdvertisement();
}


void Server::sendAdvertisement() {
    // Send to all clients
    m_net->send(m_broadcastAddressArray, Settings::SERVER_DESCRIPTION_TYPE, m_description);
    m_lastAdvertisementTime = System::time();
}


void Server::onNetwork() {
    switch (m_net->waitingMessageType()) {
    case 0:
        // No incoming message
        break;

    case Settings::CLIENT_QUERY_TYPE:
        // A client is requesting servers to advertise themselves
        sendAdvertisement();
        m_net->receive();
        break;

    default:
        // Some unknown message
        logPrintf("Discovery::Server ignored an unexpected "
                  "packet of type %d on port %d\n",
                  m_net->waitingMessageType(),
                  m_settings.clientBroadcastPort);
        m_net->receive();
        break;
    }

    // See if it is time to send an unsolicited advertisement again.
    if (m_lastAdvertisementTime + m_settings.serverAdvertisementPeriod >= System::time()) {
        sendAdvertisement();
    }
}

} // Discovery
} // G3D

