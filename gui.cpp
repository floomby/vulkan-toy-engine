#include "gui.hpp"

Gui::Gui() {

}

void Gui::updateUniformBuffer(void *buffer, size_t maxCount) {
    memcpy(buffer, data.data(), std::min(maxCount, data.size()) * sizeof(GuiVertex));
}

size_t Gui::verticiesCount() {
    return data.size();
}

void Gui::addRectangle(std::pair<float, float> c0, std::pair<float, float> c1, glm::vec4 color, int layer) {
        data.push_back({{ c0.first, c0.second, layerZOffset * layer }, color });
        data.push_back({{ c1.first, c1.second, layerZOffset * layer }, color });
        data.push_back({{ c0.first, c1.second, layerZOffset * layer }, color });
        data.push_back({{ c0.first, c0.second, layerZOffset * layer }, color });
        data.push_back({{ c1.first, c1.second, layerZOffset * layer }, color });
        data.push_back({{ c1.first, c0.second, layerZOffset * layer }, color });
}