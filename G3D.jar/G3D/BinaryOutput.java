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
  <LI> No huge files
  <LI> No compressed files
  <LI> No bit-reading
  <LI> No Vector, Color read methods
  <LI> "getByteArray" == "getCArray"
 </UL> 

 @author Morgan McGuire
 */
public class BinaryOutput {

    /**
     Index of the next byte in data to be used.
     */
    private int               position;

    /** Size of the bytes of data that are actually used, as opposed to preallocated */
    private int               dataSize;
    private byte              data[];
    private String            filename;

    private final static BigInteger maxUInt64 = BigInteger.valueOf(Long.MAX_VALUE).add(BigInteger.ONE).pow(2).subtract(BigInteger.ONE);
    private ByteOrder          byteOrder;

    /** Resize dataSize/data as necessary so that there are numBytes of space after the current position */
    private void reserveBytes(int numBytes) {
        if (position + numBytes > data.length) {
            byte temp[] = data;

            // Overestimate the amount of space needed
            data = new byte[(position + numBytes) * 2];
            System.arraycopy(temp, 0, data, 0, temp.length);
        }

        dataSize = Math.max(position + numBytes, dataSize);
    }

    public BinaryOutput(ByteOrder byteOrder) {
        this.byteOrder = byteOrder;
        dataSize = 0;
        filename = "";
        position = 0;
        data = new byte[128];
    }
    
    public BinaryOutput(String filename, ByteOrder byteOrder) {
        this.byteOrder = byteOrder;
        this.filename = filename;
        dataSize = 0;
        position = 0;
        data = new byte[128];
    }

    byte[] getByteArray() {
        return data;
    }

    /**
     * Returns the length of the file in bytes.
     */
    public int getLength() {
        return dataSize;
    }

    public int size() {
        return getLength();
    }

    public int getPosition() {
        return position;
    }

    public void setPosition(int position) {
        if (position < 0) {
            throw new IllegalArgumentException
                ("Can't set position below 0");
        }
        
        if (position > dataSize) {
            int p = position;
            position = dataSize;
            reserveBytes(dataSize - p);
        }

        this.position = position;
    }

    /** Goes back to the beginning of the file.  */
    public void reset() {
        setPosition(0);
    }

    public void writeUInt8(int v) {
        reserveBytes(1);
        data[position++] = (byte)v;
    }

    public void writeBytes(byte[] v, int size) {
        reserveBytes(size);
        System.arraycopy(v, 0, data, position, size);
        position += size;
    }

    public void writeInt8(int v) {
        reserveBytes(1);
        data[position++] = (byte)v;
    }

    public void writeBool8(boolean v) {
        writeInt8(v ? 1 : 0);
    }

    public void writeUInt16(int v) {
        if (byteOrder == ByteOrder.LITTLE_ENDIAN) {
            writeUInt8(v & 0xFF);
            writeUInt8(v >> 8);
        } else {
            writeUInt8(v >> 8);
            writeUInt8(v & 0xFF);
        }
    }

    public void writeInt32(int v) {
        if (byteOrder == ByteOrder.LITTLE_ENDIAN) {
            writeInt8(v & 0xFF); 
            writeInt8(v >> 8); 
            writeInt8(v >> 16); 
            writeInt8(v >> 24);
        } else {
            writeInt8(v >> 24); 
            writeInt8(v >> 16); 
            writeInt8(v >> 8); 
            writeInt8(v & 0xFF);
        }
    }
    
    public void writeUInt32(int v) {
        if (byteOrder == ByteOrder.LITTLE_ENDIAN) {
            writeUInt8(v & 0xFF); 
            writeUInt8(v >> 8); 
            writeUInt8(v >> 16); 
            writeUInt8(v >> 24);
        } else {
            writeUInt8(v >> 24); 
            writeUInt8(v >> 16); 
            writeUInt8(v >> 8); 
            writeUInt8(v & 0xFF);
        }
    }

    public void writeUInt64(BigInteger big) {
        if (byteOrder == ByteOrder.LITTLE_ENDIAN) {
            // Reverse the order
            for (int i = 0; i < 8; ++i) {
                writeUInt8(big.shiftRight(8 * i).intValue());
            }
        } else {
            for (int i = 7; i >= 0; i--) {
                writeUInt8(big.shiftRight(8 * i).intValue());
            }            
        }
    }

    public void writeInt64(long v) {
        if (byteOrder == ByteOrder.LITTLE_ENDIAN) {
            // Reverse the order
            for (int i = 0; i < 8; ++i) {
                writeUInt8((int)(v >> (8 * i)));
            }
        } else {
            for (int i = 7; i >= 0; i--) {
                writeUInt8((int)(v >> (8 * i)));
            }            
        }
    }

    public void writeFloat32(float v) {
        writeInt32(Float.floatToRawIntBits(v));
    }

    public void writeFloat64(double v) {
        writeInt64(Double.doubleToRawLongBits(v));
    }
    
    public void writeString(String str) {
        byte characters[] = str.getBytes();
        
        for (int i = 0; i < characters.length; ++i) {
            writeInt8(characters[i]);
        }
        
        writeInt8(0);
    }

    public void writeString32(String str) {
        writeUInt32(str.getBytes().length + 1);
        writeString(str);
    }

    /** Skips ahead n bytes. */
    public void skip(int numBytes) {
        reserveBytes(numBytes);
        position += numBytes;
    }

    public boolean hasMore() {
        return position < data.length;
    }
    
    public void commit() throws FileNotFoundException, IOException {
        FileOutputStream output = new FileOutputStream(filename);

        output.write(data, 0, dataSize);        
        output.flush();
    }

}
