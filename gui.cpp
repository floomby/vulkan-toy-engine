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

Gui::Gui() {
    setDragBox({ 0.0f, 0.0f }, { 0.0f, 0.0f });

    guiThread = std::thread(&Gui::pollChanges, this);
}

// Doing it this way wastes sizeof(GuiVertex) * 12 bytes of vram where the imaginary drag box and background are (...idk what I doing...)
// maxCount is for the size of the buffer (maybe I should just feed in the alloction so I can know the size, but this breaks encapsulation)
size_t Gui::updateBuffer(void *buffer, size_t maxCount) {
    std::scoped_lock lock(dataMutex);
    assert(maxCount >= 12);
    memcpy(static_cast<GuiVertex *>(buffer) + 12, data.data(), std::min(maxCount - 12, data.size())* sizeof(GuiVertex));
    return std::min(maxCount, data.size() + 12);
}

std::vector<GuiVertex> Gui::rectangle(std::pair<float, float> c0, std::pair<float, float> c1, glm::vec4 color, int layer) {
    return {
        {{ c0.first, c0.second, layerZOffset * layer }, color },
        {{ c1.first, c1.second, layerZOffset * layer }, color },
        {{ c0.first, c1.second, layerZOffset * layer }, color },
        {{ c0.first, c0.second, layerZOffset * layer }, color },
        {{ c1.first, c1.second, layerZOffset * layer }, color },
        {{ c1.first, c0.second, layerZOffset * layer }, color }
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

void Gui::lockPushConstant() {
    constantMutex.lock();
}

void Gui::unlockPushConstant() {
    constantMutex.unlock();
}

void Gui::rebuildBuffer() {
    std::cout << "pretend rebuilding buffer" << std::endl;
    rebuilt = true;
}

void Gui::pollChanges() {
    bool terminate = false;
    while(!terminate) {
        bool changed = false;
        GuiCommand command;
        while(guiCommands.pop(command)) {
            if(command.action == GUI_TERMINATE) {
                terminate = true;
                break;

                delete command.data;
            } else if (command.action == GUI_ADD) {
                changed = true;
                data.insert(data.end(), command.data->component->vertices.begin(), command.data->component->vertices.end());

                delete command.data;
            } else if (command.action == GUI_REMOVE) {
                changed = true;

                delete command.data;
            }
        }
        if (changed) {
            rebuildBuffer();
        }
        std::this_thread::sleep_for(pollInterval);
    }
}

Gui::~Gui() {
    guiCommands.push({ GUI_TERMINATE });
    std::cout << "Waiting to join gui thread..." << std::endl;
    guiThread.join();
}

// Idk what I am doing with these components, This seems more complicated than it needs to be

// RGBA
GuiComponent::GuiComponent(uint32_t color, std::pair<float, float> c0, std::pair<float, float> c1, int layer) {
    layoutOnly = !(color & 0xff); // if fully transparent do not create vertices for it
    this->parent = parent;


    if (!layoutOnly) {
        glm::vec4 colorVec({
            (float)(0xff000000 & color) / (float)0xff000000,
            (float)(0x00ff0000 & color) / (float)0x00ff0000,
            (float)(0x0000ff00 & color) / (float)0x0000ff00,
            (float)(0x000000ff & color) / (float)0x000000ff,
        });

        vertices = Gui::rectangle(c0, c1, colorVec, layer);
    }
}

GuiComponent::~GuiComponent() {
    for(auto& child : children)
        delete child;
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

Panel::Panel(const char *filename)
: root(YAML::LoadFile(filename)) {
    // build the tree
    
}