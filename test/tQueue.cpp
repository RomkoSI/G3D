#include "G3D/G3DAll.h"
#include "testassert.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;
#include <deque>

class BigE {
public:
    int x;
    /** Make this structure big */
    int dummy[100];

    BigE() {
        x = 7;
        for (int i = 0; i < 100; ++i) {
            dummy[i] = i;
        }
    }

    ~BigE() {
    }

    BigE(const BigE& a) : x(a.x) {
        for (int i = 0; i < 100; ++i) {
            dummy[i] = a.dummy[i];
        }
    }

    BigE& operator=(const BigE& a) {
        x = a.x;
        for (int i = 0; i < 100; ++i) {
            dummy[i] = a.dummy[i];
        }
        return *this;
    }
};


void perfQueue() {
    printf("Queue Performance:\n");

    uint64 g3dStreamSmall = 0, g3dEnquequeFSmall = 0, g3dEnquequeBSmall = 0;
    uint64 stdStreamSmall = 0, stdEnquequeFSmall = 0, stdEnquequeBSmall = 0;

    uint64 g3dStreamLarge = 0, g3dEnquequeFLarge = 0, g3dEnquequeBLarge = 0;
    uint64 stdStreamLarge = 0, stdEnquequeFLarge = 0, stdEnquequeBLarge = 0;

    int iterations = 1000000;
    int enqueuesize = 10000;
 
    // Number of elements in the queue at the beginning for streaming tests
    int qsize = 1000;
    {
        Queue<int>      g3dQ;
        std::deque<int> stdQ;


        for (int i = 0; i < qsize; ++i) {
            g3dQ.pushBack(i);
            stdQ.push_back(i);
        }

        // Run many times to filter out startup behavior
        for (int j = 0; j < 3; ++j) {

            System::beginCycleCount(g3dStreamSmall);
            {
                for (int i = 0; i < iterations; ++i) {
                    g3dQ.pushBack(g3dQ.popFront());
                }
            }
            System::endCycleCount(g3dStreamSmall);   

            System::beginCycleCount(stdStreamSmall);
            {
                for (int i = 0; i < iterations; ++i) {
                    int v = stdQ[0];
                    stdQ.pop_front();
                    stdQ.push_back(v);
                }
            }
            System::endCycleCount(stdStreamSmall);   
        }
    }
    {
        Queue<int>      g3dQ;
        System::beginCycleCount(g3dEnquequeFSmall);
        {
            for (int i = 0; i < enqueuesize; ++i) {
                g3dQ.pushFront(i);
            }
        }
        System::endCycleCount(g3dEnquequeFSmall);   

        std::deque<int> stdQ;
        System::beginCycleCount(stdEnquequeFSmall);
        {
            for (int i = 0; i < enqueuesize; ++i) {
                stdQ.push_front(i);
            }
        }
        System::endCycleCount(stdEnquequeFSmall);   
    }
    {
        Queue<int>      g3dQ;
        System::beginCycleCount(g3dEnquequeBSmall);
        {
            for (int i = 0; i < enqueuesize; ++i) {
                g3dQ.pushBack(i);
            }
        }
        System::endCycleCount(g3dEnquequeBSmall);   

        std::deque<int> stdQ;
        System::beginCycleCount(stdEnquequeBSmall);
        {
            for (int i = 0; i < enqueuesize; ++i) {
                stdQ.push_back(i);
            }
        }
        System::endCycleCount(stdEnquequeBSmall);   
    }

    /////////////////////////////////
    {
        Queue<BigE>      g3dQ;
        std::deque<BigE> stdQ;


        for (int i = 0; i < qsize; ++i) {
            BigE v;
            g3dQ.pushBack(v);
            stdQ.push_back(v);
        }

        // Run many times to filter out startup behavior
        for (int j = 0; j < 3; ++j) {
            BigE v = stdQ[0];

            System::beginCycleCount(g3dStreamLarge);
            {
                for (int i = 0; i < iterations; ++i) {
                    g3dQ.popFront();
                    g3dQ.pushBack(v);
                }
            }
            System::endCycleCount(g3dStreamLarge);   

            System::beginCycleCount(stdStreamLarge);
            {
                for (int i = 0; i < iterations; ++i) {
                    stdQ.pop_front();
                    stdQ.push_back(v);
                }
            }
            System::endCycleCount(stdStreamLarge);   
        }
    }
    {
        Queue<BigE>      g3dQ;
        System::beginCycleCount(g3dEnquequeFLarge);
        {
            BigE v;
            for (int i = 0; i < enqueuesize; ++i) {
                g3dQ.pushFront(v);
            }
        }
        System::endCycleCount(g3dEnquequeFLarge);   

        std::deque<BigE> stdQ;
        System::beginCycleCount(stdEnquequeFLarge);
        {
            BigE v;
            for (int i = 0; i < enqueuesize; ++i) {
                stdQ.push_front(v);
            }
        }
        System::endCycleCount(stdEnquequeFLarge);   
    }
    {
        Queue<BigE>      g3dQ;
        System::beginCycleCount(g3dEnquequeBLarge);
        BigE v;
        for (int i = 0; i < enqueuesize; ++i) {
            g3dQ.pushBack(v);
        }
        System::endCycleCount(g3dEnquequeBLarge);   

        std::deque<BigE> stdQ;
        System::beginCycleCount(stdEnquequeBLarge);
        {
            BigE v;
            for (int i = 0; i < enqueuesize; ++i) {
                stdQ.push_back(v);
            }
        }
        System::endCycleCount(stdEnquequeBLarge);   
    }


    printf(" Pile-up push front cycles per elt (max queue size = %d)\n", enqueuesize);
    printf("  G3D::Queue<int>             %5.02f\n",  g3dEnquequeFSmall / 
(float)enqueuesize);
    printf("  std::deque<int>             %5.02f\n",  stdEnquequeFSmall / 
(float)enqueuesize);
    printf("  G3D::Queue<BigE>            %5.02f\n",  g3dEnquequeFLarge / 
(float)enqueuesize);
    printf("  std::deque<BigE>            %5.02f\n",  stdEnquequeFLarge / 
(float)enqueuesize);
    printf("\n");

    printf(" Pile-up push back cycles per elt (max queue size = %d)\n", enqueuesize);
    printf("  G3D::Queue<int>             %5.02f\n",  g3dEnquequeBSmall / 
(float)enqueuesize);
    printf("  std::deque<int>             %5.02f\n",  stdEnquequeBSmall / 
(float)enqueuesize);
    printf("  G3D::Queue<BigE>            %5.02f\n",  g3dEnquequeBLarge / 
(float)enqueuesize);
    printf("  std::deque<BigE>            %5.02f\n",  stdEnquequeBLarge / 
(float)enqueuesize);
    printf("\n");

    printf(" Streaming cycles per iteration (queue size = %d)\n", qsize);
    printf("  G3D::Queue<int>             %5.02f\n",  g3dStreamSmall / 
(float)iterations);
    printf("  std::deque<int>             %5.02f\n",  stdStreamSmall / 
(float)iterations);
    printf("  G3D::Queue<BigE>            %5.02f\n",  g3dStreamLarge / 
(float)iterations);
    printf("  std::deque<BigE>            %5.02f\n",  stdStreamLarge / 
(float)iterations);


    printf("\n\n");
}


static String makeMessage(Queue<int>& q) {
    String s = "Expected [ ";
    for (int i = 0; i < q.size(); ++i) {
        s = s + format("%d ", i);
    }
    s = s + "], got [";
    for (int i = 0; i < q.size(); ++i) {
        s = s + format("%d ", q[i]);
    }
    return s + "]";

}


static void _check(Queue<int>& q) {
    for (int i = 0; i < q.size(); ++i) {
        testAssertM(q[i] == (i + 1),
            makeMessage(q));
    }
}


static void testCopy() {
    Queue<int> q1, q2;
    for (int i = 0; i < 10; ++i) {
        q1.pushBack(i);
    }

    q2 = q1;

    for (int i = 0; i < 10; ++i) {
        testAssert(q2[i] == q1[i]);
    }
}


void testQueue() {
    printf("Queue ");

    testCopy();

    {
        Queue<int> q;
        q.pushFront(3);
        q.pushFront(2);
        q.pushFront(1);
        _check(q);
    }


    {
        Queue<int> q;
        q.pushBack(1);
        q.pushBack(2);
        q.pushBack(3);
        _check(q);
    }

    {
        Queue<int> q;
        q.pushFront(2);
        q.pushFront(1);
        q.pushBack(3);
        _check(q);
    }


    { 
        Queue<int> q;
        q.pushFront(2);
        q.pushBack(3);
        q.pushFront(1);
        _check(q);
    }

    {
        Queue<int> q;
        q.pushBack(2);
        q.pushFront(1);
        q.pushBack(3);
        _check(q);
    }

    {
        Queue<int> q;
        q.pushBack(-1);
        q.pushBack(1);
        q.pushBack(2);
        q.pushBack(3);
        q.pushBack(-1);

        q.popFront();
        q.popBack();
        _check(q);
    }

    {
        Queue<int> q;
        q.pushBack(1);
        q.pushBack(2);
        q.pushBack(3);
        q.pushBack(-1);

        q.popBack();
        _check(q);
    }
    {
        Queue<int> q;
        q.pushBack(-1);
        q.pushBack(1);
        q.pushBack(2);
        q.pushBack(3);
 
        q.popFront();
        _check(q);
    }

    // Sanity check queue copying.
    {
        Queue<int> q;
        q.pushBack(1);
        q.pushBack(2);
        q.pushBack(3);
 
        _check(q);

        Queue<int> r(q);
        _check(r);
    }
    
    printf("succeeded\n");
}

#undef check
