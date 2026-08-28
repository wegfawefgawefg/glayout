#pragma once

#include "glayout/layout.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace glayout {

using NodeIndex = std::uint32_t;
constexpr NodeIndex invalid_node_index = std::numeric_limits<NodeIndex>::max();

enum class ContainerKind {
    Absolute,
    Row,
    Column,
    Grid,
    Stack,
    Overlay,
};

enum class LengthKind {
    Auto,
    Pixels,
    Percent,
    Fill,
    Intrinsic,
};

enum class Align {
    Start,
    Center,
    End,
    Stretch,
};

enum class Distribution {
    Start,
    Center,
    End,
    SpaceBetween,
};

enum class Edge {
    Left,
    Top,
    Right,
    Bottom,
    CenterX,
    CenterY,
};

struct Size {
    float width = 0.0f;
    float height = 0.0f;
};

struct Insets {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
};

struct Length {
    LengthKind kind = LengthKind::Auto;
    float value = 0.0f;
};

struct SizeRules {
    Length width;
    Length height;
    float min_width = 0.0f;
    float min_height = 0.0f;
    float max_width = std::numeric_limits<float>::max();
    float max_height = std::numeric_limits<float>::max();
    float aspect_ratio = 0.0f;
};

struct AnchorRule {
    Edge own_edge = Edge::Left;
    std::string target;
    Edge target_edge = Edge::Left;
    float offset = 0.0f;
};

struct GraphNode {
    std::string id;
    std::string label;
    std::string measure_key;
    ContainerKind container = ContainerKind::Absolute;
    SizeRules size;
    Insets padding;
    Rect absolute_rect{0.0f, 0.0f, 1.0f, 1.0f};
    float gap = 0.0f;
    int columns = 1;
    Align align = Align::Stretch;
    Distribution distribution = Distribution::Start;
    bool clip = false;
    bool visible = true;
    std::string mask;
    std::vector<AnchorRule> anchors;
    std::vector<GraphNode> children;
};

struct GraphLayout {
    std::string id;
    std::string label;
    int width = 0;
    int height = 0;
    float dpi_scale = 1.0f;
    FormFactor form_factor = FormFactor::Desktop;
    GraphNode root;
};

struct GraphStore {
    std::vector<GraphLayout> layouts;

    void clear();
    void add_or_replace(const GraphLayout& layout);
    const GraphLayout* find_best(std::string_view id, int target_width, int target_height,
                                 float target_dpi, FormFactor form_factor) const;
};

std::string_view to_string(ContainerKind value);
std::string_view to_string(LengthKind value);
std::string_view to_string(Align value);
std::string_view to_string(Distribution value);
std::string_view to_string(Edge value);

ContainerKind container_kind_from_string(std::string_view value);
LengthKind length_kind_from_string(std::string_view value);
Align align_from_string(std::string_view value);
Distribution distribution_from_string(std::string_view value);
Edge edge_from_string(std::string_view value);

} // namespace glayout
