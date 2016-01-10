/**
\file network.h

   Abstraction of the <a href="http://enet.bespin.org">enet</a>
   protocol for efficient, reliable, sequenced delivery of
   arbitrary-length discrete messages over UDP.  This protocol
   provides similar guarantees to TCP but gives lower latency
   and works better with NAT.

   Not threadsafe--all network routines must be on a single thread.

   \sa NetServer, NetConnection, NetChannel, NetAddress

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2013-01-03
 \edited  2013-04-12
 */   
#ifndef G3D_network_h
#define G3D_network_h
#include "G3D/g3dmath.h"
#include "G3D/Queue.h"
#include "G3D/NetAddress.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "G3D/Log.h"
#include "G3D/AtomicInt32.h"
#include "G3D/ThreadsafeQueue.h"

// enet forward declarations
struct _ENetPacket;
struct _ENetPeer;
struct _ENetHost;
struct _ENetAddress;

namespace G3D {
  
class NetServer;
class NetSendConnection;
class NetConnection;
class NetAddress;

namespace _internal {
    class NetMessageQueue;
    class NetClientSideConnection;
    class NetServerSideConnection;
}

/**
    Messages sent on different channels are asynchronous.  The receiver 
    can't tell which channel a packet arrived on (put that in the packet if you care),
    but this is a way to send out-of-band information, such as transferring a giant file on
    one channel while sending small object updates on another.

    \sa G3D::NetSendConnection::send
*/
typedef uint32 NetChannel;

/** 
  Application defined message type.
  \sa G3D::NetSendConnection::send
 */
typedef uint32 NetMessageType;

/** 
  This is the amount of time G3D will pause the network thread to 
  perform network communication, in seconds (default is 0s). This is equivalent to the timeout value for
  the POSIX select() call.  If G3DSpecification::threadedNetwork is true, then a value of 0 minimizes
  network latency.

  0 = 1 quantum (process already waiting messages, do not wait for new ones). Because the enet protocal requires confirmation messages,
  this can cause the network queue to backup.

  Set this number higher if you are sending large messages and think that you are bottlenecked by not being able to send confirmation messages.
  Set lower if you think the network time is not being fully utilized

   Or just multithread your network code.

   For the current implementation, the effective precision is 1ms, the effective accuracy can be as poor as 50ms on Windows.

   \sa serviceNetwork, G3D::G3DSpecification::threadedNetwork
    */
void setNetworkCommunicationInterval(const RealTime t);

/** \sa setNetworkCommunicationInterval */
RealTime networkCommunicationInterval();

/**
  If NOT using G3D's internal threaded networking, you must invoke this periodically to allow servicing 
  network connections.  State (such as connection status and sending messages) will only update
  inside this call--all other network calls queue for processing.

  If using G3D's internal threaded networking, this is automatically called continuously on 
  a separate thread. 

   \sa setNetworkCommunicationInterval, G3D::G3DSpecification::threadedNetwork
*/
void serviceNetwork();


/** Iterates through new messages on a NetConnection.

    Note that a DISCONNECTED NetConnection may still
    have messages waiting in its queue to be processed.

    Note that it is not possible to construct a NetMessageIterator (since
    nothing good could happen with it), so this must be initialized with a
    reference, e.g.,
    <pre>
        NetMessageIterator& msg = connection->incomingMessageIterator(); 
    </pre>

    \sa NetConnection::newMessageIterator
 */
class NetMessageIterator {
protected:
    friend class _internal::NetClientSideConnection;
    friend class NetConnection;
    friend class NetServer;

    /** The connection that owns this queue */
    weak_ptr<NetConnection>                 m_connection;
    shared_ptr<_internal::NetMessageQueue>  m_queue;

    // To avoid a circular intialization dependency, whatever creates a NetConnection
    // must then assign the m_connection field after both have been constructed.
    NetMessageIterator();

public:

    /** Size of the data in bytes for the current message. */
    size_t size() const;

    /** The raw data bytes for the current message. */
    void* data() const;

    /** Application-defined type header for this message */
    NetMessageType type() const;

    /** Application-defined channel on which this message was sent */
    NetChannel channel() const;

    /** A BinaryInput for the current message.  This is allocated on demand and
        deallocated when the iterator is incremented with operator++.  It is
        shared across copies of the iterator.
    */
    BinaryInput& binaryInput() const;

    /** A BinaryInput for the current message's header.  This is allocated on demand and
        deallocated when the iterator is incremented with operator++.  It is
        shared across copies of the iterator.
    */
    BinaryInput& headerBinaryInput() const;

    /** True if the data() and binaryInput() can be accessed for this
        message. */
    bool isValid() const;

    /** Advance to the next message and deallocate the object referenced by 
        binaryInput. */
    NetMessageIterator& operator++();
};


/**
    Note that it is not possible to construct a NetConnectionIterator (since
    nothing good could happen with it), so this must be initialized with a
    reference, e.g.,
    <pre>
        NetConnectionIterator& client = server->newConnectionIterator(); 
    </pre>

   \sa NetServer::newConnectionIterator 
 */
class NetConnectionIterator {
protected:
    friend class NetServer;

    weak_ptr<NetServer>                             m_server;

    // Store a pointer to the queue so that assignments
    // do not duplicate the queue.
    //
    // Store shared pointers in the queue to prevent the connections
    // from being prematurely deallocated as long as the iterator
    // exists.
    shared_ptr< Queue< shared_ptr<NetConnection> > > m_queue;

    NetConnectionIterator() : m_queue(new Queue< shared_ptr<NetConnection> >()) {}

public:

    /** Connection to this client */
    shared_ptr<NetConnection> connection();
    
    bool isValid() const;

    /** Advance to the next connection */
    NetConnectionIterator& operator++();
};


/** 
  Manages connections for a machine that accepts incoming ones.  This
  is similar to a TCP listener socket, but also supports efficient sending to
  all connected clients.

  If you drop all pointers to a NetServer, then its NetConnection%s will
  be DISCONNECTED but they will still exist and already-received 
  pending messages can be read from them.

  You must invoke some method on NetServer or on a NetConnection created
  from it every frame to service the network on the main thread.
*/
class NetServer : public ReferenceCountedObject {
public:
    friend class _internal::NetServerSideConnection;
    friend class NetConnectionIterator;
    friend void serviceNetwork();

    /** \sa connectToServer */
    enum {UNLIMITED_BANDWIDTH = 0, 
          MAX_CHANNELS = 255};

protected:

    typedef Table<_ENetPeer*, shared_ptr<_internal::NetServerSideConnection> > ClientTable;

    _ENetHost*                                  m_enetHost;

    // Clients hold weak pointers back to the server
    ClientTable                                 m_client;

    shared_ptr<NetSendConnection>               m_omniConnection;
    
    /** Contains the queue */
    NetConnectionIterator                       m_newConnectionIterator;

    NetServer(_ENetHost* host);

    /** Service the ENetHost, checking for incoming messages and connections and depositing them
        in the appropriate queues. Invoked by NetServerSideConnection::serviceHost(), 
        incomingConnectionIterator(), and incomingConnectionIterator(). */
    void serviceHost();

public:
    
    /** Drop all connections and stop listening for new ones. */
    ~NetServer();

    /** 
        \param myAddress Selects which local adapter and port to listen for incoming connections on

        \sa NetConnection::connectToServer
     */
    static shared_ptr<NetServer> create
        (const NetAddress&                 myAddress,
         int                               maxClients = 32,
         uint32                            numChannels = 1, 
         size_t                            incomingBytesPerSecondThrottle = UNLIMITED_BANDWIDTH,
         size_t                            outgoingBytesPerSecondThrottle = UNLIMITED_BANDWIDTH);
    
    /** A connection that sends to all connected clients (not a UDP broadcast to the subnet) */
    shared_ptr<NetSendConnection> omniConnection() {
        return m_omniConnection;
    }

    /** Causes the system to check for new incoming connections and then
        returns an iterator over them. */
    NetConnectionIterator& newConnectionIterator() {
        return m_newConnectionIterator;
    }

    /** Stop listenening for connections and shut down all clients. */
    void stop();
};

class NetSendConnection;

namespace _internal {
/** Information about a memory block that needs to be deallocated on the thread that next makes a call
    to this connection.  These were registered on the same thread (presumably) and will */
class NetworkCallbackInfo {
public:
    /** Connection on which the manager's free should be invoked.  This is 
        needed by the global queue, not the NetSendConnection. */
    shared_ptr<NetSendConnection>   connection;
    
    shared_ptr<MemoryManager>       manager;
    
    /** Data that is to be freed */
    const void*                     data;
    
    NetworkCallbackInfo() : data(NULL) {}
    NetworkCallbackInfo(const shared_ptr<NetSendConnection>& c, const shared_ptr<MemoryManager>& m, const void* d) : connection(c), manager(m), data(d) {}
};
}

/** 
 Base class for NetConnection that provides only the sending
 functionality.  This is only used for NetServer::broadcast(), where
 there is no analogous "broadcast receive".  For most applications,
 use NetConnection.
*/
class NetSendConnection : public ReferenceCountedObject {
protected:
    friend class NetServer;
    friend void addCallback(const shared_ptr<NetSendConnection>& conn, _ENetPacket* packet, const shared_ptr<MemoryManager>& manager, const void* data);

    friend void freePacketDataCallback(_ENetPacket* packet);

    /** NULL for a NetSendConnection, non-null for a NetConnection */
    _ENetPeer*              m_enetPeer;

    _ENetHost*              m_enetHost;

    /** Callbacks to be run the next time any method is invoked */
    ThreadsafeQueue<_internal::NetworkCallbackInfo> m_freeQueue;

    NetSendConnection(_ENetPeer* p, _ENetHost* h) : m_enetPeer(p), m_enetHost(h) {}

    /** Acutally send the packet with enet.  This allows code reuse with NetConnection, which
        has a different sending mechanism. */
    virtual void enetsend(NetChannel channel, _ENetPacket* packet);
    virtual void beforeSend() {}

    void processFreeQueue();

public:

    /** Schedule for sending across this connection.

        \param bytes By default, the memory will be copied, so it is safe to deallocate or change on return

        \param memoryManager If notNull, then this memory manager will be used to free \a bytes once the data
          has been transmitted. Do not modify or free \a bytes after invoking send(). If not provided, the contents 
          of bytes are immediately copied and the caller is able to mutate or free it immediately on return.
          The memoryManagers are queued and can be invoked at the time of any future method invocation. This guarantees
          that they are run on a thread that the application controls (and not the internal network threads).

        Pass a \a memoryManager when sending large packets to avoid the overhead of copying.  Do not 
        pass a memoryManager for small packets, since there is some overhead in initializing the
        deallocation record.

        \param channel See NetChannel

        Messages may be divided over individual packets if large, but
        are always observed outside of the API as atomic.

        Not guaranteed to be delivered if a disconnection
        occurs on either side before the message is completely
        transferred, or if this is currently disconnected.

        \sa networkSendBacklog
    */
    void send(NetMessageType type, const void* bytes, size_t size, NetChannel channel = 0, const shared_ptr<MemoryManager>& memoryManager = shared_ptr<MemoryManager>());
    void send(NetMessageType type, const void* bytes, size_t size, BinaryOutput& header, NetChannel channel = 0, const shared_ptr<MemoryManager>& memoryManager = shared_ptr<MemoryManager>());

    /** Send the contents of this BinaryOutput.

        This is easier and safer to use than the other version of send(), so it is preferred for general use.
        However, this method copies the memory, so it is slightly slower than the other overloaded version of send() for large buffers.

        \sa networkSendBacklog
    */
    void send(NetMessageType type, BinaryOutput& bo, NetChannel channel = 0);

    /** Includes a header.  The header should be fairly small to avoid increasing latency during the extra copies required. */
    void send(NetMessageType type, BinaryOutput& bo, BinaryOutput& header, NetChannel channel = 0);

    /** Address of the other side of the connection */
    virtual NetAddress address() const;
};


/** Return the number of network transactions pending across all NetSendConnection%s. This is related to the underlying UDP packe
    queue and should be proportional to the number of bytes sent, but in
    complicated ways.  If this number grows too large, then NetSendConnection::send is sending data faster
    than either the network can support or the other side can receive.
        
    Use with an arbitrary threshold (e.g., 500) to avoid sending too much data and degrading network
    performance.
*/
unsigned int networkSendBacklog();


/** A network connection between two machines that can send and receive messages. By default, the
    connection is sequenced and reliable. 
    
    \sa NetSendConnection, NetServer 
  */
class NetConnection : public NetSendConnection {
public:
    friend class NetMessageIterator;
    friend class NetServer;
    friend void serviceNetwork();

    enum NetworkStatus {
        WAITING_TO_CONNECT,

        /** 
         Connected to the server and the send() or incomingMessageIterator() methods
         have not yet been invoked.
        */
        JUST_CONNECTED,

        CONNECTED,

        /**
           disconnect() has been invoked and no new messages can be
           sent, but remaining messages are still in transit and will
           be lost if this process terminates or status() is not
           invoked repeatedly until the DISCONNECTED state is reached.
         */
        WAITING_TO_DISCONNECT,

        /** 
            connectToServer() never succeeded in establishing a
            connection, or one end of an established connection was
            terminated.
        */
        DISCONNECTED
    };

    
    /** \sa connectToServer */
    enum {UNLIMITED_BANDWIDTH = NetServer::UNLIMITED_BANDWIDTH, 
          MAX_CHANNELS = NetServer::MAX_CHANNELS};

protected:

    /** This is a NetworkStatus */
    AtomicInt32                     m_status;
    
    /** Contains the incoming message queue */
    NetMessageIterator              m_netMessageIterator;

    /** This is a float stored in an int to make it atomic */
    AtomicInt32                     m_latency;

    AtomicInt32                     m_latencyVariance;

    AtomicInt32                     m_sentRecently;

    NetConnection(_ENetPeer* p, _ENetHost* h);

    void updateLatencyEstimate();
    
    /** Called from serviceNetwork() on the network thread */
    virtual void serviceHost() = 0;

    virtual void beforeSend() override;

public:

    /** To catch accidental dropping of connections that are likely
        bugs, it is an error to destruct when not DISCONNECTED. Note
        that disconnect(false) is guaranteed to immediately disconnect
        but may drop pending messages.
    */
    ~NetConnection();

    /** 
        Return a connection to a server. This does not immediately establish the connection--
        Periodically poll its status() allow it to progrees and to see when the connection 
        succeeds (which may never occur).

        \param numChannels Number of channels (asynchronous message
        queues) to allocate for communication with the server. See MAX_CHANNELS.

        Setting the bandwidth throttles to the actual bandwidth available on 
        the connection may improve performance because optimal packet fragmenting
        is possible.  Setting them lower allows giving preference to another
        NetConnection; setting them higher may result in slightly decreased performance.
      */
    static shared_ptr<NetConnection> connectToServer
    (const NetAddress&                 server, 
     uint32                            numChannels = 1, 
     size_t                            incomingBytesPerSecondThrottle = UNLIMITED_BANDWIDTH,
     size_t                            outgoingBytesPerSecondThrottle = UNLIMITED_BANDWIDTH);

    /** Invoking this can change the status of the connection, so that
        it may be used in a loop condition. */
    NetworkStatus status();

    /** Estimated one-way latency measured from a large number of round trip times.
        The accuracy of the individual round trip times is at best 1ms, although the
        combined average may be more accurate. This average may double-count
        the time for particular messages depending on the send and receive pattern
        of the application, so it is biased.*/
    RealTime latency() const;

    /** A measure of variance for latency(). */
    RealTime latencyVariance() const;

    /** Check the network for new messages and return an iterator over them.
        This always returns the same iterator for a single NetConnection. */
    NetMessageIterator& incomingMessageIterator();

    virtual void disconnect(bool waitForOtherSide = true);

    // virtual void debugPrintPeerAndHost();
};


} // G3D 

#endif // G3D_network_h
