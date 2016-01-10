#include "G3D/G3DAll.h"
#include "testassert.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;
#include <string>

class TGThread : public GThread {
public:
    TGThread(const String& n): GThread(n),
      _value(0) {}

    int value() {
        // Shouldn't need lock
        return _value;
    }

    void incValue() {
        getterMutex.lock();
        ++_value;
        getterMutex.unlock();
    }
protected:
    virtual void threadMain() {
        ++_value;
        return;
    }

    int _value;
    GMutex getterMutex;
};

void testGThread() {
    printf("G3D::GThread ");

    {
        TGThread tGThread("tGThread");
        testAssert(tGThread.value() == 0);

        bool started = tGThread.start();
        testAssert(started);

        tGThread.waitForCompletion();
        testAssert(tGThread.completed());

        testAssert(tGThread.value() == 1);

        tGThread.incValue();
        testAssert(tGThread.value() == 2);
    }

    printf("passed\n");
}

