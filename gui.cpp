#include "gui.hpp"

#include <iostream>

static std::vector<GuiVertex> background = {
    {{ -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }},
    {{ -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }},
    {{ 1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }},
    {{ 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }},
    {{ -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }},
    {{ 1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }}
};

Gui::Gui() {
    setDragBox({ 0.0f, 0.0f }, { 0.0f, 0.0f });

    guiThread = std::thread(&Gui::pollChanges, this);
}

// Doing it this way wastes sizeof(GuiVertex) * 12 bytes of vram
// maxCount is for the size of the buffer (maybe I should just feed in the alloction so I can know the size)
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
    rebuilt = true;
}

void Gui::pollChanges() {
    bool terminate;
    while(!terminate) {
        GuiCommand command;
        while(guiCommands.pop(command)) {
            if(command.action == GUI_TERMINATE) {
                terminate = true;
                break;


                delete command.data;
            }
        }
        std::this_thread::sleep_for(pollInterval);
    }
}

Gui::~Gui() {
    guiCommands.push({ GUI_TERMINATE });
    std::cout << "Waiting to join gui thread" << std::endl;
    guiThread.join();
}

Gui::GuiComponent::~GuiComponent() {
    for(auto& child : children)
        delete child;
}

void Gui::GuiComponent::addComponent(std::queue<int> childIndices, GuiComponent *component) {
    if (childIndices.empty()) {
        children.push_back(component);
        return;
    }

    int index = childIndices.front();
    childIndices.pop();
    children.at(index)->addComponent(childIndices, component);
}

void Gui::GuiComponent::removeComponent(std::queue<int> childIndices) {
    if (childIndices.empty()) {
        // ...and we have this line of code
        delete this;
        return;
    }

    int index = childIndices.front();
    childIndices.pop();
    children.at(index)->removeComponent(childIndices);
}