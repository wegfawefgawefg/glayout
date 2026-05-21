#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace glayout {

enum class FormFactor {
    Desktop = 0,
    Tablet = 1,
    Phone = 2,
};

struct Object {
    int id = 0;
    std::string label;
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

struct Layout {
    int id = 0;
    std::string label;
    int width = 0;
    int height = 0;
    FormFactor form_factor = FormFactor::Desktop;
    std::vector<Object> objects;
};

void add_or_replace_object(Layout& layout, const Object& object);
bool remove_object(Layout& layout, int object_id);
bool remove_object(Layout& layout, std::string_view label);

const Object* find_object(const Layout& layout, int object_id);
const Object* find_object(const Layout& layout, std::string_view label);

const Layout* find_best_layout(const std::vector<Layout>& layouts,
                               int layout_id,
                               int target_width,
                               int target_height,
                               FormFactor preferred_form_factor);

} // namespace glayout
