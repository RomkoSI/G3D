#ifndef testassert_h
#define testassert_h

#define ON_BUILD_SERVER 0

#if ON_BUILD_SERVER
    #define testAssert(exp) testAssertM(exp, "Assertion failed.")
    #define testAssertM(exp, message) do { \
        if (!(exp)) { \
            fprintf(stderr, "TEST FAILURE: %s:%s ::: %s(%d)\n", (const char*)(#exp), String(message).c_str(), __FILE__, __LINE__); \
        } \
    } while (0)
#else
    #define testAssert(exp)         debugAssert(exp)
    #define testAssertM(exp, msg)   debugAssertM(exp, msg)
#endif

#endif
