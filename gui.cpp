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

Gui::Gui(float *mouseNormX, float *mouseNormY, int screenHeight, int screenWidth)
: root(new GuiComponent(this, true, 0, { -1.0f, -1.0f }, { 1.0f, 1.0f }, 0)), height(screenHeight), width(screenWidth) {
    setDragBox({ 0.0f, 0.0f }, { 0.0f, 0.0f });

    guiThread = std::thread(&Gui::pollChanges, this);
}

// Doing it this way wastes sizeof(GuiVertex) * Gui::dummyVertexCount bytes of vram where the imaginary drag box and background are (...idk what I doing...)
// maxCount is for the size of the buffer (maybe I should just feed in the alloction so I can know the size, but this breaks encapsulation)
size_t Gui::updateBuffer(void *buffer, size_t maxCount) {
    std::scoped_lock lock(dataMutex);
    assert(maxCount >= Gui::dummyVertexCount);
    memcpy(static_cast<GuiVertex *>(buffer) + Gui::dummyVertexCount, vertices.data(), std::min(maxCount - Gui::dummyVertexCount, vertices.size())* sizeof(GuiVertex));
    root->texture.syncTexturesToGPU(textures);
    rebuilt = false;
    return std::min(maxCount, vertices.size() + Gui::dummyVertexCount);
}

std::vector<GuiVertex> Gui::rectangle(std::pair<float, float> tl, std::pair<float, float> br, glm::vec4 color, int layer) {
    return rectangle(tl, br, color, color, layer);
}

std::vector<GuiVertex> Gui::rectangle(std::pair<float, float> tl, std::pair<float, float> br, glm::vec4 color, glm::vec4 secondaryColor, int layer) {
    return {
        {{ tl.first, tl.second, 1.0f - layerZOffset * (layer + 1) }, color, secondaryColor, { 0.0, 0.0 } },
        {{ br.first, br.second, 1.0f - layerZOffset * (layer + 1) }, color, secondaryColor, { 1.0, 1.0 } },
        {{ tl.first, br.second, 1.0f - layerZOffset * (layer + 1) }, color, secondaryColor, { 0.0, 1.0 } },
        {{ tl.first, tl.second, 1.0f - layerZOffset * (layer + 1) }, color, secondaryColor, { 0.0, 0.0 } },
        {{ br.first, br.second, 1.0f - layerZOffset * (layer + 1) }, color, secondaryColor, { 1.0, 1.0 } },
        {{ br.first, tl.second, 1.0f - layerZOffset * (layer + 1) }, color, secondaryColor, { 1.0, 0.0 } }
    };
}

std::vector<GuiVertex> Gui::rectangle(std::pair<float, float> tl, float height, float widenessRatio, glm::vec4 color, glm::vec4 secondaryColor, int layer) {
    return Gui::rectangle(tl, { tl.first + 2 * height, tl.second + 2 * height * widenessRatio * (float)this->height / this->width }, color, secondaryColor, layer);
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
        std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
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
                root->addComponent(command.data->childIndices, command.data->component);
                delete command.data;
            } else if (command.action == GUI_REMOVE) {
                changed = true;

                delete command.data;
            } else if (command.action == GUI_RESIZE) {
                changed = true; // really this is rarely false so just treat it like it is always true
                height = command.data->size.height.asInt;
                width = command.data->size.width.asInt;
                delete command.data;
            }
        }
        if (changed) {
            rebuildBuffer();
        }
        std::chrono::steady_clock::time_point done = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(pollInterval - (done - started));
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
GuiComponent::GuiComponent(Gui *context, bool layoutOnly, uint32_t color, std::pair<float, float> c0, std::pair<float, float> c1, int layer)
: texture(*GuiTexture::defaultTexture()), context(context) {
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

GuiComponent::GuiComponent(Gui *context, bool layoutOnly, uint32_t color, std::pair<float, float> c0, std::pair<float, float> c1, int layer, GuiTexture texture)
: GuiComponent(context, layoutOnly, color, color, c0, c1, layer, texture) {}

GuiComponent::GuiComponent(Gui *context, bool layoutOnly, uint32_t color, uint32_t secondaryColor, std::pair<float, float> tl, 
    float height, int layer, GuiTexture texture)
: texture(texture), context(context) {
    this->layoutOnly = layoutOnly;
    this->parent = parent;

    if (!layoutOnly) {
        glm::vec4 colorVec({
            (float)(0xff000000 & color) / 0xff000000,
            (float)(0x00ff0000 & color) / 0x00ff0000,
            (float)(0x0000ff00 & color) / 0x0000ff00,
            (float)(0x000000ff & color) / 0x000000ff,
        });

        glm::vec4 secondaryColorVec({
            (float)(0xff000000 & secondaryColor) / 0xff000000,
            (float)(0x00ff0000 & secondaryColor) / 0x00ff0000,
            (float)(0x0000ff00 & secondaryColor) / 0x0000ff00,
            (float)(0x000000ff & secondaryColor) / 0x000000ff,
        });

        vertices = context->rectangle(tl, height, texture.widenessRatio, colorVec, secondaryColorVec, layer);
    }
}

GuiComponent::GuiComponent(Gui *context, bool layoutOnly, uint32_t color, uint32_t secondaryColor, std::pair<float, float> c0,
    std::pair<float, float> c1, int layer, GuiTexture texture)
: texture(texture), context(context) {
    this->layoutOnly = layoutOnly;
    this->parent = parent;

    if (!layoutOnly) {
        glm::vec4 colorVec({
            (float)(0xff000000 & color) / 0xff000000,
            (float)(0x00ff0000 & color) / 0x00ff0000,
            (float)(0x0000ff00 & color) / 0x0000ff00,
            (float)(0x000000ff & color) / 0x000000ff,
        });

        glm::vec4 secondaryColorVec({
            (float)(0xff000000 & secondaryColor) / 0xff000000,
            (float)(0x00ff0000 & secondaryColor) / 0x00ff0000,
            (float)(0x0000ff00 & secondaryColor) / 0x0000ff00,
            (float)(0x000000ff & secondaryColor) / 0x000000ff,
        });

        std::pair<float, float> tl = { std::min(c0.first, c1.first), std::min(c0.second, c1.second) };
        std::pair<float, float> br = { std::max(c0.first, c1.first), std::max(c0.second, c1.second) };

        vertices = Gui::rectangle(tl, br, colorVec, secondaryColorVec, layer);
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

void GuiComponent::resizeVertices() {
    assert(!"Dynamic ndc not enabled for this gui component");
}

// Arguably I should just do this at the same time as maping the textures
void GuiComponent::buildVertexBuffer(std::vector<GuiVertex>& acm) {
    if (dynamicNDC) resizeVertices();

    acm.insert(acm.end(), vertices.begin(), vertices.end());

    for(const auto child : children)
        child->buildVertexBuffer(acm);
}

GuiLabel::GuiLabel(Gui *context, const char *str, uint32_t textColor, uint32_t backgroundColor, std::pair<float, float> c0, std::pair<float, float> c1, int layer)
: GuiComponent(context, false, backgroundColor, textColor, c0, c1, layer, GuiTextures::makeGuiTexture(str)), message(str) {
    dynamicNDC = true;
}

GuiLabel::GuiLabel(Gui *context, const char *str, uint32_t textColor, uint32_t backgroundColor, std::pair<float, float> tl, float height, int layer)
: GuiComponent(context, false, backgroundColor, textColor, tl, height, layer, GuiTextures::makeGuiTexture(str)), message(str) {
    dynamicNDC = true;
}

void GuiLabel::resizeVertices() {
    std::cout << "resizing vertices" << std::endl;

    float height = vertices[2].pos.y - vertices[0].pos.y;
    float right = vertices[2].pos.x + height * texture.widenessRatio * (float)context->height / context->width;

    vertices[1].pos.x = right;
    vertices[4].pos.x = right;
    vertices[5].pos.x = right;
}

Panel::Panel(const char *filename)
: root(YAML::LoadFile(filename)) {
    // build the tree
    
}