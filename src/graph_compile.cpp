#include "glayout/graph_compile.hpp"

#include <functional>
#include <unordered_set>

namespace glayout {
namespace {

// Records a source diagnostic without throwing away the rest of the graph.
void add_error(GraphCompileResult& result, std::string message) {
    result.diagnostics.push_back(
        Diagnostic{DiagnosticSeverity::Error, std::move(message), 1, 1});
}

// Flattens authoring nodes and records direct children in a separate dense table.
NodeIndex flatten_node(const GraphNode& source, NodeIndex parent, GraphCompileResult& result) {
    const NodeIndex index = static_cast<NodeIndex>(result.graph.nodes.size());
    if (source.id.empty())
        add_error(result, "graph node has an empty id");
    if (!source.id.empty() && result.graph.indices.contains(source.id))
        add_error(result, "duplicate graph node id '" + source.id + "'");

    CompiledNode node;
    node.id = source.id;
    node.label = source.label;
    node.measure_key = source.measure_key;
    node.parent = parent;
    node.container = source.container;
    node.size = source.size;
    node.padding = source.padding;
    node.absolute_rect = source.absolute_rect;
    node.gap = source.gap;
    node.columns = source.columns;
    node.align = source.align;
    node.distribution = source.distribution;
    node.clip = source.clip;
    node.visible = source.visible;
    node.mask = source.mask;
    result.graph.nodes.push_back(std::move(node));
    if (!source.id.empty())
        result.graph.indices.emplace(source.id, index);

    if (!source.children.empty()) {
        std::vector<NodeIndex> direct_children;
        direct_children.reserve(source.children.size());
        for (const GraphNode& child : source.children)
            direct_children.push_back(flatten_node(child, index, result));
        result.graph.nodes[index].first_child =
            static_cast<NodeIndex>(result.graph.children.size());
        result.graph.nodes[index].child_count = static_cast<std::uint32_t>(direct_children.size());
        result.graph.children.insert(result.graph.children.end(), direct_children.begin(),
                                     direct_children.end());
    }
    return index;
}

// Resolves authored anchors only after every target identity is known.
void flatten_anchors(const GraphNode& source, NodeIndex& cursor, GraphCompileResult& result) {
    const NodeIndex node_index = cursor++;
    CompiledNode& node = result.graph.nodes[node_index];
    node.first_anchor = static_cast<std::uint32_t>(result.graph.anchors.size());
    node.anchor_count = static_cast<std::uint32_t>(source.anchors.size());
    for (const AnchorRule& anchor : source.anchors) {
        const auto target = result.graph.indices.find(anchor.target);
        if (target == result.graph.indices.end()) {
            add_error(result, "node '" + source.id + "' anchors to missing node '" +
                                  anchor.target + "'");
            result.graph.anchors.push_back(
                CompiledAnchor{anchor.own_edge, invalid_node_index, anchor.target_edge,
                               anchor.offset});
            continue;
        }
        result.graph.anchors.push_back(
            CompiledAnchor{anchor.own_edge, target->second, anchor.target_edge, anchor.offset});
    }
    for (const GraphNode& child : source.children)
        flatten_anchors(child, cursor, result);
}

// Rejects anchor cycles before they can make layout order ambiguous.
bool anchor_cycle_from(NodeIndex index, const CompiledGraph& graph, std::vector<std::uint8_t>& marks) {
    if (marks[index] == 1)
        return true;
    if (marks[index] == 2)
        return false;
    marks[index] = 1;
    const CompiledNode& node = graph.nodes[index];
    for (std::uint32_t offset = 0; offset < node.anchor_count; ++offset) {
        const CompiledAnchor& anchor = graph.anchors[node.first_anchor + offset];
        if (anchor.target != invalid_node_index && anchor_cycle_from(anchor.target, graph, marks))
            return true;
    }
    marks[index] = 2;
    return false;
}

} // namespace

// Compiles readable authoring trees into dense runtime tables.
GraphCompileResult compile_graph(const GraphLayout& layout) {
    GraphCompileResult result;
    result.graph.id = layout.id;
    result.graph.label = layout.label;
    result.graph.source_width = layout.width;
    result.graph.source_height = layout.height;
    result.graph.source_dpi = layout.dpi_scale;
    result.graph.form_factor = layout.form_factor;
    if (layout.id.empty())
        add_error(result, "graph layout has an empty id");
    if (layout.root.id.empty())
        add_error(result, "graph layout root has an empty id");

    flatten_node(layout.root, invalid_node_index, result);
    NodeIndex cursor = 0;
    flatten_anchors(layout.root, cursor, result);

    std::vector<std::uint8_t> marks(result.graph.nodes.size(), 0);
    for (NodeIndex index = 0; index < result.graph.nodes.size(); ++index) {
        if (anchor_cycle_from(index, result.graph, marks)) {
            add_error(result, "graph contains an anchor cycle");
            break;
        }
    }

    result.ok = result.diagnostics.empty();
    return result;
}

} // namespace glayout
