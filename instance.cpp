#include "instance.hpp"

Instance::Instance(Entity* entity, InternalTexture* texture, SceneModelInfo* sceneModelInfo) noexcept {
    this->entity = entity;
    this->texture = texture;
    this->sceneModelInfo = sceneModelInfo;
    _state.highlight = 0.0;
    _highlight = false;
};

// Instance& Instance::transform(glm::mat4 transformationMatrix) noexcept {
//     state.model *= transformationMatrix;
//     return *this;
// };

UniformBufferObject *Instance::state() {
    // I read that this is bad to do as it adds much to the memory cost of doing draws (it makes sense though)
    // probably fine for right now, but probably should be changed to something else
    _state.highlight = _highlight ? 1.0 : 0.0;
    _state.model = glm::translate(position);
    return &_state;
}

bool& Instance::highlight() {
    return _highlight;
}