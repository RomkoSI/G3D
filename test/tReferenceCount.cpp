#include "G3D/G3DAll.h"
#include "testassert.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;
#ifdef G3D_LINUX
#     include <pthread.h>
#endif

class WKFoo : public ReferenceCountedObject {
public:

    String name;

    WKFoo(const String& x) : name(x) {
        //printf("Foo(\"%s\") = 0x%x\n", name.c_str(), this);
    }

    ~WKFoo() {
        //printf("~Foo(\"%s\") = 0x%x\n", name.c_str(), this);
    }
};
typedef shared_ptr<WKFoo>     WKFooRef;
typedef weak_ptr<WKFoo> WKFooWeakRef;


namespace Circle {
class A;
class B : public G3D::ReferenceCountedObject {
public:
    weak_ptr<A> weakRefToA;
};

class A : public G3D::ReferenceCountedObject {
public:
    shared_ptr<B> refToB;
};
}

void testCycle() {
    shared_ptr<Circle::A> a(new Circle::A());
    a->refToB.reset(new Circle::B());
    a->refToB->weakRefToA = a;
    a.reset();
}

void testWeakPointer() {
    printf("weak_ptr ");

    testCycle();

    WKFooWeakRef wB;
    {
        WKFooRef A(new WKFoo("A"));

        WKFooWeakRef wA(A);

        testAssert(notNull(wA.lock()));

        A.reset();

        testAssert(isNull(wA.lock()));
        
        WKFooRef B(new WKFoo("B"));
        
        A = B;
        testAssert(isNull(wA.lock()));
        testAssert(isNull(wB.lock()));
        
        wA.reset();

        testAssert(isNull(wA.lock()));

        wB = B;

        testAssert(A == B);
        //testAssert(wA == wB);

        wA.reset();
        testAssert(isNull(wA.lock()) == true);

        {
            WKFooRef C(new WKFoo("C"));
        }

        testAssert(isNull(wB.lock()) == false);
    }
    testAssert(isNull(wB.lock()) == true);

    printf("passed\n");
}


int numRCPFoo = 0;
class RCPFoo : public G3D::ReferenceCountedObject {
public:
    int x;
    RCPFoo() {
        ++numRCPFoo;
    }
    ~RCPFoo() {
        --numRCPFoo;
    }
};

typedef shared_ptr<RCPFoo> RCPFooRef;



class RefSubclass : public RCPFoo {
};
typedef shared_ptr<RefSubclass> RefSubclassRef;


class Reftest : public ReferenceCountedObject {
public:
    static Array<String> sequence;
    const char* s;
    Reftest(const char* s) : s(s){
#ifdef G3D_64BIT
      debugPrintf("alloc 0x%lx (%s)\n", long((intptr_t)this), s);
#else
      debugPrintf("alloc 0x%x (%s)\n", int((intptr_t)this), s);
#endif
        sequence.append(format("%s", s));
    }
    ~Reftest() {
#ifdef G3D_64BIT
        debugPrintf("free 0x%lx (~%s)\n", long((intptr_t)this), s);
#else
        debugPrintf("free 0x%x (~%s)\n", int((intptr_t)this), s);
#endif
        sequence.append(format("~%s", s));
    }
};
class Reftest2 : public Reftest {
public:
    Reftest2() : Reftest("2") {
    }
};
typedef shared_ptr<Reftest> ARef;
typedef shared_ptr<Reftest2> ARef2;
Array<String> Reftest::sequence;


/* Called from testRCP to test automatic down-casting */
static void subclasstest(const RCPFooRef& b) {
    (void)b;
}

static void testRCP() {
    printf("shared_ptr ");

    testAssert(numRCPFoo == 0);
    RCPFooRef a(new RCPFoo());
    testAssert(numRCPFoo == 1);
    testAssert(a.unique());

    {
        RCPFooRef b(new RCPFoo());
        testAssert(numRCPFoo == 2);
        b = a;
        testAssert(numRCPFoo == 1);
        testAssert(! a.unique());
        testAssert(! b.unique());
    }

    testAssert(a.unique());
    testAssert(numRCPFoo == 1);


    // Test allocation and deallocation of 
    // reference counted values.
    {
        ARef a(new Reftest("a"));
        ARef b(new Reftest("b"));

        a = b;
        Reftest::sequence.append("--");
        debugPrintf("---------\n");
        b.reset();
        Reftest::sequence.append("--");
        debugPrintf("---------\n");
    }

    testAssert(Reftest::sequence[0] == "a");
    testAssert(Reftest::sequence[1] == "b");
    testAssert(Reftest::sequence[2] == "~a");
    testAssert(Reftest::sequence[3] == "--");
    testAssert(Reftest::sequence[4] == "--");
    testAssert(Reftest::sequence[5] == "~b");

    Reftest::sequence.clear();

    // Test type hierarchies with reference counted values.

    {
        ARef one(new Reftest("1"));
        ARef2 two(new Reftest2());

        one = (ARef)two;
    }
    testAssert(Reftest::sequence[0] == "1");
    testAssert(Reftest::sequence[1] == "2");
    testAssert(Reftest::sequence[2] == "~1");
    testAssert(Reftest::sequence[3] == "~2");
    Reftest::sequence.clear();

    {
        ARef one(new Reftest2());
    }
    testAssert(Reftest::sequence[0] == "2");
    testAssert(Reftest::sequence[1] == "~2");
    Reftest::sequence.clear();

    {
        // Should not compile
//        ARef2 one = new Reftest("1");
    }


    // Test subclassing

    {
        RefSubclassRef s(new RefSubclass());
        RCPFooRef b;

        // s is a subclass, so this should call the templated
        // constructor and succeed.
        b = s;

        // Likewise
        subclasstest(s);
    }
	
	printf("passed.\n");
}


void testReferenceCount() {
    testWeakPointer();
    testRCP();
}
