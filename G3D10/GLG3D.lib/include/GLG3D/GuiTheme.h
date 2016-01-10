/**
  \file GuiTheme.h
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  Copyright 2000-2015, Morgan McGuire
  All rights reserved
 */

#ifndef G3D_GuiTheme_h
#define G3D_GuiTheme_h

#include "G3D/platform.h"
#include "G3D/Rect2D.h"
#include "GLG3D/Texture.h"
#include "GLG3D/GFont.h"
#include "G3D/Table.h"
#include "GLG3D/GuiText.h"

namespace G3D {

namespace _internal {
    class Morph {
    public:
        bool            active;
        Rect2D          start;
        RealTime        startTime;
        RealTime        duration;
        Rect2D          end;

        Morph();

        void morphTo(const Rect2D& s, const Rect2D& e);

        /** Morph the object using setRect */
        template<class T>
        void update(T* object) {
            RealTime now = System::time();
            float alpha = (float(now - startTime)) / (float)duration;
            if (alpha > 1.0f) {
                object->setRect(end);
                active = false;
                // The setRect will terminate the morph
            } else {
                object->setRect(start.lerp(end, alpha));
                // setRect turns off morphing, so we have to turn it back
                // on explicitly
                active = true;
            }
        }    
    };
}


/**
   Renders UI elements.
   Loads and renders G3D GUI .gtm files, which can be used with G3D::Gui.  .gtm files
   can be found in the data module data/gui directory.

   Can also be used to explicitly render a UI without actual controls behind it, for example:

  <pre>
   theme->beginRendering(rd);
       theme->renderWindow(Rect2D::xywh(80, 70, 550, 250), false, false, false, false, "Window", GuiTheme::NORMAL_WINDOW_STYLE);
       theme->renderWindow(Rect2D::xywh(500, 120, 50, 150), true, true, true, "Tools", GuiTheme::TOOL_WINDOW_STYLE);
       theme->renderCheckBox(Rect2D::xywh(100, 100, 20, 20), true, true, true, "Check box");
       theme->renderRadioButton(Rect2D::xywh(100, 120, 20, 20), true, false, false, "Radio button");
       theme->renderButton(Rect2D::xywh(100, 160, 80, 27), true, true, true, "Button");
       theme->setFont(timesFont, 12, Color3::black(), Color4::clear());
       theme->renderButton(Rect2D::xywh(200, 160, 80, 27), true, true, true, "New Font");
       theme->setFont(arialFont, 10, Color3::black(), Color4::clear());
       theme->renderHorizontalSlider(Rect2D::xywh(100, 200, 150, 27), 0.5f, true, false, "Slider");

       int s = 30;
       theme->renderButton(Rect2D::xywh(100 + s * 0, 230, 30, 30), true, false, false, GuiText("7", iconFont));
       theme->renderButton(Rect2D::xywh(100 + s * 1, 230, 30, 30), true, false, false, GuiText("4", iconFont));
       theme->renderButton(Rect2D::xywh(100 + s * 2, 230, 30, 30), true, false, false, GuiText("=", iconFont));
       theme->renderButton(Rect2D::xywh(100 + s * 3, 230, 30, 30), true, false, true, GuiText(";", iconFont));
       theme->renderButton(Rect2D::xywh(100 + s * 4, 230, 30, 30), true, false, false, GuiText("<", iconFont));
       theme->renderButton(Rect2D::xywh(100 + s * 5, 230, 30, 30), true, false, false, GuiText("8", iconFont));
       theme->setFont(arialFont, 10, Color3::black(), Color4::clear());
    theme->endRendering();
   </pre>
  @sa G3D::GuiWindow
*/
class GuiTheme : public ReferenceCountedObject {
public:

    /** Scale for sliders */
    enum SliderScale {NO_SLIDER, LINEAR_SLIDER, LOG_SLIDER};

    /** Controls the appearance of the window's borders and background.
     */
    enum WindowStyle {
        /** Regular border and title */
        NORMAL_WINDOW_STYLE,

        /** small title, thin border */
        TOOL_WINDOW_STYLE,

        /** thicker border */
        DIALOG_WINDOW_STYLE,

        /** Reserved for future use */
        DRAWER_WINDOW_STYLE,

        /** Menu; no title-bar, stays on top */
        MENU_WINDOW_STYLE,

        /** Looks like Menu, but stays in back. */
        PANEL_WINDOW_STYLE,

        /** Like NORMAL_WINDOW_STYLE, but without border and title when not moused-over */
        PARTIAL_DISAPPEARING_STYLE,

        /** Like NORMAL_WINDOW_SYLE, but the whole window disappears when not moused-over */
        FULL_DISAPPEARING_STYLE,
        /** Do not render a background, stay behind other windows, and do not 
          process events that occur in space not covered by controls. Useful for 
          giving the appearance of controls embedded directly on the screen. */
        NO_WINDOW_STYLE,


        WINDOW_STYLE_COUNT
    };
    
    enum PaneStyle {
        SIMPLE_PANE_STYLE,
        ORNATE_PANE_STYLE,

        /** NO_PANE_STYLE has no visible borders or back */
        NO_PANE_STYLE,

        PANE_STYLE_COUNT
    };

    enum ScrollPaneStyle {
        BORDERED_SCROLL_PANE_STYLE,

        BORDERLESS_SCROLL_PANE_STYLE,

        SCROLL_PANE_STYLE_COUNT
    };

    /** Controls the appearance of the button.  Tool buttons are
        square and less 3D.  They are also able to shrink to smaller
        sizes.  
     */
    enum ButtonStyle {
        NORMAL_BUTTON_STYLE,
        TOOL_BUTTON_STYLE,

        /*** creates buttons with a caption but no
        visible borders.*/
        NO_BUTTON_STYLE,

        BUTTON_STYLE_COUNT
    };

    enum CheckBoxStyle {
        /** the normal checkbox appearance */
        NORMAL_CHECK_BOX_STYLE, 

        /** makes checkbox that looks like a button */
        BUTTON_CHECK_BOX_STYLE,

        /** appears as a tool-bar button.*/
        TOOL_CHECK_BOX_STYLE,

        CHECK_BOX_STYLE_COUNT
    };

    enum RadioButtonStyle {
        NORMAL_RADIO_BUTTON_STYLE,
        BUTTON_RADIO_BUTTON_STYLE = 1,
        TOOL_RADIO_BUTTON_STYLE,

        RADIO_BUTTON_STYLE_COUNT
    };

    enum TextBoxStyle {
        NORMAL_TEXT_BOX_STYLE,
        /** Do not render the background of the text box unless it has focus */
        NO_BACKGROUND_UNLESS_FOCUSED_TEXT_BOX_STYLE
    };

public:
    /**
     Default style information for captions.
     */
    class TextStyle {
    public:
        shared_ptr<GFont>   font;
        Color4              color;
        Color4              outlineColor;
        float               size;

        TextStyle() : color(-1,-1,-1,-1), outlineColor(-1,-1,-1,-1), size(-1) {}

        void load(const Any& any);
    };

private:
    friend class GuiThemeEditor;

    /** Used for delayed text rendering. */
    class Text {
    public:
        Point2              position;
        String         text;
        GFont::XAlign       xAlign;
        GFont::YAlign       yAlign;
        float               size;
        Color4              color;
        Color4              outlineColor;
        float               wrapWidth;
        Text() : size(0), wrapWidth(finf()) {}
    };

    /** Used for delayed image rendering */
    class Image {
    public:
        Point2              position;
        Rect2D              srcRect;
        GFont::XAlign       xAlign;
        GFont::YAlign       yAlign;
    };

    /** Used for delayed Rectangle rendering */
    class Rectangle {
    public:
        Rect2D  positionRect;
        Rect2D  texCoordRect;
        Rectangle() {}
        Rectangle(const Rect2D& pRect, const Rect2D& tcRect) : positionRect(pRect), texCoordRect(tcRect) {}
    };

    /** Delayed text, organized by the associated font.*/
    Table<shared_ptr<GFont>, Array<Text> >     m_delayedStrings;
    /** Number of values in delayedText's arrays.  Used to 
        detect when we need to enter font rendering mode. */
    int                                 m_delayedStringsCount;

    Table<shared_ptr<Texture>, Array<Image> >  m_delayedImages;
    int                                 m_delayedImagesCount;

    Array<Rectangle>    m_delayedRectangles;



    /** Clears the delayedRectangles, delayedText and delayedImage data. */
    void drawDelayedParts() const;

    void drawDelayedRectangles() const;
    void drawDelayedStrings() const;
    void drawDelayedImages() const;

    /** Postpones rendering the specified text until later. Switching
        between the GUI texture and the font texture is relatively
        slow, so delaying text rendering until a few controls have
        been put down increases performance.

        Note that delayed text must be drawn before the clipping region is changed or another window is rendered.
    */
    void addDelayedText(shared_ptr<GFont> font, const String& text, const Point2& position, float size, 
                        const Color4& color, const Color4& outlineColor, GFont::XAlign xalign, 
                        GFont::YAlign yalign = GFont::YALIGN_CENTER, float wordWrapWidth = finf()) const;

    void addDelayedText(const GuiText& text, const TextStyle& defaults, const Point2& position, 
                        GFont::XAlign xalign, GFont::YAlign yalign = GFont::YALIGN_CENTER, float wordWrapWidth = finf()) const;

    void addDelayedText(const shared_ptr<Texture>& t, const Rect2D& srcRectPixels, const Point2& position, 
                        GFont::XAlign xalign, GFont::YAlign yalign = GFont::YALIGN_CENTER) const;


    enum {TEXTURE_UNIT = 0};

    enum StretchMode {TILE, STRETCH};

    // All internal coordinates are stored in texture coordinates

    /** */
    class Fill {
    public:
        StretchMode     horizontalMode;
        StretchMode     verticalMode;

        /** Source rectangle.  This does not have to correspond to the 
            dimensions of the destination rectangle at all.  When 
            rendered, mode will be used to fill the destination rectangle
            up to the correct dimensions if source is too small.  If too source
            is too large it is cropped on the bottom and right.
         */
        Rect2D          source;

        Fill() : source(Rect2D::empty()) {}
        void load(const Any& any);
        void render(class RenderDevice* rd, const Rect2D& bounds, const Vector2& texOffset, Array<GuiTheme::Rectangle>& delayedRectangles) const;        
    };

    /**
        Vertical stretch--two caps and a center fill
    */
    class StretchRectV {
    public:
        Rect2D         top;
        Fill           center;
        Rect2D         bottom;
        
        StretchRectV() : top(Rect2D::empty()), bottom(Rect2D::empty()) {}

        void load(const Any& any);

        /** Bounds width must match top.width and bottom.width */
        void render(class RenderDevice* rd, const Rect2D& bounds, const Vector2& texOffset, Array<GuiTheme::Rectangle>& delayedRectangles) const;
        
        float width() const {
            return top.width();
        }

    };
    /** 
        Horizontal stretch--two caps and a center fill
     */
    class StretchRectH {
    public:
        Rect2D         left;
        Fill           center;
        Rect2D         right;

        StretchRectH() : left(Rect2D::empty()), right(Rect2D::empty()) {}

        void load(const Any& any);

        /** Bounds height must match left.height and right.height */
        void render(class RenderDevice* rd, const Rect2D& bounds, const Vector2& texOffset, Array<GuiTheme::Rectangle>& delayedRectangles) const;
        
        float height() const {
            return left.height();
        }
    };
    
    /** Stretchable in horizontal and vertical direction. */
    class StretchRectHV {
    public:
        StretchRectH       top;
        Fill               centerLeft;
        Fill               centerCenter;
        Fill               centerRight;
        StretchRectH       bottom;

        void load(const Any& any);
        void render(class RenderDevice* rd, const Rect2D& bounds, const Vector2& texOffset, Array<GuiTheme::Rectangle>& delayedRectangles) const;
    };

    class Pad {
    public:
        Vector2      topLeft;
        Vector2      bottomRight;
        
        void load(const Any& any);
        
        /** Net width and height of the padding */
        inline Vector2 wh() const {
            return topLeft + bottomRight;
        }
    };


    class DropDownList {
    public:
        StretchRectH        base;

        /** Offsets from base of area for text display */
        Pad                 textPad;
        
        class Pair {
        public:
            /** For use during selection */
            Vector2         down;

            Vector2         up;
            void load(const Any& any);
        };
        
        class Focus {
        public:
            Pair            focused;
            Vector2         defocused;
            void load(const Any& any);
        };
 
        Focus               enabled;
        Vector2             disabled;

        TextStyle           textStyle;
        TextStyle           disabledTextStyle;

        void load(const Any& any);
        void render(RenderDevice* rd, const Rect2D& bounds, bool enabled, bool focused, bool pushed, Array<GuiTheme::Rectangle>& delayedRectangles) const;
    };

    class Button {
    public:
        /**
           General texture coordinates for any mode of the button
         */
        StretchRectHV        base;

        /**
           Displacement from bounds center.
         */
        Vector2              textOffset;

        /** Defaults */
        TextStyle            textStyle;

        TextStyle            disabledTextStyle;

        class Pair {
        public:
            /** TexOffset from base of this image */
            Vector2          down;

            /** TexOffset from base of this image */
            Vector2          up;
            void load(const Any& any);
        };

        class Focus {
        public:
            Pair      focused;
            Pair      defocused;
            void load(const Any& any);
        };
    
        Focus         enabled;
        Pair          disabled;

        void load(const Any& any);
        void render(RenderDevice* rd, const Rect2D& bounds, bool enabled, bool focused, bool pushed, Array<GuiTheme::Rectangle>& delayedRectangles) const;
    };


    class TextBox {
    public:
        /**
           General texture coordinates for any mode
         */
        StretchRectHV        base;

        Pad                  textPad;

        /** Defaults */
        TextStyle            textStyle;

        TextStyle            disabledTextStyle;

        /** For the user value*/
        TextStyle            contentStyle;

        class Focus {
        public:
            Vector2    focused;
            Vector2    defocused;
            void load(const Any& any);
        };
    
        Focus         enabled;
        Vector2       disabled;
        float         borderWidth;        

        void load(const Any& any);
        void render(RenderDevice* rd, const Rect2D& bounds, bool enabled, bool focused, Array<GuiTheme::Rectangle>& delayedRectangles) const;
    };


    class Canvas {
    public:
        /**
           General texture coordinates for any mode
         */
        StretchRectHV        base;

        Pad                  pad;

        /** Defaults */
        TextStyle            textStyle;

        TextStyle            disabledTextStyle;

        TextBox::Focus       enabled;
        Vector2              disabled;

        void load(const Any& any);
        void render(RenderDevice* rd, const Rect2D& bounds, bool enabled, bool focused, Array<GuiTheme::Rectangle>& delayedRectangles) const;
    };

    /** Used for radio and check boxes */
    class Checkable {
    public:
        class Pair {
        public:
            Rect2D    checked;
            Rect2D    unchecked;
            Pair() : checked(Rect2D::empty()), unchecked(Rect2D::empty()) {}
            void load(const Any& any);
        };

        class Focus {
        public:
            Pair      focused;
            Pair      defocused;
            void load(const Any& any);
        };
    
        Focus         enabled;
        Pair          disabled;

        Vector2       textOffset;

        /** Defaults */
        TextStyle            textStyle;
        TextStyle            disabledTextStyle;

        void load(const Any& any);
        void render(RenderDevice* rd, const Rect2D& bounds, bool enabled, bool focused, bool checked, Array<GuiTheme::Rectangle>& delayedRectangles) const;
        
        float width() const {
            return disabled.checked.width();
        }

        float height() const {
            return disabled.checked.height();
        }
    };

    /** Window close, minimize, maximize */
    class WindowButton {
    public:
        Rect2D       base;
        
        Vector2      focusedUp;
        Vector2      focusedDown;
        Vector2      defocused;
        Vector2      windowDefocused;

        WindowButton() : base(Rect2D::empty()) {}
        void load(const Any& any);
    };

    friend class GuiWindow;
    class Window {
    public:
        StretchRectHV    base;

        /** Distance from edge to border */
        Pad              borderPad;

        /** Thickness of border on each side (not counting rounded corners) */
        Pad              borderThickness;

        /** Distance from border exterior to client area */
        Pad              netClientPad;

        Vector2          focused;
        Vector2          defocused;

        /** Defaults */
        TextStyle        textStyle;
        TextStyle        defocusedTextStyle;

        /** For resizing */
        float            resizeFrameThickness;

        enum ResizeMode {
            /** Square tab in the corner whose size is based on frameThickness*/
            SQUARE, 
            FRAME} resizeMode;

        Window() : resizeMode(SQUARE) {}

        void load(const Any& any);

        /** Pass the bounds outside the border; the borderPad will automatically be added. */
        void render(RenderDevice* rd, const Rect2D& bounds, bool focused, Array<GuiTheme::Rectangle>& delayedRectangles) const;
    };

  
    class VScrollBar {
    public: 
        Rect2D      topArrow;
        Rect2D      bottomArrow;
        class Bar {
        public:
            StretchRectV    base;
            void load(const Any& any);
        };        
        
        class Focus {
        public:
            Vector2     focused;
            Vector2     defocused;
            void load(const Any& any);
        };

        class Thumb {
        public:
            StretchRectV    base;
            Focus           thumbEnabled;
            void load(const Any& any);
        };


        enum {MIN_THUMB_HEIGHT = 20};
        Bar                  bar;
        Thumb                thumb;
        Focus                enabled;
        /** Defaults */
        TextStyle        textStyle;
        TextStyle        disabledTextStyle;

        float            width;

        void load(const Any& any);

        Rect2D  barBounds(const Rect2D scrollbounds) const;
        Rect2D  thumbBounds(const Rect2D barbounds, float pos, float scale) const;

        /** Renders along the center of the vertical bounds and stretches to fill horizontally.*/
        void render(RenderDevice* rd, const Rect2D& bounds, float thumbPos, float scale, bool focused, Array<GuiTheme::Rectangle>& delayedRectangles) const;

    };


    class HScrollBar {
    public: 
        Rect2D      leftArrow;
        Rect2D      rightArrow;
        class Bar {
        public:
            StretchRectH    base;
            void load(const Any& any);
        };        
        
        class Focus {
        public:
            Vector2     focused;
            Vector2     defocused;
            void load(const Any& any);
        };

        class Thumb {
        public:
            StretchRectH    base;
            Focus           thumbEnabled;
            void load(const Any& any);
        };


        enum {MIN_THUMB_WIDTH = 20};
        Bar                  bar;
        Thumb                thumb;
        Focus                enabled;
        /** Defaults */
        TextStyle        textStyle;
        TextStyle        disabledTextStyle;

        void load(const Any& any);

        Rect2D  barBounds(const Rect2D scrollbounds) const;
        Rect2D  thumbBounds(const Rect2D barbounds, float pos, float scale) const;

        /** Renders along the center of the horizontall bounds and stretches to fit vertically.*/
        void render(RenderDevice* rd, const Rect2D& bounds, float thumbPos, float scale, bool focused, Array<GuiTheme::Rectangle>& delayedRectangles) const;

    };


    class HSlider {
    public:
        class Bar {
        public:
            StretchRectH     base;
            Vector2          enabled;
            Vector2          disabled;
            void load(const Any& any);
        };

        class Thumb {
        public:
            Rect2D           base;
            class Focus {
            public:
                Vector2      focused;
                Vector2      defocused;
                void load(const Any& any);
            };

            Focus            enabled;
            Vector2          disabled;

            Thumb() : base(Rect2D::empty()) {}
            void load(const Any& any);
        };

        Bar                  bar;
        Thumb                thumb;

        /** Defaults */
        TextStyle            textStyle;
        TextStyle        disabledTextStyle;
        void load(const Any& any);

        /** Renders along the center of the vertical bounds and stretches to fill horizontally.*/
        void render(RenderDevice* rd, const Rect2D& bounds, float thumbPos, bool enabled, bool focused, Array<GuiTheme::Rectangle>& delayedRectangles) const;

        /** Given the bounds on the graphical part of the slider, returns the bounds on the track.*/
        Rect2D trackBounds(const Rect2D& sliderBounds) const;
        Rect2D thumbBounds(const Rect2D& sliderBounds, float pos) const;
    };

    class Pane {
    public:
        /** Defaults */
        TextStyle        textStyle;
        TextStyle        disabledTextStyle;
        StretchRectHV    frame;
        Pad              clientPad;
        void load(const Any& any);
    };

    Checkable         m_checkBox;
    Checkable         m_radioButton;
    Button            m_button[BUTTON_STYLE_COUNT];

    Window            m_window[WINDOW_STYLE_COUNT];
    HSlider           m_hSlider;
    VScrollBar        m_vScrollBar;
    HScrollBar        m_hScrollBar;
    Pane              m_pane[PANE_STYLE_COUNT];
    WindowButton      m_closeButton;
    TextBox           m_textBox;
    Canvas            m_canvas;
    DropDownList      m_dropDownList;

    /** If true, the close button is on the left.  If false, it is on the right */
    bool              m_osxWindowButtons;

    StretchRectHV     m_selection;

    /** Defaults */
    TextStyle         m_textStyle;
    TextStyle         m_disabledTextStyle;
    
    shared_ptr<Texture>      m_texture;


    /** True between beginRendering and endRendering */
    bool              m_inRendering;

    RenderDevice*     m_rd;

    /** Used by push/popClientRect */
    Array<Rect2D>     m_scissorStack;

    /** Used by push/popClientRect */
    Array<CoordinateFrame> m_coordinateFrameStack;

    static void drawRect(const Rect2D& vertex, const Rect2D& texCoord, RenderDevice* rd, Array<Rectangle>& delayedRectangles);
    
    void drawCheckable(const Checkable& control, const Rect2D& bounds, bool enabled, bool focused,
                       bool selected, const GuiText& text);

    void drawWindow(const Window& window, const Rect2D& bounds, bool focused, 
                    bool close, bool closeDown, bool closeIsFocused, const GuiText& text);

    static StretchMode stringToStretchMode(const String& name);

    GuiTheme(const String& filename,
        const shared_ptr<GFont>&   fallbackFont,
        float               fallbackSize, 
        const Color4&       fallbackColor, 
        const Color4&       fallbackOutlineColor);
    
    /** Unpacks a .gtm file and loads the theme.  Usually called from constructor. */
    void loadTheme(BinaryInput& b);

    /** Loads the theme specification. */
    void loadCoords(const Any& any);
    
    Rect2D horizontalSliderToSliderBounds(const Rect2D& bounds, float captionWidth) const;

    Rect2D closeButtonBounds(const Window& window, const Rect2D& bounds) const;

    /** Returns the amount of space to reserve at the top of a pane with this caption */
    float paneTopPadding(const GuiText& caption, PaneStyle paneStyle) const;

public:
    /** Return the default text style */
    const TextStyle& defaultStyle() const {
        return m_textStyle;
    }

    /** 
     May return a cached copy.  

     @param filename Name of the .gtm file.
    */
    static shared_ptr<GuiTheme> fromFile(
        const String&  filename, 
        shared_ptr<GFont>          fallbackFont = shared_ptr<GFont>(),
        float               fallbackSize = 11, 
        const Color4&       fallbackColor = Color3::black(), 
        const Color4&       fallbackOutlineColor = Color4::clear());

    /** Call before all other render methods. 
     \sa endRendering, pauseRendering, resumeRendering */
    void beginRendering(class RenderDevice* rd);

    static weak_ptr<GuiTheme> lastThemeLoaded;

    /** 
      Offsets all subsequent rendering by r.x0y0() and sets the clipping region to r.
      Call only between beginRendering and endRendering.
      */
    void pushClientRect(const Rect2D& r);
    void popClientRect();

    /** Call after all other render methods. 
    \sa pauseRendering, beginRendering, resumeRendering*/
    void endRendering();

    /** Invoke from a custom GuiControl::render to make arbitrary RenderDevice 
        calls.  Be sure to call resumeRendering() when done.  Invokes 
        RenderDevice::pushState automatically.
     */
    void pauseRendering();

    void resumeRendering();

    /** Only call between beginRendering and endRendering */
    void renderCheckBox(const Rect2D& bounds, bool enabled, bool focused, 
                        bool checked, const GuiText& text);

    /** Render a single-line text box. Only call between beginRendering and endRendering.
        Automatically shifts text so that a cursor at character index given by 
        cursorPosition is visible on screen. */
    void renderTextBox
       (const Rect2D&           bounds, 
        bool                    enabled,
        bool                    focused, 
        const GuiText&          caption,
        float                   captionWidth,
        const GuiText&          text,
        const GuiText&          cursor,
        int                     cursorPosition,
        TextBoxStyle            style);

     /** Render the border that is used for a text box. Only call between beginRendering 
         and endRendering. Used to render the border for scrollPane. */
    void renderTextBoxBorder(   const Rect2D&    fullBounds, 
                                bool             enabled, 
                                bool             focused);

    /**
       Render a drawing area with borders that look like a text box.
       The caption for a canvas is rendered above it and the canvas
       has the full width of its bounds.
     */
    void renderCanvas
       (const Rect2D&           bounds, 
        bool                    enabled,
        bool                    focused, 
        const GuiText&          caption,
        float                   captionHeight);

    /** Render the selection region for a menu or list. */
    void renderSelection(const Rect2D& bounds);

    void renderDropDownList
    (
     const Rect2D&        bounds,
     bool                 enabled,
     bool                 focused,
     bool                 menuOpen,
     const GuiText&    contentText,
     const GuiText&    text,
     float                captionWidth);

    /** Only call between beginRendering and endRendering */
    void renderRadioButton(const Rect2D& bounds, bool enabled, bool focused, 
                           bool checked, const GuiText& text);

    /** Only call between beginRendering and endRendering */
    void renderButton(const Rect2D& bounds, bool enabled, bool focused, 
                      bool pushed, const GuiText& text, ButtonStyle buttonStyle);

    /** Only call between beginRendering and endRendering */
    void renderButtonBorder(const Rect2D& bounds, bool focused, ButtonStyle buttonStyle);

    /** Only call between beginRendering and endRendering.
        @param bounds Corresponds to the footprint of the window; dropshadows and glows may
         still render outside this area.*/
    void renderWindow(const Rect2D& bounds, bool focused, 
                      bool hasCloseButton, bool closeButtonIsDown, bool closeIsFocused,
                      const GuiText& text, WindowStyle frameStyle);

    /** Given the bounds of a full dropDownList and caption, returns
        the bounds around just the control itself. */
    Rect2D dropDownListToClickBounds(const Rect2D& bounds, float captionWidth) const;
    Rect2D textBoxToClickBounds(const Rect2D& bounds, float captionWidth) const;
    Rect2D canvasToClickBounds(const Rect2D& bounds, float captionHeight) const;

    /** Given the full bounds around a canvas, returns the internal region for rendering in. */
    Rect2D canvasToClientBounds(const Rect2D& bounds, float captionHeight) const;

    /** Given the bounds on a window's borders, returns the bounds of
     the area inside the window where controls will appear.*/
    Rect2D windowToClientBounds(const Rect2D& bounds, WindowStyle frameStyle) const;

    /** Given the area that controls should appear in for a window,
        returns the bounds that should be used to draw the window.
        Note that the window's shadow or other effects may exceed
        these bounds when rendering.*/
    Rect2D clientToWindowBounds(const Rect2D& bounds, WindowStyle frameStyle) const;
    Rect2D windowToTitleBounds(const Rect2D& bounds, WindowStyle frameStyle) const;
    Rect2D windowToCloseButtonBounds(const Rect2D& bounds, WindowStyle frameStyle) const;

    /** Returns the position of the thumb button, which is needed for processing
        UI events for the slider. */
    Rect2D horizontalSliderToThumbBounds(const Rect2D& bounds, float pos, float captionWidth) const;
    Rect2D horizontalSliderToTrackBounds(const Rect2D& bounds, float captionWidth) const;

    /** Returns the position of the thumb button, which is needed for processing
        UI events for the scrollBar. */
    Rect2D verticalScrollBarToThumbBounds(const Rect2D& bounds, float pos, float scale) const;
    Rect2D verticalScrollBarToBarBounds(const Rect2D& bounds) const;
    Rect2D horizontalScrollBarToThumbBounds(const Rect2D& bounds, float pos, float scale) const;
    Rect2D horizontalScrollBarToBarBounds(const Rect2D& bounds) const;

    Rect2D paneToClientBounds(const Rect2D& bounds, const GuiText& caption, PaneStyle paneStyle) const;
    Rect2D clientToPaneBounds(const Rect2D& bounds, const GuiText& caption, PaneStyle paneStyle) const;

    /** Computes the rendered size of this text (max of enabled and disabled versions) */
    Vector2 bounds(const GuiText& text) const;

    /** Only call between beginRendering and endRendering.
        Label is on the right, slider is aligned with the left edge
        @param pos 0 = left edge, 1 = right edge*/
    void renderHorizontalSlider(const Rect2D& bounds, float pos, bool enabled, bool focused, 
                                const GuiText& text, float captionWidth);

    /** Only call between beginRendering and endRendering.
        \param pos  0 when the thumb is at the top of the scroll bar and it is equal to 
        the other param scale when it is at the bottom of the bar
        \param scale is the maximum value that pos can be
        \pre scale > 0 and pos >= 0
    */
    void renderVerticalScrollBar(const Rect2D& bounds, float pos, float scale, bool focused);
    void renderHorizontalScrollBar(const Rect2D& bounds, float pos, float scale, bool focused);

    /** Only call between beginRendering and endRendering */
    void renderLabel(const Rect2D& bounds, const GuiText& text, 
                     GFont::XAlign xalign, GFont::YAlign yalign,
                     bool enabled, bool wordWrap = false) const;

    void renderPane(const Rect2D& fullBounds, const GuiText& caption, PaneStyle paneStyle);
    
    /** 
        Create a .gtm file from source files.  Used as a preprocess
        when creating new themes.  You probably don't need to call this
        because most people will want to use pre-existing skins
        (making a theme is a time-consuming artistic process!)

        @param sourceDir Directory containing the whiteName, blackName and coordsFile files.
        @param whiteName Image of controls on a white background, must be in sourceDir
        @param blackName Image of controls on a black background, must be in sourceDir
        @param coordsFile Text file describing the location of controls
        @param destFile Output filename for the built gui theme (should use .gtm extension)
    */
    static void makeThemeFromSourceFiles(
              const String& sourceDir,
              const String& whiteName,
              const String& blackName,
              const String& coordsFile,
              const String& destFile);

    /** Returns the size a button would have to be to enclose this caption.*/
    Vector2 minButtonSize(const GuiText& text, ButtonStyle buttonStyle) const;
    
    float scrollBarWidth() const {
        return m_vScrollBar.width;
    };

    float textBoxBorderWidth() const {
        return m_textBox.borderWidth;
    };

    Pad textBoxPadding() const {
        return m_textBox.textPad;
    };

}; // GuiTheme

} // namespace G3D

#endif // G3D_GuiTheme_H
