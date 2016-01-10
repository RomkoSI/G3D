package G3D;

import java.lang.IndexOutOfBoundsException;
import java.net.*;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;

/**
 Java implementation of ReliableConduit.
 
 Differences from the C++ version:
 <UL>
  <LI>Created standalone instead of using a NetworkDevice
 </UL> 

 */
public class ReliableConduit {

    private static enum State {RECEIVING, HOLDING, NO_MESSAGE};
    
    long                                    mSent;
    long                                    mReceived;
    long                                    bSent;
    long                                    bReceived;
    SocketChannel                           channel;

    /** Used to implement readPending*/
    Selector                                selector;

    private State                           state;

    private InetSocketAddress               addr;
    
    /**
     Type of the incoming message.
     */
    private int                             messageType;

    /** 
     Total size of the incoming message (read from the header).
     */
    private int                             messageSize;

    /** Shared buffer for receiving messages. */
    private ByteBuffer                      receiveBuffer;

    /** Total size of the receiveBuffer. */
    private int                             receiveBufferTotalSize;

    /** Size occupied by the current message... so far.  This will be
        equal to messageSize when the whole message has arrived. 
      */
    private int                             receiveBufferUsedSize;

    private void sendBuffer(BinaryOutput b) throws IOException {
        if (channel.isConnected()) {
            ByteBuffer tmpBuffer = ByteBuffer.allocate(b.size());

            // Write the header
            tmpBuffer.put(b.getByteArray(), 0, b.size());
            tmpBuffer.rewind();

            // Send the buffer
            int len = b.size();
            int sent = 0;
            int count = 0;

            try {
                final int maxCount = 100;
                while ((sent < len) && (count < maxCount)) {
                    int sentThisTime = channel.write(tmpBuffer);
                    sent += sentThisTime;
                    if (sentThisTime == 0) {
                        // We're not making any progress
                        ++count;
                        try{
                            Thread.sleep(1);
                        } catch (InterruptedException e) {
                        }
                    } else {
                        // If a byte ever gets through, reset our attempt counter
                        count = 0;
                    }
                    //System.out.println("" + sent + "/" + len + " bytes sent.");
                }

                if (count == maxCount) {
                    throw new IOException("Timed out while attempting to send.");
                }
            } catch (IOException e) {
                channel = null;
                throw e;
            }
        }
    }

    /** Returns true if this socket has a read pending. */
    private boolean readWaiting() throws IOException {
        int numReadyForRead = selector.selectNow();

        // System.out.println("readWaiting: " + numReadyForRead + " sockets ready");
        return (numReadyForRead > 0);
    }


    public int waitForMessage() throws java.io.IOException {
	return waitForMessage(0);
    }

    /** Waits for a message to come in and then returns its type. Due to network race conditions,
        may return 0, indicating that there is no message.*/
    public int waitForMessage(int timeOutSeconds) throws java.io.IOException {
	int milliseconds = Math.round(timeOutSeconds * 1000);

	if (waitingMessageType() == 0) {
	    // If the waiting message type is zero that means that there is no message
	    // currently in the local queue, so we have to wait for the next message.
	    if (timeOutSeconds > 0) {
		if (selector.select(milliseconds) == 0) {
		    // Timed out
		    return 0;
		}
	    } else {
		// Wait indefinitely
		selector.select();
	    }
        }

        return waitingMessageType();
    }

    /** Accumulates whatever part of the message (not the header) is
        still waiting on the socket into the receiveBuffer during
        state = RECEIVING mode.  Closes the socket if anything goes
        wrong.  When receiveBufferUsedSize == messageSize, the entire
        message has arrived. */
    private void receiveIntoBuffer() throws IOException {
    
        if (channel.isConnected()) {
            int received = channel.read(receiveBuffer);
            
            // This implementation assumes that SocketChannel.read always appends to the receiveBuffer.
            // Verify that this is actually how SocketChannel works (the docs are ambiguous)
            receiveBufferUsedSize += received;
            assert (receiveBuffer.position() == receiveBufferUsedSize);
            
            if (receiveBuffer.remaining() == 0) {
                state = State.HOLDING;
                assert(receiveBufferUsedSize == messageSize);
            }
            
        }
    }

    private static byte int8ToByte(int x) {
        assert x >= 0 && x <= 255;
        return (byte)x;
    }

    private static int byteToInt8(byte b) {
        if (b < 0) {
            return 256 + (int)b;
        } else {
            return b;
        }
    }

  
    /** Receives the messageType and messageSize from the socket. 
        Returns false if nothing was actually read.
     */
    private boolean receiveHeader() throws IOException {
        ByteBuffer headerBuffer = ByteBuffer.allocate(8);
        headerBuffer.order(ByteOrder.LITTLE_ENDIAN);

        if (channel.isConnected()) {

            int numRead = channel.read(headerBuffer);

            // If nothing
            if (numRead > 0) {

                // Make sure we read the entire header
                if ((numRead == 8) && (headerBuffer.remaining() == 0)) {

                    messageType = headerBuffer.getInt(0);
                    // Size is in network byte order
                    byte sizeArray[] = headerBuffer.array();
                    
                    // The header is big endian according to the G3D spec
                    messageSize =
                        (byteToInt8(sizeArray[4]) << 24) + 
                        (byteToInt8(sizeArray[5]) << 16) +
                        (byteToInt8(sizeArray[6]) << 8) + 
                         byteToInt8(sizeArray[7]);
                    receiveBuffer = ByteBuffer.allocate(messageSize);
                    state = State.RECEIVING;
                    
                    // System.out.println("Received message size = " + messageSize);
                    // System.out.println("Received header = " + sizeArray[4] + ", " + sizeArray[5] + ", " + sizeArray[6] + ", " + sizeArray[7]);
                    return true;
                } else {

                    state = State.NO_MESSAGE;
                    channel.close();
                }
            }
        }
        
        return false;
    }

    public ReliableConduit(InetSocketAddress addr) throws IOException {
        this(SocketChannel.open(addr));
    }

    public ReliableConduit(String hostname, int port) throws IOException {
        this(new InetSocketAddress(hostname, port));
    }

    public ReliableConduit(SocketChannel s) throws IOException {
        channel = s;

        receiveBuffer = ByteBuffer.allocate(0);
        receiveBufferUsedSize = 0;
        messageType = 0;
        messageSize = 0;
        state = State.NO_MESSAGE;

        try {
            // To use select we have to switch to non-blocking mode
            channel.configureBlocking(false);
                                      
            // Create and register with a Selector
            selector = Selector.open();
        } catch (java.nio.channels.ClosedChannelException e) {
            throw new IOException("Could not open Selector: " + 
                                  e.toString());
        }

        channel.register(selector, SelectionKey.OP_READ, channel);

        // Disable Nagel's algorithm to reduce latency
        channel.socket().setTcpNoDelay(true);
    }

    // The message is actually copied from the socket to an internal
    // buffer during this call.  Receive only deserializes.
    public boolean messageWaiting() throws IOException {
        switch (state) {
        case HOLDING:
            //System.out.println("messageWaiting: HOLDING");
            // We've already read the message and are waiting
            // for a receive call.
            return true;

        case RECEIVING:
            if (! ok()) {
                return false;
            }

            // We're currently receiving the message.  Read a little more.
            receiveIntoBuffer();
            
            if (messageSize == receiveBufferUsedSize) {
                // We've read the whole mesage.  Switch to holding state 
                // and return true.
                state = State.HOLDING;
                //System.out.println("messageWaiting: RECEIVING->HOLDING");
                return true;
            } else {
                // There are more bytes left to read.  We'll read them on
                // the next call.  Because the *entire* message is not ready,
                // return false.
                return false;
            }

            // Note that both paths above have a return statement

        case NO_MESSAGE:

            if (readWaiting()) {
                // Loop back around now that we're in the receive state; we
                // may be able to read the whole message before returning 
                // to the caller.
                //System.out.println("messageWaiting: NO_MESSAGE->RECEIVING");
                state = State.RECEIVING;
                receiveHeader();

                return messageWaiting();
            } else if (receiveHeader()) {
                // Sometimes Selector.selectNow returns zero even when a packet is waiting.
                state = State.RECEIVING;
                return messageWaiting();
            } else {
                // No message incoming.
                return false;
            }

            // Note that both paths above have a return statement
        }
        
        return false;
    }

    /**
     Serializes the message and schedules it to be sent as soon as possible,
     and then returns immediately.  The message can be any <B>class</B> with
     a serialize and deserialize method.  On the receiving side,
     use G3D::ReliableConduit::waitingMessageType() to detect the incoming
     message and then invoke G3D::ReliableConduit::receive(msg) where msg
     is of the same class as the message that was sent.

     The actual data sent across the network is preceeded by the
     message type and the size of the serialized message as a 32-bit
     integer.  The size is sent because TCP is a stream protocol and
     doesn't have a concept of discrete messages.
     */
    public void send(int type, BinarySerializable message) throws IOException {
        BinaryOutput binaryOutput = new BinaryOutput(ByteOrder.LITTLE_ENDIAN);
        
        // Write header
        binaryOutput.writeInt32(type);
        binaryOutput.writeInt32(0);  // Reserve space for the size field (will be overwritten later)
        
        // Serialize message
        message.serialize(binaryOutput);
        
        // Correct the message size
        int size = binaryOutput.size() - 8; // Size less header

        //System.out.println("Sent message size (less header) = " + size);
        byte output[] = binaryOutput.getByteArray();
        // Explicitly construct a "network long" (Big-endian) 32-bit int
        output[4] = (byte)((size >> 24) & 0xFF);
        output[5] = (byte)((size >> 16) & 0xFF); 
        output[6] = (byte)((size >> 8) & 0xFF); 
        output[7] = (byte)(size & 0xFF);

        //System.out.println("Sent header = " + output[4] + ", " + output[5] + ", " + output[6] + ", " + output[7]);
        
        sendBuffer(binaryOutput);
    }
    
    /** Send the same message to a number of conduits.  Useful for sending
        data from a server to many clients (only serializes once). */
    static void multisend(
        ReliableConduit[]           array, 
        int                         type,
        BinarySerializable          m) throws IOException {
        
        if (array.length > 0) {
            BinaryOutput binaryOutput = 
                new BinaryOutput(ByteOrder.LITTLE_ENDIAN);

            // Write header
            binaryOutput.writeInt32(type);
            binaryOutput.writeInt32(0);

            // Serialize message
            m.serialize(binaryOutput);
        
            // Go back to the beginning and correct the message size
            int size = binaryOutput.size() - 8; // Size less header
            byte output[] = binaryOutput.getByteArray();
            output[4] = (byte)(size >> 24); 
            output[5] = (byte)(size >> 16); 
            output[6] = (byte)(size >> 8); 
            output[7] = (byte)(size & 0xFF);
        
            for (int i = 0; i < array.length; ++i) {
                array[i].sendBuffer(binaryOutput);
            }
        }
    }

    public int waitingMessageType() throws IOException {
        if (messageWaiting()) {
            return messageType;
        } else {
            return 0;
        }
    }

    public boolean ok() {
        return (channel != null) && channel.isConnected();
    }

    /** 
        If a message is waiting, deserializes the waiting message into
        message and returns true, otherwise returns false.  You can
        determine the type of the message (and therefore, the class
        of message) using G3D::ReliableConduit::waitingMessageType().
     */
    public boolean receive(BinaryDeserializable message) throws IOException {

        if (! messageWaiting()) {
            return false;
        }

        // Deserialize
        BinaryInput b = new BinaryInput(receiveBuffer.array(), 0, 
                                        ByteOrder.LITTLE_ENDIAN, false);
        message.deserialize(b);
        
        // Don't let anyone read this message again.  We leave the buffer
        // allocated for the next caller, however.
        receiveBufferUsedSize = 0;
        state = State.NO_MESSAGE;
        messageType = 0;
        messageSize = 0;
        receiveBuffer.clear();

        // Potentially read the next message.
        messageWaiting();

        return true;
    }

    /** Removes the current message from the queue. */
    public void receive() throws IOException {
        if (! messageWaiting()) {
            return;
        }
        receiveBufferUsedSize = 0;
        state = State.NO_MESSAGE;
        messageType = 0;
        messageSize = 0;
        receiveBuffer.clear();

        // Potentially read the next message.
        messageWaiting();
    }

    public InetSocketAddress address() {
        return addr;
    }
}
