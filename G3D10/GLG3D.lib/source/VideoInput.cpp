/** 
 \file VideoInput.cpp
 \author Corey Taylor and Sam Donow
 */

#include "G3D/platform.h"
#include "G3D/fileutils.h"
#include "GLG3D/VideoInput.h"
#include "GLG3D/Texture.h"
#include "G3D/CPUPixelTransferBuffer.h"
#include "GLG3D/GLPixelTransferBuffer.h"

extern "C" {
    #ifndef G3D_NO_FFMPEG
    #   include "libavformat/avformat.h"
    #   include "libavcodec/avcodec.h"
    #   include "libavutil/avutil.h"
    #   include "libavutil/opt.h"
    #   include "libswscale/swscale.h"
    #endif
    #include <errno.h>
}

#ifdef G3D_NO_FFMPEG
#    pragma message("Warning: FFMPEG and VideoOutput are disabled in this build (" __FILE__ ")") 
#endif

namespace G3D {

shared_ptr<VideoInput> VideoInput::fromFile(const String& filename, const Settings& settings) {
    shared_ptr<VideoInput> vi(new VideoInput);

    try {
        vi->initialize(filename, settings);
    } catch (const String& s) {
        // TODO: Throw the exception
        debugAssertM(false, s);(void)s;
        vi.reset();
    }
    return vi;
}


VideoInput::VideoInput() : 
    m_currentTime(0.0f),
    m_currentIndex(0),
    m_finished(false),
    m_quitThread(false),
    m_clearBuffersAndSeek(false),
    m_seekTimestamp(-1),
    m_lastTimestamp(-1),
    m_lastIndex(-1),
    m_avFormatContext(NULL),
    m_avCodecContext(NULL),
    m_avVideoCodec(NULL),
    m_avResizeContext(NULL),
    m_avVideoStreamIdx(-1) {

}

VideoInput::~VideoInput() {
    // shutdown decoding thread
    if (m_decodingThread && !m_decodingThread->completed()) {
        m_quitThread = true;
        m_decodingThread->waitForCompletion();
    }

#ifndef G3D_NO_FFMPEG
    // shutdown ffmpeg
    avcodec_close(m_avCodecContext);
    //avformat_close_input_file(m_avFormatContext);

    // clear decoding buffers
    while (m_emptyBuffers.length() > 0) {
        Buffer* buffer = m_emptyBuffers.dequeue();
        av_free(buffer->m_frame->data[0]);
        av_free(buffer->m_frame);
        delete buffer;
    }

    while (m_decodedBuffers.length() > 0) {
        Buffer* buffer = m_decodedBuffers.dequeue();
        av_free(buffer->m_frame->data[0]);
        av_free(buffer->m_frame);
        delete buffer;
    }

    if (m_avResizeContext) {
        av_free(m_avResizeContext);
    }
#endif
}

static const char* ffmpegError(int code);

void VideoInput::initialize(const String& filename, const Settings& settings) {
    // helper for exiting VideoInput construction (exceptions caught by static ref creator)
    #define throwException(exp, msg) if (!(exp)) { throw String(msg); }
#ifndef G3D_NO_FFMPEG
    // initialize list of available muxers/demuxers and codecs in ffmpeg
    avcodec_register_all();
    av_register_all();

    m_filename = filename;
    m_settings = settings;

    int avRet = avformat_open_input(&m_avFormatContext, filename.c_str(), NULL, NULL);
    throwException(avRet >= 0, ffmpegError(avRet));

    // find and use the first video stream by default
    // this may need to be expanded to configure or accommodate multiple streams in a file
    avformat_find_stream_info(m_avFormatContext, NULL);
    AVCodecContext* contextOrig = NULL;
    for (int streamIdx = 0; streamIdx < (int)m_avFormatContext->nb_streams; ++streamIdx) {
        if (m_avFormatContext->streams[streamIdx]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            contextOrig = m_avFormatContext->streams[streamIdx]->codec;
            m_avVideoStreamIdx = streamIdx;
            break;
        }
    }

    // We load on the assumption that this is a video file
    throwException(contextOrig != NULL, ("Unable to initialize FFmpeg."));


    m_avCodecContext = avcodec_alloc_context3(m_avVideoCodec);
    
    avRet = avcodec_copy_context(m_avCodecContext, contextOrig);
    throwException(avRet >= 0, ("Unable to initialize FFmpeg."));

    // Find the video codec
    m_avVideoCodec = avcodec_find_decoder(m_avCodecContext->codec_id);
    throwException(m_avVideoCodec, ("Unable to initialize FFmpeg."));

    // Initialize the codecs
    avRet = avcodec_open2(m_avCodecContext, m_avVideoCodec, NULL);
    throwException(avRet >= 0, ("Unable to initialize FFmpeg."));

    // Create array of buffers for decoding
    int bufferSize = avpicture_get_size(PIX_FMT_RGB24, m_avCodecContext->width, m_avCodecContext->height);

    for (int bufferIdx = 0; bufferIdx < m_settings.numBuffers; ++bufferIdx) {
        Buffer* buffer = new Buffer;

        buffer->m_frame = av_frame_alloc();
        
        // Allocate the rgb buffer
        uint8* rgbBuffer = static_cast<uint8*>(av_malloc(bufferSize));

        // Initialize the frame
        avpicture_fill(reinterpret_cast<AVPicture*>(buffer->m_frame), rgbBuffer, PIX_FMT_RGB24, m_avCodecContext->width, m_avCodecContext->height);

        // add to queue of empty frames
        m_emptyBuffers.enqueue(buffer);
    }

    // Create resize context since the parameters shouldn't change throughout the video
    m_avResizeContext = sws_getContext(m_avCodecContext->width, m_avCodecContext->height, m_avCodecContext->pix_fmt, 
                                       m_avCodecContext->width, m_avCodecContext->height, PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
    debugAssert(m_avResizeContext);
    
    // everything is setup and ready to be decoded
    m_decodingThread = GThread::create("VideoInput::m_bufferThread", VideoInput::decodingThreadProc, this);
    m_decodingThread->start();
#endif
}

bool VideoInput::readNext(RealTime timeStep, shared_ptr<PixelTransferBuffer>& frame) {
    GMutexLock m(&m_bufferMutex);

    m_currentTime += timeStep;

    bool readAfterSeek = (m_seekTimestamp != -1);

    bool frameUpdated = false;
#	ifndef G3D_NO_FFMPEG
    if ((m_decodedBuffers.length() > 0) && 
        (readAfterSeek || (m_decodedBuffers[0]->m_pos <= m_currentTime))) {

        Buffer* buffer = m_decodedBuffers.dequeue();

        // reset seek
        if (readAfterSeek) {
            m_seekTimestamp = -1;
        } 

        // increment current playback index
        ++m_currentIndex;

        // adjust current playback position to the time of the frame
        m_currentTime = buffer->m_pos;

        // create new frame if existing is wrong format
        if (frame->format() != ImageFormat::SRGB8()) {
            if (frame->width() != width() || frame->height() != height()) {
                frame = CPUPixelTransferBuffer::create(width(), height(), ImageFormat::SRGB8());
            }
        }

        // copy frame
        memcpy(frame->mapWrite(), buffer->m_frame->data[0], (width() * height() * 3));
        frame->unmap();

        m_emptyBuffers.enqueue(buffer);
        frameUpdated = true;

        // check if video is finished
        if ((m_decodedBuffers.length() == 0) && m_decodingThread->completed()) {
            m_finished = true;
        }
    }
#   endif
    return frameUpdated;
}


bool VideoInput::readFromIndex(int index, shared_ptr<Texture>& frame, bool doNothingIfSameFrame) {
    
    if (doNothingIfSameFrame && (index == m_lastIndex)) {
        return true;
    }

    shared_ptr<PixelTransferBuffer> ptb = GLPixelTransferBuffer::create(width(), height(), TextureFormat::SRGB8());
    const bool retVal = readFromIndex(index, ptb, false);

    if (retVal) {
       if (notNull(frame)) {
           frame->update(ptb);
       } else {
           frame = Texture::fromPixelTransferBuffer(format("Vid_%s_frame_%d", m_filename.c_str(), index), ptb);
       }
    }

    return retVal;
}


bool VideoInput::readNext(RealTime timeStep, shared_ptr<Texture>& frame) {

    GMutexLock m(&m_bufferMutex);

    m_currentTime += timeStep;

    bool readAfterSeek = (m_seekTimestamp != -1);

    bool frameUpdated = false;
#	ifndef G3D_NO_FFMPEG
    if ((m_decodedBuffers.length() > 0) && 
        (readAfterSeek || (m_decodedBuffers[0]->m_pos <= m_currentTime))) {

        Buffer* buffer = m_decodedBuffers.dequeue();

        // reset seek
        if (readAfterSeek) {
            m_seekTimestamp = -1;
        }

        // increment current playback index
        ++m_currentIndex;

        // adjust current playback position to the time of the frame
        m_currentTime = buffer->m_pos;

        // check if the texture is re-usable and create a new one if not
        if (notNull(frame) && (frame->width() == width()) && (frame->height() == height()) && (frame->format() == ImageFormat::SRGB8())) {

            // update existing texture
            glBindTexture(frame->openGLTextureTarget(), frame->openGLID());
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            
            glTexImage2D(frame->openGLTextureTarget(), 0, frame->format()->openGLFormat, frame->width(), frame->height(), 0,
                TextureFormat::RGB8()->openGLBaseFormat, TextureFormat::RGB8()->openGLDataFormat, buffer->m_frame->data[0]);
            
            glBindTexture(frame->openGLTextureTarget(), GL_NONE);

        } else {
            // clear existing texture
            frame.reset();

            // create new texture with right dimentions
            const bool generateMipMaps = false;
            frame = Texture::fromMemory("VideoInput frame", buffer->m_frame->data[0], TextureFormat::SRGB8(), width(), height(), 1, 1, TextureFormat::AUTO(), Texture::DIM_2D, generateMipMaps, Texture::Preprocess::none());
        }

        m_emptyBuffers.enqueue(buffer);
        frameUpdated = true;

        // check if video is finished
        if ((m_decodedBuffers.length() == 0) && m_decodingThread->completed()) {
            m_finished = true;
        }
    }
#   endif

    return frameUpdated;
}

bool VideoInput::readFromIndex(int index, shared_ptr<PixelTransferBuffer>& frame, bool doNothingIfSameFrame) {
   
    if (doNothingIfSameFrame && (index == m_lastIndex)) {
        return true;
    }
    m_lastIndex = index;
    //If there is no need to seek, don't seek
    if (index == m_currentIndex) {
        return readNext(0.0, frame);
    }

    setIndex(index);

    // wait for seek to complete
    while (! m_decodingThread->completed() && m_clearBuffersAndSeek) {
        System::sleep(0.001);
    }
    bool foundFrame = false;

    // wait for a new frame after seek and read it
    while (! m_decodingThread->completed() && !foundFrame) {

        // check for frame
        m_bufferMutex.lock();
        foundFrame = (m_decodedBuffers.length() > 0);
        m_bufferMutex.unlock();

        if (foundFrame) {
            // read new frame
            readNext(0.0, frame);
        } else {
            // let decode run more
            System::sleep(0.001);
        }
    }

    // invalidate video if seek failed
    if (! foundFrame) {
        m_finished = true;
    }

    return foundFrame;
}


void VideoInput::setTimePosition(RealTime pos) {
    // find the closest index to seek to
    setIndex(iFloor(pos * fps()));
}


void VideoInput::setIndex(int index) {
   
    m_currentIndex = index;
    m_currentTime  = index / fps();

#   ifndef G3D_NO_FFMPEG
        // calculate timestamp in stream time base units
        m_seekTimestamp = static_cast<int64>(fuzzyEpsilon32 + m_currentTime / av_q2d(m_avFormatContext->streams[m_avVideoStreamIdx]->time_base)) + m_avFormatContext->streams[m_avVideoStreamIdx]->start_time;

        // tell decoding thread to clear buffers and start at this position
        m_clearBuffersAndSeek = true;
#   endif
}


void VideoInput::skipTime(RealTime length) {
    setTimePosition(m_currentTime + length);
}


void VideoInput::skipFrames(int length) {
    setIndex(m_currentIndex + length);
}


int VideoInput::width() const {
#   ifdef G3D_NO_FFMPEG
        return 0;
#   else
        return m_avCodecContext->width;
#   endif
}


int VideoInput::height() const {
#   ifdef G3D_NO_FFMPEG
        return 0;
#   else
        return m_avCodecContext->height;
#   endif
}


RealTime VideoInput::fps() const {
#   ifdef G3D_NO_FFMPEG
        return 1;
#   else
        // return FFmpeg's calculated base frame rate
        return av_q2d(m_avFormatContext->streams[m_avVideoStreamIdx]->r_frame_rate);
#   endif
}


RealTime VideoInput::length() const {
#   ifdef G3D_NO_FFMPEG
        return 0;
#   else
        // return duration in seconds calculated from stream duration in FFmpeg's stream time base
        return m_avFormatContext->streams[m_avVideoStreamIdx]->duration * av_q2d(m_avFormatContext->streams[m_avVideoStreamIdx]->time_base);
#   endif
}


RealTime VideoInput::pos() const {
    return m_currentTime;
}


int VideoInput::numFrames() const {
    return static_cast<int>(length() * fps());
}


int VideoInput::index() const {
    return m_currentIndex;
}


void VideoInput::decodingThreadProc(void* param) {
#ifndef G3D_NO_FFMPEG
    VideoInput* vi = reinterpret_cast<VideoInput*>(param);

    // allocate avframe to hold decoded frame
    AVFrame* decodingFrame = avcodec_alloc_frame();
    debugAssert(decodingFrame);

    Buffer* emptyBuffer = NULL;

    AVPacket packet;
    av_init_packet(&packet);
    bool useExistingSeekPacket = true;

    while (!vi->m_quitThread) {

        // seek to frame if requested
        if (vi->m_clearBuffersAndSeek) {
            seekToTimestamp(vi, decodingFrame, &packet, useExistingSeekPacket);
            vi->m_clearBuffersAndSeek = false;
        }

        // get next available empty buffer
        if (emptyBuffer == NULL) {
            // yield while no available buffers
            System::sleep(0.005f);

            // check for a new buffer
            GMutexLock lock(&vi->m_bufferMutex);
            if (vi->m_emptyBuffers.length() > 0) {
                emptyBuffer = vi->m_emptyBuffers.dequeue();
            }
        }

        if (emptyBuffer && !vi->m_quitThread) {

            // decode next frame
            if (!useExistingSeekPacket) {
                // exit thread if video is complete (or errors)
                if (av_read_frame(vi->m_avFormatContext, &packet) != 0) {
                    vi->m_quitThread = true;
                }
            }

            // reeset now that we are decoding the frame and not waiting on a free buffer
            useExistingSeekPacket = false;

            // ignore frames other than our video frame
            if (packet.stream_index == vi->m_avVideoStreamIdx) {

                // decode the frame
                int completedFrame = 0;
                avcodec_decode_video2(vi->m_avCodecContext, decodingFrame, &completedFrame, &packet);

                // we have a valid frame, let's use it!
                if (completedFrame != 0) {

                    // Convert the image from its native format to RGB
                    sws_scale(vi->m_avResizeContext, decodingFrame->data, decodingFrame->linesize, 0, vi->m_avCodecContext->height, emptyBuffer->m_frame->data, emptyBuffer->m_frame->linesize);
                    
                    // calculate start time based off of presentation time stamp
                    emptyBuffer->m_pos = packet.dts * av_q2d(vi->m_avCodecContext->time_base);

                    // store original time stamp of frame
                    emptyBuffer->m_timestamp = packet.dts;

                    // set last decoded timestamp
                    vi->m_lastTimestamp = packet.dts;

                    // add frame to decoded queue
                    vi->m_bufferMutex.lock();
                    vi->m_decodedBuffers.enqueue(emptyBuffer);

                    // get new buffer if available since we have the lock
                    if (vi->m_emptyBuffers.length() > 0) {
                        emptyBuffer = vi->m_emptyBuffers.dequeue();
                    } else {
                        emptyBuffer = NULL;
                    }
                    vi->m_bufferMutex.unlock();
                }
            }
        }          

        // always clean up packaet allocated during av_read_frame
        if (packet.data != NULL) {
            av_free_packet(&packet);
        }
    }

    // free codec decoding frame
    av_free(decodingFrame);
#endif
}




// decoding thread helpers
void VideoInput::seekToTimestamp(VideoInput* vi, AVFrame* decodingFrame, AVPacket* packet, bool& validPacket) {
#ifndef G3D_NO_FFMPEG
    // maximum number of frames to decode before seeking (1 second)
    const int64 MAX_DECODE_FRAMES = iRound(vi->fps());

    GMutexLock lock(&vi->m_bufferMutex);
    // remove frames before target timestamp
    while ((vi->m_decodedBuffers.length() > 0)) {
        if ((vi->m_decodedBuffers[0]->m_timestamp != vi->m_seekTimestamp)) {
            vi->m_emptyBuffers.enqueue(vi->m_decodedBuffers.dequeue());
        } else {
            // don't remove buffers past desired frame!
            break;
        }
    }

    // will be set below if valid
    validPacket = false;

    if (vi->m_decodedBuffers.length() == 0) {
        int nextIndex = av_index_search_timestamp(vi->m_avFormatContext->streams[vi->m_avVideoStreamIdx], vi->m_seekTimestamp, AVSEEK_FLAG_BACKWARD);
        int64 seekDiff = vi->m_seekTimestamp - vi->m_lastTimestamp;
        if ((nextIndex > vi->m_currentIndex && ((seekDiff <= 0) || (seekDiff > MAX_DECODE_FRAMES)))) {
            // don't need to seek if we are close enough to just decode

                // flush FFmpeg decode buffers for seek
                avcodec_flush_buffers(vi->m_avCodecContext);

                int seekRet = av_seek_frame(vi->m_avFormatContext, vi->m_avVideoStreamIdx, vi->m_seekTimestamp, AVSEEK_FLAG_BACKWARD);
                debugAssert(seekRet >= 0); (void)seekRet;
        }
        // read frames up to desired frame since can only seek to a key frame
        do {
            int readRet = av_read_frame(vi->m_avFormatContext, packet);
            debugAssert(readRet >= 0);

            int completedFrame = 0;
            if ((readRet >= 0) && (packet->stream_index == vi->m_avVideoStreamIdx)) {

                // if checking the seek find that we're at the frame we want, then use it
                // otherwise quit seeking and the next decoded frame will be the target frame
                if (packet->dts >= vi->m_seekTimestamp) {
                    validPacket = true;
              //      ++nextIndex;
                } else {
                    avcodec_decode_video2(vi->m_avCodecContext, decodingFrame, &completedFrame, packet);      
                }
            }

            // only delete the packet if we're reading past it, otherwise save for decoder
            if (!validPacket) {
                av_free_packet(packet);
            }

        } while (!validPacket);    
    }
#endif // G3D_NO_FFMPEG
}


// static helpers
static const char* ffmpegError(int code) {
#ifndef G3D_NO_FFMPEG
    switch (iAbs(code)) {
    case AVERROR_UNKNOWN:
        return "Unknown error";
/*    
    case AVERROR_IO:
        return "I/O error";

    case AVERROR_NUMEXPECTED:
        return "Number syntax expected in filename.";

        // gives the compilation error, "case 22 already used."
//    case AVERROR_INVALIDDATA:
//        return "Invalid data found.";

    case AVERROR_NOMEM:
        return "Not enough memory.";

    case AVERROR_NOFMT:
        return "Unknown format";

    case AVERROR_NOTSUPP:
        return "Operation not supported.";

    case AVERROR_NOENT:
        return "No such file or directory.";

    case AVERROR_PATCHWELCOME:
        return "Not implemented in FFmpeg";
*/
    default:
        return "Unrecognized error code.";
    }
#endif
}

} // namespace G3D

