#pragma once

#include "glayout/graph_types.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace glayout {

struct CompiledAnchor {
    Edge own_edge = Edge::Left;
    NodeIndex target = invalid_node_index;
    Edge target_edge = Edge::Left;
    float offset = 0.0f;
};

struct CompiledNode {
    std::string id;
    std::string label;
    std::string measure_key;
    NodeIndex parent = invalid_node_index;
    NodeIndex first_child = invalid_node_index;
    std::uint32_t child_count = 0;
    std::uint32_t first_anchor = 0;
    std::uint32_t anchor_count = 0;
    ContainerKind container = ContainerKind::Absolute;
    SizeRules size;
    Insets padding;
    Rect absolute_rect;
    float gap = 0.0f;
    int columns = 1;
    Align align = Align::Stretch;
    Distribution distribution = Distribution::Start;
    bool clip = false;
    bool visible = true;
    std::string mask;
};

struct CompiledGraph {
    std::string id;
    std::string label;
    int source_width = 0;
    int source_height = 0;
    float source_dpi = 1.0f;
    FormFactor form_factor = FormFactor::Desktop;
    std::vector<CompiledNode> nodes;
    std::vector<NodeIndex> children;
    std::vector<CompiledAnchor> anchors;
    std::unordered_map<std::string, NodeIndex> indices;
};

struct GraphCompileResult {
    bool ok = false;
    CompiledGraph graph;
    std::vector<Diagnostic> diagnostics;
};

GraphCompileResult compile_graph(const GraphLayout& layout);

} // namespace glayout
