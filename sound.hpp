#pragma once

#include <AL/al.h>
#include <AL/alc.h>

#include <map>
#include <mutex>
#include <string>
#include <vector>

class Sound {
public:
    Sound();
    ~Sound();

    std::vector<std::string> listDevices();
    void setDevice(const char *name);
    void playSound(const char *name);
private:
    void loadSound(const char *name);
    std::mutex lock;
    // vorbis_file has the codec_setup field and I have no idea if that will be valid ever
    // (so maybe we just don't ever derefference it from anything in this map)
    std::map<std::string, ALuint> cached;
    ALCdevice *device = nullptr;
    ALCcontext *context = nullptr;
};