#include <G3D/G3DAll.h>
#include <qrencode.h>


/*
IP4 URL format:

http://255.255.255.255:65535
0123456789012345678901234567
0         1         2

The longest possible IP4 URL is 28 bytes. That means that the following encodings are available
for them:

qrencoding  qrversion  mode pixels bytes
Q              2        AN   25x25  29
L              2        8    25x25  32 
H              3        AN   29x29  35
Q              3        8    29x29  32 
H              4        8    33x33  34

Q/2/AN gives the best size and error correction, so there is no reason to use anything else
(assuming that all QR readers can handle the HTTP in upper-case)

However, qr.lib only supports Kanji and 8 mode
*/

static int casesensitive = 0;

// for raw data
static int rawdata = 0;

// The version determines the size of the code.
// Full table: http://www.qrcode.com/en/vertable1.html
static int version = 3;
static int micro = 0;

// Error correction level: L(ow = 7%), M(ed=15%), Q(uartile=25%), H(igh=30%)
static QRecLevel level = QR_ECLEVEL_Q;

// 8 bits/char (see http://en.wikipedia.org/wiki/QR_code)
static QRencodeMode hint = QR_MODE_8;

// C encoder based on sample qr.lib code
static QRcode* encode(const char* intext, int length) {
	QRcode* code;

	if (micro) {
		if (rawdata) {
			code = QRcode_encodeDataMQR(length, (const unsigned char*)intext, version, level);
		} else {
			code = QRcode_encodeStringMQR((char *)intext, version, level, hint, casesensitive);
		}
	} else {
		if (rawdata) {
			code = QRcode_encodeData(length, (const unsigned char*)intext, version, level);
		} else {
			code = QRcode_encodeString((char *)intext, version, level, hint, casesensitive);
		}
	}

	return code;
}


static shared_ptr<PixelTransferBuffer> addressToPTB(const NetAddress& addr) {
    String str = String("HTTP://") + addr.ipString() + ":" + format("%d", addr.port());
    debugAssert(str.size() <= 28);
    QRcode* qrcode = encode(str.c_str(), (int)str.size());

    const int N = qrcode->width;
    shared_ptr<PixelTransferBuffer> buffer = CPUPixelTransferBuffer::create(N, N, ImageFormat::L8());
    
    unorm8* dst = reinterpret_cast<unorm8*>(buffer->mapWrite());
    const unsigned char* src = qrcode->data;

    const unorm8 BLACK = unorm8::fromBits(0x00);
    const unorm8 WHITE = unorm8::fromBits(0xFF);

    for (int i = 0; i < N * N; ++i) {
        // The bytes are in the opposite convention
        dst[i] = (src[i] & 1) ? BLACK : WHITE;
    }
    
    dst = NULL;
    buffer->unmap();
    QRcode_free(qrcode);

    return buffer;
}


shared_ptr<Texture> qrEncodeHTTPAddress(const NetAddress& addr) {
    shared_ptr<PixelTransferBuffer> ptb = addressToPTB(addr);
    return Texture::fromPixelTransferBuffer("QR code", ptb, ImageFormat::L8(), Texture::DIM_2D);
}
