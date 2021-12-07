#pragma once

#include <AL/al.h>
#include <AL/alc.h>

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
    ALCdevice *device = nullptr;
    ALCcontext *context = nullptr;
};