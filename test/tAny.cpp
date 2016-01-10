/**
 \file tAny.cpp
  
 \author Morgan McGuire  
 \author Shawn Yarbrough

 \created 2009-11-03
 \edited  2011-10-30

 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */

#include "G3D/G3DAll.h"
#include "testassert.h"
#include <sstream>

static void testRefCount1() {

    // Explicit allocation so that we can control destruction
    Any* a = new Any(Any::ARRAY);

    Any* b = new Any();
    // Create alias
    *b = *a;

    // a->m_data->referenceCount.m_value should now be 2
    
    delete b;
    b = NULL;

    // Reference count should now be 1

    delete a;
    a = NULL;

    // Reference count should now be zero (and the object deallocated)
}

static void testRefCount2() {

    // Explicit allocation so that we can control destruction
    Any* a = new Any(Any::TABLE);

    // Put something complex the table, so that we have chains of dependencies
    a->set("x", Any(Any::TABLE));

    Any* b = new Any();
    // Create alias
    *b = *a;

    // a->m_data->referenceCount.m_value should now be 2
    
    delete b;
    b = NULL;

    // Reference count should now be 1

    delete a;
    a = NULL;

    // Reference count should now be zero (and the object deallocated)
}

static void testConstruct() {

    {
        Any x(char(3));
        testAssertM(x.type() == Any::NUMBER, Any::toString(x.type())+" when expecting NUMBER");
    }

    {
        Any x(short(3));
        testAssertM(x.type() == Any::NUMBER, Any::toString(x.type())+" when expecting NUMBER");
    }

    {
        Any x(int(3));
        testAssertM(x.type() == Any::NUMBER, Any::toString(x.type())+" when expecting NUMBER");
    }

    {
        Any x(long(3));
        testAssertM(x.type() == Any::NUMBER, Any::toString(x.type())+" when expecting NUMBER");
    }

    {
        Any x(int32(3));
        testAssertM(x.type() == Any::NUMBER, Any::toString(x.type())+" when expecting NUMBER");
    }

    {
        Any x(int64(3));
        testAssertM(x.type() == Any::NUMBER, Any::toString(x.type())+" when expecting NUMBER");
    }

    {
        Any x(3.1);
        testAssertM(x.type() == Any::NUMBER, Any::toString(x.type())+" when expecting NUMBER");
    }

    {
        Any x(3.1f);
        testAssertM(x.type() == Any::NUMBER, Any::toString(x.type())+" when expecting NUMBER");
    }

    {
        Any x(NULL);
        testAssertM(x.type() == Any::NUMBER, Any::toString(x.type())+" when expecting NUMBER");
    }

    {
        Any x(true);
        testAssertM(x.type() == Any::BOOLEAN, Any::toString(x.type())+" when expecting BOOLEAN");
    }

    {
        Any x("hello");
        testAssertM(x.type() == Any::STRING, Any::toString(x.type())+" when expecting STRING");
    }

    {
        Any x(String("hello"));
        testAssertM(x.type() == Any::STRING, Any::toString(x.type())+" when expecting STRING");
    }

    {
        Any y("hello");
        Any x = y;
        testAssertM(x.type() == Any::STRING, Any::toString(x.type())+" when expecting STRING");
    }

}

static void testCast() {
    {
        Any a(3);
        int x = int(a.number());
        testAssert(x == 3);
    }
    {
        Any a(3);
        int x = a;
        testAssert(x == 3);
    }
    {
        Any a(3.1);
        double x = a;
        testAssert(x == 3.1);
    }
    {
        Any a(3.1f);
        float x = a;
        testAssert(fuzzyEq(x, 3.1f));
    }
    {
        Any a(true);
        bool x = a;
        testAssert(x == true);
    }
    {
        Any a("hello");
        String x = a;
        testAssert(x == "hello");
    }
}


static void testPlaceholder() {
    Any t(Any::TABLE);
    
    testAssert(! t.containsKey("hello"));

    // Verify exceptions
    try {
        Any t(Any::TABLE);
        Any a = t["hello"]; 
        testAssertM(false, "Placeholder failed to throw KeyNotFound exception.");
    } catch (const Any::KeyNotFound& e) {
        // Supposed to be thrown
        (void)e;
    } catch (...) {
        testAssertM(false, "Threw wrong exception.");
    }

    try {
        Any t(Any::TABLE);
        t["hello"].number();
        testAssert(false);
    } catch (const Any::KeyNotFound& e) {
    (void)e;}

    try {
        Any t(Any::TABLE);
        Any& a = t["hello"];
        (void)a;
    } catch (const Any::KeyNotFound& e) { 
        testAssert(false);
        (void)e;
    }

    try {
        Any t(Any::TABLE);
        t["hello"] = 3;
    } catch (const Any::KeyNotFound& e) { 
        testAssert(false);
        (void)e;
    }
}


static void testParse() {
    {
        const String& src = "name[ \"foo\", b4r, { a = b, c = d}]";
        Any a = Any::parse(src);
        a.verifyType(Any::ARRAY);
        a[0].verifyType(Any::STRING);
        testAssert(a[0].string() == "foo"); 
        testAssert(a[1].string() == "b4r");
        testAssert(a[2]["a"].string() == "b");
    }
    {
        const String& src =  
        "[v = 1,\r\n/*\r\n*/\r\nx = 1]";
        Any a = Any::parse(src);
        testAssert(a.type() == Any::TABLE);
        testAssert(a.size() == 2);

        Any val1 = a["v"];
        testAssert(val1.type() == Any::NUMBER);
        testAssert(val1.number() == 1);
    }
    {
        const String& src =  
        "{\n\
            val0 : (1);\n\
           \n\
           // Comment 1\n\
           val1 : 3;\n\
           \
           // Comment 2\n\
           // Comment 3\n\
           val2 : None;\n\
           val3 : none;\n\
           val4 : NIL;\n\
        }";

        Any a = Any::parse(src);
        testAssert(a.type() == Any::TABLE);
        testAssert(a.size() == 5);

        Any val1 = a["val1"];
        testAssert(val1.type() == Any::NUMBER);
        testAssert(val1.number() == 3);
        testAssert(val1.comment() == "Comment 1");
        testAssert(a["val2"].isNil());       
        testAssert(a["val3"].string() == "none");
        testAssert(a["val4"].isNil());
    }
    {
        const String& src =
        "(\n\
            //Comment 1\n\
            /**Comment 2*/\n\
            )";
        Any a = Any::parse(src);
        a.verifyType(Any::ARRAY);
        a.verifyType(Any::TABLE);
        testAssert(a.size() == 0);

        a.append(1, 2);
        testAssert(a.size() == 2);
        testAssert(a[0] == 1);
    }
    {
        const String& src = "table{}";
        Any a = Any::parse(src);
        a.verifyType(Any::TABLE);
        a.verifyName("table");
        a.set("val1", 1);
        testAssert(a.size() == 1);
        testAssert(a["val1"] == 1);
    }
    {
         /**tests compatibility with JSON data format*/
        Any a;
        a.load("jsontest.any");
        a.verifyType(Any::TABLE);
    }
}

static void testTableReader() {
    Any a(Any::TABLE);
    a["HI"] = 3;
    a["hello"] = false;

    AnyTableReader r(a);
    float f = 0;
    bool b = true;

    f = 0;
    r.get("HI", f);
    testAssert(f == 3);

    r.get("hello", b);
    testAssert(b == false);
}

void testAny() {

    printf("G3D::Any ");
    testTableReader();
    testParse();

    testRefCount1();
    testRefCount2();
    testConstruct();
    testCast();
    testPlaceholder();

    std::stringstream errss;

    try {

        Any any, any2;

        any.load("Any-load.txt");
        any2 = any;
        if (any != any2) {
            any2.save("Any-failed.txt");
            throw "Two objects of class Any differ after assigning one to the other.";
        }

        any .save("Any-save.txt");
        any2.load("Any-save.txt");
        if (any != any2) {
            any2.save("Any-failed.txt");
            throw "Any-load.txt and Any-save.txt differ.";
        }

        // Trigger the destructors explicitly to help test reference counting
        any = Any();
        any2 = Any();

    } catch( const Any::KeyNotFound& err ) {
        errss << "failed: Any::KeyNotFound key=" << err.key.c_str();
    } catch( const Any::IndexOutOfBounds& err ) {
        errss << "failed: Any::IndexOutOfBounds index=" << err.index << " size=" << err.size;
    } catch( const ParseError& err ) {
        errss << "failed: ParseError: \"" << err.message.c_str() << "\" " << err.filename.c_str() << " line " << err.line << ":" << err.character;
    } catch( const std::exception& err ) {
        errss << "failed: std::exception \"" << err.what() << "\"\n";
    } catch( const String& err ) {
        errss << "failed: String \"" << err.c_str() << "\"\n";
    } catch( const char* err ) {
        errss << "failed: const char* \"" << err << "\"\n";
    }

    if (! errss.str().empty()) {
        testAssertM(false, String(errss.str().c_str()));
    }

    printf("passed\n");

};    // void testAny()
