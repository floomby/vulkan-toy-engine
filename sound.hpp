#pragma once

#include <AL/al.h>
#include <AL/alc.h>

#include <list>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <queue>

#include "utilities.hpp"

enum class PlaybackDataKind {
    BUFFER,
    CAMMERA
};

struct PlaybackData {
    PlaybackDataKind kind;
    ALuint buffer;
    glm::vec3 spatial[4];
};

class SoundPlayer {
public:
    SoundPlayer(ALCdevice *device, ALCcontext *context, uint totalSources);
    ~SoundPlayer();
    void submit(PlaybackData&& data);
private:
    ALCdevice *device = nullptr;
    ALCcontext *context = nullptr;
    std::thread playbackThread;
    std::list<ALuint> activeSources;
    std::queue<ALuint> idleSources;
    std::vector<ALuint> sources;
    std::atomic<bool> done = false;
    void run();
    std::queue<PlaybackData> playbackQueue;
    std::mutex playbackQueueLock;
};

class Sound {
public:
    Sound();
    ~Sound();

    std::vector<std::string> listDevices();
    void setDevice(const char *name);
    void playSound(const char *name, const glm::vec3& position = glm::vec3(0.0f), const glm::vec3& velocity = glm::vec3(0.0f), bool assertMono = false);
    void setCammeraPosition(const glm::vec3& position, const glm::vec3& up, const glm::vec3& at, const glm::vec3& velocity);
private:
    void loadSound(const char *name, bool assertMono);
    std::mutex lock;
    // vorbis_file has the codec_setup field and I have no idea if that will be valid ever
    // (so maybe we just don't ever derefference it from anything in this map)
    std::map<std::string, ALuint> cached;
    ALCdevice *device = nullptr;
    ALCcontext *context = nullptr;

    SoundPlayer *player = nullptr;
};
