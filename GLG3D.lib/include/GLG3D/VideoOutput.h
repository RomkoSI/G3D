/** 
  \file GLG3D/VideoOutput.h
  \created 2010-01-01
  \edited  2015-03-02

 G3D Innovation Engine
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/

#ifndef G3D_VideoOutput_h
#define G3D_VideoOutput_h

#include "G3D/G3DString.h"
#include "G3D/g3dmath.h"
#include "G3D/Image.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/Texture.h"

// forward declarations for ffmpeg
struct AVOutputFormat;
struct AVFormatContext;
struct AVStream;
struct AVFrame;

namespace G3D {

/** 
 \brief Saves video to disk in a variety of popular formats, including AVI and MPEG. 

 \beta
 */
class VideoOutput : public ReferenceCountedObject {
public:
    /// A mirror of AVCodecID defined by internal ffmpeg headers that we don't want to expose.
    enum InternalCodecID {
        CODEC_ID_NONE,
        CODEC_ID_MPEG1VIDEO,
        CODEC_ID_MPEG2VIDEO,
        CODEC_ID_MPEG2VIDEO_XVMC,
        CODEC_ID_H261,
        CODEC_ID_H263,
        CODEC_ID_RV10,
        CODEC_ID_RV20,
        CODEC_ID_MJPEG,
        CODEC_ID_MJPEGB,
        CODEC_ID_LJPEG,
        CODEC_ID_SP5X,
        CODEC_ID_JPEGLS,
        CODEC_ID_MPEG4,
        CODEC_ID_RAWVIDEO,
        CODEC_ID_MSMPEG4V1,
        CODEC_ID_MSMPEG4V2,
        CODEC_ID_MSMPEG4V3,
        CODEC_ID_WMV1,
        CODEC_ID_WMV2,
        CODEC_ID_H263P,
        CODEC_ID_H263I,
        CODEC_ID_FLV1,
        CODEC_ID_SVQ1,
        CODEC_ID_SVQ3,
        CODEC_ID_DVVIDEO,
        CODEC_ID_HUFFYUV,
        CODEC_ID_CYUV,
        CODEC_ID_H264,
        CODEC_ID_INDEO3,
        CODEC_ID_VP3,
        CODEC_ID_THEORA,
        CODEC_ID_ASV1,
        CODEC_ID_ASV2,
        CODEC_ID_FFV1,
        CODEC_ID_4XM,
        CODEC_ID_VCR1,
        CODEC_ID_CLJR,
        CODEC_ID_MDEC,
        CODEC_ID_ROQ,
        CODEC_ID_INTERPLAY_VIDEO,
        CODEC_ID_XAN_WC3,
        CODEC_ID_XAN_WC4,
        CODEC_ID_RPZA,
        CODEC_ID_CINEPAK,
        CODEC_ID_WS_VQA,
        CODEC_ID_MSRLE,
        CODEC_ID_MSVIDEO1,
        CODEC_ID_IDCIN,
        CODEC_ID_8BPS,
        CODEC_ID_SMC,
        CODEC_ID_FLIC,
        CODEC_ID_TRUEMOTION1,
        CODEC_ID_VMDVIDEO,
        CODEC_ID_MSZH,
        CODEC_ID_ZLIB,
        CODEC_ID_QTRLE,
        CODEC_ID_SNOW,
        CODEC_ID_TSCC,
        CODEC_ID_ULTI,
        CODEC_ID_QDRAW,
        CODEC_ID_VIXL,
        CODEC_ID_QPEG,
        CODEC_ID_PNG,
        CODEC_ID_PPM,
        CODEC_ID_PBM,
        CODEC_ID_PGM,
        CODEC_ID_PGMYUV,
        CODEC_ID_PAM,
        CODEC_ID_FFVHUFF,
        CODEC_ID_RV30,
        CODEC_ID_RV40,
        CODEC_ID_VC1,
        CODEC_ID_WMV3,
        CODEC_ID_LOCO,
        CODEC_ID_WNV1,
        CODEC_ID_AASC,
        CODEC_ID_INDEO2,
        CODEC_ID_FRAPS,
        CODEC_ID_TRUEMOTION2,
        CODEC_ID_BMP,
        CODEC_ID_CSCD,
        CODEC_ID_MMVIDEO,
        CODEC_ID_ZMBV,
        CODEC_ID_AVS,
        CODEC_ID_SMACKVIDEO,
        CODEC_ID_NUV,
        CODEC_ID_KMVC,
        CODEC_ID_FLASHSV,
        CODEC_ID_CAVS,
        CODEC_ID_JPEG2000,
        CODEC_ID_VMNC,
        CODEC_ID_VP5,
        CODEC_ID_VP6,
        CODEC_ID_VP6F,
        CODEC_ID_TARGA,
        CODEC_ID_DSICINVIDEO,
        CODEC_ID_TIERTEXSEQVIDEO,
        CODEC_ID_TIFF,
        CODEC_ID_GIF,
        CODEC_ID_DXA,
        CODEC_ID_DNXHD,
        CODEC_ID_THP,
        CODEC_ID_SGI,
        CODEC_ID_C93,
        CODEC_ID_BETHSOFTVID,
        CODEC_ID_PTX,
        CODEC_ID_TXD,
        CODEC_ID_VP6A,
        CODEC_ID_AMV,
        CODEC_ID_VB,
        CODEC_ID_PCX,
        CODEC_ID_SUNRAST,
        CODEC_ID_INDEO4,
        CODEC_ID_INDEO5,
        CODEC_ID_MIMIC,
        CODEC_ID_RL2,
	CODEC_ID_8SVX_EXP,
	CODEC_ID_8SVX_FIB,
        CODEC_ID_ESCAPE124,
        CODEC_ID_DIRAC,
        CODEC_ID_BFI,
        CODEC_ID_CMV,
        CODEC_ID_MOTIONPIXELS,
        CODEC_ID_TGV,
        CODEC_ID_TGQ,
        CODEC_ID_TQI,
        CODEC_ID_AURA,
        CODEC_ID_AURA2,
        CODEC_ID_V210X,
        CODEC_ID_TMV,
        CODEC_ID_V210,
        CODEC_ID_DPX,
        CODEC_ID_MAD,
        CODEC_ID_FRWU,
        CODEC_ID_FLASHSV2,
        CODEC_ID_CDGRAPHICS,
        CODEC_ID_R210,
        CODEC_ID_ANM,
        CODEC_ID_BINKVIDEO,
        CODEC_ID_IFF_ILBM,
        CODEC_ID_IFF_BYTERUN1,
        CODEC_ID_KGV1,
        CODEC_ID_YOP,
        CODEC_ID_VP8,
        CODEC_ID_PICTOR,
        CODEC_ID_ANSI,
        CODEC_ID_A64_MULTI,
        CODEC_ID_A64_MULTI5,
        CODEC_ID_R10K,
        CODEC_ID_MXPEG,
        CODEC_ID_LAGARITH,
        CODEC_ID_PRORES,
        CODEC_ID_JV,
        CODEC_ID_DFA,
        CODEC_ID_WMV3IMAGE,
        CODEC_ID_VC1IMAGE,
        CODEC_ID_UTVIDEO,
        CODEC_ID_BMV_VIDEO,
        CODEC_ID_VBLE,
        CODEC_ID_DXTORY,
        CODEC_ID_V410,
        CODEC_ID_XWD,
        CODEC_ID_CDXL,
        CODEC_ID_XBM,
        CODEC_ID_ZEROCODEC,
        CODEC_ID_MSS1,
        CODEC_ID_MSA1,
        CODEC_ID_TSCC2,
        CODEC_ID_MTS2,
        CODEC_ID_CLLC,
        CODEC_ID_MSS2,
        CODEC_ID_LAST
    };

    class Settings {
    public:
        
        /** FFmpeg codec id */
        InternalCodecID codec;

        /** Frames per second the video should be encoded as */
        float           fps;

        /** Frame width */
        int             width;

        /** Frame height */
        int             height;

        /** Stream avarage bits per second if needed by the codec.*/
        int             bitrate;

        /** If unset, uses default for codec as defined by FFmpeg. */
        int             fourcc;

        struct
        {
            /** Uncompressed pixel format used with raw codec */
            const ImageFormat*  format;

            /** True if the input images must be inverted before encoding */
            bool                invert;
        } raw;

        struct
        {
            /** Max number of B-Frames if needed by the codec */
            int         bframes;

            /** GOP (Group of Pixtures) size if needed by the codec */
            int         gop;
        } mpeg;

        /** For Settings created by the static factory methods, the
            file extension (without the period) recommended for this
            kind of file. */
        String      extension;

        /** For Settings created by the static factory methods, the a
            brief human-readable description, suitable for use in a
            drop-down box for end users. */
        String      description;

        /** Defaults to MPEG-4 */
        Settings(InternalCodecID codec = CODEC_ID_H264, int width = 640, int height = 480, 
                 float fps = 30.0f, int customFourCC = 0);

        /** Settings that can be used to when writing an uncompressed
            AVI video (with BGR pixel format output). 

            This preserves full quality. It can be played on most
            computers.
        */
        static Settings rawAVI(int width, int height, float fps = 30.0f);

        /** Vendor-independent industry standard, also known as
            H.264. 

            This is the most advanced widely supported format and
            provides a good blend of quality and size.

            THIS IS THE ONLY TESTED FORMAT.
        */
        static Settings MPEG4(int width, int height, float fps = 30.0f);
        
        /** Windows Media Video 2 (WMV) format, which is supported by
            Microsoft's Media Player distributed with Windows.  This
            is the best-supported format and codec for Windows.*/
        static Settings WMV(int width, int height, float fps = 30.0f);

        /** 
            AVI file using Cinepak compression, an older but widely supported
            format for providing good compatibility and size but poor quality.

            Wikipedia describes this format as: "Cinepak is a video
            codec developed by SuperMatch, a division of SuperMac
            Technologies, and released in 1992 as part of Apple
            Computer's QuickTime video suite. It was designed to
            encode 320x240 resolution video at 1x (150 kbyte/s) CD-ROM
            transfer rates. The codec was ported to the Microsoft
            Windows platform in 1993."
            */
        static Settings CinepakAVI(int width, int height, float fps = 30.0f);
    };

protected:

    VideoOutput();

    void initialize(const String& filename, const Settings& settings);
    void encodeFrame(const uint8* frame, const ImageFormat* format, bool invertY = false);

    /** Flips the frame and adjusts format if needed.
        This routine does not support any planar input formats */
    const uint8* convertFrame(const uint8* frame, const ImageFormat* format, bool invertY);

    Settings            m_settings;
    String              m_filename;

    bool                m_isInitialized;
    bool                m_isFinished;
    int                 m_framecount;
    // FFMPEG management
    AVOutputFormat*     m_avOutputFormat;
    AVFormatContext*    m_avFormatContext;
    AVStream*           m_avStream;

    uint8*              m_avInputBuffer;
    AVFrame*            m_avInputFrame;

    uint8*              m_avEncodingBuffer;
    int                 m_avEncodingBufferSize;

    /** Used by convertFrame to hold the temporary frame being prepared for output.*/
    Array<uint8>        m_temp;

public:


    /**
       Video files have a file format and a codec.  VideoOutput
       chooses the file format based on the filename's extension
       (e.g., .avi creates an AVI file) and the codec based on Settings::codec
     */
    static shared_ptr<VideoOutput> create(const String& filename, const Settings& settings);

    /** Tests each codec for whether it is supported on this operating system. */
    static void getSupportedCodecs(Array<InternalCodecID>& c);
    static void getSupportedCodecs(Array<String>& c);

    /** Returns true if this operating system/G3D build supports this codec. */
    static bool supports(InternalCodecID c);

    /** Returns a human readable name for the codec. */
    static const char* toString(InternalCodecID c);

    ~VideoOutput();

    const String& filename() const { return m_filename; }

    void append(const shared_ptr<Texture>& frame, bool invertY = false); 

    void append(const shared_ptr<PixelTransferBuffer>& frame); 

    /** @brief Append the current frame on the RenderDevice to this
        video.  


        @param useBackBuffer If true, read from the back
        buffer (the current frame) instead of the front buffer.
        to be replaced/removed.
     */
    void append(class RenderDevice* rd, bool useBackBuffer = false); 

    /** Appends all frames from \a in, which must have the same dimensions as this.*/
    void append(const shared_ptr<class VideoInput>& in);


    /** Aborts writing video file and ends encoding */
    void abort();

    /** Finishes writing video file and ends encoding */
    void commit();

    bool finished()       { return m_isFinished; }

};

} // namespace G3D

#endif // G3D_VIDEOOUTPUT_H
