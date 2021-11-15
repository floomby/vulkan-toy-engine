#include "gui.hpp"

#include <iostream>

// Tbh the layers should probably just be created when the vertex buffer is built and be just the parent layer plus one

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

Gui::Gui(float *mouseNormX, float *mouseNormY, int screenHeight, int screenWidth, Engine *context)
: context(context), root(new GuiComponent(this, true, 0, { -1.0f, -1.0f }, { 1.0f, 1.0f }, 0, {}, RMODE_NONE)), height(screenHeight), width(screenWidth),
lua(LuaWrapper(true)) {
    // idLookup.erase(0);
    // root->id = UINT32_MAX;
    // idLookup.insert({ root->id, root });
    setDragBox({ 0.0f, 0.0f }, { 0.0f, 0.0f });

    guiThread = std::thread(&Gui::pollChanges, this);

    createBuffers(50);
    whichBuffer = 0;
    usedSizes[0] = usedSizes[1] = Gui::dummyVertexCount;

    lua.exportEnumToLua<GuiLayoutType>();

    lua.apiExport();
}

Gui::~Gui() {
    destroyBuffer(0);
    destroyBuffer(1);
    delete root;
    guiCommands.push({ GUI_TERMINATE });
    std::cout << "Waiting to join gui thread..." << std::endl;
    guiThread.join();
    assert(idLookup.empty());
}

// Doing it this way wastes sizeof(GuiVertex) * Gui::dummyVertexCount bytes of vram where the imaginary drag box and background are (...idk what I doing...)
// maxCount is for the size of the buffer (maybe I should just feed in the alloction so I can know the size, but this breaks encapsulation)
std::tuple<size_t, std::map<uint32_t, uint>, VkBuffer> Gui::updateBuffer() {
    std::scoped_lock lock(dataMutex);
    root->textures[0].syncTexturesToGPU(textures);
    rebuilt = false;
    return { usedSizes[whichBuffer], idToBuffer, gpuBuffers[whichBuffer] };
}

// There is a problem with this, for the sake of keeping complexity low I am abondinging to be reevaluaded if the gui rebuilding
// turns out to be too slow
// void Gui::setTextureIndexInBuffer(GuiVertex *buffer, uint index, int textureIndex) {
//     std::cout << "here --- " << index << std::endl;
//     for (int i = 0; i < 6; i++) {
//         std::cout << "texture index " << (buffer + 6 * index + i)->texIndex << std::endl;
//         (buffer + 6 * index + i)->texIndex += textureIndex;
//     }
// }

std::vector<GuiVertex> Gui::rectangle(std::pair<float, float> tl, std::pair<float, float> br,
    glm::vec4 color, glm::vec4 secondaryColor, int layer, uint32_t id, uint32_t renderMode) {
    return {
        {{ tl.first, tl.second, 1.0f - layerZOffset * (layer + 1) }, color, secondaryColor, { 0.0, 0.0 }, 0, id, renderMode, 0 },
        {{ br.first, br.second, 1.0f - layerZOffset * (layer + 1) }, color, secondaryColor, { 1.0, 1.0 }, 0, id, renderMode, 0 },
        {{ tl.first, br.second, 1.0f - layerZOffset * (layer + 1) }, color, secondaryColor, { 0.0, 1.0 }, 0, id, renderMode, 0 },
        {{ tl.first, tl.second, 1.0f - layerZOffset * (layer + 1) }, color, secondaryColor, { 0.0, 0.0 }, 0, id, renderMode, 0 },
        {{ br.first, br.second, 1.0f - layerZOffset * (layer + 1) }, color, secondaryColor, { 1.0, 1.0 }, 0, id, renderMode, 0 },
        {{ br.first, tl.second, 1.0f - layerZOffset * (layer + 1) }, color, secondaryColor, { 1.0, 0.0 }, 0, id, renderMode, 0 }
    };
}

std::vector<GuiVertex> Gui::rectangle(std::pair<float, float> tl, std::pair<float, float> br,
    glm::vec4 color, int layer, uint32_t id, uint32_t renderMode) {
    return rectangle(tl, br, color, color, layer, id, renderMode);
}

std::vector<GuiVertex> Gui::rectangle(std::pair<float, float> tl, float height, float widenessRatio,
    glm::vec4 color, glm::vec4 secondaryColor, int layer, uint32_t id, uint32_t renderMode) {
    return Gui::rectangle(tl, { tl.first + 2 * height, tl.second + 2 * height * widenessRatio * (float)this->height / this->width }, color, secondaryColor, layer, id, renderMode);
}

void Gui::setDragBox(std::pair<float, float> c0, std::pair<float, float> c1) {
    // dragBox = rectangle(c0, c1, { 0.0f, 1.0f, 0.0f, 0.3f }, 1);
    _pushConstant.dragBox[0].x = c0.first;
    _pushConstant.dragBox[0].y = c0.second;
    _pushConstant.dragBox[1].x = c1.first;
    _pushConstant.dragBox[1].y = c1.second;
}

void Gui::setCursorID(uint32_t id) {
    _pushConstant.guiID = id;
}

void Gui::submitCommand(GuiCommand command) {
    guiCommands.push(command);
}

// I had a reason for this silly looking thing, but it kind of went away I think
GuiPushConstant *Gui::pushConstant() {
    return &_pushConstant;
}

// void Gui::lockPushConstant() {
//     constantMutex.lock();
// }

// void Gui::unlockPushConstant() {
//     constantMutex.unlock();
// }

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
            } else if (command.action == GUI_ADD) {
                changed = true;
                // data.insert(data.end(), command.data->component->vertices.begin(), command.data->component->vertices.end());
                root->addComponent(command.data->childIndices, command.data->component);
                delete command.data;
            } else if (command.action == GUI_REMOVE) {
                changed = true;
                root->removeComponent(command.data->childIndices);
                delete command.data;
            } else if (command.action == GUI_RESIZE) {
                changed = true; // really this is rarely false so just treat it like it is always true
                height = command.data->size.height.asInt;
                width = command.data->size.width.asInt;
                delete command.data;
            } else if (command.action == GUI_CLICK) {
                idLookup.at(command.data->id)->click(command.data->position.x.asFloat, command.data->position.y.asFloat);
                delete command.data;
            } else if (command.action == GUI_LOAD) {
                try {
                    // fromFile(command.data->str);
                    root->addComponent(command.data->childIndices, fromFile(command.data->str, root->getComponent(command.data->childIndices)->layer + 1));
                } catch (const std::exception& e) {
                    std::cerr << e.what() << std::endl;
                    // I may need to clean up the state in the lua_State "object"
                }
                changed = true;
                delete command.data;
            }
        }
        if (changed) {
            rebuildBuffer();
        }
        // std::chrono::steady_clock::time_point done = std::chrono::steady_clock::now();
        // std::this_thread::sleep_for(pollInterval - (done - started));
        std::this_thread::yield();
    }
}

// RGBA is the order

// Every gui component ultimately comes from one of these 3 constructors (there probably should be just one)
// TODO Fix this
GuiComponent::GuiComponent(Gui *context, bool layoutOnly, uint32_t color, std::pair<float, float> c0, std::pair<float, float> c1, int layer, std::map<std::string, int> luaHandlers, uint32_t renderMode)
: textures({ *GuiTexture::defaultTexture() }), context(context), luaHandlers(luaHandlers) {
    textureIndexMap.resize(textures.size());
    this->layoutOnly = layoutOnly; // if fully transparent do not create vertices for it
    this->parent = parent;
    id = context->idCounter++;
    this->layer = layer;
    this->renderMode = renderMode;
    context->idLookup.insert({ id, this });

    if (!layoutOnly) {
        glm::vec4 colorVec({
            (float)(0xff000000 & color) / 0xff000000,
            (float)(0x00ff0000 & color) / 0x00ff0000,
            (float)(0x0000ff00 & color) / 0x0000ff00,
            (float)(0x000000ff & color) / 0x000000ff,
        });

        std::pair<float, float> tl = { std::min(c0.first, c1.first), std::min(c0.second, c1.second) };
        std::pair<float, float> br = { std::max(c0.first, c1.first), std::max(c0.second, c1.second) };

        vertices = Gui::rectangle(tl, br, colorVec, layer, id, renderMode);
    }
}

GuiComponent::GuiComponent(Gui *context, bool layoutOnly, uint32_t color, uint32_t secondaryColor, std::pair<float, float> tl, 
    float height, int layer, std::vector<GuiTexture> textures, std::map<std::string, int> luaHandlers, uint32_t renderMode)
: textures(textures), context(context), luaHandlers(luaHandlers) {
    textureIndexMap.resize(textures.size());
    this->layoutOnly = layoutOnly;
    this->parent = parent;
    id = context->idCounter++;
    this->layer = layer;
    this->renderMode = renderMode;
    context->idLookup.insert({ id, this });

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

        vertices = context->rectangle(tl, height, textures[0].widenessRatio, colorVec, secondaryColorVec, layer, id, renderMode);
    }
}

GuiComponent::GuiComponent(Gui *context, bool layoutOnly, uint32_t color, uint32_t secondaryColor, std::pair<float, float> c0,
    std::pair<float, float> c1, int layer, std::vector<GuiTexture> textures, std::map<std::string, int> luaHandlers, uint32_t renderMode)
: textures(textures), context(context), luaHandlers(luaHandlers) {
    textureIndexMap.resize(textures.size());
    this->layoutOnly = layoutOnly;
    this->parent = parent;
    id = context->idCounter++;
    this->layer = layer;
    this->renderMode = renderMode;
    context->idLookup.insert({ id, this });

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

        vertices = Gui::rectangle(tl, br, colorVec, secondaryColorVec, layer, id, renderMode);
    }
}

GuiComponent::GuiComponent(Gui *context, bool layoutOnly, uint32_t color, std::pair<float, float> c0,
    std::pair<float, float> c1, int layer, std::vector<GuiTexture> textures, std::map<std::string, int> luaHandlers, uint32_t renderMode)
: GuiComponent(context, layoutOnly, color, color, c0, c1, layer, textures, luaHandlers, renderMode) {}

GuiComponent::~GuiComponent() {
    context->idLookup.erase(id);

    for(auto& child : children)
        delete child;
}

GuiComponent *GuiComponent::getComponent(std::queue<uint> childIndices) {
    return getComponent_i(childIndices);
}

GuiComponent *GuiComponent::getComponent_i(std::queue<uint>& childIndices) {
    if (childIndices.empty()) {
        return this;
    }

    int index = childIndices.front();
    childIndices.pop();
    return children.at(index)->getComponent_i(childIndices);
}


void GuiComponent::addComponent(std::queue<uint> childIndices, GuiComponent *component) {
    addComponent(childIndices, component, this);
}

void GuiComponent::addComponent(std::queue<uint>& childIndices, GuiComponent *component, GuiComponent *parent) {
    if (childIndices.empty()) {
        children.push_back(component);
        component->parent = parent;
        return;
    }

    int index = childIndices.front();
    childIndices.pop();
    children.at(index)->addComponent(childIndices, component, this);
}

void GuiComponent::removeComponent(std::queue<uint>& childIndices) {
    // This is fine, you should never delete the root
    int index = childIndices.front();
    childIndices.pop();

    if (childIndices.empty()) {
        if (index >= children.size()) return;
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
    for (int i = 0; i < textures.size(); i++) {
        ResourceID id = textures[i].resourceID();
        if (resources.contains(id)) {
            if (i == activeTexture) setTextureIndex(resources.at(id));
            textureIndexMap[i] = idx;
        } else {
            acm.push_back(&textures[i]);
            resources.insert({ id, idx });
            if (i == activeTexture) setTextureIndex(idx);
            textureIndexMap[i] = idx;
            idx++;
        }
    }

    for(const auto child : children)
        child->mapTextures(acm, resources, idx);
}

// returns the id with the highest layer (if 2 or more have equal it only returns the one which comes first in the tree)
uint32_t GuiComponent::allContaining(std::vector<GuiComponent *>& acm, float x, float y) {
    std::pair<int, int> note = { id, layer };
    allContaining(acm, x, y, note);
    return note.first;
}

// Idk if I even want to use this to look for the mouse position because it relys on having the guicomponent pointers be valid
// which means it has to run in the gui thread (or it could segfault) which means it will lag if the gui thread is busy rebuilding the gui.
// This is nice to have around though, but probably better is the raw buffer searching method because it has no dependency on anything the
// gui thread is doing and is pretty faster anyways.
uint32_t GuiComponent::allContaining(std::vector<GuiComponent *>& acm, float x, float y, std::pair<int, int>& idMaxLayer) {
    if (containsNDCPoint(x, y)) {
        acm.push_back(this);
        if (layer > idMaxLayer.second) {
            idMaxLayer.first = id;
            idMaxLayer.second = layer;
        }
    }

    for(const auto child : children)
        child->allContaining(acm, x, y);

    return UINT32_MAX; // just need an always invalid id here
}

void GuiComponent::resizeVertices() {
    assert(!"Dynamic ndc not enabled for this gui component");
}

void GuiComponent::click(float x, float y) {
    if (luaHandlers.contains("onClick"))
        context->lua.callFunction(luaHandlers["onClick"]);
}

// Arguably I should just do this at the same time as maping the textures
void GuiComponent::buildVertexBuffer(std::vector<GuiVertex>& acm, std::map<uint32_t, uint>& indexMap, uint& index) {
    if (dynamicNDC) resizeVertices();

    acm.insert(acm.end(), vertices.begin(), vertices.end());
    indexMap.insert({ id, index });
    index++;

    for(const auto child : children)
        child->buildVertexBuffer(acm, indexMap, index);
}

GuiLabel::GuiLabel(Gui *context, const char *str, uint32_t textColor, uint32_t backgroundColor, std::pair<float, float> c0, std::pair<float, float> c1, int layer,
std::map<std::string, int> luaHandlers)
: GuiComponent(context, false, backgroundColor, textColor, c0, c1, layer, { GuiTextures::makeGuiTexture(str) }, luaHandlers, RMODE_TEXT), message(str) {
    dynamicNDC = true;
    textures.push_back(GuiTextures::makeGuiTexture("cleared"));
}

GuiLabel::GuiLabel(Gui *context, const char *str, uint32_t textColor, uint32_t backgroundColor, std::pair<float, float> tl, float height, int layer,
std::map<std::string, int> luaHandlers)
: GuiComponent(context, false, backgroundColor, textColor, tl, height, layer, { GuiTextures::makeGuiTexture(str) }, luaHandlers, RMODE_TEXT), message(str) {
    dynamicNDC = true;
    textures.push_back(GuiTextures::makeGuiTexture("cleared"));
}

void GuiLabel::resizeVertices() {
    std::cout << "resizing vertices" << std::endl;

    float height = vertices[2].pos.y - vertices[0].pos.y;
    float right = vertices[2].pos.x + height * textures[0].widenessRatio * (float)context->height / context->width;

    vertices[1].pos.x = right;
    vertices[4].pos.x = right;
    vertices[5].pos.x = right;
}

// void GuiLabel::click(float x, float y) {
//     // static bool called;
//     // if (!called) called = true;
//     // context->guiMessages.push({ Gui::GUI_TOGGLE_TEXTURE, new Gui::GuiMessageData { id, called ? 0 : -1 } });
//     activeTexture = (int)!(bool)activeTexture;
//     context->rebuildBuffer();
// }

// Panel::Panel(const char *filename)
// : root(YAML::LoadFile(filename)) {
//     // build the tree
// }

GuiComponent *Gui::fromFile(std::string name, int baseLayer) {
    auto tree = lua.loadGuiFile(name.c_str());
    return fromLayout(tree, baseLayer);
}

GuiComponent *Gui::fromLayout(GuiLayoutNode *tree, int baseLayer) {
    GuiComponent *ret = nullptr;
    switch (tree->kind) {
        case GuiLayoutType::PANEL:
            ret = new GuiComponent(this, false, tree->color, { tree->x, tree->y }, { tree->x + tree->width, tree->y + tree->height }, baseLayer, tree->handlers);
            break;
        case GuiLayoutType::TEXT_BUTTON:
            ret = new GuiLabel(this, tree->text.c_str(), tree->color, 0x000000ff, { tree->x, tree->y }, { tree->x + tree->width, tree->y + tree->height }, baseLayer, tree->handlers);
            break;
        default:
            throw std::runtime_error("Unsupported gui layout kind - aborting.");
    }
    ret->children.reserve(tree->children.size());
    for (auto& child : tree->children) {
        ret->children.push_back(fromLayout(child, baseLayer + 1));
    }
    return ret;
}