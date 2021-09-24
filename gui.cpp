#include "gui.hpp"

Gui::Gui() {

}

void Gui::updateUniformBuffer(void *buffer, size_t maxCount) {
    memcpy(buffer, data.data(), std::min(maxCount, data.size()) * sizeof(GuiVertex));
}