package G3D;

import sun.net.dns.ResolverConfiguration;

import java.io.*;

/**
 * Differences from C++ version:
 */
public class TextOutput {

    public static class Settings {
        public static final int WRAP_NONE = 0;
        public static final int WRAP_WITHOUT_BREAKING = 1;
        public static final int WRAP_ALWAYS = 0;

        public static final int NEWLINE_WINDOWS = 0;
        public static final int NEWLINE_UNIX = 1;

        public int              wordWrap        = WRAP_WITHOUT_BREAKING;
        public int              newlineStyle    = NEWLINE_WINDOWS;
        public boolean          allowWordWrapInsideDoubleQuotes = false;
        public int              numColumns      = 80;
        public int              spacesPerIndent = 4;
        public boolean          convertNewlines = true;

        public Settings clone() {
            Settings s = new Settings();
            s.wordWrap = wordWrap;
            s.newlineStyle = newlineStyle;
            s.allowWordWrapInsideDoubleQuotes = allowWordWrapInsideDoubleQuotes;
            s.numColumns = numColumns;
            s.spacesPerIndent = spacesPerIndent;
            s.convertNewlines = convertNewlines;
            return s;
        }
    }

    private Settings    mSettings;
    private boolean     startingNewLine = true;
    private int         currentColumn = 0;
    private boolean     inDQuote = false;
    private String      filename = "";
    private int         indentLevel = 0;
    private String      newline = "\r\n";

    /** Actual number of spaces to indent, taking word-wrapping into account. */
    private int         indentSpaces = 0;

    private StringBuffer data = new StringBuffer();


    public TextOutput(Settings s) {
        mSettings = s.clone();
    }

    public TextOutput() {
        this(new Settings());
    }


    public TextOutput(File filename, Settings s) {
        this(s);
        this.filename = filename.toString();
    }

    public TextOutput(File filename) {
        this(new Settings());
        this.filename = filename.toString();
    }

    public TextOutput(String filename, Settings s) {
        this(s);
        this.filename = filename;
    }

    public TextOutput(String filename) {
        this(new Settings());
        this.filename = filename;
    }

    static private int iClamp(int val, int lo, int hi) {
        return Math.min(hi, Math.max(val, lo));
    }


    public void setIndentLevel(int i) {
        indentLevel = i;

        // If there were more pops than pushes, don't let that take us below 0 indent.
        // Don't ever indent more than the number of columns.
        indentSpaces =
            iClamp(mSettings.spacesPerIndent * indentLevel,
                   0,
                   mSettings.numColumns - 1);
    }


    public void setSettings(Settings _opt) {
        mSettings = _opt.clone();

        // Check wrapping of indent level
        setIndentLevel(indentLevel);

        newline = (mSettings.newlineStyle == Settings.NEWLINE_WINDOWS) ? "\r\n" : "\n";
    }


    public void pushIndent() {
        setIndentLevel(indentLevel + 1);
    }


    public void popIndent() {
        setIndentLevel(indentLevel - 1);
    }

    /** Replace all non-printable characters with escape codes like \n */
    static private String escapeString(String string) {
        String result = "";

        for (int i = 0; i < string.length(); ++i) {
            char c = string.charAt(i);
            switch (c) {
            case '\0':
                result += "\\0";
                break;

            case '\r':
                result += "\\r";
                break;

            case '\n':
                result += "\\n";
                break;

            case '\t':
                result += "\\t";
                break;

            case '\\':
                result += "\\\\";
                break;

            default:
                result += c;
            }
        }

        return result;
    }

    /** Escapes any non-printable characters */
    public void writeString(String string) {
        // Convert special characters to escape sequences
        print("\"" + escapeString(string) + "\"");
    }

    public void writeNumber(double n) {
        print("" + n + " ");
    }


    public void writeNumber(int n) {
        print("" + n+ " ");
    }

    public void writeSymbol(String string) {
        if (string.length() > 0) {
            print("" +  string + " ");
        }
    }

    public void writeSymbols(
        String a,
        String b) {

        writeSymbol(a);
        writeSymbol(b);
    }


    private String convertNewlines(String in) {
        // TODO: can be significantly optimized in cases where
        // single characters are copied in order by walking through
        // the array and copying substrings as needed.

        String out = "";

        if (mSettings.convertNewlines) {
            out = "";
            for (int i = 0; i < in.length(); ++i) {
                if (in.charAt(i) == '\n') {
                    // Unix newline
                    out += newline;
                } else if ((in.charAt(i) == '\r') && (i + 1 < in.length()) && (in.charAt(i + 1) == '\n')) {
                    // Windows newline
                    out += newline;
                    ++i;
                } else {
                    out += in.charAt(i);
                }
            }
        } else {
            out = in;
        }

        return out;
    }


    public void writeNewline() {
        print(newline);
    }


    public void writeNewlines(int numLines) {
        for (int i = 0; i < numLines; ++i) {
            writeNewline();
        }
    }

    public void print(String str) {
        wordWrapIndentAppend(str);
    }

    private void wordWrapIndentAppend(String str) {
        // TODO: keep track of the last space character we saw so we don't
        // have to always search.

        if ((mSettings.wordWrap == Settings.WRAP_NONE) ||
            (currentColumn + str.length() <= mSettings.numColumns)) {
            // No word-wrapping is needed

            // Add one character at a time.
            // TODO: optimize for strings without newlines to add multiple
            // characters.
            for (int i = 0; i < str.length(); ++i) {
                indentAppend(str.charAt(i));
            }
            return;
        }

        // Number of columns to wrap against
        int cols = mSettings.numColumns - indentSpaces;

        // Copy forward until we exceed the column size,
        // and then back up and try to insert newlines as needed.
        for (int i = 0; i < str.length(); ++i) {

            indentAppend(str.charAt(i));
            if ((str.charAt(i) == '\r') && (i + 1 < str.length()) && (str.charAt(i + 1) == '\n')) {
                // \r\n, we need to hit the \n to enter word wrapping.
                ++i;
                indentAppend(str.charAt(i));
            }

            if (currentColumn >= cols) {

                // True when we're allowed to treat a space as a space.
                boolean unquotedSpace = mSettings.allowWordWrapInsideDoubleQuotes || ! inDQuote;

                // Cases:
                //
                // 1. Currently in a series of spaces that ends with a newline
                //     strip all spaces and let the newline
                //     flow through.
                //
                // 2. Currently in a series of spaces that does not end with a newline
                //     strip all spaces and replace them with single newline
                //
                // 3. Not in a series of spaces
                //     search backwards for a space, then execute case 2.

                // Index of most recent space
                int lastSpace = data.length() - 1;

                // How far back we had to look for a space
                int k = 0;
                int maxLookBackward = currentColumn - indentSpaces;

                // Search backwards (from current character), looking for a space.
                while ((k < maxLookBackward) &&
                    (lastSpace > 0) &&
                    (! ((data.charAt(lastSpace) == ' ') && unquotedSpace))) {
                    --lastSpace;
                    ++k;

                    if ((data.charAt(lastSpace) == '\"') && ! mSettings.allowWordWrapInsideDoubleQuotes) {
                        unquotedSpace = ! unquotedSpace;
                    }
                }

                if (k == maxLookBackward) {
                    // We couldn't find a series of spaces

                    if (mSettings.wordWrap == Settings.WRAP_ALWAYS) {
                        // Strip the last character we wrote, force a newline,
                        // and replace the last character;
                        data = data.delete(data.length() - 1, data.length());
                        writeNewline();
                        indentAppend(str.charAt(i));
                    } else {
                        // Must be Options::WRAP_WITHOUT_BREAKING
                        //
                        // Don't write the newline; we'll come back to
                        // the word wrap code after writing another character
                    }
                } else {
                    // We found a series of spaces.  If they continue
                    // to the new string, strip spaces off both.  Otherwise
                    // strip spaces from data only and insert a newline.

                    // Find the start of the spaces.  firstSpace is the index of the
                    // first non-space, looking backwards from lastSpace.
                    int firstSpace = lastSpace;
                    while ((k < maxLookBackward) &&
                        (firstSpace > 0) &&
                        (data.charAt(firstSpace) == ' ')) {
                        --firstSpace;
                        ++k;
                    }

                    if (k == maxLookBackward) {
                        ++firstSpace;
                    }

                    if (lastSpace == data.length() - 1) {
                        // Spaces continued up to the new string
                        data.delete(firstSpace + 1, data.length());
                        writeNewline();

                        // Delete the spaces from the new string
                        while ((i < str.length() - 1) && (str.charAt(i + 1) == ' ')) {
                            ++i;
                        }
                    } else {
                        // Spaces were somewhere in the middle of the old string.
                        // replace them with a newline.

                        // Copy over the characters that should be saved
                        StringBuffer temp = new StringBuffer();
                        for (int j = lastSpace + 1; j < data.length(); ++j) {
                            char c = data.charAt(j);

                            if (c == '\"') {
                                // Undo changes to quoting (they will be re-done
                                // when we paste these characters back on).
                                inDQuote = !inDQuote;
                            }
                            temp.append(c);
                        }

                        // Remove those characters and replace with a newline.
                        data = data.delete(firstSpace + 1, data.length());
                        writeNewline();

                        // Write them back
                        for (int j = 0; j < temp.length(); ++j) {
                            indentAppend(temp.charAt(j));
                        }

                        // We are now free to continue adding from the
                        // new string, which may or may not begin with spaces.

                    } // if spaces included new string
                } // if hit indent
            } // if line exceeded
        } // iterate over str
    }


    public void indentAppend(char c) {

        if (startingNewLine) {
            for (int j = 0; j < indentSpaces; ++j) {
                data.append(' ');
            }
            startingNewLine = false;
            currentColumn = indentSpaces;
        }

        data.append(c);

        // Don't increment the column count on return character
        // newline is taken care of below.
        if (c != '\r') {
            ++currentColumn;
        }

        if (c == '\"') {
            inDQuote = ! inDQuote;
        }

        startingNewLine = (c == '\n');
        if (startingNewLine) {
            currentColumn = 0;
        }
    }


    public void commit() {
        commit(true);
    }

    public void commit(boolean flush) {
        try {
            BufferedWriter out = new BufferedWriter(new FileWriter(filename));
            out.write(data.toString());
            out.close();
        } catch (IOException e) {
            throw new IllegalStateException(e.toString());
        }
    }


    public String commitString() {
        return data.toString();
    }
}
