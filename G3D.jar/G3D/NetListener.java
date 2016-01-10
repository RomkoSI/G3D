package G3D;

import java.lang.IndexOutOfBoundsException;
import java.net.*;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;

/**
 Java implementation of NetListener.
 
 Differences from the C++ version:
 <UL>
  <LI>Created standalone instead of using a NetworkDevice
 </UL> 
 */
public class NetListener {
    
    ServerSocketChannel   channel;
    ServerSocket          serverSocket;

    /** Used for clientWaiting */
    Selector              selector;

    /**
       Throws IOException
     */
    public NetListener(int port) throws java.io.IOException {

        // There are two ways of creating a socket in Java. One can directly
        // instantiate a bound ServerSocket, or create a socket channel
        // that produces an unbound socket (which must then be bound).
        // We use channels throughout the G3D Java networking because
        // they allow Select to be used.

        channel = ServerSocketChannel.open();
        serverSocket = channel.socket();
        serverSocket.bind(new InetSocketAddress(port));


        try {
            // To use select we have to switch to non-blocking mode
            channel.configureBlocking(false);
        
            // Create and register with a Selector
            selector = Selector.open();

        } catch (java.nio.channels.ClosedChannelException e) {
            throw new IOException("Could not open Selector: " + 
                                  e.toString());
        }

        assert selector.isOpen();
        assert (channel.validOps() & SelectionKey.OP_ACCEPT) != 0;

        SelectionKey key = 
            channel.register(
                             selector,                              
                             SelectionKey.OP_ACCEPT,
                             channel);
    }


    public ReliableConduit waitForConnection() throws java.io.IOException {
	return waitForConnection(0);
    }

    /** Block until a connection is received.  Returns null in the
	event of a timeout or thread interruption. 

	@param timeOutSeconds If zero, wait indefinitely
    */
    public ReliableConduit waitForConnection(float timeOutSeconds) throws java.io.IOException {

	int milliseconds = Math.round(timeOutSeconds * 1000);

	// Blocking select
	int numReady = 0;

	if (timeOutSeconds > 0) {
	    numReady = selector.select(milliseconds);
 	} else {
	    numReady = selector.select();
	}

        if (numReady == 0) {
	    // Timed out
	    return null;
        }

        SocketChannel clientChannel = channel.accept();

	if (clientChannel == null) {
            throw new java.io.IOException("Accepted clientChannel was null" +
					  " in channel.accept()");
	}

        return new ReliableConduit(clientChannel);
    }

    /** True if a client is waiting (i.e., if waitForConnection will
        return immediately). */
    public boolean clientWaiting() {
        try {
            // Non-blocking select
            int numReadyForRead = selector.selectNow();
            return numReadyForRead > 0;
        } catch (java.io.IOException e) {
            // Something went wrong
            return false;
        }
    }

    public boolean ok() {
        return (serverSocket != null) &&
            (! serverSocket.isClosed()) &&
            serverSocket.isBound();
    }

}

