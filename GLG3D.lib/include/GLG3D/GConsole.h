/**
 @file GConsole.h

 Copyright 2006, Morgan McGuire, morgan3d@users.sf.net
 All rights reserved.

 Available under the BSD License.
 */

#ifndef G3D_GCONSOLE_H
#define G3D_GCONSOLE_H

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/Queue.h"
#include "G3D/Set.h"
#include "G3D/Rect2D.h"
#include "GLG3D/GFont.h"
#include "GLG3D/OSWindow.h"
#include "GLG3D/Widget.h"

namespace G3D {

class RenderDevice;

typedef shared_ptr<class GConsole> GConsoleRef;

/**
 Command-line console.

 When enter is pressed, onCommand is invoked.  The default implementation calls
 the callback function; this allows you to add a command processor either
 by subclassing GConsole or by passing a function to the constructor.

 <ul>
 <li>~: Open console (or write your own code that calls setActive)
 <li>Esc: close console
 <li>Enter: issue command
 <li>Up arrow: scroll through history
 <li>Down arrow: scroll through history
 <li>Left arrow: cursor left
 <li>Right arrow: cursor right
 <li>Home: Cursor all the way to the left
 <li>End: Cursor all the way to the right
 <li>Ctrl+V, Shift+Ins, Ctrl+Y: Paste clipboard contents (Win32 only)
 <li>Ctrl+K: Cut from cursor to end of line (and copy to clipboard on Win32)
 <li>Tab: Complete current command or filename
 <li>Shift+Tab: Complete current command or filename (forward search)
 </ul>

 <B>Beta API</B>
 <dt>Future versions may support access to the constants for blink rate and key repeat,
 provide colored fonts and line wrapping.

 */
class GConsole : public Widget {
public:
    /** To allow later change to std::wstring */
    typedef String string;

    class Settings {
    public:
        /** Cursor flashes per second. */
        float               blinkRate;

        /** Keypresses per second. */
        float               keyRepeatRate;

        /** Pixel height between lines when displayed. (font is slightly smaller than this) */
        float               lineHeight;

        /** Number of lines visible at any time. */
        int                 numVisibleLines;

        /** Maximum number of lines of scrollback */
        int                 maxBufferLength;

        /** Delay before the first key repeat in seconds. */
        RealTime            keyRepeatDelay;

        /** If true, commands are shown in the buffer. */
        bool                commandEcho;

        /** If true, tab completion includes filenames from the local disk. */
        bool                performFilenameCompletion;

        /** If true, tab completion includes issued commands and commands in the completionSeed array. */
        bool                performCommandCompletion;

        /** Number of unique tokens to keep for command completion purposes.
            Does not include commandCompletionSeed elements in the count. */
        int                 maxCompletionHistorySize;

        Color4              defaultCommandColor;

        Color4              defaultPrintColor;

        Color4              backgroundColor;

        /** 
         Commands that can be completed by TAB, in addition to those in the history.
         Include common keywords here, for example, to seed the command completion buffer.
         Commands that were actually typed by the user will take precedence.
         */
        Array<string>       commandCompletionSeed;

        Settings();
        
    }; // Settings

    typedef void(*Callback)(const string&, void*);

protected:

    /** Invoked when the user presses enter.  
        Default implementation calls m_callback. */
    virtual void onCommand(const string& command);

protected:

    Settings                     m_settings;

    Callback                     m_callback;
    void*                        m_callbackData;

    /** Key that is currently auto-repeating. */
    GKeySym                      m_repeatKeysym;

    shared_ptr<GFont>            m_font;

    /** Current history line being retrieved when using UP/DOWN.
        When a history command is used unmodified,
        the history index sticks.  Otherwise it resets to the end
        of the list on Enter.*/
    int                          m_historyIndex;

    /** When true, the history item has been modified since the last UP/DOWN. */
    bool                         m_resetHistoryIndexOnEnter;

    /** Previously executed commands. */
    Array<string>                m_history;

public:

    mutable Rect2D                       m_rect;

protected:

    class Text {
    public:
        string          value;
        Color4          color;

        inline Text() {}
        inline Text(const string& s, const Color4& c) : value(s), color(c) {}
    };

    /** Previously displayed text. */
    Queue<Text>         m_buffer;

    /** Number of lines before the end of the buffer that are visible (affected
        by page up/page down).*/
    int                 m_bufferShift;

    /** True when the console is open and processing events.*/
    bool                m_active;

    /** Currently entered command. */
    string              m_currentLine;

    ///////////////////////////////////////////////////////////////////////////
    /** True when we have already generated a list of potential completions and
        are now simply scrolling through them.*/
    bool                m_inCompletion;

    /** String to prepend onto the current completion list during scrolling.*/
    string              m_completionBeginStr;

    /** String to append onto the current completion list during scrolling.*/
    string              m_completionEndStr;

    /** Filled out by beginCompletion.*/
    Array<string>       m_completionArray;

    /** Index of the current completion in the m_completionArray */
    int                 m_completionArrayIndex;

    /** Buffer of identifiers to use for completions.  Updated by print and by issueCommand.*/
    Queue<string>       m_completionHistory;

    /** All the strings that are in m_completionHistory.*/
    Set<string>         m_completionHistorySet;

    /** Called from processCompletion the first time TAB is pressed. */
    void beginCompletion();

    /** Invoked from processRepeatKeysym when a non-completion key is pressed. 
        */
    void endCompletion();

    /** Parses the string and adds new tokens to the completion history. 
        Called from issueCommand and print.*/
    virtual void addToCompletionHistory(const string& s);

    /** Only called from addToCompletionHistory. */
    void addTokenToCompletionHistory(const string& s);

    /** Invoked from processRepeatKeysym to handle command completion keys. */
    void completeCommand(int direction);

    /** Called from beginCompletion to append filename and directory-based completions onto
        fcomplete.
     */
    void generateFilenameCompletions(Array<string>& fcomplete);

    ///////////////////////////////////////////////////////////////////////////

    /** Position of the cursor within m_currentLine (0 is the first slot) */
    mutable int                 m_cursorPos;

    /** Time at which setRepeatKeysym was called. */
    mutable RealTime            m_keyDownTime;

    /** Time at which the key will repeat (if down). */
    mutable RealTime            m_keyRepeatTime;


    /** Called from onEvent when a key is pressed. */
    void setRepeatKeysym(GKeySym key);

    /** Called from onEvent when the repeat key is released. */
    void unsetRepeatKeysym();

    /** Called from render and onEvent to enact the action triggered by the repeat key. */
    void processRepeatKeysym();

    /** Invoked when the user presses enter. */
    void issueCommand();

    /** Called from repeatKeysym on UP/DOWN. */
    void historySelect(int direction);

    GConsole(const shared_ptr<GFont>& f, const Settings& s, Callback c, void* callbackData);

    /** Issues text to the buffer. */
    virtual void print(const string& s, const Color4& c);

public:

    static GConsoleRef create
    (
     const shared_ptr<GFont>& f, 
     const Settings& s = Settings(), 
     Callback c = NULL, 
     void* callbackData = NULL);
    
    virtual ~GConsole();

    void setCallback(Callback c, void* callbackData = NULL);

    virtual Rect2D bounds() const override {
        return m_rect;
    }

    virtual float depth() const override {
        return 0.5f;
    }

    void setActive(bool a);

    inline bool active() const {
        return m_active;
    }

    /** Insert the string as if it was typed at the command line.
        If the string contains newlines they will cause commands to issue. 
        */
    void paste(const string& s);

    /** Clear displayed text. */
    void clearBuffer();

    /** Clear command history. */
    void clearHistory();

    void print(const string& s);

    /** Print to the buffer. */
    void __cdecl printf(const char* fmt, ...) G3D_CHECK_PRINTF_METHOD_ARGS;

    /** Print to the buffer. */
    void __cdecl vprintf(const char*, va_list argPtr) G3D_CHECK_VPRINTF_METHOD_ARGS;

    /** Call to render the console */
    void render(RenderDevice* rd) const override;

    ////////////////////////////////
    // Inherited from Widget
    
    /** Pass all events to the console. It returns true if it processed (consumed) the event.*/
    virtual bool onEvent(const GEvent& event) override;

    virtual void onNetwork() override;
    
    virtual void onAI() override;

    virtual void onUserInput(UserInput* ui) override;

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;

    virtual void onPose(Array<shared_ptr<Surface> >& posedArray, Array<shared_ptr<Surface2D>>& posed2DArray) override;
};

} //G3D

#endif
