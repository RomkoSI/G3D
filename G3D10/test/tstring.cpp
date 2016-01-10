#include "G3D/G3DAll.h"
#include "testassert.h"
#include "testTreeBuilder.h"

// Expects s to start with "gbuffer_emissive" and the rest of the characters to be non-letters
void helperTestFinds(const String& s) {
    testAssertM(s.find("g", 0) == 0, "String find() broken");
    testAssertM(s.find('g', 0) == 0, "String find() broken");
    testAssertM(s.find('g', 1) == std::string::npos, "String find() broken");
    testAssertM(s.find("gbuffer", 0) == 0, "String find() broken");
    testAssertM(s.find(String("gbuffer"), 0) == 0, "String find() broken");
    testAssertM(s.find("e", 0) == 5, "String find() broken");
    testAssertM(s.find("e", 7) == 8, "String find() broken");
    testAssertM(s.find('e', 9) == 15, "String find() broken");

    testAssertM(s.rfind("b") == 1, "String rfind() broken");
    testAssertM(s.rfind('b') == 1, "String rfind() broken");
    testAssertM(s.rfind('b', 1) == 1, "String rfind() broken");
    testAssertM(s.rfind('b', 0) == std::string::npos, "String rfind() broken");
    testAssertM(s.rfind("gbuffer") == 0, "String rfind() broken");
    testAssertM(s.rfind(String("gbuffer")) == 0, "String rfind() broken");
    testAssertM(s.rfind("e") == 15, "String rfind() broken");
    testAssertM(s.rfind("e", 12) == 8, "String rfind() broken");
    testAssertM(s.rfind('e', 6) == 5, "String rfind() broken");
    testAssertM(s.rfind("ff", 4) == 3, "String rfind() broken");
    testAssertM(s.rfind("ff", 3) == 3, "String rfind() broken");
    testAssertM(s.rfind("ff", 2) == std::string::npos, "String rfind() broken");

    testAssertM(s.find_first_of("fb") == 1, "String find_first_of() broken");
    testAssertM(s.find_first_of('g', 0) == 0, "String find_first_of() broken");
    testAssertM(s.find_first_of("fb", 2) == 3, "String find_first_of() broken");
    testAssertM(s.find_first_of("gbuffer", 0) == 0, "String find_first_of() broken");
    testAssertM(s.find_first_of("gbuffer", 5) == 5, "String find_first_of() broken");
    testAssertM(s.find_first_of("gbuffer", 7) == 8, "String find_first_of() broken");
    testAssertM(s.find_first_of(String("gbuffer"), 0) == 0, "String find_first_of() broken");
    testAssertM(s.find_first_of("we", 0) == 5, "String find_first_of() broken");
    testAssertM(s.find_first_of("xe", 7) == 8, "String find_first_of() broken");
    testAssertM(s.find_first_of("ze", 9) == 15, "String find_first_of() broken");

/*  TODO: find first not of, etc.
    testAssertM(s.find_first_not_of("fb") == 0, "String find_first_not_of() broken");
    testAssertM(s.find_first_not_of('g', 0) == 1, "String find_first_not_of() broken");
    testAssertM(s.find_first_not_of("fb", 2) == 2, "String find_first_not_of() broken");
    testAssertM(s.find_first_not_of("gbuffer", 0) == 7, "String find_first_not_of() broken");
    testAssertM(s.find_first_not_of("gbuffer", 8) == 9, "String find_first_not_of() broken");
    testAssertM(s.find_first_not_of(String("gbuffer"), 0) == 7, "String find_first_not_of() broken");
	
    testAssertM(s.find_last_of("fb") == 4, "String find_last_of() broken");
    testAssertM(s.find_last_of('g', 0) == 0, "String find_last_of() broken");
    testAssertM(s.find_last_of("fb", 2) == 1, "String find_last_of() broken");
    testAssertM(s.find_last_of("gbuffer", 0) == 0, "String find_last_of() broken");
    testAssertM(s.find_last_of("gbuffer", 5) == 5, "String find_last_of() broken");
    testAssertM(s.find_last_of("gbuffer") == 15, "String find_last_of() broken");
    testAssertM(s.find_last_of(String("gbuffer")) == 15, "String find_last_of() broken");
    testAssertM(s.find_last_of("wr", 6) == 6, "String find_last_of() broken");
    testAssertM(s.find_last_of("wr", 5) == std::string::npos, "String find_last_of() broken");
    testAssertM(s.find_last_of("xe", 7) == 5, "String find_last_of() broken");
    testAssertM(s.find_last_of("ze", 9) == 8, "String find_last_of() broken");

    testAssertM(s.find_last_not_of("gbuffrxuiob") == s.length()-1, "String find_last_not_of() broken");
    testAssertM(s.find_last_not_of('g') == s.length()-1, "String find_last_not_of() broken");
*/   
}

/** Testing functions here(http://www.cplusplus.com/reference/string/string/). Doesn't include C++11 functionality, iterator tests, relational operators, stream operators, get_allocator, or getline */
void teststring() {
    
    printf("string...");
    testAssertM(std::string("Hello\n") == String("Hello\n").c_str(), "String.c_str() not equal to std::string");
    testAssertM(replace("abcd","bc","dog")  == "adogd" ,"replace failed");
    testAssertM(replace("aaa" , "a", "aa")  == "aaaaaa","replace failed");
    testAssertM(replace("","a","b")         == "","replace failed");
    testAssertM(replace("abc","","d")       == "abc","replace failed");
    testAssertM(replace("aabac","a","")     == "bc","replace failed");

    testAssertM(findSlash("abc/abc") == 3,"findSlash failed");
    testAssertM(findSlash("abc\\abc") == 3,"findSlash failed");
    testAssertM(findSlash("\abc",2) == String::npos, "findSlash failed");
    const char* gbuffer_emissive = "gbuffer_emissive";
    // Elementary functionality tests
    {
        testAssertM(String(gbuffer_emissive) == String(gbuffer_emissive), "Equality of Strings broken");
        testAssertM(!(String(gbuffer_emissive) == String("gbufferPemissive")), "Equality of Strings broken");
        testAssertM(String("") == String(""), "Equality of Strings broken");
        testAssertM(!(String(" ") == String("   ")), "Equality of Strings broken");

        testAssertM(String(gbuffer_emissive) == gbuffer_emissive, "Equality between String and c_Str broken");
        testAssertM(!(String(gbuffer_emissive) == "gbuffer_emissiv"), "Equality between String and c_str broken");

        testAssertM(strcmp(String(gbuffer_emissive).c_str(), gbuffer_emissive) == 0, ".c_str() broken");

        testAssertM(String("gbuffer") + String("_emissive") == String(gbuffer_emissive), "String + String broken");
        testAssertM(!(String("gbuffer") + String("emissive") == String(gbuffer_emissive)), "String + String broken");

        testAssertM(String("gbuffer") + "_emissive" == String(gbuffer_emissive), "String + c_str broken");
        testAssertM(!(String("gbuffer") + "emissive" == String(gbuffer_emissive)), "String + c_str broken");
    }

    // Capacity tests
    {
        String s(gbuffer_emissive);
        testAssertM(s.size() == s.length(), "String size() != String length()");
        testAssertM(s.size() == 16, "String size() is broken");
        s += s;
        testAssertM(s.size() == 32, "String size() is broken");
        testAssertM(s.capacity() >= s.size(), "String capacity() is broken (less than size)");
        testAssertM(s.max_size() >= s.capacity(), "String max_size() is broken (less than capacity)");

        size_t reserveSize = 500;
        s.reserve(reserveSize);
        testAssertM(s.capacity() >= reserveSize, "String reserve() is broken");

        testAssertM(!s.empty(), "String empty() is broken");
        s.clear();
        testAssertM(s == "", "String clear() is broken");
        testAssertM(s.empty(), "String empty() is broken");
    }

    // Modifier and Element Access Tests
    const String getty_s    = "Four score and seven years ago our fathers brought forth on this continent a new nation, conceived in liberty, and dedicated to the proposition that all men are created equal.\nNow we are engaged in a great civil war, testing whether that nation, or any nation so conceived and so dedicated, can long endure. We are met on a great battlefield of that war. We have come to dedicate a portion of that field, as a final resting place for those who here gave their lives that that nation might live. It is altogether fitting and proper that we should do this.\nBut, in a larger sense, we can not dedicate, we can not consecrate, we can not hallow this ground. The brave men, living and dead, who struggled here, have consecrated it, far above our poor power to add or detract. The world will little note, nor long remember what we say here, but it can never forget what they did here. It is for us the living, rather, to be dedicated here to the unfinished work which they who fought here have thus far so nobly advanced. It is rather for us to be here dedicated to the great task remaining before us葉hat from these honored dead we take increased devotion to that cause for which they gave the last full measure of devotion葉hat we here highly resolve that these dead shall not have died in vain葉hat this nation, under God, shall have a new birth of freedom預nd that government of the people, by the people, for the people, shall not perish from the earth.";
    const char* getty_c_str = "Four score and seven years ago our fathers brought forth on this continent a new nation, conceived in liberty, and dedicated to the proposition that all men are created equal.\nNow we are engaged in a great civil war, testing whether that nation, or any nation so conceived and so dedicated, can long endure. We are met on a great battlefield of that war. We have come to dedicate a portion of that field, as a final resting place for those who here gave their lives that that nation might live. It is altogether fitting and proper that we should do this.\nBut, in a larger sense, we can not dedicate, we can not consecrate, we can not hallow this ground. The brave men, living and dead, who struggled here, have consecrated it, far above our poor power to add or detract. The world will little note, nor long remember what we say here, but it can never forget what they did here. It is for us the living, rather, to be dedicated here to the unfinished work which they who fought here have thus far so nobly advanced. It is rather for us to be here dedicated to the great task remaining before us葉hat from these honored dead we take increased devotion to that cause for which they gave the last full measure of devotion葉hat we here highly resolve that these dead shall not have died in vain葉hat this nation, under God, shall have a new birth of freedom預nd that government of the people, by the people, for the people, shall not perish from the earth.";
    {
        String s0("gbuffer");
        s0 += String("_emissive");
        String s1("gbuffer");
        s1 += String("emissive");
        String s2("gbuffer_");
        s2 += String("emissive");
        String s3(s2);
        testAssertM(s3 == s2, "String(String) is broken");
        testAssertM(s0 == s2, "String += String broken");
        testAssertM(!(s1 == s2), "String += String broken");
        String s4("");
        s4 += getty_c_str;
        testAssertM(getty_s == s4, "Long strings or addition to empty string is broken");
        testAssertM(strcmp(s4.c_str(), getty_c_str) == 0,  "Long strings or addition to empty string is broken");
        s0 += getty_s;
        s3 += getty_s;
        testAssertM(s0==s3, "String += String when second is long is broken");
        testAssertM(!(s2==s3), "String += String when second is long is broken");
        s0 = gbuffer_emissive;
        s0 += '1';
        s0 += '2';
        testAssertM(s0 == "gbuffer_emissive12", "String += char broken");

        s0 = gbuffer_emissive;
        s1 = "";
        testAssertM(s0 != s1, "String reassignment fails");
        for (int i = 0; i < s0.size(); ++i) {
            s1.push_back(s0[i]);
        }
        testAssertM(s0 == s1, "String push_back() is broken");
        s0.append("12");
        testAssertM(s0 == "gbuffer_emissive12", "String append() is broken");
        s0.append("");
        testAssertM(s0 == "gbuffer_emissive12", "String append() is broken for empty string");
        s0.append(s0);
        testAssertM(s0 == "gbuffer_emissive12gbuffer_emissive12", "String append() is broken for appending to itself");
        s0 = "";
        s0.append(gbuffer_emissive, 7);
        testAssertM(s0 == "gbuffer", "String append() is broken."); 
        s0 = "";
        s0.append(String(gbuffer_emissive), 7, 1);
        testAssertM(s0 == "_", "String append() is broken."); 
        s0 = "";
        s0.append(5, '|');
        testAssertM(s0 == "|||||", "String append() is broken."); 
        
        s0.assign(s1);
        testAssertM(s0 == gbuffer_emissive, "String assign() failed");
        s0.clear();
        s0.assign(gbuffer_emissive);
        testAssertM(s0 == gbuffer_emissive, "String assign() failed");
        s0.assign(s1, 2, 100);
        testAssertM(s0 == "uffer_emissive",  "String assign() failed");
        s0.clear();
        s0.assign(s1, 2, std::string::npos);
        testAssertM(s0 == "uffer_emissive",  "String assign() failed");

        s0.clear();
        s0.assign(s1, 2, std::string::npos);
        testAssertM(s0 == "uffer_emissive",  "String assign() failed");
        s0.assign(gbuffer_emissive, 7);
        testAssertM(s0 == "gbuffer",  "String assign() failed");
        s0.assign(7, 'g');
        testAssertM(s0 == "ggggggg",  "String assign() failed");

        // TODO: full insert coverage
        /*
        s0 = gbuffer_emissive;
        s0.insert(7, String("_hastalavista"), 0, 4);
        testAssertM(s0 == "gbuffer_has_emissive", "String insert failed");
        s0 = gbuffer_emissive;
        s0.insert(7, "_has");
        testAssertM(s0 == "gbuffer_has_emissive", "String insert failed");
        */
        s0 = "gbuffer_has_emissive";
        s0.erase(7, 4);
        testAssertM(s0 == "gbuffer_emissive", "String erase failed");
        s0.erase(7, 500);
        testAssertM(s0 == "gbuffer", "String erase failed");

        /* TODO: implement string::replace, string::swap for SSEString
        s0 = gbuffer_emissive;
        s0.replace(7, 1, getty_s);
        testAssertM("gbuffer" + getty_s + "emissive" == s0, "String replace failed");
        
        s0 = gbuffer_emissive;
        s0.replace(7, 1, getty_c_str);
        testAssertM("gbuffer" + getty_s + "emissive" == s0, "String replace failed");

        s0 = gbuffer_emissive;
        s0.replace(7,  100,  getty_c_str, 4);
        testAssertM("gbufferFour" == s0, "String replace failed");

        s0 = gbuffer_emissive;
        s0.replace(7,  1,  5, '|');
        testAssertM("gbuffer|||||emissive" == s0, "String replace failed");

        s0 = gbuffer_emissive;
        s1 = getty_s;
        s0.swap(s1);
        testAssertM(s0 == getty_s, "String swap broken");
        testAssertM(s1 == gbuffer_emissive, "String swap broken");
        */
    }
    
    // String operations (c_str() tested previously, so not included here)
    {
        testAssertM(String(gbuffer_emissive).data()[4] == 'f', "String data() broken");
        String s0(gbuffer_emissive);

        // TODO: implement string::copy
        /*
        char copyBuffer[20];
        s0.copy(copyBuffer, 20, 8); // 20 is arbitatry number greater than remaining length of string
        for(int i = 0; i < s0.length() - 8; ++i) {
            testAssertM(copyBuffer[i] == s0[8+i], "String copy() broken");
        }
        */
              
        helperTestFinds(String(gbuffer_emissive));
        String s1(gbuffer_emissive); 
        s1 += " ";
        helperTestFinds(s1);
        s1.append(200, '|');
        helperTestFinds(s1);


        String s2(gbuffer_emissive);
        testAssertM(s2.substr(0, 7) == "gbuffer", "String substr() broken");
        testAssertM(s2.substr(1, 6) == "buffer", "String substr() broken");
        testAssertM(s2.substr(1, 500) == "buffer_emissive", "String substr() broken");
        s2 += " ";
        testAssertM(s2.substr(0, 7) == "gbuffer", "String substr() broken");
        testAssertM(s2.substr(1, 6) == "buffer", "String substr() broken");
        testAssertM(s2.substr(1, 500) == "buffer_emissive ", "String substr() broken");
        s2.append(200, '|');
        testAssertM(s2.substr(0, 7) == "gbuffer", "String substr() broken");
        testAssertM(s2.substr(1, 6) == "buffer", "String substr() broken");
        testAssertM(s2.substr(1, 500).substr(0,16) == "buffer_emissive ", "String substr() broken");

        String hello_world("hello_world");
        String hello_world_weirdconstruction("hello");
        hello_world_weirdconstruction.append("_");
        hello_world_weirdconstruction = hello_world_weirdconstruction + "world";

        String Hello_world("Hello_world");
        String jello_world("jello_world");
        String hello_word("hello_word");
        String hello_world_plus_getty("hello_world");
        hello_world_plus_getty += getty_s;

        testAssertM(hello_world.compare(hello_world) == 0, "String compare() is broken");
        testAssertM(hello_world.compare("hello_world") == 0, "String compare() is broken");
        testAssertM(hello_world.compare(Hello_world) > 0, "String compare() is broken");
        testAssertM(hello_world.compare(jello_world) < 0, "String compare() is broken");
        testAssertM(hello_world.compare(hello_world_weirdconstruction) == 0, "String compare() is broken");
        testAssertM(hello_world.compare(hello_world_plus_getty) < 0,  "String compare() is broken");
        testAssertM(hello_world.compare(hello_word) > 0,  "String compare() is broken");

    }

    //testing greatest common prefix
    {
        testAssertM(greatestCommonPrefix("", "Hello") == "", "String gcp() broken");
        testAssertM(greatestCommonPrefix("Hello World", "Hello") == "", "String gcp() broken");
        testAssertM(greatestCommonPrefix("Hello World", "Hello G3D") == "Hello ", "String gcp() broken");
        testAssertM(greatestCommonPrefix("test/default", "test/sample") == "test/", "String gcp() broken");
        testAssertM(greatestCommonPrefix("test/", "test/sample") == "test/", "String gcp() broken");
        testAssertM(greatestCommonPrefix("G3D:Cornell Box", "G3D/Cornell Box") == "", "String gcp() broken");
    }

    //prefix tree building tests
    {
        Array<String> hello("Hello G3D", "Hello World", "G3D/scene/demo", "G3D/scene/scene");
        hello.sort();
        testTreeBuilder tree;
        buildPrefixTree(hello, tree);
        testAssertM(tree.output == "-G3D/scene/\n -demo\n -scene\n-Hello \n -G3D\n -World\n", "G3D::buildPrefixTree is broken");

        hello.clear();
        hello.append("Glossy Box", "Glossy Box Water", "Glossy Box Mirror");
        hello.sort();
        tree.clear();
        buildPrefixTree(hello, tree);
        testAssertM(tree.output == "-Glossy \n -Box\n -Box \n  -Mirror\n  -Water\n", "G3D::buildPrefixTree is broken");
    }

    printf(" passed\n");
}

void perfstring() {
}

