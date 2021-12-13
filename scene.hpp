#pragma once

#include "utilities.hpp"
#include "base.hpp"

#include <map>

class Engine;
class Server;

class CubeMap : public Textured {
public:
    // front, back, up, down, right, left
    CubeMap(Engine *context, std::array<const char *, 6> files);
    CubeMap();
    CubeMap(const CubeMap&);
    CubeMap& operator=(const CubeMap&);
    CubeMap(CubeMap&& other) noexcept;
    CubeMap& operator=(CubeMap&& other) noexcept;
    ~CubeMap();

    // VkSampler sampler;
    // VkImageView imageView;
private:
    VkImage image;
    VmaAllocation allocation;
    Engine *context;
};

struct InstanceZSorter {
    InstanceZSorter(Scene *context);
    bool operator() (int a, int b);
private:
    Scene *context;
};

class Scene {
public:
    static constexpr std::array<glm::vec3, 3> teamColors = {{ 
        { 1.0f, 1.0f, 1.0f },
        { 0.0f, 1.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f }
    }};

    // vertex and index offsets for the model
    Scene(Engine *context, std::vector<std::tuple<const char *, const char *, const char *, const char *>>, std::array<const char *, 6> skyboxImages);
    Scene(Server *context);
    ~Scene();

    // I don't really want to accidentally be copying or moving the scene even though it is now safe to do so
    Scene(const Scene& other) = delete;
    Scene(Scene&& other) noexcept = delete;
    Scene& operator=(const Scene& other) = delete;
    Scene& operator=(Scene&& other) noexcept = delete;

    CubeMap skybox;
    int missingIcon;

    void addInstance(const std::string& name, glm::vec3 position, glm::vec3 heading);
    // O(n) time complexity where n = # of instances
    // I think I can do a binary search instead of a linear search
    inline std::vector<Instance *>::iterator getInstance(InstanceID id) {
        return find_if(state.instances.begin(), state.instances.end(), [id](auto x) -> bool { return x->id == id; });
    }

    // Right now these are public so the engine can see them to copy them to vram
    std::map<std::string, Entity *> entities;
    std::map<std::string, Weapon *> weapons;
    std::map<std::string, UnitAI *> ais;
    std::vector<EntityTexture> textures;
    std::vector<SceneModelInfo> models;

    void makeBuffers();

    std::vector<Utilities::Vertex> vertexBuffer;
    std::vector<uint32_t> indexBuffer;

    // std::vector<Instance> instances;
    ObservableState state;
    
    void updateUniforms(int idx);

    // InstanceZSorter zSorter;

    void initUnitAIs(LuaWrapper *lua, const char *directory);
private:
    Base* context;

    std::vector<Entity *> loadEntitiesFromLua(const char *directory);
    std::vector<Weapon *> loadWeaponsFromLua(const char *directory);
};