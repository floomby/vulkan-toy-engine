#include "instance.hpp"

Instance::Instance(Entity* entity, InternalTexture* texture, SceneModelInfo* sceneModelInfo) noexcept {
    this->entity = entity;
    this->texture = texture;
    this->sceneModelInfo = sceneModelInfo;
    coordinates = glm::mat4(1.0f);
};

Instance& Instance::Transform(glm::mat4 transformationMatrix) noexcept {
    coordinates *= transformationMatrix;
    return *this;
};