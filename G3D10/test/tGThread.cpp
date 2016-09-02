#include "G3D/G3DAll.h"
#include "testassert.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;
#include <string>

class TThread : public Thread {
public:
    TThread(const String& n): Thread(n),
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

void testThread() {
    printf("G3D::Thread ");

    {
        TThread tThread("tThread");
        testAssert(tThread.value() == 0);

        bool started = tThread.start();
        testAssert(started);

        tThread.waitForCompletion();
        testAssert(tThread.completed());

        testAssert(tThread.value() == 1);

        tThread.incValue();
        testAssert(tThread.value() == 2);
    }

    printf("passed\n");
}

