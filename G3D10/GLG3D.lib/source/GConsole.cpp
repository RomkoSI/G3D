/** 
  @file GConsole.cpp

 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/

#include "G3D/stringutils.h"
#include "G3D/fileutils.h"
#include "G3D/debugPrintf.h"
#include "G3D/TextInput.h"
#include "G3D/FileSystem.h"
#include "GLG3D/GConsole.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Draw.h"
#include "GLG3D/SlowMesh.h"

namespace G3D {

static weak_ptr<GConsole> lastGConsole;

static void gconsolePrintHook(const String& s) {
    GConsoleRef last = lastGConsole.lock();
    if (last) {
        last->print(s);
    }
}

GConsole::Settings::Settings() : 
    blinkRate(3),
    keyRepeatRate(18),
    lineHeight(13),
    numVisibleLines(15),
    maxBufferLength(2000),
    keyRepeatDelay(0.25f),
    commandEcho(true),
    performFilenameCompletion(true),
    performCommandCompletion(true),
    maxCompletionHistorySize(3000),
    defaultCommandColor(Color3::white()),
    defaultPrintColor(0.8f, 1.0f, 0.8f),
    backgroundColor(0, 0, 0, 0.3f) {
}

GConsoleRef GConsole::create(const shared_ptr<GFont>& f, const Settings& s, Callback callback, void* data) {
    GConsoleRef c(new GConsole(f, s, callback, data));
    lastGConsole = c;

    if (consolePrintHook() == NULL) {
        setConsolePrintHook(gconsolePrintHook);
    }

    return c;
}

GConsole::GConsole(const shared_ptr<GFont>& f, const Settings& s, Callback callback, void* data) :
    m_settings(s),
    m_callback(callback),
    m_callbackData(data),
    m_font(f),
    m_resetHistoryIndexOnEnter(true),
    m_rect(Rect2D::xywh(-finf(), -(float)inf(), (float)inf(), (float)inf())),
    m_bufferShift(0),
    m_inCompletion(false),
    m_cursorPos(0),
    m_active(false) {

    debugAssert(m_font);

    unsetRepeatKeysym();
    m_keyDownTime = System::time();
    setActive(true);

    m_historyIndex = m_history.size() - 1;
}


GConsole::~GConsole() {
    // Intentionally empty
}


void GConsole::setCallback(Callback callback, void* callbackData) {
    m_callback = callback;
    m_callbackData = callbackData;
}


void GConsole::setActive(bool a) {
    if (m_active != a) {
        unsetRepeatKeysym();
        m_active = a;

        if (m_manager != NULL) {
            if (m_active) {
                m_manager->setFocusedWidget(dynamic_pointer_cast<Widget>(shared_from_this()));
                // Conservative; these bounds will be refined in render
                m_rect = Rect2D::xywh(-(float)inf(), -(float)inf(), (float)inf(), (float)inf());
            } else {
                m_manager->defocusWidget(dynamic_pointer_cast<Widget>(shared_from_this()));
                m_rect = Rect2D::xywh(0,0,0,0);
            }
        }
    }
}


void GConsole::onPose(Array<shared_ptr<Surface> >& posedArray, Array<Surface2DRef>& posed2DArray) {
    if (m_active) {
        posed2DArray.append(dynamic_pointer_cast<Surface2D>(shared_from_this()));
    }
}


void GConsole::issueCommand() {

    string oldCommandLine = m_currentLine;
    
    m_currentLine = string();
    m_cursorPos = 0;

    // Jump back to the end
    m_bufferShift = 0;

    if (m_settings.commandEcho) {
        print(oldCommandLine, m_settings.defaultCommandColor);
    } else {
        addToCompletionHistory(oldCommandLine);
    }

    m_history.push(oldCommandLine);

    if (m_resetHistoryIndexOnEnter) {
        // Note that we need to go one past the end of the list so that
        // the first up arrow allows us to hit the last element.
        m_historyIndex = m_history.size();
        m_resetHistoryIndexOnEnter = true;
    }

    onCommand(oldCommandLine);
}


void GConsole::onCommand(const string& cmd) {
    if (m_callback) {
        m_callback(cmd, m_callbackData);
    }
}


void GConsole::clearBuffer() {
    m_buffer.clear();
    m_bufferShift = 0;
}


void GConsole::clearHistory() {
    m_history.clear();
}


void GConsole::paste(const string& s) {
    if (s.empty()) {
        // Nothing to do
        return;
    }

    size_t i = 0;

    // Separate the string by newlines and paste each individually
    do {
        size_t j = s.find('\n', i);

        bool issue = true;

        if (j == string::npos) {
            j = s.size();
            issue = false;
        }
            
        string insert = s.substr(i, j - i + 1);

        if (! insert.empty()) {
            if (insert[0] == '\r') {
                // On Win32, we can conceivably get carriage returns next to newlines in a paste
                insert = insert.substr(1, insert.size() - 1);
            }

            if (! insert.empty() && (insert[insert.size() - 1] == '\r')) {
                insert = insert.substr(0, insert.size() - 1);
            }

            if (! insert.empty()) {
                string begin  = m_currentLine.substr(0, max(0, m_cursorPos - 1));
                string end    = m_currentLine.substr(m_cursorPos, m_currentLine.size() - m_cursorPos + 1);

                m_currentLine = begin + insert + end;
                m_cursorPos += (int)insert.size();
            }
        }

        if (issue) {
            issueCommand();
        }

        i = j + 1;
    } while (i < s.size());
}


void __cdecl GConsole::printf(const char* fmt, ...) {
    va_list arg_list;
    va_start(arg_list, fmt);
    vprintf(fmt, arg_list);
    va_end(arg_list);
}


void __cdecl GConsole::vprintf(const char* fmt, va_list argPtr) {
    print(vformat(fmt, argPtr), m_settings.defaultPrintColor);
}


void GConsole::print(const string& s) {
    print(s, m_settings.defaultPrintColor);
}


void GConsole::print(const string& s, const Color4& c) {
    // Break by newlines
    size_t firstNewline = (int)s.find('\n');
    if ((firstNewline != String::npos) && (firstNewline != s.size() - 1)) {
        // There are newlines in the middle of this string
        Array<String> lines = stringSplit(s, '\n');
        for (int i = 0; i < lines.size() - 1; ++i) {
            print(lines[i] + "\n", c);
        }

        if (lines.last() != "") {
            print(lines.last() + "\n", c);
        }
        return;
    }

    addToCompletionHistory(s);

    // If the buffer is too long, pop one from the front
    if (m_buffer.size() >= m_settings.maxBufferLength) {
        m_buffer.popFront();
    }

    m_buffer.pushBack(Text(s, c));
}


static void parseForCompletion(
    const GConsole::string&  source,
    const int                x,
    GConsole::string&        beginStr,
    GConsole::string&        matchStr,
    GConsole::string&        endStr) {

    // Search backwards for a non-identifier character (start one before cursor)
    int i = x - 1;

    while ((i >= 0) && (isDigit(source[i]) || isLetter(source[i]))) {
        --i;
    }

    beginStr = source.substr(0, i + 1);
    matchStr = source.substr(i + 1, x - i - 1);
    endStr   = source.substr(x, source.size() - x + 1);
}


/*inline static bool isQuote(char c) {
    return (c == '\'') || (c == '\"');
}
*/


void GConsole::generateFilenameCompletions(Array<string>& files) {

    if (m_cursorPos == 0) {
        // Nothing to do
        return;
    }

    // Walk backwards, looking for a slash space or a quote that breaks the filename)
    int i = m_cursorPos - 1;

    while ((i > 0) && 
            ! isWhiteSpace(m_currentLine[i - 1]) &&
            ! isQuote(m_currentLine[i - 1])) {
        --i;
    }

    const String& filespec = m_currentLine.substr(i, m_cursorPos - i + 1) + "*";

    FileSystem::list(filespec, files);
}


void GConsole::beginCompletion() {
    m_completionArray.fastClear();

    // Separate the current line into two pieces; before and after the current word.
    // A word follows normal C++ identifier rules.
    string matchStr;

    parseForCompletion(m_currentLine, m_cursorPos, m_completionBeginStr, matchStr, m_completionEndStr);

    // Push the current command on so that we can TAB back to it
    m_completionArray.push(matchStr);
    m_completionArrayIndex = 0;

    // Don't insert the same completion more than once
    static Set<string> alreadySeen;

    if (m_settings.performFilenameCompletion) {
        static Array<string> fcomplete;

        generateFilenameCompletions(fcomplete);

        for (int i = 0; i < fcomplete.size(); ++i) {
            const string& s = fcomplete[i];
            if (! alreadySeen.contains(s)) {
                m_completionArray.push(s);
            }
        }

        fcomplete.fastClear();
    }


    if (m_settings.performCommandCompletion && ! matchStr.empty()) {
        // Generate command completions against completionHistory
        for (int i = 0; i < m_completionHistory.size(); ++i) {
            const string& s = m_completionHistory[i];
            if (beginsWith(s, matchStr) && ! alreadySeen.contains(s)) {
                m_completionArray.push(s);
            }
        }

        // Generate command completions against seed array
        for (int i = 0; i < m_settings.commandCompletionSeed.size(); ++i) {
            const string& s = m_settings.commandCompletionSeed[i];
            if (beginsWith(s, matchStr) && ! alreadySeen.contains(s)) {
                m_completionArray.push(s);
            }
        }
    }

    if (m_completionArray.size() > 1) {
        // We found at least one new alternative to the current string
        m_inCompletion = true;
    }

    alreadySeen.clear();
}


void GConsole::endCompletion() {
    // Cancel the current completion
    m_inCompletion = false;
}


void GConsole::addTokenToCompletionHistory(const string& s) {
    // See if already present
    if (m_completionHistorySet.contains(s)) {
        return;
    }

    // See if we need to remove a queue element
    if (m_completionHistory.size() > m_settings.maxCompletionHistorySize) {
        m_completionHistorySet.remove(m_completionHistory.popFront());
    }

    m_completionHistory.pushBack(s);
    m_completionHistorySet.insert(s);
}


void GConsole::addToCompletionHistory(const string& s) {
    // Parse tokens.  

    // This algorithm treats a token as a legal C++ identifier, number, or string.
    // A better algorithm might follow the one from emacs, which considers pathnames
    // and operator-separated tokens to also be tokens when combined.

    static bool initialized = false;
    static TextInput::Settings settings;
    if (! initialized) {

        settings.cppBlockComments = false;
        settings.cppLineComments = false;
        settings.msvcFloatSpecials = false;

        initialized = true;
    }

    try {
        TextInput t(TextInput::FROM_STRING, s, settings);

        while (t.hasMore()) {
            Token x = t.read();

            // No point in considering one-character completions
            if (x.string().size() > 1) {
                if (x.type() == Token::STRING) {
                    // Recurse into the string to grab its tokens
                    addToCompletionHistory(x.string());
                } else {
                    // Add the raw unparsed string contents
                    addTokenToCompletionHistory(x.string());
                } // if string
            } // if
        } // while
    } catch (...) {
        // In the event of a parse exception we just give up on this string;
        // the worst that will happen is that we'll miss the opportunity to 
        // add some tokens.
    }
}


void GConsole::completeCommand(int direction) {
    if (! m_inCompletion) {
        beginCompletion();

        if (! m_inCompletion) {
            // No identifier under cursor
            return;
        }
    }

    // Compose new command line
    m_completionArrayIndex = (m_completionArrayIndex + m_completionArray.size() + direction) % m_completionArray.size();

    const string& str = m_completionArray[m_completionArrayIndex];
    m_currentLine = m_completionBeginStr + str + m_completionEndStr;
    m_cursorPos = (int)(m_completionBeginStr.size() + str.size());

    m_resetHistoryIndexOnEnter = true;
}


void GConsole::processRepeatKeysym() {
    if ((m_repeatKeysym.sym != GKey::TAB) && m_inCompletion) {
        endCompletion();
    }

    switch (m_repeatKeysym.sym) {
    case GKey::UNKNOWN:
        // No key
        break;

    case GKey::RIGHT:
        if (m_cursorPos < (int)m_currentLine.size()) {
            ++m_cursorPos;
        }
        break;

    case GKey::LEFT:
        if (m_cursorPos > 0) {
            --m_cursorPos;
        }
        break;

    case GKey::HOME:
        m_cursorPos = 0;
        break;

    case GKey::END:
        m_cursorPos = (int)m_currentLine.size();
        break;

    case GKey::DELETE:
        if (m_cursorPos < (int)m_currentLine.size()) {
            m_currentLine = 
                m_currentLine.substr(0, m_cursorPos) + 
                m_currentLine.substr(m_cursorPos + 1, string::npos);
            m_resetHistoryIndexOnEnter = true;
        }
        break;

    case GKey::BACKSPACE:
        if (m_cursorPos > 0) {
            m_currentLine = 
                m_currentLine.substr(0, m_cursorPos - 1) + 
                ((m_cursorPos < (int)m_currentLine.size()) ? 
                  m_currentLine.substr(m_cursorPos, string::npos) :
                  string());
            m_resetHistoryIndexOnEnter = true;
           --m_cursorPos;
        }
        break;

    case GKey::UP:
        if (m_historyIndex > 0) {
            historySelect(-1);
        }
        break;

    case GKey::DOWN:
        if (m_historyIndex < m_history.size() - 1) {
            historySelect(+1);
        }
        break;

    case GKey::TAB:
        // Command completion
        if ((m_repeatKeysym.mod & GKeyMod::SHIFT) != 0) {
            completeCommand(-1);
        } else {
            completeCommand(1);
        }
        break;

    case GKey::PAGEUP:
        if (m_bufferShift < m_buffer.length() - m_settings.numVisibleLines + 1) {
            ++m_bufferShift;
        }
        break;

    case GKey::PAGEDOWN:
        if (m_bufferShift > 0) {
            --m_bufferShift;
        }
        break;

    case GKey::RETURN:
        issueCommand();
        break;

    default:
      
        // This key wasn't processed by the console
        debugAssertM(false, "Unexpected repeat key");
        
    }
}


void GConsole::historySelect(int direction) {
    m_historyIndex += direction;
    m_currentLine = m_history[m_historyIndex];
    m_cursorPos = (int)m_currentLine.size();
    m_resetHistoryIndexOnEnter = false;
}


void GConsole::setRepeatKeysym(GKeySym key) {
    m_keyDownTime = System::time();
    m_keyRepeatTime = m_keyDownTime + m_settings.keyRepeatDelay;
    m_repeatKeysym = key;
}


void GConsole::unsetRepeatKeysym() {
    m_repeatKeysym.sym = GKey::UNKNOWN;
}


bool GConsole::onEvent(const GEvent& event) {

    if (! m_active) {
        if ((event.type == GEventType::CHAR_INPUT) &&
            ((event.character.unicode & 0xFF) == '~')) {

            // '~': Open console
            setActive(true);
            return true;

        } else {

            // Console is closed, ignore key
            return false;
        }
    }

    switch (event.type) {
    case GEventType::KEY_DOWN:
        switch (event.key.keysym.sym) {
        case GKey::ESCAPE:
            // Close the console
            setActive(false);
            return true;

        case GKey::RIGHT:
        case GKey::LEFT:
        case GKey::DELETE:
        case GKey::BACKSPACE:
        case GKey::UP:
        case GKey::DOWN:
        case GKey::PAGEUP:
        case GKey::PAGEDOWN:
        case GKey::RETURN:
        case GKey::HOME:
        case GKey::END:
            setRepeatKeysym(event.key.keysym);
            processRepeatKeysym();
            return true;
            break;

        case GKey::TAB:
            setRepeatKeysym(event.key.keysym);
            processRepeatKeysym();
            // Tab is used for command completion and shouldn't repeat
            unsetRepeatKeysym();
            return true;
            break;

        default:

            if ((((event.key.keysym.mod & GKeyMod::CTRL) != 0) &&
                 ((event.key.keysym.sym == 'v') || (event.key.keysym.sym == 'y'))) ||

                (((event.key.keysym.mod & GKeyMod::SHIFT) != 0) &&
                (event.key.keysym.sym == GKey::INSERT))) {

                // Paste (not autorepeatable)
                paste(OSWindow::clipboardText());
                return true;

            } else if (((event.key.keysym.mod & GKeyMod::CTRL) != 0) &&
                (event.key.keysym.sym == 'k')) {

                // Cut (not autorepeatable)
                string cut = m_currentLine.substr(m_cursorPos);
                m_currentLine = m_currentLine.substr(0, m_cursorPos);

                OSWindow::setClipboardText(cut);

                return true;

            } else if ((event.key.keysym.sym >= GKey::SPACE) &&
                (event.key.keysym.sym <= 'z')) {
                // Suppress this event. Actually handle the key press on the CHAR_INPUT event
                return true;
            } else { // Text input is done through CHAR_INPUT events instead
                // This key wasn't processed by the console
                return false;
            }
        }
        break;

    case GEventType::KEY_UP:
        if (event.key.keysym.sym == m_repeatKeysym.sym) {
            unsetRepeatKeysym();
            return true;
        }
        break;
    case GEventType::CHAR_INPUT:
        // Insert character
        char c = event.character.unicode & 0xFF;
        m_currentLine = 
                m_currentLine.substr(0, m_cursorPos) + 
                c +
                ((m_cursorPos < (int)m_currentLine.size()) ? 
                  m_currentLine.substr(m_cursorPos, string::npos) :
                  string());
        ++m_cursorPos;

        m_resetHistoryIndexOnEnter = true;
        break;
    }

    return false;
}


void GConsole::render(RenderDevice* rd) const {
    if (! m_active) {
        return;
    }

    static const float  pad = 2;
    const float         fontSize = m_settings.lineHeight - 3;
   
    Rect2D rect;

    static RealTime then = System::time();
    RealTime now = System::time();

    bool hasKeyDown = (m_repeatKeysym.sym != GKey::UNKNOWN);

    // Amount of time that the last simulation step took.  
    // This is used to limit the key repeat rate
    // so that it is not faster than the frame rate.
    RealTime frameTime = then - now;

    // If a key is being pressed, process it on a steady repeat schedule.
    if (hasKeyDown && (now > m_keyRepeatTime)) {
        const_cast<GConsole*>(this)->processRepeatKeysym();
        m_keyRepeatTime = max(now + frameTime * 1.1, now + 1.0 / m_settings.keyRepeatRate);
    }
    then = now;

    // Only blink the cursor when keys are not being pressed or
    // have not recently been pressed.
    bool solidCursor = hasKeyDown || (now - m_keyRepeatTime < 1.0 / m_settings.blinkRate);
    if (! solidCursor) {
        static const RealTime zero = System::time();
        solidCursor = isOdd((int)((now - zero) * m_settings.blinkRate));
    }

    {
        float w = (float)rd->width();
        float h = (float)rd->height();
        float myHeight = m_settings.lineHeight * m_settings.numVisibleLines + pad * 2;

        rect = m_rect = Rect2D::xywh(pad, h - myHeight - pad, w - pad * 2, myHeight);
    }

    rd->push2D();

        rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

        if (m_settings.backgroundColor.a > 0) {
            Draw::rect2D(rect, rd, m_settings.backgroundColor);
        }

        if (m_bufferShift > 0) {
            // Draw a line indicating that we aren't looking at the bottom of the buffer
            SlowMesh mesh(PrimitiveType::LINES);
            mesh.setColor(Color3::white());
            const Vector2 v(rect.x0() - 0.3f, rect.y1() - m_settings.lineHeight + 1 - 0.3f);
            mesh.makeVertex(v);
            mesh.makeVertex(v + Vector2(rect.width(), 0));
            mesh.render(rd);
        }
        
        static Array<GFont::CPUCharVertex> charVertexArray;
        static Array<int> indexArray;
        charVertexArray.fastClear();
        indexArray.fastClear();

        // Show PGUP/PGDN commands
        if (m_buffer.size() >= m_settings.numVisibleLines) {
            m_font->appendToCharVertexArray(charVertexArray, indexArray, rd, "pgup ^", rect.x1y0() - Vector2(2, 0), fontSize * 0.75f, Color4(1,1,1,0.7f), Color4::clear(), GFont::XALIGN_RIGHT, GFont::YALIGN_TOP);

            m_font->appendToCharVertexArray(charVertexArray, indexArray, rd, "pgdn v", rect.x1y1() - Vector2(2, 0), fontSize * 0.75f, Color4(1,1,1,0.7f), Color4::clear(), GFont::XALIGN_RIGHT, GFont::YALIGN_BOTTOM);
        }

        rect = Rect2D::xyxy(rect.x0y0() + Vector2(2,1), rect.x1y1() - Vector2(2, 1));
        // Print history
        for (int count = 0; count < m_settings.numVisibleLines - 1; ++count) {
            int q = m_buffer.size() - count - 1 - m_bufferShift;
            if (q >= 0) {
                m_font->appendToCharVertexArray(charVertexArray, indexArray, rd, m_buffer[q].value, rect.x0y1() - Vector2(0, m_settings.lineHeight * (count + 2)), fontSize, m_buffer[q].color);
            }
        }

        m_font->appendToCharVertexArray(charVertexArray, indexArray, rd, m_currentLine, rect.x0y1() - Vector2(0, m_settings.lineHeight), fontSize, m_settings.defaultCommandColor);

        // Draw cursor
        if (solidCursor) {
            // Put cursor under a specific character.  We need to check bounds to do this because we might not
            // have a fixed width font.
            Vector2 bounds;
            if (m_cursorPos > 0) {
                bounds = m_font->bounds(m_currentLine.substr(0, m_cursorPos), fontSize);
            }

            m_font->appendToCharVertexArray(charVertexArray, indexArray, rd, "_", rect.x0y1() + Vector2(bounds.x, -m_settings.lineHeight), fontSize, m_settings.defaultCommandColor);
        }
        
        m_font->renderCharVertexArray(rd, charVertexArray, indexArray);

    rd->pop2D();
}


void GConsole::onNetwork() {
}


void GConsole::onAI() {
}


void GConsole::onUserInput(UserInput* ui) {
    if (m_active && (m_manager->focusedWidget().get() != this)) {
        // Something else has stolen the focus; turn off the console.
        setActive(false);
    }
}


void GConsole::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
}

} // G3D
