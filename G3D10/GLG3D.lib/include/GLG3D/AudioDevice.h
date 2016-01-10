/* 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/
#ifndef GLG3D_AudioDevice_h
#define GLG3D_AudioDevice_h

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/G3DString.h"

#ifndef G3D_NO_FMOD

// Forward declarations from FMOD's namespace needed for defining the
// AudioDevice classes. This avoids exposing FMOD to the programmer
// directly.
namespace FMOD {
    class System;
    class Sound;
    class Channel;
};

namespace G3D {

class Sound;
class AudioDevice;
class AudioChannel;

/** A playing Sound.
    
    \sa Sound::play 

	\beta
*/
class AudioChannel {
protected:
    friend class AudioDevice;
	friend class Sound;

    FMOD::Channel*  m_fmodChannel;

    AudioChannel(FMOD::Channel* f) : m_fmodChannel(f) {}

    /** Delete resources */
    void cleanup();

public:

	bool paused() const;

	void setPaused(bool paused);

    /** \param v on [0, 1] */
    void setVolume(float v);

    /** \param p -1.0 = left, 0.0 = center, 1.0 = right */
    void setPan(float p);

    /** Playback frequency in Hz */
    void setFrequency(float hz);
};



/** 
 \brief Initializes the audio system.

 G3D automatically initializes and cleans up AudioDevice and invokes AudioDevice::update from
 RenderDevice::swapBuffers, so 
 this class is rarely accessed by programs explicitly.

 Under VisualStudio, the current FMOD implementation causes large delays when run under a debugger.
 Use CTRL+F5 to launch your program to avoid this. 

 \beta
*/
class AudioDevice {
protected:
    friend class Sound;
    friend class AudioChannel;

    /** \brief Append-only dynamic array of weak pointers for objects to be shut down on AudioDevice::cleanup() */
    template<class T>
    class WeakCleanupArray {
    private:

        Array<weak_ptr<T> >         m_array;
        int                         m_rememberCallsSinceLastCheck;
    public:
        WeakCleanupArray() : m_rememberCallsSinceLastCheck(-2) {}

        shared_ptr<T> remember(const shared_ptr<T>& r) {
            ++m_rememberCallsSinceLastCheck;

            // Amortized O(1) cleanup of old pointer values 
            if (m_rememberCallsSinceLastCheck > m_array.length()) {
                for (int i = 0; i < m_array.length(); ++i) {
                    if (isNull(m_array[i].lock())) {
                        m_array.fastRemove(i);
                        --i;
                    }
                }
                m_rememberCallsSinceLastCheck = 0;
            }

            m_array.append(r);
            return r;
        }

        void cleanup() {
            for (int i = 0; i < m_array.length(); ++i) {
                const shared_ptr<T>& r = m_array[i].lock();
                if (notNull(r)) {
                    r->cleanup();
                }
            }
            m_array.clear();
        }
    };

    FMOD::System*                   m_fmodSystem;

    /** For cleaning up during shutdown */
    WeakCleanupArray<Sound>         m_soundArray;

    /** For cleaning up during shutdown */
    WeakCleanupArray<AudioChannel>  m_channelArray;

	bool							m_enable;

public:

    enum {ANY_FREE = -1};

    static AudioDevice* instance;

    AudioDevice();

    ~AudioDevice();

    /** Invoke once per frame on the main thread to service the audio system. RenderDevice::swapBuffers automatically invokes this. */
    void update();

	/** The value from init() of the enableSound argument */
	bool enabled() const {
		return m_enable;
	}

    /** \param numVirtualChannels Number of channels to allocate.  There is no reason not to make this fairly
        large. The limit is 4093 and 1000 is the default inherited from FMOD.
		
		\param enableSound If false, then AudioDevice exists but no sounds will play and FMOD is not initialized.
		This is convenient for debugging a program that uses sound, so that Sound objects can still be instantiated
		but no disk access or sound-related performance delays will occur.
		*/
    void init(bool enableSound = true, int numVirtualChannels = 1000);

    /** Destroys all Sounds and AudioChannels and shuts down the FMOD library. */
    void cleanup();
};


/** \brief Sound file loaded into memory that can be played on an AudioChannel.

    Analogous to a graphics texture...a (typically) read-only value that can be

	\sa AudioChannel
	\beta
  */
class Sound : public ReferenceCountedObject {
protected:
    friend class AudioDevice;
	friend class AudioChannel;

    FMOD::Sound*    m_fmodSound;

    Sound();

    /** Delete resources */
    void cleanup();

public:

    static shared_ptr<Sound> create(const String& filename, bool loop = false);

	static const int16 DEFAULT_FREQUENCY = -1;

    /** Returns the channel on which the sound is playing so that it can be terminated or
        adjusted. The caller is not required to retain the AudioChannel pointer to keep the 
        sound playing. */
    shared_ptr<AudioChannel> play(float initialVolume = 1.0f, float initialPan = 0.0f, float initialFrequency = DEFAULT_FREQUENCY, bool startPaused = false);

    ~Sound();
};

} // namespace
#endif
#endif

