#include "gui.hpp"

#include <iostream>

// we use ndc
// This is just in the vertex shader now
// static std::vector<GuiVertex> background = {
//     {{ -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }},
//     {{ -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }},
//     {{ 1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }},
//     {{ 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }},
//     {{ -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }},
//     {{ 1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }}
// };

Gui::Gui(float *mouseNormX, float *mouseNormY)
: root(new GuiComponent(true, 0, { -1.0f, -1.0f }, { 1.0f, 1.0f }, 0)) {
    setDragBox({ 0.0f, 0.0f }, { 0.0f, 0.0f });

    guiThread = std::thread(&Gui::pollChanges, this);
}

// Doing it this way wastes sizeof(GuiVertex) * 12 bytes of vram where the imaginary drag box and background are (...idk what I doing...)
// maxCount is for the size of the buffer (maybe I should just feed in the alloction so I can know the size, but this breaks encapsulation)
size_t Gui::updateBuffer(void *buffer, size_t maxCount) {
    std::scoped_lock lock(dataMutex);
    assert(maxCount >= 12);
    memcpy(static_cast<GuiVertex *>(buffer) + 12, vertices.data(), std::min(maxCount - 12, vertices.size())* sizeof(GuiVertex));
    root->texture.syncTexturesToGPU(textures);
    rebuilt = false;
    return std::min(maxCount, vertices.size() + 12);
}

std::vector<GuiVertex> Gui::rectangle(std::pair<float, float> c0, std::pair<float, float> c1, glm::vec4 color, int layer) {
    return {
        {{ c0.first, c0.second, 1.0f - layerZOffset * (layer + 1) }, color, { 0.0, 0.0 } },
        {{ c1.first, c1.second, 1.0f - layerZOffset * (layer + 1) }, color, { 1.0, 1.0 } },
        {{ c0.first, c1.second, 1.0f - layerZOffset * (layer + 1) }, color, { 0.0, 1.0 } },
        {{ c0.first, c0.second, 1.0f - layerZOffset * (layer + 1) }, color, { 0.0, 0.0 } },
        {{ c1.first, c1.second, 1.0f - layerZOffset * (layer + 1) }, color, { 1.0, 1.0 } },
        {{ c1.first, c0.second, 1.0f - layerZOffset * (layer + 1) }, color, { 1.0, 0.0 } }
    };
}

void Gui::setDragBox(std::pair<float, float> c0, std::pair<float, float> c1) {
    // dragBox = rectangle(c0, c1, { 0.0f, 1.0f, 0.0f, 0.3f }, 1);
    _pushConstant.dragBox[0].x = c0.first;
    _pushConstant.dragBox[0].y = c0.second;
    _pushConstant.dragBox[1].x = c1.first;
    _pushConstant.dragBox[1].y = c1.second;
}

void Gui::submitCommand(GuiCommand command) {
    guiCommands.push(command);
}

GuiPushConstant *Gui::pushConstant() {
    return &_pushConstant;
}

// void Gui::lockPushConstant() {
//     constantMutex.lock();
// }

// void Gui::unlockPushConstant() {
//     constantMutex.unlock();
// }

void Gui::rebuildBuffer() {
    std::vector<GuiTexture *> buildingTextures;
    int idx = 0;
    std::vector<GuiVertex> buildingVertices;

    root->mapTextures(buildingTextures, idx);
    root->buildVertexBuffer(buildingVertices);

    std::scoped_lock lock(dataMutex);
    vertices = buildingVertices;
    textures = buildingTextures;
    rebuilt = true;
}

void Gui::pollChanges() {
    bool terminate = false;
    while(!terminate) {
        // std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
        bool changed = false;
        GuiCommand command;
        while(guiCommands.pop(command)) {
            if(command.action == GUI_TERMINATE) {
                terminate = true;
                break;

                delete command.data;
            } else if (command.action == GUI_ADD) {
                changed = true;
                // data.insert(data.end(), command.data->component->vertices.begin(), command.data->component->vertices.end());
                root->addComponent(command.data->childIndices, command.data->component  );
                delete command.data;
            } else if (command.action == GUI_REMOVE) {
                changed = true;

                delete command.data;
            }
        }
        if (changed) {
            rebuildBuffer();
        }
        // std::chrono::steady_clock::time_point done = std::chrono::steady_clock::now();
        // we should actually not sleep a fixed amount here, but instead see how much time elapsed and then decide to sleep for pollInterval - elapsed
        // std::this_thread::sleep_for(pollInterval - (done - started));
    }
    // TODO delete all components
}

Gui::~Gui() {
    delete root;
    guiCommands.push({ GUI_TERMINATE });
    std::cout << "Waiting to join gui thread..." << std::endl;
    guiThread.join();
}

// Idk what I am doing with these components, This seems more complicated than it needs to be

// RGBA
GuiComponent::GuiComponent(bool layoutOnly, uint32_t color, std::pair<float, float> c0, std::pair<float, float> c1, int layer)
: texture(*GuiTexture::defaultTexture()) {
    this->layoutOnly = layoutOnly; // if fully transparent do not create vertices for it
    this->parent = parent;

    if (!layoutOnly) {
        glm::vec4 colorVec({
            (float)(0xff000000 & color) / 0xff000000,
            (float)(0x00ff0000 & color) / 0x00ff0000,
            (float)(0x0000ff00 & color) / 0x0000ff00,
            (float)(0x000000ff & color) / 0x000000ff,
        });

        std::pair<float, float> tl = { std::min(c0.first, c1.first), std::min(c0.second, c1.second) };
        std::pair<float, float> br = { std::max(c0.first, c1.first), std::max(c0.second, c1.second) };

        vertices = Gui::rectangle(tl, br, colorVec, layer);
    }
}

GuiComponent::GuiComponent(bool layoutOnly, uint32_t color, std::pair<float, float> c0, std::pair<float, float> c1, int layer, GuiTexture texture)
: texture(texture) {
    this->layoutOnly = layoutOnly;
    this->parent = parent;

    if (!layoutOnly) {
        glm::vec4 colorVec({
            (float)(0xff000000 & color) / 0xff000000,
            (float)(0x00ff0000 & color) / 0x00ff0000,
            (float)(0x0000ff00 & color) / 0x0000ff00,
            (float)(0x000000ff & color) / 0x000000ff,
        });

        std::pair<float, float> tl = { std::min(c0.first, c1.first), std::min(c0.second, c1.second) };
        std::pair<float, float> br = { std::max(c0.first, c1.first), std::max(c0.second, c1.second) };

        vertices = Gui::rectangle(tl, br, colorVec, layer);
    }
}

GuiComponent::~GuiComponent() {
    for(auto& child : children)
        delete child;
}

void GuiComponent::addComponent(std::queue<int>& childIndices, GuiComponent *component) {
    addComponent(childIndices, component, this);
}

void GuiComponent::addComponent(std::queue<int>& childIndices, GuiComponent *component, GuiComponent *parent) {
    if (childIndices.empty()) {
        children.push_back(component);
        component->parent = parent;
        return;
    }

    int index = childIndices.front();
    childIndices.pop();
    children.at(index)->addComponent(childIndices, component, this);
}

void GuiComponent::removeComponent(std::queue<int>& childIndices) {
    int index = childIndices.front();
    childIndices.pop();

    if (childIndices.empty()) {
        delete children.at(index);
        children.erase(children.begin() + index);
        return;
    }

    children.at(index)->removeComponent(childIndices);
}

// This can overflow since I use one byte on the gpu to represent index
void GuiComponent::setTextureIndex(int textureIndex) {
    for(auto& vertex : vertices)
        vertex.texIndex = textureIndex;
}

// There some overhead associated with moving/copying texture objects even though they gpu resorces are not changed in any way, so pointers
std::vector<GuiTexture *>& GuiComponent::mapTextures(std::vector<GuiTexture *>& acm, int& idx) {
    std::map<ResourceID, int> resourceMap;
    mapTextures(acm, resourceMap, idx);

    return acm;
}

void GuiComponent::mapTextures(std::vector<GuiTexture *>& acm, std::map<ResourceID, int>& resources, int& idx) {
    ResourceID id = texture.resourceID();
    if (resources.contains(id)) {
        setTextureIndex(resources.at(id));
    } else {
        acm.push_back(&texture);
        resources.insert({ id, idx });
        setTextureIndex(idx);
        idx++;
    }

    for(const auto child : children)
        child->mapTextures(acm, resources, idx);
}

// Arguably I should just do this at the same time as maping the textures
void GuiComponent::buildVertexBuffer(std::vector<GuiVertex>& acm) {
    acm.insert(acm.end(), vertices.begin(), vertices.end());

    for(const auto child : children)
        child->buildVertexBuffer(acm);
}

GuiLabel::GuiLabel(const char *str, uint32_t textColor, uint32_t backgroundColor, std::pair<float, float> c0, std::pair<float, float> c1, int layer)
: GuiComponent(false, backgroundColor, c0, c1, layer, GuiTextures::makeGuiTexture(str)), message(str) {

}

Panel::Panel(const char *filename)
: root(YAML::LoadFile(filename)) {
    // build the tree
    
}