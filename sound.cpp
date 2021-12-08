#include <cassert>
#include <cstring>
#include <stdexcept>
#include <sstream>

#include "sound.hpp"

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#define alErrorGuard(function, ...) alcCallImpl(__FILE__, __LINE__, function, device, __VA_ARGS__)

void checkAlcErrors(const std::string& filename, const size_t line, ALCdevice* device) {
    ALCenum error = alcGetError(device);
    if (error != ALC_NO_ERROR) {
        std::ostringstream ss;
        ss << "OpenAL error: " << filename << ": " << line << " - " ;
        switch(error) {
            case ALC_INVALID_VALUE:
                ss << "ALC_INVALID_VALUE: an invalid value was passed to an OpenAL function";
                break;
            case ALC_INVALID_DEVICE:
                ss << "ALC_INVALID_DEVICE: a bad device was passed to an OpenAL function";
                break;
            case ALC_INVALID_CONTEXT:
                ss << "ALC_INVALID_CONTEXT: a bad context was passed to an OpenAL function";
                break;
            case ALC_INVALID_ENUM:
                ss << "ALC_INVALID_ENUM: an unknown enum value was passed to an OpenAL function";
                break;
            case ALC_OUT_OF_MEMORY:
                ss << "ALC_OUT_OF_MEMORY: an unknown enum value was passed to an OpenAL function";
                break;
            default:
                ss << "UNKNOWN ALC ERROR: 0x" << std::hex << error;
        }
        ss << std::endl;
        throw std::runtime_error(ss.str());
    }
}

template<typename alcFunction, typename... Params>
auto alcCallImpl(const char* filename, const size_t line, alcFunction function, ALCdevice* device, Params... params)
    -> typename std::enable_if_t<std::is_same_v<void, decltype(function(params...))>, void> {
    function(std::forward<Params>(params)...);
    checkAlcErrors(filename, line, device);
}

template<typename alcFunction, typename ReturnType, typename... Params>
auto alcCallImpl(const char* filename, const size_t line, alcFunction function, ReturnType& returnValue, ALCdevice* device,  Params... params)
    -> typename std::enable_if_t<!std::is_same_v<void, decltype(function(params...))>, void> {
    returnValue = function(std::forward<Params>(params)...);
    checkAlcErrors(filename, line, device);
}

Sound::Sound() {
    ALboolean enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
    if (enumeration == AL_FALSE) throw std::runtime_error("Missing openal ALC_ENUMERATION_EXT");
}

Sound::~Sound() {
    for (const auto [name, buffer] : cached) {
        alErrorGuard(alDeleteBuffers, 1, &buffer);
    }
    if (context) {
        alcMakeContextCurrent(NULL);
        alcDestroyContext(context);
    }
    if (device) alcCloseDevice(device);
}

std::vector<std::string> Sound::listDevices() {
    std::vector<std::string> ret;
    auto it = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
    while(it && (*it || *(it + 1))) {
        ret.push_back(std::string(it));
        it += strlen(it) + 1;
    }
    return ret;
}

void Sound::setDevice(const char *name) {
    if (device) alcCloseDevice(device);
    if (context) {
        alcMakeContextCurrent(NULL);
        alcDestroyContext(context);
    }

    device = alcOpenDevice(name);
    if (!device) throw std::runtime_error("Unable to open openal device");

    context = alcCreateContext(device, NULL);
    if (!alcMakeContextCurrent(context)) throw std::runtime_error("Unable to make openal context");
}

#include <bit>
#include <fstream>
#include <iostream>

namespace StreamCallbacks {

    static size_t read(void* buffer, size_t elementSize, size_t elementCount, void* dataSource) {
        assert(elementSize == 1);
        std::ifstream& stream = *static_cast<std::ifstream*>(dataSource);
        stream.read(static_cast<char*>(buffer), elementCount);
        const std::streamsize bytesRead = stream.gcount();
        stream.clear(); // In case we read past EOF
        return static_cast<size_t>(bytesRead);
    }

    static int seek(void* dataSource, ogg_int64_t offset, int origin) {
        static const std::vector<std::ios_base::seekdir> seekDirections{
            std::ios_base::beg, std::ios_base::cur, std::ios_base::end
        };

        std::ifstream& stream = *static_cast<std::ifstream*>(dataSource);
        stream.seekg(offset, seekDirections.at(origin));
        stream.clear(); // In case we seeked to EOF
        return 0;
    }

    static long tell(void* dataSource) {
        std::ifstream& stream = *static_cast<std::ifstream*>(dataSource);
        const auto position = stream.tellg();
        assert(position >= 0);
        return static_cast<long>(position);
    }
}

static void addChunk(std::vector<char>& buffer, size_t newSize) {
    auto oldSize = buffer.size();
    assert(newSize > oldSize);
    buffer.resize(newSize);
    memset(buffer.data() + oldSize, 0, newSize - oldSize);
}


// This does not stream, it buffers everything into ram first, and it also caches
// It has poor effeciency, use the (yet to be implemented) streaming version to play back big files
void Sound::loadSound(const char *name) {
    lock.lock();
    if (cached.contains(name)) {
        lock.unlock();
        return;
    }
    lock.unlock();
    OggVorbis_File vorbisFile;
    bool decoding = true;

    std::ifstream stream;
    stream.open(name, std::ios::binary);

    const ov_callbacks callbacks { StreamCallbacks::read, StreamCallbacks::seek, nullptr, StreamCallbacks::tell };
    int err;
    if((err = ov_open_callbacks(&stream, &vorbisFile, NULL, 0, callbacks)) < 0) {
        std::ostringstream ss;
        ss << "Invalid ogg vorbis file: 0x" << std::hex << err;
        throw std::runtime_error(ss.str());
    }

    vorbis_info* vorbisInfo = ov_info(&vorbisFile, -1);
    std::cout << "File info: " << vorbisInfo->rate << "Hz, "
        << vorbisInfo->channels << " channels" << std::endl;

    size_t bytesRead = 0;
    std::vector<char> pcmBuffer;
    addChunk(pcmBuffer, 4096 * 2);

    int section = 0;

    while (decoding) {
        long ret = ov_read(&vorbisFile, pcmBuffer.data() + bytesRead, 4096, 0, 2, 1, &section);
        if (ret == 0) {
            decoding = false;
            /* end of stream */
        } else if (ret < 0) {
            if (ret == OV_EBADLINK) {
                throw std::runtime_error("Corrupt bitstream section!");
            }
            std::cerr << "Some sort of other vorbis error 0x" << std::hex << ret << std::endl;
            /* some other error in the stream.  Not a problem, just reporting it in
                case we (the app) cares.  In this case, we don't. */
        } else {
            bytesRead += ret;
            if (bytesRead + 4096 > pcmBuffer.size()) addChunk(pcmBuffer, pcmBuffer.size() * 2);
        }
    }

    ALenum format;
    if(vorbisInfo->channels == 1)
        format = AL_FORMAT_MONO16;
    else if(vorbisInfo->channels == 2)
        format = AL_FORMAT_STEREO16;
    else {
        std::ostringstream ss;
        ss << "Unsupported sound file format: " << vorbisInfo->channels << " channels";
        throw std::runtime_error(ss.str());
    }

    // auto pcmFile = std::fstream("check.pcm", std::ios::out | std::ios::binary);
    // pcmFile.write(pcmBuffer.data(), bytesRead);
    // pcmFile.close();

    lock.lock();
    if (!cached.contains(name)) {
        ALuint buffer;
        alErrorGuard(alGenBuffers, 1, &buffer);
        alErrorGuard(alBufferData, buffer, format, pcmBuffer.data(), bytesRead, vorbisInfo->rate);
        
        cached.insert({ name, buffer });
    }
    lock.unlock();

    ov_clear(&vorbisFile);
}

void Sound::playSound(const char *name) {
    ALuint buffer;
    lock.lock();
    if (cached.contains(name)) {
        buffer = cached.at(name);
        lock.unlock();
    } else {
        lock.unlock();
        loadSound(name);
        lock.lock();
        buffer = cached.at(name);
        lock.unlock();
    }

    ALuint source;
    alErrorGuard(alGenSources, 1, &source);
    alErrorGuard(alSourcef, source, AL_PITCH, 1);
    alErrorGuard(alSourcef, source, AL_GAIN, 1.0f);
    alErrorGuard(alSource3f, source, AL_POSITION, 0, 0, 0);
    alErrorGuard(alSource3f, source, AL_VELOCITY, 0, 0, 0);
    alErrorGuard(alSourcei, source, AL_LOOPING, AL_FALSE);
    alErrorGuard(alSourcei, source, AL_BUFFER, buffer);
    alErrorGuard(alSourcePlay, source);

    ALint state = AL_PLAYING;

    while(state == AL_PLAYING) {
        alErrorGuard(alGetSourcei, source, AL_SOURCE_STATE, &state);
    }

    alErrorGuard(alDeleteSources, 1, &source);
}