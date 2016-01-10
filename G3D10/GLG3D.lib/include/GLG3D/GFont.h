/**
 \file G3D/GFont.h
 
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2002-11-02
 \edited  2014-07-25
 */

#ifndef G3D_GFont_h
#define G3D_GFont_h

#include "G3D/G3DString.h"
#include "G3D/BinaryInput.h"
#include "G3D/CoordinateFrame.h"
#include "GLG3D/Texture.h"

namespace G3D {

/**
 Font class for use with RenderDevice.  Renders variable size and color 
 fonts from high-resolution bitmaps.

 Although GFont optimizes text rendering as much as possible for fully 
 dynamic strings, text rendering is (inherently) slow.  You can achieve 
 better performance for static text by creating bitmap textures with whole words 
 and sentences on them.
 <P>
 
 Some fonts are provided with G3D in the
 <CODE>data/font</CODE> directory.  See the <CODE>copyright.txt</CODE>
 file in that directory for information about the source of these
 files and rules for distribution. 

 You can make new fonts with the GFont::makeFont static function.
 */
class GFont : public ReferenceCountedObject {
public:

    struct CPUCharVertex {
        Vector2 texCoord;
        Vector2 position;
        Color4  color;
        Color4  borderColor;
        CPUCharVertex() {}
        CPUCharVertex(const Vector2& tc, const Vector2& p, const Color4& c, const Color4& bc) 
            : texCoord(tc), position(p), color(c), borderColor(bc) {}
    };


    /** Constant for draw2D.  Specifies the horizontal alignment
    of an entire string relative to the supplied x,y position */
    enum XAlign {XALIGN_RIGHT, XALIGN_LEFT, XALIGN_CENTER};

    /** Constant for draw2D.  Specifies the vertical alignment
    of the characters relative to the supplied x,y position.
      */
    enum YAlign {YALIGN_TOP, YALIGN_BASELINE, YALIGN_CENTER, YALIGN_BOTTOM};

    /** Constant for draw2D.  Proportional width (default)
        spaces characters based on their size.  Fixed spacing gives
        uniform spacing regardless of character width. */
    enum Spacing {PROPORTIONAL_SPACING, FIXED_SPACING};

private:

    /** Must be a power of 2.  Number of characters in the set (typically 128 or 256)*/
    int                     charsetSize;

    /** The actual width of the character. */ 
    Array<int>              subWidth;

    /** The width of the box, in texels, around the character. */
    int                     charWidth;
    int                     charHeight;

    /** Y distance from top of the bounding box to the font baseline. */
    int                     baseline;

    shared_ptr<Texture>     m_texture;

    String                  m_name;

    /** Packs vertices for rendering the string
        into the array as tex/vertex, tex/vertex, ...
     */
    Vector2 appendToPackedArray
       (const String&       s,
        float               x,
        float               y,
        float               w,
        float               h,
        Spacing             spacing,
        const Color4&       color,
        const Color4&       borderColor,
        Array<CPUCharVertex>& vertexArray,
        Array<int>&         indexArray) const;

    GFont(const String& filename, BinaryInput& b);

    float m_textureMatrix[16];

public:
    
    inline String name() const {
        return m_name;
    }

    /** Returns the underlying texture used by the font.  This is rarely needed by applications */
    inline shared_ptr<Texture> texture() const {
        return m_texture;
    }

    /** 
        4x4 matrix transforming texel coordinates to the range [0,1]. Rarely needed by applications.
     */
    inline const float* textureMatrix() const {
        return m_textureMatrix;
    }

    /** Font size at which there is no scaling.  Fonts will appear sharpest at power-of-two
        multiples of this size. */
    float nativeSize() const {
        return charHeight / 1.5f;
    }
    /** Load a new font from disk (fonts are be cached in memory, so repeatedly loading
        the same font is fast as long as the first was not garbage collected).

        The filename must be a G3D .fnt file.

        <P> If a font file is not found, an assertion will fail, an
        exception will be thrown, and texelSize() will return (0, 0).
        <P> Several fonts in this format at varying resolutions are
        available in the font of the G3D data module.

        See GFont::makeFont for creating new fonts in the FNT format.
    */
    static shared_ptr<GFont> fromFile(const String& filename);

    /** see GFont::fromFile */
    static shared_ptr<GFont> fromMemory(const String& name, const uint8* bytes, const int size);

    /**
     \brief Converts an 8-bit font texture and INI file as produced by
     the Bitmap Font Builder program to a G3D FNT font.  
     
     
     outfile should end with ".fnt" or be "" for the default.  <P>

      The Bitmap Font Builder program can be downloaded from http://www.lmnopc.com/bitmapfontbuilder/

      Use the full ASCII character set; the conversion will strip
      infrequently used characters automatically. Write out TGA files
      with characters CENTER aligned and right side up using this
      program.  Then, also write out an INI file; this contains the
      width of each character in the font.  Example: 

      <PRE>
      GFont::makeFont(256,
                      "c:/tmp/g3dfont/news",
                      "d:/graphics3d/book/cpp/data/font/news.fnt"); 
      </PRE> 


      \param charsetSize Must be 128 or 256; indicates whether the "extended" characters
      should be represented in the final texture.

      \param infileBase The name of the texture and font metrics files, with no extension.
      The texture filename must be .tga.  The font metrics filename must end in .ini.
      The input texture must be a power of two in each dimension.  Intensity is treated
      as the alpha value in this image.  The input texture must be a 16x16 or 16x8 grid
      of characters

      \param outfile Defaults to \a infileBase + ".fnt"

      \sa adjustINIWidths, recenterGlyphs
     */
    static void makeFont
       (int            charsetSize,
        const String&  infileBase, 
        String         outfile = "");

    /** \brief Adjusts the pre-computed widths in an .INI file in
        preparation for invoking makeFont on a scaled image.
        
        \sa makeFont, recenterGlyphs
     */
    static void adjustINIWidths(const String& srcFile, const String& dstFile, float scale);

    /** \brief Copies blocks of src so that they are centered in the
     corresponding squares of dst.  Assumes each is a 16x16 grid.
     Useful when you have shrunk a font texture prior to invoking
     makeFont and want to use the original resolution to obtain good
     MIP-boundaries.*/
    static void recenterGlyphs(const shared_ptr<Image>& src, shared_ptr<Image>& dst);


    /** Returns the natural character width and height of this font. */
    Vector2 texelSize() const;

    /**
     Draws a proportional width font string.  Assumes device->push2D()
     has been called.  Leaves all rendering state as it was, except for the
     texture coordinate on unit 0.

     @param size The distance between successive lines of text.  Specify
     texelSize().y / 1.5 to get 1:1 texel to pixel

     @param outline If this color has a non-zero alpha, a 1 pixel border of
     this color is drawn about the text.

     @param spacing Fixed width fonts are spaced based on the width of the 'M' character.

     @return Returns the x and y bounds (ala get2DStringBounds) of the printed string.

     You can draw rotated text by setting the RenderDevice object to world matrix
     manually.  The following example renders the word "ANGLE" on a 45-degree angle
     at (100, 100).
    
    \code
    app->renderDevice->push2D();
        CoordinateFrame cframe(
            Matrix3::fromAxisAngle(Vector3::unitZ(), toRadians(45)),
            Vector3(100, 100, 0));
        app->renderDevice->setObjectToWorldMatrix(cframe);
        app->debugFont->draw2D("ANGLE", Vector2(0, 0), 20);
    app->renderDevice->pop2D();
    \endcode
     */
    Vector2 draw2D
       (RenderDevice*       renderDevice,
        const String&       s,
        const Point2&       pos2D,
        float               size    = 12,
        const Color4&       color   = Color3::black(),
        const Color4&       outline = Color4::clear(),
        XAlign              xalign  = XALIGN_LEFT,
        YAlign              yalign  = YALIGN_TOP,
        Spacing             spacing = PROPORTIONAL_SPACING) const;

    /** Wordwraps at \a maxWidth */
    Vector2 draw2DWordWrap
       (RenderDevice*       renderDevice,
        float               maxWidth,
        const String&       s,
        const Point2&       pos2D,
        float               size    = 12,
        const Color4&       color   = Color3::black(),
        const Color4&       outline = Color4::clear(),
        XAlign              xalign  = XALIGN_LEFT,
        YAlign              yalign  = YALIGN_TOP,
        Spacing             spacing = PROPORTIONAL_SPACING) const;
    

    /**
     Renders flat text on a plane in 3D, obeying the z-buffer.

     Text is visible from behind.  The text is oriented so that it
     reads "forward" when the pos3D z-axis points towards the viewer.

     Note that text, like all transparent objects, should be rendered
     in back to front sorted order to achieve proper alpha blending.

     @param size In meters of the height of a line of text.

     This doesn't follow the same optimized rendering path as draw2D and is intended
     mainly for debugging.
     */
    Vector2 draw3D
       (RenderDevice*               renderDevice,
        const String&               s,
        const CoordinateFrame&      pos3D,
        float                       size    = 0.1f,
        const Color4&               color   = Color3::black(),
        const Color4&               outline = Color4::clear(),
        XAlign                      xalign  = XALIGN_LEFT,
        YAlign                      yalign  = YALIGN_TOP,
        Spacing                     spacing = PROPORTIONAL_SPACING) const;

    /**
     Renders flat text on a plane in 3D that always faces the viewer, obeying the z-buffer.

     Note that text, like all transparent objects, should be rendered
     in back to front sorted order to achieve proper alpha blending.

     @param pos3D In object space

     @param size In meters of the height of a line of text.
     */
    Vector2 draw3DBillboard
       (RenderDevice*               renderDevice,
        const String&               s,
        const Point3&               pos3D,
        float                       size    = 0.1f,
        const Color4&               color   = Color3::black(),
        const Color4&               outline = Color4::clear(),
        XAlign                      xalign  = XALIGN_CENTER,
        YAlign                      yalign  = YALIGN_CENTER,
        Spacing                     spacing = PROPORTIONAL_SPACING) const;
        
    /**
     \brief Computes the bounding extent of \a s at the given font size.
     Useful for drawing centered text and boxes around text.
     */
    Vector2 bounds
       (const String&               s,
        float                       size = 12,
        Spacing                     spacing = PROPORTIONAL_SPACING) const;

    Vector2 boundsWordWrap
       (float                       maxWidth,
        const String&               s,
        float                       size = 12,
        Spacing                     spacing = PROPORTIONAL_SPACING) const;
    /**

    \param s The original string.  This will be modified to contain
     the remainder after word wrapping, with leading and trailing whitespace
     removed.

    \param firstLine This will be filled with the first line of text.  It is
    guaranteed to have width less than \a maxWidth.

    \param maxWidth In pixels

    \return The character in the original s where the cut occured.

    \sa G3D::wordWrap, G3D::TextOutput::Settings::WordWrapMode
    */
    int wordWrapCut
       (float                       maxWidth,
        String&                     s,
        String&                     firstLine,
        float                       size = 12,
        Spacing                     spacing = PROPORTIONAL_SPACING) const;

    /**
    returns the number of leading charectors that can be rendered in less than maxWidth
    */
    int wordSplitByWidth
        (const float maxWidth,
        const String& s,
        float size = 12,
        Spacing spacing = PROPORTIONAL_SPACING) const;

    /**
       For high performance when rendering substantial amounts of text,
       call:

       \code
       rd->pushState();
          Array<CPUCharVertex> cpuCharArray;
          for (...) {
             font->appendToCharVertexArray(cpuCharArray, ...);
          }
          font->renderCharVertexArray(rd, cpuCharArray);
       rd->popState();
       \endcode

       This amortizes the cost of the font setup across multiple calls.
     */
    void renderCharVertexArray(RenderDevice* rd, const Array<CPUCharVertex>& cpuCharArray, Array<int>& indexArray) const;

    /** For high-performance rendering of substantial amounts of text. */
    Vector2 appendToCharVertexArray
       (Array<CPUCharVertex>&       cpuCharArray,
        Array<int>&                 indexArray,
        RenderDevice*               renderDevice,
        const String&               s,
        const Point2&               pos2D,
        float                       size    = 12,
        const Color4&               color   = Color3::black(),
        const Color4&               outline = Color4::clear(),
        XAlign                      xalign  = XALIGN_LEFT,
        YAlign                      yalign  = YALIGN_TOP,
        Spacing                     spacing = PROPORTIONAL_SPACING) const;

    /** For high-performance rendering of substantial amounts of text.
    \beta */
    Vector2 appendToCharVertexArrayWordWrap
       (Array<CPUCharVertex>&       cpuCharArray,
        Array<int>&                 indexArray,
        RenderDevice*               renderDevice,
        float                       wrapWidth,
        const String&               s,
        const Point2&               pos2D,
        float                       size    = 12,
        const Color4&               color   = Color3::black(),
        const Color4&               outline = Color4::clear(),
        XAlign                      xalign  = XALIGN_LEFT,
        YAlign                      yalign  = YALIGN_TOP,
        Spacing                     spacing = PROPORTIONAL_SPACING) const;
};

}
#endif
