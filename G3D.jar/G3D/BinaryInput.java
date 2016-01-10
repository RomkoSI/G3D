package G3D;

import java.lang.IndexOutOfBoundsException;
import java.lang.Long;
import java.net.*;
import java.io.*;
import java.math.BigInteger;
import java.nio.ByteOrder;

/**
 Sequential or random access byte-order independent binary file
 access. 

 Differences from the C++ version:
 <UL>
  <LI> Can read from streams
  <LI> Can read from URLs
  <LI> No array reads
  <LI> No huge files
  <LI> No compressed files
  <LI> No bit-reading
  <LI> No Vector, Color read methods
 </UL> 

 The primary difference between BinaryInput and java.nio.ByteBuffer is
 that BinaryInput can handle unsigned values, which is important for
 interacting with C++ code and file formats.  It is also convient that
 BinaryInput matches the G3D::BinaryInput API when porting G3D C++
 programs to Java.

 @author Morgan McGuire
 */
public class BinaryInput {

    private final static BigInteger maxUInt64 = 
        BigInteger.valueOf(Long.MAX_VALUE).add(BigInteger.ONE).pow(2).
        subtract(BigInteger.ONE);

    /**
     Index of the next byte in data to be used.
     */
    private int               position;
    private byte              data[];
    private String            filename;
    private ByteOrder         byteOrder;
    
    /** Unlike the C++ API, we must include a byte offset into data
         since there is no way to create a pointer to a subarray. */
    public BinaryInput(
                       byte data[], 
                       int offset, 
                       ByteOrder byteOrder, 
                       boolean copyMemory) {

        this.byteOrder = byteOrder;
        if (copyMemory) {
            this.data = new byte[data.length];
            System.arraycopy(data, offset, this.data, 0, data.length); 
        } else {
            this.data = data;
        }
    }

    /** Copies the data by default */
    public BinaryInput(byte data[], int offset, ByteOrder byteOrder) {
        this(data, offset, byteOrder, true);
    }

    public BinaryInput(URL url, ByteOrder byteOrder) throws IOException {
        this(url.openStream(), byteOrder);
        //        filename = TODO
    }

    public BinaryInput(String filename, ByteOrder byteOrder) 
        throws FileNotFoundException, IOException {

        this(new FileInputStream(filename), byteOrder);
        this.filename = filename;
    }

    /**
     * Closes the stream when initialization is done.
     */
    public BinaryInput(InputStream stream) throws IOException {
        this(stream, ByteOrder.LITTLE_ENDIAN);
    }

    /**
     * Closes the stream when initialization is done.
     */
    public BinaryInput(InputStream stream, 
                       ByteOrder byteOrder) throws IOException {
        this.byteOrder = byteOrder;

        // Read whatever is immediately available
        int dataSize = stream.available();
        byte data[] = new byte[dataSize];
        int numBytes = 0;

        numBytes = stream.read(data, numBytes, dataSize);

        int buffer;
        buffer = stream.read();

        // read returns -1 when stream is empty
        while (buffer > -1) {
            if (numBytes == dataSize) {
                // resize the array
                int newDataSize = dataSize * 2;
                byte newData[] = new byte[newDataSize];
                for (int d = 0; d < dataSize; d++) {
                    newData[d] = data[d];
                }
                data = newData;
                dataSize = newDataSize;
            }
            data[numBytes++] = (byte)buffer;
            buffer = stream.read();
        }

        stream.close();

        if (numBytes == dataSize) {
            this.data = data;
        } else {
            // resize the array
            this.data = new byte[numBytes];
            for (int d = 0; d < numBytes; d++) {
                this.data[d] = data[d];
            }
        }

        position = 0;
    }

    /**
     * Returns the length of the file in bytes.
     */
    public int getLength() {
        return data.length;
    }

    public int size() {
        return getLength();
    }

    public int getPosition() {
        return position;
    }

    public void setPosition(int position) {
        if (position > data.length) {
            throw new IllegalArgumentException
                ("Can't set position past 1 + end of file (file length = " + 
                 data.length + ")");
        } else if (position < 0) {
            throw new IllegalArgumentException
                ("Can't set position below 0");
        }

        this.position = position;
    }

    /** Goes back to the beginning of the file.  */
    public void reset() {
        setPosition(0);
    }

    public int readUInt8() {
        int i = readInt8();
        if (i < 0) {
            i += 256;
        }
        return i;
    }

    public boolean readBool8() {
        return readInt8() != 0;
    }

    public int readUInt16() {
        if (byteOrder == ByteOrder.LITTLE_ENDIAN) {
            return readUInt8() + (readUInt8() << 8);
        } else {
            return (readUInt8() << 8) + readUInt8();
        }
    }

    public long readUInt32() {
        if (byteOrder == ByteOrder.LITTLE_ENDIAN) {
            return 
                readUInt8() + 
                (readUInt8() << 8) + 
                (readUInt8() << 16) + 
                (readUInt8() << 24);
        } else {
            return 
                (readUInt8() << 24) + 
                (readUInt8() << 16) + 
                (readUInt8() << 8) + 
                readUInt8();
        }
    }

    public int readInt8() {
        return (int)data[position++];
    }

    public short readInt16() {
        int i = readUInt16();
        if (i > 32767) {
            i -= 65536;
        }
        return (short)i;
    }

    public int readInt32() {

        long i = readUInt32();
        if (i > 2147483648L) {
            i += 4294967296L;
        }
        return (int)i;
    }

    private BigInteger readInt64BigInteger() {
        byte val[] = new byte[8];        
        if (byteOrder == ByteOrder.LITTLE_ENDIAN) {
            // Reverse the order
            for (int i = 0; i < 8; ++i) {
                val[i] = data[position + 7 - i];
            }
        } else {
            System.arraycopy(data, position, val, 0, 8);
        }

        setPosition(position + 8);

        return new BigInteger(val);
    }

    /** Java does not support unsigned types, so we must return a BigInteger here. 
        Use .longValue to extract a useful value when it is known to be within the
        signed long range. */
    public BigInteger readUInt64() {
        BigInteger big = readInt64BigInteger();
        
        if (big.compareTo(BigInteger.ZERO) == -1) {            
            // The value was negative, so shift up by long value
            return big.add(maxUInt64);
        } else {
            return big;
        }
    }

    public long readInt64() {
        return readInt64BigInteger().longValue();
    }

    public float readFloat32() {
        return Float.intBitsToFloat(readInt32());
    }

    public double readFloat64() {
        return Double.longBitsToDouble(readInt64());
    }

    /** Reads until NULL or the end of the file is encountered. */
    public String readString() {
        int length = 0;
        while (data[position + length] != '\0') {
            ++length;
        }

        return readString(length + 1);
    }
    
    public String readString32() {
        return readString((int)readUInt32());
    }

    public String readString(int length) {
        String s = new String(data, position, length - 1);
        position += length;
        return s;
    }

    /** Reads until NULL or the end of the file is encountered. If the string has odd length 
        (including NULL), reads another byte. */
	public String readStringEven() {
        int length = 0;
        while (data[position + length] != '\0') {
            ++length;
        }

        return readString(length + 2 - (length % 2));
    } 

    /** Skips ahead n bytes. */
    public void skip(int numBytes) {
        position += numBytes;
    }

    public void readBytes(byte bytes[], int n) {
        System.arraycopy(data, position, bytes, 0, n);
        skip(n);
    }

    public boolean hasMore() {
        return position < data.length;
    }

}



