/* 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/

#include "GLG3D/AudioDevice.h"

#if (! defined(G3D_NO_FMOD))

#include "../../fmod/include/fmod.hpp"
#include "../../fmod/include/fmod_errors.h"

#include "G3D/FileSystem.h"
#include "G3D/Log.h"

namespace G3D {

AudioDevice* AudioDevice::instance = NULL;

static void ERRCHECK(FMOD_RESULT result) {
    alwaysAssertM(result == FMOD_OK, format("FMOD error! (%d) %s", result, FMOD_ErrorString(result)));
}
    
    
AudioDevice::AudioDevice() : m_fmodSystem(NULL), m_enable(false) {
    instance = this;
}
    
    
void AudioDevice::init(bool enable, int numVirtualChannels) {
m_enable = enable;
if (enable) {
    alwaysAssertM(m_fmodSystem == NULL, "Already initialized");
    FMOD_RESULT result = FMOD::System_Create(&m_fmodSystem);
    ERRCHECK(result);
            
    unsigned int version;
    result = m_fmodSystem->getVersion(&version);
    ERRCHECK(result);
            
    alwaysAssertM(version >= FMOD_VERSION, format("FMOD lib version %08x doesn't match header version %08x", version, FMOD_VERSION));
            
    void* extradriverdata = NULL;
    result = m_fmodSystem->init(numVirtualChannels, FMOD_INIT_NORMAL, extradriverdata);
    ERRCHECK(result);
} else {
        logPrintf("WARNING: AudioDevice is not enabled. Set G3DSpecification::audio = true before invoking initGLG3D() to enable audio.\n");
}
        
}
    
    
void AudioDevice::cleanup() {
    if (notNull(m_fmodSystem)) {
        m_channelArray.cleanup();
        m_soundArray.cleanup();
            
        FMOD_RESULT result;
        result = m_fmodSystem->close();
        ERRCHECK(result);
        result = m_fmodSystem->release();
        ERRCHECK(result);
        m_fmodSystem = NULL;
    }
}
    
    
AudioDevice::~AudioDevice() {
    cleanup();
}
    
    
void AudioDevice::update() {
if (notNull(m_fmodSystem)) {
        FMOD_RESULT result = m_fmodSystem->update();
        ERRCHECK(result);
}
}
    
    
////////////////////////////////////////////////////////////////////////
    
void AudioChannel::setVolume(float v) {
    if (notNull(m_fmodChannel)) m_fmodChannel->setVolume(v);
}
    
    
void AudioChannel::setPan(float p) {
    if (notNull(m_fmodChannel)) m_fmodChannel->setPan(p);
}
    
    
void AudioChannel::setFrequency(float hz) {
    if (notNull(m_fmodChannel)) m_fmodChannel->setFrequency(hz);
}
    
    
void AudioChannel::cleanup() {
    m_fmodChannel = NULL;
}
    
    
bool AudioChannel::paused() const {
bool paused = false;
    if (notNull(m_fmodChannel)) m_fmodChannel->getPaused(&paused);
return paused;
}
    
    
void AudioChannel::setPaused(bool p) {
    if (notNull(m_fmodChannel)) m_fmodChannel->setPaused(p);
}
    
////////////////////////////////////////////////////////////////////////
    
    
Sound::Sound() : m_fmodSound(NULL) {
}
    
    
shared_ptr<AudioChannel> Sound::play(float initialVolume, float initialPan, float initialFrequency, bool startPaused) {
    if (isNull(m_fmodSound)) { return shared_ptr<AudioChannel>(); }
        
    static const bool START_PAUSED = true;
    FMOD::Channel* channel = NULL;
    FMOD_RESULT result = AudioDevice::instance->m_fmodSystem->playSound(m_fmodSound, NULL, START_PAUSED, &channel);
    ERRCHECK(result);
        
    const shared_ptr<AudioChannel>& ch = shared_ptr<AudioChannel>(new AudioChannel(channel));
    ch->setVolume(initialVolume);
    ch->setPan(initialPan);
    if (initialFrequency > 0) { ch->setFrequency(initialFrequency); }
        
    if (! startPaused) {
            ch->setPaused(false);
    }
        
    return AudioDevice::instance->m_channelArray.remember(ch);
}
    
    
shared_ptr<Sound> Sound::create(const String& filename, bool loop) {
    alwaysAssertM(notNull(AudioDevice::instance), "AudioDevice not initialized");
    const shared_ptr<Sound> s(new Sound());
        
    FMOD_MODE mode = FMOD_DEFAULT;
        
    if (loop) {
        mode |= FMOD_LOOP_NORMAL;
    }
        
    if (! FileSystem::exists(filename)) {
        throw filename + " not found in Sound::create";
    } else {
        FileSystem::markFileUsed(filename);
    }
 
    if (AudioDevice::instance->enabled()) {
        const FMOD_RESULT result = AudioDevice::instance->m_fmodSystem->createSound(filename.c_str(), mode, 0, &s->m_fmodSound);
        ERRCHECK(result);
    }
        
    return AudioDevice::instance->m_soundArray.remember(s);
}
    
    
Sound::~Sound() {
    cleanup();
}
    
    
void Sound::cleanup() {
    if (m_fmodSound) {
        m_fmodSound->release();
        m_fmodSound = NULL;
    }
}

} // namespace G3D

#endif
