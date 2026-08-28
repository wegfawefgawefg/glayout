#include "glayout/graph_canvas.hpp"

#include <algorithm>
#include <cmath>

namespace glayout {
namespace {

bool contains(Rect rect, float x, float y) {
    return x >= rect.x && y >= rect.y && x <= rect.x + rect.w && y <= rect.y + rect.h;
}

bool selected(const GraphCanvasState& state, std::string_view id) {
    return std::find(state.selection.begin(), state.selection.end(), id) != state.selection.end();
}

const GraphNode* find_parent(const GraphNode& parent, std::string_view id) {
    for (const GraphNode& child : parent.children) {
        if (child.id == id) return &parent;
        if (const GraphNode* found = find_parent(child, id)) return found;
    }
    return nullptr;
}

NodeIndex compiled_index(const CompiledGraph& compiled, std::string_view id) {
    const auto found = compiled.indices.find(std::string(id));
    return found == compiled.indices.end() ? invalid_node_index : found->second;
}

Rect resolved_rect(const CompiledGraph& compiled, const std::vector<ResolvedNode>& resolved,
                   std::string_view id) {
    const NodeIndex index = compiled_index(compiled, id);
    return index == invalid_node_index || index >= resolved.size() ? Rect{} : resolved[index].border;
}

Rect resolved_content(const CompiledGraph& compiled, const std::vector<ResolvedNode>& resolved,
                      std::string_view id) {
    const NodeIndex index = compiled_index(compiled, id);
    return index == invalid_node_index || index >= resolved.size() ? Rect{} : resolved[index].content;
}

CanvasHandle hit_rect(Rect rect, float x, float y, float size) {
    const float half = size * 0.5f;
    const auto point = [&](float px, float py) {
        return contains({px - half, py - half, size, size}, x, y);
    };
    if (point(rect.x, rect.y)) return CanvasHandle::TopLeft;
    if (point(rect.x + rect.w, rect.y)) return CanvasHandle::TopRight;
    if (point(rect.x, rect.y + rect.h)) return CanvasHandle::BottomLeft;
    if (point(rect.x + rect.w, rect.y + rect.h)) return CanvasHandle::BottomRight;
    if (contains({rect.x - half, rect.y + half, size, std::max(0.0f, rect.h - size)}, x, y))
        return CanvasHandle::Left;
    if (contains({rect.x + rect.w - half, rect.y + half, size,
                  std::max(0.0f, rect.h - size)}, x, y))
        return CanvasHandle::Right;
    if (contains({rect.x + half, rect.y - half, std::max(0.0f, rect.w - size), size}, x, y))
        return CanvasHandle::Top;
    if (contains({rect.x + half, rect.y + rect.h - half,
                  std::max(0.0f, rect.w - size), size}, x, y))
        return CanvasHandle::Bottom;
    return contains(rect, x, y) ? CanvasHandle::Center : CanvasHandle::None;
}

std::string hit_node(const CompiledGraph& compiled, const std::vector<ResolvedNode>& resolved,
                     float x, float y) {
    for (std::size_t reverse = compiled.nodes.size(); reverse > 1; --reverse) {
        const NodeIndex index = static_cast<NodeIndex>(reverse - 1);
        if (index < resolved.size() && resolved[index].visible &&
            contains(resolved[index].border, x, y))
            return compiled.nodes[index].id;
    }
    return {};
}

void start_drag(const GraphLayout& source, const CompiledGraph& compiled,
                const std::vector<ResolvedNode>& resolved, GraphCanvasState& state,
                float x, float y) {
    state.dragging = true;
    state.changed = false;
    state.start_x = x;
    state.start_y = y;
    state.drag_nodes.clear();
    state.guides.clear();
    for (const std::string& id : state.selection) {
        const GraphNode* node = find_graph_node(source, id);
        if (!node) continue;
        state.drag_nodes.push_back(
            {id, resolved_rect(compiled, resolved, id), node->absolute_rect, node->size,
             node->anchors});
    }
}

float snap(float value, float step, float threshold, bool enabled, bool& snapped) {
    if (!enabled || step <= 0.0f) return value;
    const float candidate = std::round(value / step) * step;
    if (std::fabs(candidate - value) > threshold) return value;
    snapped = true;
    return candidate;
}

void collect_sibling_edges(const GraphLayout& source, const CompiledGraph& compiled,
                           const std::vector<ResolvedNode>& resolved,
                           const GraphCanvasState& state, std::vector<float>& x_edges,
                           std::vector<float>& y_edges) {
    if (state.drag_nodes.empty()) return;
    const GraphNode* parent = find_parent(source.root, state.drag_nodes.front().id);
    if (!parent) return;
    for (const GraphNode& node : parent->children) {
        if (selected(state, node.id)) continue;
        const NodeIndex index = compiled_index(compiled, node.id);
        if (index == invalid_node_index || index >= resolved.size()) continue;
        const Rect rect = resolved[index].border;
        x_edges.insert(x_edges.end(), {rect.x, rect.x + rect.w * 0.5f, rect.x + rect.w});
        y_edges.insert(y_edges.end(), {rect.y, rect.y + rect.h * 0.5f, rect.y + rect.h});
    }
}

float snap_edges(float value, const std::vector<float>& edges, float threshold,
                 bool& snapped) {
    float best = value;
    float distance = threshold + 1.0f;
    for (float edge : edges) {
        const float candidate = std::fabs(edge - value);
        if (candidate < distance && candidate <= threshold) {
            best = edge;
            distance = candidate;
        }
    }
    snapped = best != value;
    return best;
}

void update_anchors(GraphNode& node, const CanvasDragNode& start, float dx, float dy) {
    node.anchors = start.anchors;
    for (AnchorRule& anchor : node.anchors) {
        if (anchor.own_edge == Edge::Left || anchor.own_edge == Edge::Right ||
            anchor.own_edge == Edge::CenterX)
            anchor.offset += dx;
        else
            anchor.offset += dy;
    }
}

void move_absolute(GraphNode& node, const CanvasDragNode& start, Rect parent,
                   float dx, float dy) {
    if (parent.w <= 0.0f || parent.h <= 0.0f) return;
    node.absolute_rect = start.absolute;
    node.absolute_rect.x += dx / parent.w;
    node.absolute_rect.y += dy / parent.h;
}

void resize_absolute(GraphNode& node, const CanvasDragNode& start, Rect parent,
                     CanvasHandle handle, float dx, float dy) {
    if (parent.w <= 0.0f || parent.h <= 0.0f) return;
    node.absolute_rect = start.absolute;
    const float nx = dx / parent.w;
    const float ny = dy / parent.h;
    if (handle == CanvasHandle::Left || handle == CanvasHandle::TopLeft ||
        handle == CanvasHandle::BottomLeft) {
        node.absolute_rect.x += nx;
        node.absolute_rect.w -= nx;
    }
    if (handle == CanvasHandle::Right || handle == CanvasHandle::TopRight ||
        handle == CanvasHandle::BottomRight)
        node.absolute_rect.w += nx;
    if (handle == CanvasHandle::Top || handle == CanvasHandle::TopLeft ||
        handle == CanvasHandle::TopRight) {
        node.absolute_rect.y += ny;
        node.absolute_rect.h -= ny;
    }
    if (handle == CanvasHandle::Bottom || handle == CanvasHandle::BottomLeft ||
        handle == CanvasHandle::BottomRight)
        node.absolute_rect.h += ny;
    node.absolute_rect.w = std::max(0.005f, node.absolute_rect.w);
    node.absolute_rect.h = std::max(0.005f, node.absolute_rect.h);
}

void resize_flow(GraphNode& node, const CanvasDragNode& start, CanvasHandle handle,
                 float dx, float dy) {
    node.size = start.size;
    float width = start.resolved.w;
    float height = start.resolved.h;
    if (handle == CanvasHandle::Left || handle == CanvasHandle::TopLeft ||
        handle == CanvasHandle::BottomLeft)
        width -= dx;
    if (handle == CanvasHandle::Right || handle == CanvasHandle::TopRight ||
        handle == CanvasHandle::BottomRight)
        width += dx;
    if (handle == CanvasHandle::Top || handle == CanvasHandle::TopLeft ||
        handle == CanvasHandle::TopRight)
        height -= dy;
    if (handle == CanvasHandle::Bottom || handle == CanvasHandle::BottomLeft ||
        handle == CanvasHandle::BottomRight)
        height += dy;
    node.size.width = {LengthKind::Pixels, std::max(1.0f, width)};
    node.size.height = {LengthKind::Pixels, std::max(1.0f, height)};
}

bool reorder_flow(GraphLayout& source, const CompiledGraph& compiled,
                  const std::vector<ResolvedNode>& resolved, std::string_view id,
                  float x, float y) {
    GraphNode* parent = find_graph_parent(source, id);
    if (!parent || (parent->container != ContainerKind::Row &&
                    parent->container != ContainerKind::Column &&
                    parent->container != ContainerKind::Grid))
        return false;
    const auto current = std::find_if(parent->children.begin(), parent->children.end(),
                                      [&](const GraphNode& child) { return child.id == id; });
    if (current == parent->children.end()) return false;
    std::size_t destination = parent->children.size();
    for (std::size_t index = 0; index < parent->children.size(); ++index) {
        const Rect rect = resolved_rect(compiled, resolved, parent->children[index].id);
        const float center_x = rect.x + rect.w * 0.5f;
        const float center_y = rect.y + rect.h * 0.5f;
        const bool before = parent->container == ContainerKind::Column
                                ? y < center_y
                            : parent->container == ContainerKind::Row
                                ? x < center_x
                                : y < center_y - rect.h * 0.5f ||
                                      (std::fabs(y - center_y) <= rect.h * 0.5f &&
                                       x < center_x);
        if (before) {
            destination = index;
            break;
        }
    }
    const std::size_t origin = static_cast<std::size_t>(current - parent->children.begin());
    if (destination > origin) --destination;
    if (origin == destination) return false;
    GraphNode moved = std::move(parent->children[origin]);
    parent->children.erase(parent->children.begin() + static_cast<std::ptrdiff_t>(origin));
    parent->children.insert(parent->children.begin() + static_cast<std::ptrdiff_t>(destination),
                            std::move(moved));
    return true;
}

} // namespace

CanvasHandle graph_canvas_hit_handle(Rect rect, float x, float y, float handle_size) {
    return hit_rect(rect, x, y, handle_size);
}

// Begins selection or manipulation against resolved native geometry.
CanvasResult graph_canvas_press(GraphLayout& source, const CompiledGraph& compiled,
                                const std::vector<ResolvedNode>& resolved,
                                GraphCanvasState& state, float x, float y,
                                bool additive_selection) {
    CanvasResult result;
    if (!state.primary.empty() && selected(state, state.primary)) {
        const CanvasHandle handle = hit_rect(resolved_rect(compiled, resolved, state.primary),
                                             x, y, 9.0f);
        if (handle != CanvasHandle::None && handle != CanvasHandle::Center) {
            state.handle = handle;
            start_drag(source, compiled, resolved, state, x, y);
            result.transaction_started = true;
            return result;
        }
    }
    const std::string hit = hit_node(compiled, resolved, x, y);
    if (hit.empty()) {
        if (!additive_selection && !state.selection.empty()) {
            state.selection.clear();
            state.primary.clear();
            result.selection_changed = true;
        }
        return result;
    }
    if (additive_selection) {
        const auto found = std::find(state.selection.begin(), state.selection.end(), hit);
        if (found == state.selection.end()) state.selection.push_back(hit);
        else state.selection.erase(found);
    } else {
        state.selection = {hit};
    }
    state.primary = state.selection.empty() ? std::string{} : hit;
    state.handle = CanvasHandle::Center;
    result.selection_changed = true;
    if (!state.selection.empty()) {
        start_drag(source, compiled, resolved, state, x, y);
        result.transaction_started = true;
    }
    return result;
}

// Applies snapping-aware absolute movement or constraint-aware flow resizing.
CanvasResult graph_canvas_drag(GraphLayout& source, const CompiledGraph& compiled,
                               const std::vector<ResolvedNode>& resolved,
                               GraphCanvasState& state, float x, float y) {
    CanvasResult result;
    if (!state.dragging) return result;
    float dx = x - state.start_x;
    float dy = y - state.start_y;
    state.guides.clear();
    bool snapped_x = false;
    bool snapped_y = false;
    dx = snap(dx, state.grid_step, state.snap_distance, state.snap_grid, snapped_x);
    dy = snap(dy, state.grid_step, state.snap_distance, state.snap_grid, snapped_y);
    if (state.snap_siblings && !state.drag_nodes.empty()) {
        std::vector<float> x_edges;
        std::vector<float> y_edges;
        collect_sibling_edges(source, compiled, resolved, state, x_edges, y_edges);
        const Rect start = state.drag_nodes.front().resolved;
        bool edge_x = false;
        bool edge_y = false;
        const float next_x = snap_edges(start.x + dx, x_edges, state.snap_distance, edge_x);
        const float next_y = snap_edges(start.y + dy, y_edges, state.snap_distance, edge_y);
        if (edge_x) dx = next_x - start.x;
        if (edge_y) dy = next_y - start.y;
        snapped_x |= edge_x;
        snapped_y |= edge_y;
    }
    if (snapped_x && !state.drag_nodes.empty())
        state.guides.push_back({true, state.drag_nodes.front().resolved.x + dx});
    if (snapped_y && !state.drag_nodes.empty())
        state.guides.push_back({false, state.drag_nodes.front().resolved.y + dy});

    bool changed = false;
    for (const CanvasDragNode& start : state.drag_nodes) {
        GraphNode* node = find_graph_node(source, start.id);
        GraphNode* parent = find_graph_parent(source, start.id);
        if (!node || !parent) continue;
        const Rect parent_rect = resolved_content(compiled, resolved, parent->id);
        if (!start.anchors.empty() && state.handle == CanvasHandle::Center) {
            update_anchors(*node, start, dx, dy);
            changed = true;
        } else if (parent->container == ContainerKind::Absolute) {
            if (state.handle == CanvasHandle::Center)
                move_absolute(*node, start, parent_rect, dx, dy);
            else
                resize_absolute(*node, start, parent_rect, state.handle, dx, dy);
            changed = true;
        } else if (state.handle != CanvasHandle::Center) {
            resize_flow(*node, start, state.handle, dx, dy);
            changed = true;
        }
    }
    state.changed |= changed;
    result.changed = changed;
    return result;
}

// Commits flow reordering and closes one native-canvas transaction.
CanvasResult graph_canvas_release(GraphLayout& source, const CompiledGraph& compiled,
                                  const std::vector<ResolvedNode>& resolved,
                                  GraphCanvasState& state, float x, float y) {
    CanvasResult result;
    if (!state.dragging) return result;
    if (state.handle == CanvasHandle::Center && state.selection.size() == 1)
        state.changed |= reorder_flow(source, compiled, resolved, state.primary, x, y);
    state.dragging = false;
    state.drag_nodes.clear();
    state.guides.clear();
    result.changed = state.changed;
    result.transaction_finished = true;
    state.changed = false;
    return result;
}

bool graph_canvas_nudge(GraphLayout& source, const CompiledGraph& compiled,
                        const std::vector<ResolvedNode>& resolved,
                        GraphCanvasState& state, float dx, float dy) {
    if (state.selection.empty()) return false;
    state.handle = CanvasHandle::Center;
    start_drag(source, compiled, resolved, state, 0.0f, 0.0f);
    const bool changed = graph_canvas_drag(source, compiled, resolved, state, dx, dy).changed;
    state.dragging = false;
    state.changed = false;
    state.drag_nodes.clear();
    state.guides.clear();
    return changed;
}

void graph_canvas_clear(GraphCanvasState& state) {
    state.selection.clear();
    state.primary.clear();
    state.dragging = false;
    state.drag_nodes.clear();
    state.guides.clear();
}

bool graph_canvas_selection_bounds(const CompiledGraph& compiled,
                                   const std::vector<ResolvedNode>& resolved,
                                   const GraphCanvasState& state, Rect& output) {
    bool found = false;
    for (const std::string& id : state.selection) {
        const Rect rect = resolved_rect(compiled, resolved, id);
        if (rect.w <= 0.0f || rect.h <= 0.0f) continue;
        if (!found) output = rect;
        else {
            const float left = std::min(output.x, rect.x);
            const float top = std::min(output.y, rect.y);
            const float right = std::max(output.x + output.w, rect.x + rect.w);
            const float bottom = std::max(output.y + output.h, rect.y + rect.h);
            output = {left, top, right - left, bottom - top};
        }
        found = true;
    }
    return found;
}

} // namespace glayout
