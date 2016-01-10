#include "G3D/G3DAll.h"
#include "testassert.h"
using G3D::uint8;
using G3D::uint16;
using G3D::uint32;
using G3D::uint64;

class Message {
public:

	int32			i32;
	int64			i64;
	String		s;
	float32			f;

	Message() : i32(Random::common().integer(-10000, 10000)), i64(Random::common().integer(-10000000, 1000000)), f(uniformRandom(-10000, 10000)) {
		for (int x = 0; x < 20; ++x) {
			s = s + (char)('A' + Random::common().integer(0, 26));
		}
	}

	bool operator==(const Message& m) const {
		return 
			(i32 == m.i32) &&
			(i64 == m.i64) &&
			(s == m.s) &&
			(f == m.f);
	}

	void serialize(BinaryOutput& b) const {
		b.writeInt32(i32);
		b.writeInt64(i64);
		b.writeString(s);
		b.writeFloat32(f);
	}

	void deserialize(BinaryInput& b) {
		i32 = b.readInt32();
		i64 = b.readInt64();
		s   = b.readString();
		f	= b.readFloat32();
	}
};


void testReliableConduit(NetworkDevice* nd) {
	printf("ReliableConduit ");
/**
	testAssert(nd);

	{
		uint16 port = 10011;
        NetListenerRef listener = NetListener::create(port);

        ReliableConduitRef clientSide = ReliableConduit::create(NetAddress("localhost", port));
		ReliableConduitRef serverSide = listener->waitForConnection();

		testAssert(clientSide->ok());
		testAssert(serverSide->ok());
	
		int type = 10;
		Message a;
		clientSide->send(type, a);

		// Wait for message
		while (!serverSide->waitingMessageType());

		Message b;
		testAssert((int)serverSide->waitingMessageType() == type);
		serverSide->receive(b);

		testAssert(a == b);

		a = Message();
		serverSide->send(type, a);

		// Wait for message
		while (! clientSide->waitingMessageType());
		testAssert((int)clientSide->waitingMessageType() == type);
		clientSide->receive(b);

		testAssert(a == b);

		// Make sure no more messages are waiting
		testAssert(clientSide->waitingMessageType() == 0);
		testAssert(serverSide->waitingMessageType() == 0);
	}
    */
	//printf("passed\n");
	printf("test disabled\n");
}
