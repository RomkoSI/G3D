#include "G3D/G3DAll.h"
#include "testassert.h"

#define CHECK_EXC_POS(e, lnum, chnum)                                       \
        testAssertM((int)(e).line == (int)(lnum) && (int)(e).character == (int)(chnum), "");

#define CHECK_TOKEN_POS(t, lnum, chnum)                                     \
        testAssertM((int)(t).line() == (int)(lnum)                        \
                      && (int)(t).character() == (int)(chnum), "");         

#define CHECK_TOKEN_TYPE(t, typ, etyp)                                      \
        testAssertM((t).type() == (typ), "");                             \
        testAssertM((t).extendedType() == (etyp), "");                    

#define CHECK_SYM_TOKEN(ti, str, lnum, chnum)                               \
    {                                                                       \
        Token _t;                                                           \
        _t = (ti).read();                                                   \
        CHECK_TOKEN_TYPE(_t, Token::SYMBOL, Token::SYMBOL_TYPE);            \
                                                                            \
        CHECK_TOKEN_POS(_t, (lnum), (chnum));                               \
        testAssertM(_t.string() == (str), "");                            \
    }

#define CHECK_END_TOKEN(ti, lnum, chnum)                                    \
    {                                                                       \
        Token _t;                                                           \
        _t = (ti).read();                                                   \
        CHECK_TOKEN_TYPE(_t, Token::END, Token::END_TYPE);                  \
        CHECK_TOKEN_POS(_t, (lnum), (chnum));                               \
    }

#define CHECK_ONE_SPECIAL_SYM(s)                                            \
    {                                                                       \
        TextInput ti(TextInput::FROM_STRING, "\n a" s "b\n ");              \
        CHECK_SYM_TOKEN(ti, "a", 2, 2);                                     \
        CHECK_SYM_TOKEN(ti,   s, 2, 3);                                     \
        CHECK_SYM_TOKEN(ti, "b", 2, 3 + strlen(s));                         \
        CHECK_END_TOKEN(ti,      3, 2);                                     \
    }

static void tfunc1();
static void tfunc2();
static void tfunc3();

void testTextInput2() {
    
    tfunc1();
    tfunc2();
    tfunc3();
    
    // Formerly would loop infinitely if EOF seen in multi-line comment.
    {
        TextInput ti(TextInput::FROM_STRING, "/* ... comment to end");
        CHECK_END_TOKEN(ti, 1, 22);
    }

    // Formerly would terminate quoted string after "foobar", having
    // mistaken \377 for EOF.
    {
        // This is a quoted string "foobarybaz", but with the 'y' replaced by
        // character 0xff (Latin-1 'y' with diaeresis a.k.a. HTML &yuml;).
        // It should parse into a quoted string with exactly those chars.

        TextInput ti(TextInput::FROM_STRING, "\"foobar\377baz\"");
        ti.readString("foobar\377baz");
        CHECK_END_TOKEN(ti, 1, 13);
    }

    {
        TextInput ti(TextInput::FROM_STRING, "[ foo \n  bar\n");
        bool got_exc = false;
        try {
            ti.readSymbols("[", "foo", "]");
        } catch (TextInput::WrongSymbol e) {
            got_exc = true;
            testAssertM(e.expected == "]", "");
            testAssertM(e.actual == "bar", "");
            CHECK_EXC_POS(e, 2, 3);
        }
        testAssertM(got_exc, "");
    }

    // Test file pseudonym creation.
    {
        TextInput ti(TextInput::FROM_STRING, "foo");
        Token t;
        t = ti.read();
        CHECK_TOKEN_TYPE(t, Token::SYMBOL, Token::SYMBOL_TYPE);
        CHECK_TOKEN_POS(t, 1, 1);
        testAssertM(t.string() == "foo", "");
    }
    
    // Test filename override.
    {
        TextInput::Settings tio;
        tio.sourceFileName = "<stdin>";
        TextInput ti(TextInput::FROM_STRING, "foo", tio);
        Token t;
        t = ti.read();
        CHECK_TOKEN_TYPE(t, Token::SYMBOL, Token::SYMBOL_TYPE);
        CHECK_TOKEN_POS(t, 1, 1);
        testAssertM(t.string() == "foo", "");
    }

    // Signed numbers, parsed two different ways
    {
        TextInput t(TextInput::FROM_STRING, "- 5");
        Token x = t.read();
        CHECK_TOKEN_TYPE(x, Token::SYMBOL, Token::SYMBOL_TYPE);
        testAssertM(x.string() == "-", "");
        
        x = t.read();
        CHECK_TOKEN_TYPE(x, Token::NUMBER, Token::INTEGER_TYPE);
        testAssertM(x.number() == 5, "");
    }

    {
        TextInput t(TextInput::FROM_STRING, "0x32");
        Token x = t.read();        
        CHECK_TOKEN_TYPE(x, Token::NUMBER, Token::HEX_INTEGER_TYPE);
        testAssertM(x.number() == 0x32, "");
    }

    {
        TextInput::Settings opt;
        opt.signedNumbers = false;
        TextInput t(TextInput::FROM_STRING, "-5", opt);
        testAssertM(t.readNumber() == -5, "");
    }

    {
        TextInput::Settings opt;
        opt.signedNumbers = false;
        TextInput t(TextInput::FROM_STRING, "- 5", opt);
        try {
            t.readNumber();
            testAssertM(false, "Incorrect parse");
        } catch (...) {
        }
    }

    // Test Nan and inf    
    {
        TextInput::Settings opt;
        opt.msvcFloatSpecials = true;
        TextInput t(TextInput::FROM_STRING, "-1.#INF00", opt);
        double n = t.readNumber();
        testAssertM(n == -inf(), "");
    }
    {
        TextInput::Settings opt;
        opt.msvcFloatSpecials = true;
        TextInput t(TextInput::FROM_STRING, "1.#INF00", opt);
        testAssertM(t.readNumber() == inf(), "");
    }
    {
        TextInput::Settings opt;
        opt.msvcFloatSpecials = true;
        TextInput t(TextInput::FROM_STRING, "-1.#IND00", opt);
        testAssertM(isNaN(t.readNumber()), "");
    }

    {
        TextInput::Settings opt;
        opt.simpleFloatSpecials = true;
        TextInput t(TextInput::FROM_STRING, "-inf", opt);
        double n = t.readNumber();
        testAssertM(n == -inf(), "");
    }
    {
        TextInput::Settings opt;
        opt.simpleFloatSpecials = true;
        TextInput t(TextInput::FROM_STRING, "inf", opt);
        testAssertM(t.readNumber() == inf(), "");
    }
    {
        TextInput::Settings opt;
        opt.simpleFloatSpecials = true;
        TextInput t(TextInput::FROM_STRING, "+inf", opt);
        testAssertM(t.readNumber() == inf(), "");
    }
    {
        TextInput::Settings opt;
        opt.simpleFloatSpecials = true;
        TextInput t(TextInput::FROM_STRING, "nan", opt);
        testAssertM(isNaN(t.readNumber()), "");
    }

    {
        TextInput t(TextInput::FROM_STRING, "fafaosadoas");
        testAssertM(t.hasMore(), "");
        t.readSymbol();
        testAssertM(! t.hasMore(), "");
    }
    
}

static void tfunc1() {
    CHECK_ONE_SPECIAL_SYM(":");
    CHECK_ONE_SPECIAL_SYM("::");

    CHECK_ONE_SPECIAL_SYM("*");
    CHECK_ONE_SPECIAL_SYM("*=");
    CHECK_ONE_SPECIAL_SYM("/");
    CHECK_ONE_SPECIAL_SYM("/=");
    CHECK_ONE_SPECIAL_SYM("!");
    CHECK_ONE_SPECIAL_SYM("!=");
    CHECK_ONE_SPECIAL_SYM("~");
    CHECK_ONE_SPECIAL_SYM("~=");
    CHECK_ONE_SPECIAL_SYM("=");
    CHECK_ONE_SPECIAL_SYM("==");
    CHECK_ONE_SPECIAL_SYM("^");
    // Formerly (mistakenly) tokenized as symbol "^"
    CHECK_ONE_SPECIAL_SYM("^=");	
}

static void tfunc2() {
    CHECK_ONE_SPECIAL_SYM(">");
    CHECK_ONE_SPECIAL_SYM(">>");
    CHECK_ONE_SPECIAL_SYM(">=");
    CHECK_ONE_SPECIAL_SYM("<");
    CHECK_ONE_SPECIAL_SYM("<<");
    CHECK_ONE_SPECIAL_SYM("<=");
    CHECK_ONE_SPECIAL_SYM("|");
    CHECK_ONE_SPECIAL_SYM("||");
    CHECK_ONE_SPECIAL_SYM("|=");
    CHECK_ONE_SPECIAL_SYM("&");
    CHECK_ONE_SPECIAL_SYM("&&");
    CHECK_ONE_SPECIAL_SYM("&=");

    CHECK_ONE_SPECIAL_SYM("\\");

    CHECK_ONE_SPECIAL_SYM(".");
    CHECK_ONE_SPECIAL_SYM("..");
    CHECK_ONE_SPECIAL_SYM("...");	
}

static void tfunc3() {
#define CHECK_ONE_SPECIAL_PROOF_SYM(s)                                      \
    {                                                                       \
        TextInput::Settings ps;                                             \
        ps.proofSymbols = true;                                             \
        TextInput ti(TextInput::FROM_STRING, "\n a" s "b\n ", ps);          \
        CHECK_SYM_TOKEN(ti, "a", 2, 2);                                     \
        CHECK_SYM_TOKEN(ti,   s, 2, 3);                                     \
        CHECK_SYM_TOKEN(ti, "b", 2, 3 + strlen(s));                         \
        CHECK_END_TOKEN(ti,      3, 2);                                     \
    }

    // proof symbols
    CHECK_ONE_SPECIAL_PROOF_SYM("=>");
    CHECK_ONE_SPECIAL_PROOF_SYM("::>");
    CHECK_ONE_SPECIAL_PROOF_SYM("<::");
    CHECK_ONE_SPECIAL_PROOF_SYM(":>");
    CHECK_ONE_SPECIAL_PROOF_SYM("<:");
    CHECK_ONE_SPECIAL_PROOF_SYM("|-");
    CHECK_ONE_SPECIAL_PROOF_SYM("::=");
    CHECK_ONE_SPECIAL_PROOF_SYM(":=");
    CHECK_ONE_SPECIAL_PROOF_SYM("<-");

#undef CHECK_ONE_SPECIAL_PROOF_SYM	
}
