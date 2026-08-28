#include "glayout/graph_types.hpp"

#include <algorithm>
#include <cmath>

namespace glayout {
namespace {

// Scores a variant while strongly preferring the requested form factor.
float variant_score(const GraphLayout& layout, int width, int height, float dpi,
                    FormFactor form_factor) {
    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    const float source_aspect = layout.height > 0
                                    ? static_cast<float>(layout.width) /
                                          static_cast<float>(layout.height)
                                    : 1.0f;
    const float resolution = std::hypot(static_cast<float>(layout.width - width),
                                        static_cast<float>(layout.height - height));
    const float form_penalty = layout.form_factor == form_factor ? 0.0f : 1'000'000.0f;
    const float dpi_penalty = std::fabs(layout.dpi_scale - dpi) * 500.0f;
    return form_penalty + std::fabs(source_aspect - aspect) * 10'000.0f + resolution + dpi_penalty;
}

} // namespace

// Maintains one graph variant for each authored identity and target tuple.
void GraphStore::clear() {
    layouts.clear();
}

void GraphStore::add_or_replace(const GraphLayout& layout) {
    auto existing = std::find_if(layouts.begin(), layouts.end(), [&](const GraphLayout& item) {
        return item.id == layout.id && item.width == layout.width && item.height == layout.height &&
               item.dpi_scale == layout.dpi_scale && item.form_factor == layout.form_factor;
    });
    if (existing == layouts.end()) {
        layouts.push_back(layout);
        return;
    }
    *existing = layout;
}

const GraphLayout* GraphStore::find_best(std::string_view id, int target_width, int target_height,
                                         float target_dpi, FormFactor form_factor) const {
    const GraphLayout* best = nullptr;
    float best_score = 0.0f;
    for (const GraphLayout& layout : layouts) {
        if (layout.id != id)
            continue;
        const float score = variant_score(layout, target_width, target_height, target_dpi, form_factor);
        if (!best || score < best_score) {
            best = &layout;
            best_score = score;
        }
    }
    return best;
}

// Exposes stable source spellings for persisted graph values.
std::string_view to_string(ContainerKind value) {
    switch (value) {
    case ContainerKind::Absolute: return "absolute";
    case ContainerKind::Row: return "row";
    case ContainerKind::Column: return "column";
    case ContainerKind::Grid: return "grid";
    case ContainerKind::Stack: return "stack";
    case ContainerKind::Overlay: return "overlay";
    }
    return "absolute";
}

std::string_view to_string(LengthKind value) {
    switch (value) {
    case LengthKind::Auto: return "auto";
    case LengthKind::Pixels: return "pixels";
    case LengthKind::Percent: return "percent";
    case LengthKind::Fill: return "fill";
    case LengthKind::Intrinsic: return "intrinsic";
    }
    return "auto";
}

std::string_view to_string(Align value) {
    switch (value) {
    case Align::Start: return "start";
    case Align::Center: return "center";
    case Align::End: return "end";
    case Align::Stretch: return "stretch";
    }
    return "stretch";
}

std::string_view to_string(Distribution value) {
    switch (value) {
    case Distribution::Start: return "start";
    case Distribution::Center: return "center";
    case Distribution::End: return "end";
    case Distribution::SpaceBetween: return "space_between";
    }
    return "start";
}

std::string_view to_string(Edge value) {
    switch (value) {
    case Edge::Left: return "left";
    case Edge::Top: return "top";
    case Edge::Right: return "right";
    case Edge::Bottom: return "bottom";
    case Edge::CenterX: return "center_x";
    case Edge::CenterY: return "center_y";
    }
    return "left";
}

// Converts permissive source spellings to deterministic defaults.
ContainerKind container_kind_from_string(std::string_view value) {
    if (value == "row") return ContainerKind::Row;
    if (value == "column") return ContainerKind::Column;
    if (value == "grid") return ContainerKind::Grid;
    if (value == "stack") return ContainerKind::Stack;
    if (value == "overlay") return ContainerKind::Overlay;
    return ContainerKind::Absolute;
}

LengthKind length_kind_from_string(std::string_view value) {
    if (value == "pixels" || value == "px") return LengthKind::Pixels;
    if (value == "percent" || value == "pct") return LengthKind::Percent;
    if (value == "fill") return LengthKind::Fill;
    if (value == "intrinsic") return LengthKind::Intrinsic;
    return LengthKind::Auto;
}

Align align_from_string(std::string_view value) {
    if (value == "start") return Align::Start;
    if (value == "center") return Align::Center;
    if (value == "end") return Align::End;
    return Align::Stretch;
}

Distribution distribution_from_string(std::string_view value) {
    if (value == "center") return Distribution::Center;
    if (value == "end") return Distribution::End;
    if (value == "space_between") return Distribution::SpaceBetween;
    return Distribution::Start;
}

Edge edge_from_string(std::string_view value) {
    if (value == "top") return Edge::Top;
    if (value == "right") return Edge::Right;
    if (value == "bottom") return Edge::Bottom;
    if (value == "center_x") return Edge::CenterX;
    if (value == "center_y") return Edge::CenterY;
    return Edge::Left;
}

} // namespace glayout
