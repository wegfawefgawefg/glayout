#include "glayout/layout.hpp"

#include <iostream>
#include <vector>

int main() {
    std::vector<glayout::Layout> layouts{
        glayout::Layout{100, "Title", 1280, 720, glayout::FormFactor::Desktop, {}},
        glayout::Layout{100, "Title", 1920, 1080, glayout::FormFactor::Desktop, {}},
        glayout::Layout{100, "Title", 1080, 1920, glayout::FormFactor::Phone, {}},
    };

    glayout::Object play_button{};
    play_button.id = 1;
    play_button.label = "play_button";
    play_button.x = 0.4f;
    play_button.y = 0.45f;
    play_button.w = 0.2f;
    play_button.h = 0.08f;
    glayout::add_or_replace_object(layouts[1], play_button);

    const glayout::Layout* layout =
        glayout::find_best_layout(layouts, 100, 1920, 1080, glayout::FormFactor::Desktop);
    if (!layout) {
        std::cerr << "No layout found\n";
        return 1;
    }

    std::cout << "Selected layout: " << layout->label << " " << layout->width << "x"
              << layout->height << "\n";

    const glayout::Object* object = glayout::find_object(*layout, "play_button");
    if (object) {
        std::cout << "Found object: " << object->label << " at " << object->x << ", "
                  << object->y << "\n";
    }

    return 0;
}
