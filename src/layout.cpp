#include "glayout/layout.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace glayout {
namespace {

float layout_score(const Layout& layout, int target_width, int target_height) {
    if (layout.width <= 0 || layout.height <= 0 || target_width <= 0 || target_height <= 0)
        return std::numeric_limits<float>::max();

    float target_aspect = static_cast<float>(target_width) / static_cast<float>(target_height);
    float layout_aspect = static_cast<float>(layout.width) / static_cast<float>(layout.height);
    float aspect_distance = std::fabs(target_aspect - layout_aspect);
    float dx = static_cast<float>(target_width - layout.width);
    float dy = static_cast<float>(target_height - layout.height);
    float resolution_distance = std::sqrt(dx * dx + dy * dy);

    return aspect_distance * 1000.0f + resolution_distance;
}

const Layout* find_best_matching_form_factor(const std::vector<Layout>& layouts,
                                             int layout_id,
                                             int target_width,
                                             int target_height,
                                             FormFactor preferred_form_factor,
                                             bool require_form_factor) {
    const Layout* best = nullptr;
    float best_score = std::numeric_limits<float>::max();

    for (const Layout& layout : layouts) {
        if (layout.id != layout_id)
            continue;
        if (require_form_factor && layout.form_factor != preferred_form_factor)
            continue;

        float score = layout_score(layout, target_width, target_height);
        if (score < best_score) {
            best = &layout;
            best_score = score;
        }
    }

    return best;
}

} // namespace

void add_or_replace_object(Layout& layout, const Object& object) {
    for (Object& existing : layout.objects) {
        if (existing.id == object.id) {
            existing = object;
            return;
        }
    }

    layout.objects.push_back(object);
}

bool remove_object(Layout& layout, int object_id) {
    auto it = std::remove_if(layout.objects.begin(),
                             layout.objects.end(),
                             [object_id](const Object& object) {
                                 return object.id == object_id;
                             });
    if (it == layout.objects.end())
        return false;

    layout.objects.erase(it, layout.objects.end());
    return true;
}

bool remove_object(Layout& layout, std::string_view label) {
    auto it = std::remove_if(layout.objects.begin(),
                             layout.objects.end(),
                             [label](const Object& object) {
                                 return object.label == label;
                             });
    if (it == layout.objects.end())
        return false;

    layout.objects.erase(it, layout.objects.end());
    return true;
}

const Object* find_object(const Layout& layout, int object_id) {
    for (const Object& object : layout.objects) {
        if (object.id == object_id)
            return &object;
    }

    return nullptr;
}

const Object* find_object(const Layout& layout, std::string_view label) {
    for (const Object& object : layout.objects) {
        if (object.label == label)
            return &object;
    }

    return nullptr;
}

const Layout* find_best_layout(const std::vector<Layout>& layouts,
                               int layout_id,
                               int target_width,
                               int target_height,
                               FormFactor preferred_form_factor) {
    const Layout* best = find_best_matching_form_factor(layouts,
                                                        layout_id,
                                                        target_width,
                                                        target_height,
                                                        preferred_form_factor,
                                                        true);
    if (best)
        return best;

    return find_best_matching_form_factor(layouts,
                                          layout_id,
                                          target_width,
                                          target_height,
                                          preferred_form_factor,
                                          false);
}

} // namespace glayout
