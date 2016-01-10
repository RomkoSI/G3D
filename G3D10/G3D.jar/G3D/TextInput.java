package G3D;

import java.io.*;

/**
 * Differences from the C++ version:
 *   - No push method
 *   - Exceptions not thrown on bad types
 *
 */
public class TextInput {

    public static class Settings {
        public boolean          cComments                       = true;
        public boolean 	        cppComments                     = true;
        public boolean          escapeSequencesInStrings        = true;
        public int              otherCommentCharacter           = 0;
        public int              otherCommentCharacter2          = 0;
        public boolean          signedNumbers                   = true;
        public boolean          singleQuotedStrings             = true;
        public String           sourceFileName                  = "";
        public int              startingLineNumberOffset        = 0;
        public boolean          msvcSpecials                    = false;

        public Settings clone() {
            Settings s = new Settings();

            s.cComments = cComments;
            s.cppComments = cppComments;
            s.escapeSequencesInStrings = escapeSequencesInStrings;
            s.otherCommentCharacter = otherCommentCharacter;
            s.otherCommentCharacter2 = otherCommentCharacter2;
            s.signedNumbers = signedNumbers;
            s.singleQuotedStrings = singleQuotedStrings;
            s.sourceFileName = sourceFileName;
            s.startingLineNumberOffset = startingLineNumberOffset;
            s.msvcSpecials = msvcSpecials;

            return s;
        }
    }

    public static class Token {
        public static final int STRING = 0;
        public static final int SYMBOL = 1;
        public static final int NUMBER = 2;
        public static final int END    = 3;

        public static final int SINGLE_QUOTED_TYPE = 0;
        public static final int DOUBLE_QUOTED_TYPE = 1;
        public static final int SYMBOL_TYPE        = 2;
        public static final int INTEGER_TYPE       = 3;
        public static final int FLOATING_POINT_TYPE = 4;
        public static final int END_TYPE            = 5;

        private int     m_type;
        private int     m_extendedType;
        private String  m_string;
        private int     m_line;
        private int     m_character;

        public Token (int type, int extendedType, String s, int L, int c) {
            m_type = type;
            m_extendedType = extendedType;
            m_string = s;
            m_line = L;
            m_character = c;
        }

        public int type() {
            return m_type;
        }

        public int extendedType() {
            return m_extendedType;
        }

        public String string() {
            return m_string;
        }

        public int line() {
            return m_line;
        }

        /** Always zero in the current Java implementation. */
        public int character() {
            return m_character;
        }

        public double number() {
            return Double.parseDouble(m_string);
        }
    }

    private Settings        m_settings;
    private StreamTokenizer tokenizer;

    /** Settings are not cloned */
    private TextInput(Reader reader, Settings settings) {
        m_settings = settings;

        tokenizer = new StreamTokenizer(reader);

        // Configure based on settings
        tokenizer.parseNumbers();
        tokenizer.eolIsSignificant(false);
        tokenizer.slashStarComments(m_settings.cComments);
        tokenizer.slashSlashComments(m_settings.cppComments);
        tokenizer.lowerCaseMode(false);

        tokenizer.quoteChar('\"');//ASCII 34
        if (m_settings.singleQuotedStrings) {
            tokenizer.quoteChar('\''); // ASCII 39
        }

        if (m_settings.otherCommentCharacter != 0) {
            tokenizer.commentChar(m_settings.otherCommentCharacter);
        }

        if (m_settings.otherCommentCharacter2 != 0) {
            tokenizer.commentChar(m_settings.otherCommentCharacter2);
        }
    }

    public static TextInput fromString(String string, Settings settings) {
        return new TextInput(new StringReader(string), settings.clone());
    }

    public static TextInput fromString(String string) {
        return TextInput.fromString(string, new Settings());
    }

    public static TextInput fromFile(String filename) throws FileNotFoundException {
        return TextInput.fromFile(filename, new Settings());
    }

    public static TextInput fromFile(String filename, Settings settings) throws FileNotFoundException{
        Settings s = settings.clone();
        s.sourceFileName = filename;
        return new TextInput(new FileReader(filename), s);
    }

    public static TextInput fromFile(File file) throws FileNotFoundException {
        return TextInput.fromFile(file.toString());
    }

    public static TextInput fromFile(File file, Settings settings) throws FileNotFoundException {
        return TextInput.fromFile(file.toString(), settings.clone());
    }

    public boolean hasMore() {
        try {
            boolean isEof = (tokenizer.nextToken() == StreamTokenizer.TT_EOF);
            tokenizer.pushBack();
            return ! isEof;
        } catch (IOException e) {
            return false;
        }
    }

    public Token read() {
        int C = 0;
        int L = tokenizer.lineno();

        int type = Token.SYMBOL;
        int extendedType = Token.SYMBOL_TYPE;

        try {
            tokenizer.nextToken();
        } catch (IOException e) {
            return new Token(Token.END, Token.END_TYPE, "", L, C);
        }

        switch (tokenizer.ttype) {
        case StreamTokenizer.TT_NUMBER:

            // Java can't distinguish int and float tokens
            extendedType = Token.FLOATING_POINT_TYPE;

            return new Token(Token.NUMBER, extendedType, "" + tokenizer.nval, L, C);

        case StreamTokenizer.TT_WORD:
            // TODO: handle MSVC specials

            type = Token.SYMBOL;
            extendedType = Token.SYMBOL_TYPE;

            if (tokenizer.sval.startsWith("\"")) {
                type = Token.STRING;
                extendedType = Token.DOUBLE_QUOTED_TYPE;
            }

            if (m_settings.singleQuotedStrings && tokenizer.sval.startsWith("\'")) {
                type = Token.STRING;
                extendedType = Token.SINGLE_QUOTED_TYPE;
            }

            return new Token(type, extendedType, tokenizer.sval, L, C);

        case StreamTokenizer.TT_EOF:
            return new Token(Token.END, Token.END_TYPE, "", L, C);

        case StreamTokenizer.TT_EOL:
            // We don't consider EOL significant, so ignore this token
            return read();

        case '\'':
        case '\"':
            // Quoted string
            extendedType = Token.DOUBLE_QUOTED_TYPE;

            if (m_settings.singleQuotedStrings && (tokenizer.ttype == '\'')) { // ASCII 39
               extendedType = Token.SINGLE_QUOTED_TYPE;
            }

            return new Token(Token.STRING, extendedType, tokenizer.sval, L, C);

        default:
            // 1-character symbol
            return new Token(Token.SYMBOL, extendedType, new Character((char)tokenizer.ttype).toString(), L, C);
        }
    }

    public double readNumber() {
        // TODO: Throw an exception if this is not a number
        return read().number();
    }

    public String readString() {
        // TODO:: Throw an exception if this is not a string
        return read().string();
    }

    public Token readStringToken() {
        // TODO: Throw an exception if this is not a string
        return read();
    }

    public void readString(String s) {
        // TODO: Throw an exception if this is not the right string
        read();
    }

    public String readSymbol() {

        return readSymbolToken().string();
    }

    public void readSymbols(String m, String n) {
        readSymbol(m);
        readSymbol(n);        
    }

    public Token readSymbolToken() {
        Token tok = read();
        if (tok.type() != Token.SYMBOL) {
            throw new IllegalStateException("Expected a symbol.");
        }
        return tok;
    }

    public void readSymbol(String symbol) {
        String s = readSymbol();
        if (! symbol.equals(s)) {
            throw new IllegalStateException("Expected symbol \'" + symbol + "\', found symbol \'" + s + "\'");
        }
    }

    //void readSymbols(String ...);

    public Token peek() {
        Token t = read();
        tokenizer.pushBack();
        return t;
    }

    public int peekLineNumber() {
        return peek().line();
    }

    public int peekCharacterNumber() {
        return peek().character();
    }

    public String filename() {
        return m_settings.sourceFileName;
    }
}

