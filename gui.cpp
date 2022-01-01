// I think some of the constructors and stuff work, but are unused. I am going to implement some new features here.
// The *only* thing I care about is that it works with the lua as expected
// Frankly the entire gui is super jank, I don't think the overall design is bad just severly unrefined and has some bad ways of doing things

#include "engine.hpp"

#include <iostream>

Gui::Gui(float *mouseNormX, float *mouseNormY, int screenHeight, int screenWidth, Engine *context)
: context(context), root(new GuiComponent(this, true, 0, 0, { -1.0f, -1.0f }, { 1.0f, 1.0f }, 0, {}, RMODE_NONE)), height(screenHeight), width(screenWidth) {
    setDragBox({ 0.0f, 0.0f }, { 0.0f, 0.0f });

    lua = new LuaWrapper(true);

    createBuffers(50);
    whichBuffer = 0;
    usedSizes[0] = usedSizes[1] = Gui::dummyVertexCount;

    lua->apiExport();
    lua->exportEnumToLua<GuiLayoutKind>();
    lua->exportEnumToLua<ToggleKind>();
    lua->exportEnumToLua<InsertionMode>();
    lua->exportEnumToLua<IEngage>();
    lua->exportEnumToLua<IntrinicStates>();
    guiThread = std::thread(&Gui::pollChanges, this);
    root->parent = nullptr;
}

Gui::~Gui() {
    destroyBuffer(0);
    destroyBuffer(1);
    delete root;
    delete lua;
    guiCommands.push({ GUI_TERMINATE });
    std::cout << "Waiting to join gui thread..." << std::endl;
    guiThread.join();
    assert(idLookup.empty());
    assert(namedComponents.empty());
}

std::tuple<size_t, std::map<uint32_t, uint>, VkBuffer> Gui::updateBuffer() {
    std::scoped_lock lock(dataMutex);
    if (needTextureSync) {
        context->rewriteHudDescriptors(textures);
        needTextureSync = false;
    }
    rebuilt = false;
    return { usedSizes[whichBuffer], idToBuffer, gpuBuffers[whichBuffer] };
}

// There is a problem with this, for the sake of keeping complexity low I am abandoning to be reevaluaded if the gui rebuilding
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
    return Gui::rectangle(tl, { tl.first + 2 * height, tl.second + 2 * height * widenessRatio * (float)this->height / this->width },
        color, secondaryColor, layer, id, renderMode);
}

void Gui::setDragBox(std::pair<float, float> c0, std::pair<float, float> c1) {
    // dragBox = rectangle(c0, c1, { 0.0f, 1.0f, 0.0f, 0.3f }, 1);
    pushConstant.dragBox[0].x = c0.first;
    pushConstant.dragBox[0].y = c0.second;
    pushConstant.dragBox[1].x = c1.first;
    pushConstant.dragBox[1].y = c1.second;
}

// These setter methods are silly
void Gui::setCursorID(uint32_t id) {
    pushConstant.guiID = id;
}

void Gui::submitCommand(GuiCommand&& command) {
    std::scoped_lock l(queueLock);
    guiCommands.push(command);
}

void Gui::pollChanges() {
    bool terminate = false;
    lua->enableCallbacksOnThisThread();
    lua->enableKeyBindings();
    while(!terminate) {
        changed = false;
        queueLock.lock();
        while(!guiCommands.empty()) {
            GuiCommand command = guiCommands.front();
            guiCommands.pop();
            queueLock.unlock();
            if(command.action == GUI_TERMINATE) {
                terminate = true;
                break;
            } else if (command.action == GUI_ADD) {
                if (command.data->flags == GUIF_INDEX) {
                    root->addComponent(command.data->childIndices, command.data->component);
                    changed = true;
                // data.insert(data.end(), command.data->component->vertices.begin(), command.data->component->vertices.end());
                } else if (command.data->flags == GUIF_NAMED) {
                    if (namedComponents.contains(command.data->str)) {
                        auto comp = namedComponents.at(command.data->str);
                        comp->addComponent({}, command.data->component);
                        changed = true;
                    } else {
                        std::cerr << "Unable to find named component: " << command.data->str << std::endl;
                    }
                }
                delete command.data;
            } else if (command.action == GUI_REMOVE) {
                if (command.data->flags == GUIF_INDEX) {
                    root->removeComponent(command.data->childIndices);
                    changed = true;
                // data.insert(data.end(), command.data->component->vertices.begin(), command.data->component->vertices.end());
                } else if (command.data->flags == GUIF_NAMED) {
                    if (namedComponents.contains(command.data->str)) {
                        auto comp = namedComponents.at(command.data->str);
                        comp->removeComponent(command.data->childIndices);
                        changed = true;
                    } else {
                        std::cerr << "Unable to find named component: " << command.data->str << std::endl;
                    }
                } else if (command.data->flags == GUIF_PANEL_NAME) {
                    try {
                        auto comp = this->root->findPanelRoot(command.data->str);
                        lua->removeGuiTable(command.data->str);
                        comp->removeComponent(command.data->childIndices);
                        changed = true;
                    } catch (const std::exception& e) {
                        std::cerr << e.what() << std::endl;
                    }
                }
                delete command.data;
            } else if (command.action == GUI_RESIZE) {
                changed = true; // really this is rarely false so just treat it like it is always true
                height = command.data->size.height.asInt;
                width = command.data->size.width.asInt;
                delete command.data;
            } else if (command.action == GUI_CLICK) {
                if (withCapture && command.data->id != withCapture->id) {
                    uncaptureKeyboard();
                }
                idLookup.at(command.data->id)->click(command.data->position.x.asFloat, command.data->position.y.asFloat, command.data->flags);
                delete command.data;
            } else if (command.action == GUI_HOVER) {
                idLookup.at(command.data->id)->hover();
                delete command.data;
            } else if (command.action == GUI_LOAD) {
                try {
                    if ((command.data->flags & GUIF_INSERT_MODE) == GUIF_INDEX) {
                        auto tmp = command.data->flags & GUIF_LUA_TABLE ?
                            fromTable(command.data->str2, root->getComponent(command.data->childIndices)->layer + 1) :
                            fromFile(command.data->str2, root->getComponent(command.data->childIndices)->layer + 1);
                        if (tmp) root->addComponent(command.data->childIndices, tmp);
                    } else if ((command.data->flags & GUIF_INSERT_MODE) == GUIF_NAMED) {
                        if (namedComponents.contains(command.data->str)) {
                            auto comp = namedComponents.at(command.data->str);
                            auto tmp = command.data->flags & GUIF_LUA_TABLE ?
                                fromTable(command.data->str2, comp->layer + 1) :
                                fromFile(command.data->str2, comp->layer + 1);
                            if (tmp) comp->addComponent({}, tmp);
                        } else {
                            std::cerr << "Unable to find named component: " << command.data->str << std::endl;
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << e.what() << std::endl;
                    // I may need to clean up the state in the lua_State "object"
                }
                changed = true;
                delete command.data;
            } else if (command.action == GUI_VISIBILITY) {
                if (command.data->flags == GUIF_INDEX) {
                    auto comp = root->getComponent(command.data->childIndices);
                    if (comp->visible != (bool)command.data->action) {
                        comp->visible = (bool)command.data->action;
                        changed = true;
                    }
                } else if (command.data->flags == GUIF_NAMED) {
                    if (namedComponents.contains(command.data->str)) {
                        auto comp = namedComponents.at(command.data->str);
                        if (comp->visible != (bool)command.data->action) {
                            comp->visible = (bool)command.data->action;
                            changed = true;
                        }
                    } else {
                        std::cerr << "Unable to find named component: " << command.data->str << std::endl;
                    }
                } else if (command.data->flags == GUIF_PANEL_NAME) {
                    auto comp = root->findPanelRoot(command.data->str);
                    if (comp) {
                        if (comp->visible != (bool)command.data->action) {
                            comp->visible = (bool)command.data->action;
                            changed = true;
                        }
                    } else {
                        std::cerr << "Unable to find panel " << command.data->str << std::endl;
                    }
                }
                delete command.data;
            } else if (command.action == GUI_NOTIFY) {
                root->propegateEngineNotification(command.data->str);
                delete command.data;
            } else if (command.action == GUI_TEXT) {
                if (command.data->flags == GUIF_INDEX) {
                    auto comp = root->getComponent(command.data->childIndices);
                    if (comp->getText() != command.data->str2) {
                        comp->setText(std::move(command.data->str2));
                        changed = true;
                    }
                } else if (command.data->flags == GUIF_NAMED) {
                    if (namedComponents.contains(command.data->str)) {
                        auto comp = namedComponents.at(command.data->str);
                        if (comp->getText() != command.data->str2) {
                            comp->setText(std::move(command.data->str2));
                            changed = true;
                        }
                    } else {
                        std::cerr << "Unable to find named component: " << command.data->str << std::endl;
                    }
                } else std::cerr << "Unsupported component mode" << std::endl;
                delete command.data;
            } else if (command.action == GUI_CODEPOINT_INPUT) {
                if (withCapture) {
                    withCapture->typing(command.data->action);
                }
                delete command.data;
            } else if (command.action == GUI_KEY_INPUT) {
                if (withCapture) {
                    withCapture->keyInput(command.data->action);
                }
                delete command.data;
            } else if (command.action == GUI_KEYBINDING) {
                lua->callKeyBinding(command.data->id, command.data->flags);
                delete command.data;
            } else if (command.action == GUI_TOGGLE) {
                if (command.data->flags == GUIF_INDEX) {
                    auto comp = root->getComponent(command.data->childIndices);
                    if (comp->activeTexture != command.data->action) {
                        comp->setToggleState(command.data->action);
                        changed = true;
                    }
                } else if (command.data->flags == GUIF_NAMED) {
                    if (namedComponents.contains(command.data->str)) {
                        auto comp = namedComponents.at(command.data->str);
                        if (comp->activeTexture != command.data->action) {
                            comp->setToggleState(command.data->action);
                            changed = true;
                        }
                    } else {
                        std::cerr << "Unable to find named component: " << command.data->str << std::endl;
                    }
                }
                delete command.data;
            }
            queueLock.lock();
        }
        queueLock.unlock();
        if (changed) {
            rebuildBuffer(guiThreadNeedTextureSync);
            guiThreadNeedTextureSync = false;
        }
        lua->dispatchCallbacks();
        std::this_thread::yield();
    }
}

GuiComponent *GuiComponent::findPanelRoot(const std::string& panelName) {
    if (panelName == this->panelName) return this;
    auto it = children.begin();
    GuiComponent *ret = nullptr;
    while (!ret && it != children.end()) {
        ret = (*it)->findPanelRoot(panelName);
        it++;
    }
    return ret;
}

// Every gui component ultimately comes from one of these 3 constructors (there probably should be just one)
// TODO Fix this, all these constructors are confusing to work with
GuiComponent::GuiComponent(Gui *context, bool layoutOnly, uint32_t color, uint32_t secondaryColor, const std::pair<float, float>& c0, const std::pair<float, float>& c1,
    int layer, const std::map<std::string, int>& luaHandlers, uint32_t renderMode)
: context(context), luaHandlers(luaHandlers) {
    if (renderMode != RMODE_IMAGE) {
        textures.push_back(GuiTexture::defaultTexture());
    }
    textureIndexMap.resize(textures.size());
    this->layoutOnly = layoutOnly; // if fully transparent do not create vertices for it
    this->parent = parent;
    id = context->idCounter++;
    this->layer = layer;
    this->renderMode = renderMode;
    context->idLookup.insert({ id, this });

    if (!layoutOnly) {
        glm::vec4 colorVec = Api::util_colorIntToVec(color);
        glm::vec4 secondaryColorVec = Api::util_colorIntToVec(secondaryColor);

        std::pair<float, float> tl = { std::min(c0.first, c1.first), std::min(c0.second, c1.second) };
        std::pair<float, float> br = { std::max(c0.first, c1.first), std::max(c0.second, c1.second) };

        vertices = Gui::rectangle(tl, br, colorVec, secondaryColorVec, layer, id, renderMode);
    }
    for (const auto& tex : textures) {
        if (context->guiThreadNeedTextureSync) break;
        if (!context->alreadyHaveTexture(tex)) context->guiThreadNeedTextureSync = true;
    }
}

GuiComponent::GuiComponent(Gui *context, bool layoutOnly, uint32_t color, uint32_t secondaryColor, std::pair<float, float> tl, 
    float height, int layer, std::vector<std::shared_ptr<GuiTexture>> textures, std::map<std::string, int> luaHandlers, uint32_t renderMode)
: textures(textures), context(context), luaHandlers(luaHandlers) {
    textureIndexMap.resize(textures.size());
    this->layoutOnly = layoutOnly;
    this->parent = parent;
    id = context->idCounter++;
    this->layer = layer;
    this->renderMode = renderMode;
    context->idLookup.insert({ id, this });

    if (!layoutOnly) {
        glm::vec4 colorVec = Api::util_colorIntToVec(color);
        glm::vec4 secondaryColorVec = Api::util_colorIntToVec(secondaryColor);

        vertices = context->rectangle(tl, height, textures[0]->widenessRatio, colorVec, secondaryColorVec, layer, id, renderMode);
    }
    for (const auto& tex : textures) {
        if (context->guiThreadNeedTextureSync) break;
        if (!context->alreadyHaveTexture(tex)) context->guiThreadNeedTextureSync = true;
    }
}

GuiComponent::GuiComponent(Gui *context, bool layoutOnly, uint32_t color, uint32_t secondaryColor, std::pair<float, float> c0,
    std::pair<float, float> c1, int layer, std::vector<std::shared_ptr<GuiTexture>> textures, std::map<std::string, int> luaHandlers, uint32_t renderMode)
: textures(textures), context(context), luaHandlers(luaHandlers) {
    textureIndexMap.resize(textures.size());
    this->layoutOnly = layoutOnly;
    this->parent = parent;
    id = context->idCounter++;
    this->layer = layer;
    this->renderMode = renderMode;
    context->idLookup.insert({ id, this });

    if (!layoutOnly) {
        glm::vec4 colorVec = Api::util_colorIntToVec(color);
        glm::vec4 secondaryColorVec = Api::util_colorIntToVec(secondaryColor);

        std::pair<float, float> tl = { std::min(c0.first, c1.first), std::min(c0.second, c1.second) };
        std::pair<float, float> br = { std::max(c0.first, c1.first), std::max(c0.second, c1.second) };

        vertices = Gui::rectangle(tl, br, colorVec, secondaryColorVec, layer, id, renderMode);
    }
    for (const auto& tex : textures) {
        if (context->guiThreadNeedTextureSync) break;
        if (!context->alreadyHaveTexture(tex)) context->guiThreadNeedTextureSync = true;
    }
}

GuiComponent::~GuiComponent() {
    context->idLookup.erase(id);

    for(auto& child : children)
        delete child;

    if (context->withCapture == this) context->withCapture = nullptr;
    if (!name.empty()) {
        context->namedComponents.erase(name);
    }
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
    assert(parent);
    if (childIndices.empty()) {
        parent->children.erase(std::remove(parent->children.begin(), parent->children.end(), this), parent->children.end());
        delete this;
        return;
    }

    throw std::runtime_error("I think this code is broken");
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

static std::map<ResourceID, int> lastTextureMap;

bool Gui::alreadyHaveTexture(std::shared_ptr<GuiTexture> texture) const {
    return lastTextureMap.contains(texture->resourceID());
}

// There some overhead associated with moving/copying texture objects even though they gpu resorces are not changed in any way, so pointers
std::vector<std::shared_ptr<GuiTexture>>& GuiComponent::mapTextures(std::vector<std::shared_ptr<GuiTexture>>& acm, int& idx) {
    std::map<ResourceID, int> resourceMap;
    mapTextures(acm, resourceMap, idx);

    lastTextureMap = resourceMap;
    return acm;
}

void GuiComponent::mapTextures(std::vector<std::shared_ptr<GuiTexture>>& acm, std::map<ResourceID, int>& resources, int& idx) {
    for (int i = 0; i < textures.size(); i++) {
        ResourceID id = textures[i]->resourceID();
        if (resources.contains(id)) {
            if (i == activeTexture) setTextureIndex(resources.at(id));
            textureIndexMap[i] = idx;
        } else {
            acm.push_back(textures[i]);
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

void GuiComponent::click(float x, float y, int mods) {
    auto offset = context->lua->getPanelHandlerOffset(panelName);
    if (luaHandlers.contains("onClick"))
        context->lua->callFunction(luaHandlers["onClick"] + offset, mods, name);
}

void GuiComponent::hover() {
    if (!tooltip.empty()) {
        auto what = new Gui::GuiMessageData();
        what->str = tooltip;
        context->guiMessages.push({ Gui::ENG_SETTOOLTIP, what });
    }
}

void GuiComponent::toggle() {
    auto offset = context->lua->getPanelHandlerOffset(panelName);
    if (luaHandlers.contains("onToggle"))
        context->lua->callFunction(luaHandlers["onToggle"] + offset, activeTexture);

    context->changed = true;
}

void GuiComponent::setToggleState(uint state) {
    if (state >= textures.size()) throw std::runtime_error("Toggle state is greater than active textures");

    activeTexture = state;
}

void GuiComponent::typing(uint codepoint) {
    throw std::runtime_error("This component does not support editing");
}

void GuiComponent::keyInput(uint key) {
    throw std::runtime_error("This component does not support editing");
}

void GuiComponent::propegateEngineNotification(const std::string& notification) {
    auto offset = context->lua->getPanelHandlerOffset(panelName);
    if (luaHandlers.contains(notification))
        context->lua->callFunction(luaHandlers[notification] + offset, activeTexture);
    
    for(const auto child : children)
        child->propegateEngineNotification(notification);
}

// Arguably I should just do this at the same time as maping the textures
void GuiComponent::buildVertexBuffer(std::vector<GuiVertex>& acm, std::map<uint32_t, uint>& indexMap, uint& index) {
    if (dynamicNDC) resizeVertices();

    if (index == Gui::dummyCompomentCount) oldTextures.clear();
    
    if (visible) {
        acm.insert(acm.end(), vertices.begin(), vertices.end());
        for (const auto& vert : vertices) {
            if (vert.pos.x == 0.0f && vert.pos.y == 0.0f)
                std::cout << "We found the problem" << std::endl;
        }
        indexMap.insert({ id, index });
        index++;

        for(const auto child : children)
            child->buildVertexBuffer(acm, indexMap, index);
    }
}

GuiLabel::GuiLabel(Gui *context, const char *str, uint32_t textColor, uint32_t backgroundColor, std::pair<float, float> c0, std::pair<float, float> c1, int layer,
std::map<std::string, int> luaHandlers, bool hoverable)
: GuiComponent(context, false, backgroundColor, textColor, c0, c1, layer, { context->context->glyphCache->makeGuiTexture(str) }, luaHandlers, 
RMODE_TEXT | int(!hoverable) * RFLAG_NO_HOVER ), message(str) {
    dynamicNDC = true;
    // textures.push_back(context->context->glyphCache->makeGuiTexture("cleared"));
    for (const auto& tex : textures) {
        if (context->guiThreadNeedTextureSync) break;
        if (!context->alreadyHaveTexture(tex)) context->guiThreadNeedTextureSync = true;
    }
}

GuiLabel::GuiLabel(Gui *context, const char *str, uint32_t textColor, uint32_t backgroundColor, std::pair<float, float> tl, float height, int layer,
std::map<std::string, int> luaHandlers)
: GuiComponent(context, false, backgroundColor, textColor, tl, height, layer, { context->context->glyphCache->makeGuiTexture(str) }, luaHandlers, RMODE_TEXT), message(str) {
    dynamicNDC = true;
    // textures.push_back(context->context->glyphCache->makeGuiTexture("cleared"));
    for (const auto& tex : textures) {
        if (context->guiThreadNeedTextureSync) break;
        if (!context->alreadyHaveTexture(tex)) context->guiThreadNeedTextureSync = true;
    }
}

void GuiLabel::resizeVertices() {
    float height = vertices[2].pos.y - vertices[0].pos.y;
    float right = vertices[2].pos.x + height * textures[0]->widenessRatio * (float)context->height / context->width;

    vertices[1].pos.x = right;
    vertices[4].pos.x = right;
    vertices[5].pos.x = right;
}

void GuiImage::resizeVertices() {
    float height = vertices[2].pos.y - vertices[0].pos.y;
    float right = vertices[2].pos.x + height * textures[0]->widenessRatio * (float)context->height / context->width;

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

GuiComponent *Gui::fromFile(std::string name, int baseLayer) {
    auto tree = lua->loadGuiFile(name.c_str());
    if (!tree) return nullptr;
    auto ret = fromLayout(tree, baseLayer, name);
    ret->visible = false;
    delete tree;
    return ret;
}

GuiComponent *Gui::fromTable(std::string name, int baseLayer) {
    auto tree = lua->loadGuiTable(name.c_str());
    if (!tree) return nullptr;
    auto ret = fromLayout(tree, baseLayer, name);
    ret->visible = false;
    delete tree;
    return ret;
}

GuiComponent *Gui::fromLayout(GuiLayoutNode *tree, int baseLayer, const std::string& panelName) {
    GuiComponent *ret = nullptr;
    switch (tree->kind) {
        case GuiLayoutKind::PANEL:
            ret = new GuiComponent(this, false, tree->color, tree->secondaryColor, { tree->x, tree->y }, { tree->x + tree->width, tree->y + tree->height },
                baseLayer + tree->layerOverride, tree->handlers);
            break;
        case GuiLayoutKind::TEXT:
        case GuiLayoutKind::TEXT_BUTTON:
            ret = new GuiLabel(this, tree->text.c_str(), tree->color, tree->secondaryColor, { tree->x, tree->y },
                { tree->x + tree->width, tree->y + tree->height }, baseLayer + tree->layerOverride, tree->handlers, tree->kind == GuiLayoutKind::TEXT_BUTTON);
            break;
        case GuiLayoutKind::TEXT_EDITABLE:
            ret = new GuiEditable(this, tree->text.c_str(), tree->color, tree->secondaryColor, { tree->x, tree->y },
                { tree->x + tree->width, tree->y + tree->height }, baseLayer + tree->layerOverride, tree->handlers);
            break;
        case GuiLayoutKind::IMAGE_BUTTON:
            ret = new GuiImage(this, tree->text.c_str(), tree->color, tree->secondaryColor, { tree->x, tree->y }, { tree->x + tree->width, tree->y + tree->height },
                tree->imageStates, baseLayer + tree->layerOverride, tree->handlers);
            break;
        default:
            throw std::runtime_error("Unsupported gui layout kind - aborting.");
    }
    ret->panelName = panelName;
    if (!tree->name.empty()) {
        if (namedComponents.contains(tree->name)) {
            throw std::runtime_error("Component with this name already exists");
        }
        namedComponents.insert({ tree->name, ret });
        ret->name = tree->name;
    }
    ret->tooltip = tree->tooltip;
    ret->children.reserve(tree->children.size());
    for (auto& child : tree->children) {
        ret->children.push_back(fromLayout(child, baseLayer + tree->layerOverride + 1, panelName));
    }
    return ret;
}

GuiImage::GuiImage(Gui *context, const char *file, uint32_t color, uint32_t secondaryColor, const std::pair<float, float>& tl, const std::pair<float, float>& br,
    const std::vector<std::string>& images, int layer, std::map<std::string, int> luaHandlers)
: GuiComponent(context, false, color, secondaryColor, tl, br, layer, luaHandlers, RMODE_IMAGE), state(0) {
    dynamicNDC = true;
    for (const auto& image : images) {
        textures.push_back(GuiTextures::makeGuiTexture(image));
    }
    textureIndexMap.resize(textures.size());
    for (const auto& tex : textures) {
        if (context->guiThreadNeedTextureSync) break;
        if (!context->alreadyHaveTexture(tex)) context->guiThreadNeedTextureSync = true;
    }
}

void GuiImage::click(float x, float y, int mods) {
    if (textures.size() == 1) {
        GuiComponent::click(x, y, mods);
        return;
    }
    activeTexture = (activeTexture + 1) % textures.size();
    this->toggle();
}

// void GuiImage::toggle() {
//     if (luaHandlers.contains("onToggle"))
//         context->lua->callFunction(luaHandlers["onToggle"], activeTexture);
// }

Textured::Textured()
: imageView(VK_NULL_HANDLE), sampler(VK_NULL_HANDLE) { }

void GuiComponent::setText(const std::string& text) {
    throw std::runtime_error("This component does not support text setting");
}

void GuiLabel::setText(const std::string& text) {
    oldTextures.insert(oldTextures.end(), textures.begin(), textures.end());
    message = text;
    textures = { context->context->glyphCache->makeGuiTexture(text) };
    context->guiThreadNeedTextureSync = true;
}

void GuiComponent::setText(std::string&& text) {
    throw std::runtime_error("This component does not support text setting");
}

void GuiLabel::setText(std::string&& text) {
    oldTextures.insert(oldTextures.end(), textures.begin(), textures.end());
    message = std::move(text);
    textures = { context->context->glyphCache->makeGuiTexture(message) };
    context->guiThreadNeedTextureSync = true;
}

const std::string& GuiComponent::getText() {
    throw std::runtime_error("This component does not support text getting");
}

const std::string& GuiLabel::getText() {
    return message;
}

GuiEditable::GuiEditable(Gui *context, const char *str, uint32_t textColor, uint32_t backgroundColor, std::pair<float, float> c0, std::pair<float, float> c1,
int layer, std::map<std::string, int> luaHandlers)
: GuiLabel(context, str, textColor, backgroundColor, c0, c1, layer, luaHandlers, true) { }

void GuiEditable::click(float x, float y, int mods) {
    GuiComponent::click(x, y, mods);
    context->withCapture = this;
    auto what = new Gui::GuiMessageData();
    what->bl = true;
    context->guiMessages.push({ Gui::ENG_CAPTURE, what });
}

void GuiEditable::typing(uint codepoint) {
    // std::cout << "got codepoint 0x" << std::hex << codepoint << std::endl;
    editText.push_back(codepoint);
    this->setText(context->converter.to_bytes(editText));
    // bleck, I shouldn't need to do this in the way I am
    context->changed = true;
}

void GuiEditable::keyInput(uint key) {
    if (key == GLFW_KEY_ENTER) {
        context->uncaptureKeyboard();
        auto offset = context->lua->getPanelHandlerOffset(panelName);
        if (luaHandlers.contains("onTextUpdated"))
            context->lua->callFunction(luaHandlers["onTextUpdated"] + offset);
    } else if (key == GLFW_KEY_BACKSPACE) {
        if (!editText.empty()) {
            editText.pop_back();
            this->setText(context->converter.to_bytes(editText));
            context->changed = true;
        }
    }
}

void Gui::uncaptureKeyboard() {
    auto what = new Gui::GuiMessageData();
    what->bl = false;
    guiMessages.push({ Gui::ENG_CAPTURE, what });
    withCapture = nullptr;
}