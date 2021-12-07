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

std::int32_t convert_to_int(char* buffer, std::size_t len)
{
    std::int32_t a = 0;
    if(std::endian::native == std::endian::little)
        std::memcpy(&a, buffer, len);
    else
        for(std::size_t i = 0; i < len; ++i)
            reinterpret_cast<char*>(&a)[3 - i] = buffer[i];
    return a;
}

bool load_wav_file_header(std::ifstream& file,
                          std::uint8_t& channels,
                          std::int32_t& sampleRate,
                          std::uint8_t& bitsPerSample,
                          ALsizei& size)
{
    char buffer[4];
    if(!file.is_open())
        return false;

    // the RIFF
    if(!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read RIFF" << std::endl;
        return false;
    }
    if(std::strncmp(buffer, "RIFF", 4) != 0)
    {
        std::cerr << "ERROR: file is not a valid WAVE file (header doesn't begin with RIFF)" << std::endl;
        return false;
    }

    // the size of the file
    if(!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read size of file" << std::endl;
        return false;
    }

    // the WAVE
    if(!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read WAVE" << std::endl;
        return false;
    }
    if(std::strncmp(buffer, "WAVE", 4) != 0)
    {
        std::cerr << "ERROR: file is not a valid WAVE file (header doesn't contain WAVE)" << std::endl;
        return false;
    }

    // "fmt/0"
    if(!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read fmt/0" << std::endl;
        return false;
    }

    // this is always 16, the size of the fmt data chunk
    if(!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read the 16" << std::endl;
        return false;
    }

    // PCM should be 1?
    if(!file.read(buffer, 2))
    {
        std::cerr << "ERROR: could not read PCM" << std::endl;
        return false;
    }

    // the number of channels
    if(!file.read(buffer, 2))
    {
        std::cerr << "ERROR: could not read number of channels" << std::endl;
        return false;
    }
    channels = convert_to_int(buffer, 2);

    // sample rate
    if(!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read sample rate" << std::endl;
        return false;
    }
    sampleRate = convert_to_int(buffer, 4);

    // (sampleRate * bitsPerSample * channels) / 8
    if(!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read (sampleRate * bitsPerSample * channels) / 8" << std::endl;
        return false;
    }

    // ?? dafaq
    if(!file.read(buffer, 2))
    {
        std::cerr << "ERROR: could not read dafaq" << std::endl;
        return false;
    }

    // bitsPerSample
    if(!file.read(buffer, 2))
    {
        std::cerr << "ERROR: could not read bits per sample" << std::endl;
        return false;
    }
    bitsPerSample = convert_to_int(buffer, 2);

    // data chunk header "data"
    if(!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read data chunk header" << std::endl;
        return false;
    }
    if(std::strncmp(buffer, "data", 4) != 0)
    {
        std::cerr << "ERROR: file is not a valid WAVE file (doesn't have 'data' tag)" << std::endl;
        return false;
    }

    // size of data
    if(!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read data size" << std::endl;
        return false;
    }
    size = convert_to_int(buffer, 4);

    /* cannot be at the end of file */
    if(file.eof())
    {
        std::cerr << "ERROR: reached EOF on the file" << std::endl;
        return false;
    }
    if(file.fail())
    {
        std::cerr << "ERROR: fail state set on the file" << std::endl;
        return false;
    }

    return true;
}

char* load_wav(const std::string& filename,
               std::uint8_t& channels,
               std::int32_t& sampleRate,
               std::uint8_t& bitsPerSample,
               ALsizei& size)
{
    std::ifstream in(filename, std::ios::binary);
    if(!in.is_open())
    {
        std::cerr << "ERROR: Could not open \"" << filename << "\"" << std::endl;
        return nullptr;
    }
    if(!load_wav_file_header(in, channels, sampleRate, bitsPerSample, size))
    {
        std::cerr << "ERROR: Could not load wav header of \"" << filename << "\"" << std::endl;
        return nullptr;
    }

    char* data = new char[size];

    in.read(data, size);

    return data;
}

void Sound::playSound(const char *name) {
    std::uint8_t channels;
    std::int32_t sampleRate;
    std::uint8_t bitsPerSample;
    ALsizei size;
    char *buf;
    if(!(buf = load_wav(name, channels, sampleRate, bitsPerSample, size))) {
        std::cerr << "ERROR: Could not load wav" << std::endl;
        return;
    }

    ALuint buffer;
    alErrorGuard(alGenBuffers, 1, &buffer);

    ALenum format;
    if(channels == 1 && bitsPerSample == 8)
        format = AL_FORMAT_MONO8;
    else if(channels == 1 && bitsPerSample == 16)
        format = AL_FORMAT_MONO16;
    else if(channels == 2 && bitsPerSample == 8)
        format = AL_FORMAT_STEREO8;
    else if(channels == 2 && bitsPerSample == 16)
        format = AL_FORMAT_STEREO16;
    else
    {
        std::cerr
            << "ERROR: unrecognised wave format: "
            << channels << " channels, "
            << bitsPerSample << " bps" << std::endl;
        return;
    }

    alErrorGuard(alBufferData, buffer, format, buf, size, sampleRate);
    delete buf;

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

    while(state == AL_PLAYING)
    {
        alErrorGuard(alGetSourcei, source, AL_SOURCE_STATE, &state);
    }

    alErrorGuard(alDeleteSources, 1, &source);
    alErrorGuard(alDeleteBuffers, 1, &buffer);
}