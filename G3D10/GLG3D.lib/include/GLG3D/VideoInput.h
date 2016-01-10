/**
\file GLG3D/VideoInput.h

\maintainer Corey Taylor

\created 2008-08-01
\edited  2011-08-24
*/

#ifndef G3D_VideoInput_h
#define G3D_VideoInput_h

#include "G3D/G3DString.h"
#include "G3D/ReferenceCount.h"
#include "G3D/Queue.h"
#include "G3D/GThread.h"
#include "GLG3D/Texture.h"

// forward declarations for ffmpeg
struct AVFrame;
struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVPacket;
struct SwsContext;

namespace G3D {

    /**
    Read video files from MPG, MP4, AVI, MOV, OGG, ASF, and WMV files.

    There are three ways to read: by frame index, by time position, and
    selectively reading a frame if it is time for it to display.

    Reading frames in non-sequential order can decrease performance due
    to seek times.

    Example using VideoInput and VideoOutput on the CPU to concatenate
    two videos:

    \code
    #include <G3D/G3DAll.h>

    int main(int argc, const char* argv[]) {
    if (argc < 3) {
    printf("Syntax: videoconcat <infile1> <infile2> <outfile>\n");
    return -1;
    }
    VideoInput::Ref  in1 = VideoInput::fromFile(argv[1]);
    VideoInput::Ref  in2 = VideoInput::fromFile(argv[2]);

    if (! in1->sameResolution(in2)) {
    printf("Error...videos must have the same dimensions.\n");
    return -2;
    }

    VideoOutput::Ref out = VideoOutput::create(argv[3], VideoOutput::Settings::MPEG4(in1->width(), in1->height()));

    out->append(in1);
    out->append(in2);

    out->commit();
    return 0;
    }
    \endcode
    */
    class VideoInput : public ReferenceCountedObject {
    public:

        class Settings {
        public:
            Settings() : numBuffers(2) {}

            /** Number of asynchronous buffers to allocate. */
            int     numBuffers;
        };


        /** @return NULL if the file is not found or cannot be opened. */
        static shared_ptr<VideoInput> fromFile(const String& filename, const Settings& settings = Settings());

        ~VideoInput();

        /** Gets the frame at frame index and returns false if index is out of bounds.
        use posToIndex to get the frame index from a real time
        @param doNothingIfIsSameFrame : if set to true, we assume this is the last frame we decoded, so if the index is also unchanged, we change nothing.*/
        bool        readFromIndex(int index, shared_ptr<Texture>& frame, bool doNothingIfSameFrame = false);
        /** Gets the frame at frame index and returns false if index is out of bounds. */
        bool        readFromIndex(int index, shared_ptr<PixelTransferBuffer>& frame, bool doNothingIfSameFrame = false);

        /** Seek to playback position
        @param pos Seek time, in seconds.*/
        void        setTimePosition(RealTime pos);
        /** Seek to playback position
        @param index Seek frame index (zero based)*/
        void        setIndex(int index);

        /** Seek ahead in playback position
        @param length seek time in seconds. */
        void        skipTime(RealTime length);

        /** Seek ahead @a length frames. */
        void        skipFrames(int length);

        /** Horizontal pixels in each frame */
        int         width() const;

        /** Vertical pixels in each frame */
        int         height() const;

        /** Preferred playback speed in frames per second */
        RealTime    fps() const;

        /** Length of video in seconds */
        RealTime    length() const;

        /** Current playback position in seconds */
        RealTime    pos() const;

        int posToIndex(RealTime pos) const {
            return iFloor(pos * fps());
        };

        /** Length of video in frames */
        int         numFrames() const;

        /** Current playback frame index */
        int         index() const;

        bool        finished() const    { return m_finished; }

        /** Convenient shorthand for checking that the width and height of two videos is the same. */
        bool sameResolution(const shared_ptr<VideoInput>& other) const {
            return width() == other->width() && height() == other->height();
        }

        /** Advances the current file position to pos() + timeStep. If
        that advance stepped over a frame boundary, sets frame to that
        frame and returns true. Otherwise, returns false. */
        bool        readNext(RealTime timeStep, shared_ptr<Texture>& frame);
        /** Advances the current file position to pos() + timeStep. If
        that advance stepped over a frame boundary, sets frame to that
        frame and returns true. Otherwise, returns false. */
        bool        readNext(RealTime timeStep, shared_ptr<PixelTransferBuffer>& frame);

    private:

        VideoInput();



        void initialize(const String& filename, const Settings& settings);

        static void decodingThreadProc(void* param);
        static void seekToTimestamp(VideoInput* vi, AVFrame* decodingFrame, AVPacket* packet, bool& validPacket);

        Settings            m_settings;
        String              m_filename;

        RealTime            m_currentTime;
        int                 m_currentIndex;
        bool                m_finished;

        struct Buffer {
            AVFrame*    m_frame;
            RealTime    m_pos;
            int64       m_timestamp;
        };

        Queue<Buffer*>      m_decodedBuffers;
        Queue<Buffer*>      m_emptyBuffers;
        GMutex              m_bufferMutex;

        shared_ptr<GThread> m_decodingThread;
        volatile bool       m_quitThread;

        volatile bool       m_clearBuffersAndSeek;
        int64               m_seekTimestamp;
        int64               m_lastTimestamp;
        int64               m_lastIndex;

        // ffmpeg management
        AVFormatContext*    m_avFormatContext;
        AVCodecContext*     m_avCodecContext;
        AVCodec*            m_avVideoCodec;
        SwsContext*         m_avResizeContext;
        int                 m_avVideoStreamIdx;
    };

} // namespace G3D

#endif // G3D_VIDEOINPUT_H
