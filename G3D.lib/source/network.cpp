#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "G3D/network.h"
#include "G3D/units.h"
#include "G3D/ThreadsafeQueue.h"
#include "G3D/GThread.h"
#ifdef G3D_OSX
#   include <netdb.h>
#endif
#ifdef G3D_WINDOWS
#   include <Mmsystem.h>
#endif
#ifdef G3D_LINUX
#   include <netdb.h>


#endif
#include "enet/enet.h"

/*
enet is not very modular.  It expects applications to know their role ahead of time (e.g., THE server, client
that connects to 3 servers, client that connects to 6 servers), rather than
supporting a group of modules that interact and add different features.
An ENetHost is designed to model a machine.  G3D uses one per client connection and one per 
listener/related set of server connections.

enet is also not threadsafe--it is not safe to call send and receive functions on different threads, which is required
for networking.  But single threading those calls under reliable transport increases latency because the network cannot
perform useful communication while other work continues.

Server side of a connection:
  ENetHost is like a TCP listener socket, with some extra information limiting total connections.  You have one per server.
  ENetPeer is like a TCP socket.  You have one per client.

Client side of a connection:
  Create a ENetHost and then an ENetPeer

  enet_peer_send queues packets for sending but does not perform the 
  actual transfer.  Call enet_host_flush to send immediately or
  wait for your own next scheduled call to enet_host_service.

*/  
namespace G3D {

/** Start the network thread if G3D was initialized to use threaded networking and the thread is not yet started. */
static void maybeStartNetworkThread();

static _ENetAddress toENetAddress(const NetAddress& src) {
    _ENetAddress dst;
    dst.host = htonl(src.ip());
    dst.port = src.port();
    return dst;
}

/** Initialized in initializeNetwork() */
static RealTime         s_networkCommunicationInterval;

static AtomicInt32      s_backlog;

/** Protects s_allServers and s_allClientConnections */
static GMutex           s_allServerAndClientConnectionMutex;
static Array< weak_ptr<NetServer> > s_allServers;

namespace _internal {
    class NetClientSideConnection;
}
static Array< weak_ptr<_internal::NetClientSideConnection> > s_allClientConnections;

static unsigned int backlogForPeer(_ENetPeer* enetPeer) {
    const size_t outgoingReliableCommandCount     = enet_list_size(&(enetPeer->outgoingReliableCommands));      
    const size_t outgoingUnreliableCommandCount   = enet_list_size(&(enetPeer->outgoingUnreliableCommands));    
    return (unsigned int)(outgoingReliableCommandCount + outgoingUnreliableCommandCount);
}


void setNetworkCommunicationInterval(const RealTime t) {
    s_networkCommunicationInterval = t;
}


RealTime networkCommunicationInterval() {
    return s_networkCommunicationInterval;
}


static uint32 networkCommunicationIntervalMilliseconds() {
    return uint32(s_networkCommunicationInterval / units::milliseconds());
}


unsigned int networkSendBacklog() {
    return s_backlog.value();
}


namespace _internal {

class NetMessage {
public:

    NetMessageType          type;
    NetChannel              channel;
    ENetPacket*             packet;
    ENetPacket*             header;

    /** Only for outgoing messages */
    ENetPeer*               enetPeer;

    /** Only for outgoing messages */
    ENetHost*               enetHost;


    NetMessage() : type(0), channel(0), packet(NULL), header(NULL), enetPeer(NULL), enetHost(NULL) {}


    NetMessage(_ENetPacket* p, _ENetPacket* h, ENetPeer* peer = NULL, ENetHost* host = NULL) : 
        packet(p), header(h), enetPeer(peer), enetHost(host) {
        const uint32* data = reinterpret_cast<const uint32*>(header->data);
        type = ntohl(data[0]);
        channel = ntohl(data[1]);
    }


    void destroy() {
        enet_packet_destroy(packet);
        enet_packet_destroy(header);
        packet = NULL;
        header = NULL;
    }
};
} // namespace _internal

static ThreadsafeQueue<_internal::NetMessage>& sendQueue() {
    static ThreadsafeQueue<_internal::NetMessage> q;
    return q;
}

/** Called from serviceNetwork() */
static void processSendQueue() {
    ThreadsafeQueue<_internal::NetMessage>& queue = sendQueue();

    _internal::NetMessage message;
    while (queue.popFront(message)) {

        debugAssert(notNull(message.enetHost));

        if (isNull(message.enetPeer)) {
            // Must be a NetSendConnection broadcast message
            enet_host_broadcast(message.enetHost, message.channel, message.header);
            enet_host_broadcast(message.enetHost, message.channel, message.packet);
        } else {
            enet_peer_send(message.enetPeer, message.channel, message.header);
            enet_peer_send(message.enetPeer, message.channel, message.packet);
        }
        enet_host_flush(message.enetHost);
    }
}


namespace _internal {
/** State of a NetMessageIterator, indirected from that class so that naive copying 
    NetMessageIterators can be fast and avoid duplicating the actual messages
    in the queue. */
class NetMessageQueue : public ReferenceCountedObject {
protected:
    friend class NetClientSideConnection;
    friend class NetServerSideConnection;

    mutable GMutex           m_mutex;

    /** BinaryInput for the first packet, or NULL */
    BinaryInput*             m_binaryInput;

    /** BinaryInput for the first packet's header, or NULL */
    BinaryInput*             m_headerBinaryInput;

    /** Incoming packets waiting for iterators */
    Queue<NetMessage>        m_packetQueue;

    /** Header packet describing the next packet, which has not yet arrived.
        Set to NULL as soon as that packet arrives */
    _ENetPacket*             m_header;

public:

    NetMessageQueue() : m_binaryInput(NULL), m_headerBinaryInput(NULL), m_header(NULL) {}


    ~NetMessageQueue() {
        GMutexLock lock(&m_mutex);
        delete m_headerBinaryInput;
        delete m_binaryInput;

        // Destroy any unread packets
        for (int i = 0; i < m_packetQueue.size(); ++i) {
            m_packetQueue[i].destroy();
        }
    }


    /** Number of packets in this queue */
    int size() const {
        GMutexLock lock(&m_mutex);
        return m_packetQueue.size();
    }


    void popFrontDiscard() {
        GMutexLock lock(&m_mutex);
        delete m_binaryInput;
        m_binaryInput = NULL;

        delete m_headerBinaryInput;
        m_headerBinaryInput = NULL;

        m_packetQueue[0].destroy();
        m_packetQueue.popFront();
    }


    /** Add this packet to the back of the queue.  Packets come in pairs, 
        where the first is the G3D header and the second is the actual
        data.  The queue automatically keeps track of which this is. 
        
        Called on the network thread.
        */
    void halfPushBack(ENetPacket* p) {
        GMutexLock lock(&m_mutex);
        if (isNull(m_header)) {
            m_header = p;
        } else {
            // This is the data packet
            debugAssertM(m_header->dataLength >= sizeof(uint32) * 2, "Packet is too small");
            m_packetQueue.pushBack(NetMessage(p, m_header));

            // The header packet is no longer needed here
            m_header = NULL;
        }
    }

    // The following methods are called on the application thread...but it is the application's responsibility to verify that there is an element in the queue first,
    // so this code just has to ensure that the queue is not reallocated while being accessed, not make sure that there is something in the queue.
    ENetPacket* packet() const {
        GMutexLock lock(&m_mutex);
        return m_packetQueue[0].packet;
    }


    NetMessageType type() const {
        GMutexLock lock(&m_mutex);
        return m_packetQueue[0].type;
    }


    NetChannel channel() const {
        GMutexLock lock(&m_mutex);
        return m_packetQueue[0].channel;
    }


    BinaryInput& binaryInput() {
        GMutexLock lock(&m_mutex);
        if (isNull(m_binaryInput)) {
            ENetPacket* packet = m_packetQueue[0].packet;
            m_binaryInput = new BinaryInput(packet->data, packet->dataLength, G3D_LITTLE_ENDIAN, false, false);
        }

        return *m_binaryInput;
    }


    BinaryInput& headerBinaryInput() {
        GMutexLock lock(&m_mutex);
        if (isNull(m_headerBinaryInput)) {
            ENetPacket* header = m_packetQueue[0].header;
            const size_t G3D_HEADER_SIZE = 8;
            m_headerBinaryInput = new BinaryInput(reinterpret_cast<uint8*>(header->data) + G3D_HEADER_SIZE, header->dataLength - G3D_HEADER_SIZE, G3D_LITTLE_ENDIAN, false, false);
        }

        return *m_headerBinaryInput;
    }
};}

namespace _internal {
/** A connection that connected to a server */
class NetClientSideConnection : public NetConnection {
protected:
    friend class NetConnection;
    friend void G3D::serviceNetwork();

    void onDisconnect() {
        if (m_status.value() != DISCONNECTED) {
            m_status = DISCONNECTED;
            enet_host_destroy(m_enetHost);
            m_enetHost = NULL;
        }
    }

    virtual void serviceHost() override {
        if (m_sentRecently.value() != 0) {
            updateLatencyEstimate();
        }

        if (isNull(m_enetHost)) { 
            debugAssert(m_status.value() == DISCONNECTED);
            return;
        }

        ENetEvent event;
        int result = 0;
        // Note that the following code assigns result inside the conditional
        while ((m_status.value() != DISCONNECTED) && ((result = enet_host_service(m_enetHost, &event, networkCommunicationIntervalMilliseconds())) > 0)) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                m_enetPeer = event.peer;
                m_status = JUST_CONNECTED;
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                // Insert into the appropriate queue
                m_netMessageIterator.m_queue->halfPushBack(event.packet);
                updateLatencyEstimate();
                break;
       
            case ENET_EVENT_TYPE_DISCONNECT:
                onDisconnect();
                break;

            case ENET_EVENT_TYPE_NONE:
                break;
            } // switch
        } // while

        if (result < 0) {
            // The other side abruptly closed the connection
            onDisconnect();
        }
    }


    NetClientSideConnection(_ENetPeer* p, _ENetHost* h) : NetConnection(p, h) {
        debugAssert(p != NULL);
        debugAssert(h != NULL);
    }

public:

    ~NetClientSideConnection() {
        disconnect(false);
    }

    virtual void disconnect(bool waitForOtherSide) override {
        // Lock the entire system.  Grabbing this lock intentionally prevents serviceNetwork() from making progress or 
        // trying to access any clients.
        GMutexLock lock(&s_allServerAndClientConnectionMutex);
        if (m_status.value() == DISCONNECTED) {
            debugAssert(m_enetHost == NULL);
        } else {        
            NetConnection::disconnect(waitForOtherSide);
            if (! waitForOtherSide) {
                // Destroy my host now since I will not receive more events
                onDisconnect();
            }
        }
    }

};} // NetClientSideConnection


namespace _internal {
/** A connection on the server that was created by some other client.  */
class NetServerSideConnection : public NetConnection {
protected:
    friend class G3D::NetServer;

    // Store a weak pointer to break a cyclic dependency.
    // The server is not kept alive by connections alone and
    // will be shut down if the program drops its own pointers.
    weak_ptr<NetServer>                  m_server;

    NetServerSideConnection(const shared_ptr<NetServer>& s, _ENetPeer* p) : 
        NetConnection(p, s->m_enetHost), m_server(s) {
        debugAssert(notNull(p));
        m_status = JUST_CONNECTED;
    }

    virtual void serviceHost() override {
        if (m_server.expired()) {
            // The caller has dropped all pointers to the 
            // server, thus closing the connection.
            m_status = DISCONNECTED;

            // Drop our pointer to the host, but do not destroy it--that host
            // is shared by all other NetServerSideConnections off our NetServer.
            m_enetHost = NULL;
        } else {
            m_server.lock()->serviceHost();
        }
    }

public:
};} // NetServerSideConnection

// For each peer
static int32 backlogForHost(ENetHost* host) {
    debugAssert(notNull(host));
    int32 b = 0;
    for (unsigned int p = 0; p < host->peerCount; ++p) {
        b += backlogForPeer(host->peers + p);
    }
    return b;
}


void serviceNetwork() {
    // Process the G3D send queue first, filling the enet communication queues in a
    // threadsafe fashion. This way, servicing the host below can empty these queues.
    processSendQueue();

    int32 b = 0;

    // Service all server enet hosts (and flush those that are gone)
    s_allServerAndClientConnectionMutex.lock();
    for (int i = 0; i < s_allServers.length(); ++i) {
        shared_ptr<NetServer> s = s_allServers[i].lock();
        if (notNull(s) && notNull(s->m_enetHost)) {
            b += backlogForHost(s->m_enetHost);
            s->serviceHost();
        } else {
            s_allServers.fastRemove(i);
            --i;
        }
    }

    // Service all client enet hosts (and flush those that are gone)
    for (int i = 0; i < s_allClientConnections.length(); ++i) {
        shared_ptr<_internal::NetClientSideConnection> c = s_allClientConnections[i].lock();
        if (notNull(c) && notNull(c->m_enetHost)) {
            b += backlogForHost(c->m_enetHost);
            c->serviceHost();
        } else {
            s_allClientConnections.fastRemove(i);
            --i;
        }
    }
    s_allServerAndClientConnectionMutex.unlock();

    // Update the estimate of the total network backlog
    s_backlog = b;
}


namespace _internal {
class NetworkThread : public GThread {
public:

    /** Set to 0 by cleanupNetwork() */
    AtomicInt32 keepGoing;

    NetworkThread() : GThread("G3D::NetworkThread"), keepGoing(1) {}

    virtual void threadMain() override {
        while (keepGoing.value() != 0) {

            serviceNetwork();

            // Yield, since we can't access enet's internal select() call to make it process
            // multiple hosts at once and sleep until one is needed
            System::sleep(0);
        }
    }
};}

static shared_ptr<_internal::NetworkThread> s_networkThread;

namespace _internal {
/** Called by System */
void initializeNetwork() {
#   ifdef G3D_WINDOWS
        // Request millisecond accuracy on timers for enet
        timeBeginPeriod(1);
#   endif

    s_networkCommunicationInterval = 0;

    ENetCallbacks callbacks;
    System::memset(&callbacks, 0, sizeof(callbacks));

    callbacks.malloc    = &System::malloc;
    callbacks.free      = &System::free;
    callbacks.no_memory = NULL;
    const int result = enet_initialize_with_callbacks(ENET_VERSION, &callbacks);
    alwaysAssertM(result == 0, format("enet initialization failed with code %d", result));

#   ifdef G3D_DEBUG
    {
        // Verify that G3D and ENet are in sync.  There's no way to test this outside of
        // this file because G3D hides ENet.

        NetAddress  g3dAddr("1.2.3.4", 5);
        ENetAddress enetAddr;
        enet_address_set_host(&enetAddr, "1.2.3.4");
        enetAddr.port = 5;

        ENetAddress enetAddr2 = toENetAddress(g3dAddr);
        debugAssert(enetAddr2.host == enetAddr.host);
        debugAssert(enetAddr2.port == enetAddr.port);
    }
#   endif    
}}

static GMutex s_networkThreadMutex;

static void maybeStartNetworkThread() {
	s_networkThreadMutex.lock();
    if (isNull(s_networkThread) && _internal::g3dInitializationSpecification().threadedNetworking) {
        s_networkThread = shared_ptr<_internal::NetworkThread>(new _internal::NetworkThread());
        s_networkThread->start();
    }
	s_networkThreadMutex.unlock();
}

namespace _internal {
/** Called by cleanupG3D */
void cleanupNetwork() {
    if (notNull(s_networkThread)) {
        s_networkThread->keepGoing = 0;
        // Wait for the thread to shut down
        System::sleep(5 * units::milliseconds());
        s_networkThread.reset();
    }

#   ifdef G3D_WINDOWS
        // End request millisecond accuracy on timers for enet
        timeEndPeriod(1);
#   endif
    enet_deinitialize();
}}


NetConnectionIterator& NetConnectionIterator::operator++() {
    debugAssert(isValid());

    // Check for new messages
    shared_ptr<NetServer> server(m_server.lock());

    m_queue->popFront();
    return *this;
}


bool NetConnectionIterator::isValid() const {
    return (m_queue->size() > 0);
}


shared_ptr<NetConnection> NetConnectionIterator::connection() {
    debugAssert(isValid());
    return (*m_queue)[0];
}

/////////////////////////////////////////////////////////

/** Packets that scheduled a memory manager during NetConnection2::send 
    store their manager in this table. */
static Table<ENetPacket*, _internal::NetworkCallbackInfo>& callbackTable() {
    static Table<ENetPacket*, _internal::NetworkCallbackInfo> t;
    return t;
}


/** Registered callback for all ENet packets with a memory manager.  This is how ENet tells us 
    that it has processed a packet and we are allowed to free the data. */
void freePacketDataCallback(_ENetPacket* packet) {
    ENetPacket* ignore = NULL;

    _internal::NetworkCallbackInfo callbackInfo;
    if (callbackTable().getRemove(packet, ignore, callbackInfo)) {
        callbackInfo.connection->m_freeQueue.pushBack(callbackInfo);
    } else {
        debugPrintf("Warning: tried to free a packet that had no callback registered\n");
    }
}


void NetSendConnection::processFreeQueue() {
    _internal::NetworkCallbackInfo callbackInfo;
    while (m_freeQueue.popFront(callbackInfo)) {
        callbackInfo.manager->free((void*)callbackInfo.data);
    }
}


void addCallback(const shared_ptr<NetSendConnection>& conn, ENetPacket* packet, const shared_ptr<MemoryManager>& manager, const void* data) {
    callbackTable().set(packet, _internal::NetworkCallbackInfo(conn, manager, data));
}



/////////////////////////////////////////////////////////////////////////

shared_ptr<NetServer> NetServer::create
   (const NetAddress&                 myAddress,
    int                               maxClients,
    uint32                            numChannels,  
    size_t                            incomingBytesPerSecondThrottle,
    size_t                            outgoingBytesPerSecondThrottle) {

    maybeStartNetworkThread();
    // Lock the entire system.  Grabbing this lock intentionally prevents serviceNetwork() from making progress or 
    // trying to access any clients.
    GMutexLock lock(&s_allServerAndClientConnectionMutex);

    _ENetAddress addr = toENetAddress(myAddress);
    _ENetHost* host = enet_host_create(&addr, maxClients, numChannels,
        (enet_uint32)incomingBytesPerSecondThrottle, (enet_uint32)outgoingBytesPerSecondThrottle);

    shared_ptr<NetServer> n(new NetServer(host));

    s_allServers.append(n);
    return n;
}


NetServer::NetServer(_ENetHost* h) : 
    m_enetHost(h),
    m_omniConnection(new NetSendConnection(NULL, h)) {
}


NetServer::~NetServer() {
    if (notNull(m_enetHost)) {
        stop();
    }
}


void NetServer::stop() {
    // Lock the entire system.  Grabbing this lock intentionally prevents serviceNetwork() from making progress or 
    // trying to access any clients.
    GMutexLock lock(&s_allServerAndClientConnectionMutex);

    // Shut down all connections.  Can't iterate through the table
    // because events received could cause modification of that table.
    Array< shared_ptr<_internal::NetServerSideConnection> > connections;
    m_client.getValues(connections);

    for (int i = 0; i < connections.size(); ++i) {
        connections[i]->disconnect(false);
    }

    // Flush any pending communication
    enet_host_flush(m_enetHost);

    // Explicitly remove me from the list of servers accessed by serviceNetwork so 
    // that there is not a race between garbage collection and servicing the network
    // after this is shut down.
    shared_ptr<NetServer> me = dynamic_pointer_cast<NetServer>(shared_from_this());
    for (int i = 0; i < s_allServers.size(); ++i) {
        if (s_allServers[i].lock().get() == this) {
            s_allServers.fastRemove(i);
            break;
        }
    }

    enet_host_destroy(m_enetHost);
    m_enetHost = NULL;
}


void NetServer::serviceHost() {
    alwaysAssertM(notNull(m_enetHost), "Cannot perform more actions after NetServer::stop()");
   
    ENetEvent event;
    int result = 0;
    // Note that the following code assigns result inside the conditional
    while ((result = enet_host_service(m_enetHost, &event, networkCommunicationIntervalMilliseconds() )) > 0) {
        switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT: 
            {
                // The server has received a connection.
                debugAssert(notNull(event.peer));
                shared_ptr<_internal::NetServerSideConnection> client(new _internal::NetServerSideConnection(dynamic_pointer_cast<NetServer>(shared_from_this()), event.peer));
                client->m_netMessageIterator.m_connection = client;
                m_newConnectionIterator.m_queue->pushBack(client);
                m_client.set(event.peer, client);
            } break;

        case ENET_EVENT_TYPE_RECEIVE:
            {
                const shared_ptr<_internal::NetServerSideConnection>& client = m_client[event.peer];
                // Insert into the appropriate queue
                client->m_netMessageIterator.m_queue->halfPushBack(event.packet);  
                client->updateLatencyEstimate();
            }
            break;
       
        case ENET_EVENT_TYPE_DISCONNECT:
            // Remove the peer (the queue is reference counted and will still
            // allow iteration of any yet-unread messages).  The peer pointer is
            // no longer valid.
            m_client[event.peer]->m_status = NetConnection::DISCONNECTED;
            m_client[event.peer]->m_enetHost = NULL;

            m_client.remove(event.peer);
            break;

        case ENET_EVENT_TYPE_NONE:
            break;
        } // switch
    } // while

    debugAssert(result == 0);
}

/////////////////////////////////////////////////////////////////////////

/** \sa NetMessageQueue::halfPushBack */
static ENetPacket* makeHeader(NetMessageType type, NetChannel channel, BinaryOutput& userData) {
    if (userData.size() == 0) {
        uint32 data[2];
        data[0] = htonl(type);
        data[1] = htonl(channel);
        return enet_packet_create(data, sizeof(data), ENET_PACKET_FLAG_RELIABLE);
    } else {
        // Copy the data to insert the header
        const size_t dataSize = size_t(8 + userData.size());
        ENetPacket* packet = enet_packet_create(NULL, dataSize, ENET_PACKET_FLAG_RELIABLE);

        uint32* data = (uint32*)(packet->data);
        data[0] = htonl(type);
        data[1] = htonl(channel);

        // Copy the data into the packet, starting 2 x uint32 in
        userData.commit((uint8*)(data + 2));

        return enet_packet_create(data, dataSize, ENET_PACKET_FLAG_RELIABLE);
    }
}


void NetSendConnection::send(NetMessageType type, const void* bytes, size_t size, BinaryOutput& header, NetChannel channel, const shared_ptr<MemoryManager>& memoryManager) {
    beforeSend();

    debugAssert(notNull(m_enetHost));
    const uint32 extraFlags = isNull(memoryManager) ? 0 : ENET_PACKET_FLAG_NO_ALLOCATE;
    ENetPacket* packet = enet_packet_create(bytes, size, ENET_PACKET_FLAG_RELIABLE | extraFlags);

    // Register the callback (in a threadsafe way) before calling
    if (notNull(memoryManager)) {
        packet->freeCallback = &freePacketDataCallback;
        addCallback(dynamic_pointer_cast<NetSendConnection>(shared_from_this()), packet, memoryManager, bytes);
    }

    sendQueue().pushBack(_internal::NetMessage(packet,  makeHeader(type, channel, header), m_enetPeer, m_enetHost));
}


static BinaryOutput emptyHeader;

void NetSendConnection::send(NetMessageType type, const void* bytes, size_t size, NetChannel channel, const shared_ptr<MemoryManager>& memoryManager) {
    send(type, bytes, size, emptyHeader, channel, memoryManager);
}


void NetSendConnection::send(NetMessageType type, BinaryOutput& bo, NetChannel channel) {
    send(type, bo, emptyHeader, channel);
}


void NetSendConnection::send(NetMessageType type, BinaryOutput& bo, BinaryOutput& header, NetChannel channel) {
    beforeSend();
    ENetPacket* packet = enet_packet_create(NULL, size_t(bo.size()), ENET_PACKET_FLAG_RELIABLE);
    bo.commit(packet->data);

    sendQueue().pushBack(_internal::NetMessage(packet,  makeHeader(type, channel, header), m_enetPeer, m_enetHost));
}


void NetSendConnection::enetsend(NetChannel channel, _ENetPacket* packet) {
    enet_host_broadcast(m_enetHost, channel, packet);
}


/////////////////////////////////////////////////////////////////////////

NetMessageIterator::NetMessageIterator() :
    m_queue(new _internal::NetMessageQueue()) {
}


/** Size of the data in bytes. */
size_t NetMessageIterator::size() const {
    alwaysAssertM(isValid(), "Not a valid message!");
    return m_queue->packet()->dataLength;
}


/** The raw data bytes. */
void* NetMessageIterator::data() const {
    alwaysAssertM(isValid(), "Not a valid message!");
    return m_queue->packet()->data;
}


BinaryInput& NetMessageIterator::binaryInput() const {
    alwaysAssertM(isValid(), "Not a valid message!");
    return m_queue->binaryInput();
}


bool NetMessageIterator::isValid() const {
    return m_queue->size() > 0;
}


NetMessageIterator& NetMessageIterator::operator++() {
    debugAssert(isValid());
        
    // Remove the already-processed element
    m_queue->popFrontDiscard();

    return *this;
}


NetMessageType NetMessageIterator::type() const {
    debugAssert(isValid());
    return m_queue->type();
}


NetChannel NetMessageIterator::channel() const {
    debugAssert(isValid());
    return m_queue->channel();    
}

BinaryInput& NetMessageIterator::headerBinaryInput() const {
    debugAssert(isValid());
    return m_queue->headerBinaryInput();
}

///////////////////////////////////////////////////////////////////////

static int32 floatToInt(float f) {
    return *(int32*)(&f);
}

static float intToFloat(int32 i) {
    return *(float*)(&i);
}


NetConnection::NetConnection(_ENetPeer* peer, _ENetHost* host) : 
    NetSendConnection(peer, host), 
    m_status(WAITING_TO_CONNECT),
    m_latency(0),
    m_latencyVariance(floatToInt(finf())) {}


NetAddress NetSendConnection::address() const {
    if (notNull(m_enetPeer)) {
        return NetAddress(ntohl(m_enetPeer->address.host), m_enetPeer->address.port);
    } else {
        return NetAddress(0xFFFF, 0);
    }
}


void NetConnection::beforeSend() {
    if (m_status.value() == JUST_CONNECTED) {
        m_status = CONNECTED;
    }
    m_sentRecently = 1;
}


void NetConnection::updateLatencyEstimate() {
    static const int32 INF = floatToInt(finf());

    if (m_latencyVariance.value() == INF) {
        // First time
        m_latency = floatToInt(float(m_enetPeer->lastRoundTripTime) * units::milliseconds() / 2.0f);
        // When values are scaled by a constant, the variance is scaled by the constant squared: (1/2)^2 = 1/4
        m_latencyVariance = floatToInt((m_enetPeer->lastRoundTripTimeVariance) * square(units::milliseconds() / 2.0f));
    } else {
        // Compute moving average.  Enet is maintaining its own variance estimate, and the way that estimate is
        // computed casts serious doubt on whether it is useful, but we track it as well
        const float emwaRate = 0.1f;
        m_latency = floatToInt(lerp(intToFloat(m_latency.value()), float(m_enetPeer->lastRoundTripTime) * units::milliseconds() / 2.0f, emwaRate));
        m_latencyVariance = floatToInt(lerp(intToFloat(m_latencyVariance.value()), float(m_enetPeer->lastRoundTripTimeVariance) * square(units::milliseconds() / 2.0f), emwaRate));
    }
}


RealTime NetConnection::latency() const {
    return intToFloat(m_latency.value());
}


RealTime NetConnection::latencyVariance() const {
    return intToFloat(m_latencyVariance.value());
}


shared_ptr<NetConnection> NetConnection::connectToServer
    (const NetAddress&                 server, 
     uint32                            numChannels, 
     size_t                            incomingBytesPerSecondThrottle,
     size_t                            outgoingBytesPerSecondThrottle) {
    maybeStartNetworkThread();

    debugAssertM(int(MAX_CHANNELS) == int(ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT),
                 "G3D internal error: MAX_CHANNELS does not match enet constant");

    // Lock the entire system.  Grabbing this lock intentionally prevents serviceNetwork() from making progress or 
    // trying to access any clients.
    s_allServerAndClientConnectionMutex.lock();
    ENetHost* host = enet_host_create(NULL, 1, numChannels, (enet_uint32)incomingBytesPerSecondThrottle, (enet_uint32)outgoingBytesPerSecondThrottle);
    
    const _ENetAddress addr = toENetAddress(server);
    _ENetPeer* peer = enet_host_connect(host, &addr, numChannels, 0);
    
    shared_ptr<_internal::NetClientSideConnection> connection(new _internal::NetClientSideConnection(peer, host));
    connection->m_netMessageIterator.m_connection = connection;

    s_allClientConnections.append(connection);
    s_allServerAndClientConnectionMutex.unlock();

    return connection;
}


NetConnection::~NetConnection() {
    debugAssert(m_status.value() == DISCONNECTED);
}


NetConnection::NetworkStatus NetConnection::status() {
    processFreeQueue();
    return NetworkStatus(m_status.value());
}


#if 0
void NetConnection::debugPrintPeerAndHost() {
    if (notNull(m_enetPeer)) {
        size_t outgoingReliableCommandCount     = enet_list_size(&(m_enetPeer->outgoingReliableCommands));      
        size_t outgoingUnreliableCommandCount   = enet_list_size(&(m_enetPeer->outgoingUnreliableCommands));    
        size_t sentReliableCommandCount         = enet_list_size(&(m_enetPeer->sentReliableCommands));   
        size_t sentUnreliableCommandCount       = enet_list_size(&(m_enetPeer->sentUnreliableCommands));
        size_t dispatchedCommandCount           = enet_list_size(&(m_enetPeer->dispatchedCommands));
        size_t acknowledgementCount             = enet_list_size(&(m_enetPeer->acknowledgements));

        enet_uint32 packetsSent                 = m_enetPeer->packetsSent;
        enet_uint32 packetsLost                 = m_enetPeer->packetsLost;
        enet_uint32 roundTripTime               = m_enetPeer->roundTripTime;


        debugPrintf("______________________\nenet peer stats \n______________________\n" 
                    " Outgoing Reliable Command Count: %d\n Outgoing Unreliable Command Count: %d\n"
                    " Sent Reliable Command Count: %d\n Sent Unreliable Command Count: %d\n"
                    " Dispatched Command Count: %d\n Acknowledgement Count: %d\n______________________\n"
                    " Packets Sent Count: %d\n Packets Lost Count %d\n Mean Round-Trip Time: %d ms\n______________________\n",
                    outgoingReliableCommandCount, outgoingUnreliableCommandCount,
                    sentReliableCommandCount, sentUnreliableCommandCount, dispatchedCommandCount, acknowledgementCount,
                    packetsSent, packetsLost, roundTripTime);
    }
    if (notNull(m_enetHost)) {
        size_t bufferCount          = m_enetHost->bufferCount;
        size_t commandCount         = m_enetHost->commandCount;
        size_t dispatchQueueSize    = enet_list_size(&(m_enetHost->dispatchQueue));
        size_t totalReceivedPackets = m_enetHost->totalReceivedPackets;
        size_t totalSentPackets     = m_enetHost->totalSentPackets;
        debugPrintf("______________________\nenet host stats \n______________________\n" 
            " Buffer Count: %d\n Command Count: %d\n Dispatch Queue Size: %d\n"
            " Total Received Packets: %d\n Total Sent Packets: %d\n",
            bufferCount, commandCount, dispatchQueueSize, totalReceivedPackets, totalSentPackets);
    }
}
#endif

void NetConnection::disconnect(bool waitForOtherSide) {
    // Lock the entire system.  Grabbing this lock intentionally prevents serviceNetwork() from making progress or 
    // trying to access any clients.
    GMutexLock lock(&s_allServerAndClientConnectionMutex);

    if (m_status.value() == DISCONNECTED) {
        // Note that if this is a NetServerSideConnection, then the 
        // host might not have been destroyed, but this connection should no
        // longer have a pointer to it.
        debugAssert(isNull(m_enetHost));
        return;
    }

    if (waitForOtherSide) {
        m_status = WAITING_TO_DISCONNECT;
        enet_peer_disconnect_later(m_enetPeer, 0);
        enet_host_flush(m_enetHost);
        serviceHost();
    } else {
        // Force immediate disconnect (although make a last attempt to service the host)
        serviceHost();
        enet_peer_disconnect_now(m_enetPeer, 0);
        enet_host_flush(m_enetHost);
        serviceHost();
        enet_peer_reset(m_enetPeer);
        m_enetHost = NULL;
        m_status = DISCONNECTED;
    }
}


NetMessageIterator& NetConnection::incomingMessageIterator() {
    // Deallocate anything that was pending deallocation
    processFreeQueue();
    if (m_status.value() == JUST_CONNECTED) {
        m_status = CONNECTED;
    }
    return m_netMessageIterator;
}

//////////////////////////////////////////////

String NetAddress::hostname() const {
    const ENetAddress a = toENetAddress(*this);
    char name[2048];
    const int r = enet_address_get_host(&a, name, sizeof(name));

    if (r == 0) {
        return String(name);
    } else {
        return ipString();
    }
}


String NetAddress::localHostname() {
    static String hostname;
    
    if (hostname.empty()) {
        char ac[2048];
        if (gethostname(ac, sizeof(ac)) == -1) {
            logPrintf("Warning: Error while getting local host name\n");
            hostname = "localhost";
        } else {
            hostname = gethostbyname(ac)->h_name;
        }
    }
    return hostname;
}

} // namspace G3D
