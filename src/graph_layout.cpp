#include "glayout/graph_layout.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace glayout {
namespace {

float clamp_length(float value, float minimum, float maximum) {
    return std::clamp(value, minimum, maximum);
}

Rect inset_rect(Rect rect, Insets insets) {
    const float width = std::max(0.0f, rect.w - insets.left - insets.right);
    const float height = std::max(0.0f, rect.h - insets.top - insets.bottom);
    return Rect{rect.x + insets.left, rect.y + insets.top, width, height};
}

float edge_value(Rect rect, Edge edge) {
    switch (edge) {
    case Edge::Left: return rect.x;
    case Edge::Top: return rect.y;
    case Edge::Right: return rect.x + rect.w;
    case Edge::Bottom: return rect.y + rect.h;
    case Edge::CenterX: return rect.x + rect.w * 0.5f;
    case Edge::CenterY: return rect.y + rect.h * 0.5f;
    }
    return 0.0f;
}

void place_edge(Rect& rect, Edge edge, float value) {
    switch (edge) {
    case Edge::Left: rect.x = value; break;
    case Edge::Top: rect.y = value; break;
    case Edge::Right: rect.x = value - rect.w; break;
    case Edge::Bottom: rect.y = value - rect.h; break;
    case Edge::CenterX: rect.x = value - rect.w * 0.5f; break;
    case Edge::CenterY: rect.y = value - rect.h * 0.5f; break;
    }
}

Size intrinsic_size(const CompiledNode& node, const ResolveInput& input, Rect available) {
    if (!input.measure || node.measure_key.empty())
        return {};
    return input.measure(MeasureRequest{node.measure_key, available.w, available.h},
                         input.measure_user_data);
}

float resolve_length(Length length, float available, float intrinsic, float fallback) {
    switch (length.kind) {
    case LengthKind::Pixels: return length.value;
    case LengthKind::Percent: return available * length.value;
    case LengthKind::Intrinsic: return intrinsic;
    case LengthKind::Fill: return fallback;
    case LengthKind::Auto: return intrinsic > 0.0f ? intrinsic : fallback;
    }
    return fallback;
}

Size resolve_size(const CompiledNode& node, const ResolveInput& input, Rect available,
                  float fallback_width, float fallback_height) {
    const Size measured = intrinsic_size(node, input, available);
    float width = resolve_length(node.size.width, available.w, measured.width, fallback_width);
    float height = resolve_length(node.size.height, available.h, measured.height, fallback_height);
    width = clamp_length(width, node.size.min_width, node.size.max_width);
    height = clamp_length(height, node.size.min_height, node.size.max_height);
    if (node.size.aspect_ratio > 0.0f) {
        if (node.size.width.kind != LengthKind::Auto)
            height = width / node.size.aspect_ratio;
        else
            width = height * node.size.aspect_ratio;
    }
    return Size{width, height};
}

class Resolver {
  public:
    Resolver(const CompiledGraph& graph, const ResolveInput& input,
             std::vector<ResolvedNode>& output)
        : graph_(graph), input_(input), output_(output) {}

    void run() {
        if (graph_.nodes.empty())
            return;
        Rect root = inset_rect(input_.viewport, input_.safe_area);
        place_node(0, root, input_.viewport);
        apply_anchors();
        refresh_contents_and_clips(0, input_.viewport);
    }

  private:
    void place_node(NodeIndex index, Rect border, Rect inherited_clip) {
        const CompiledNode& node = graph_.nodes[index];
        ResolvedNode& resolved = output_[index];
        resolved.border = border;
        resolved.content = inset_rect(border, node.padding);
        resolved.content_extent = Size{resolved.content.w, resolved.content.h};
        resolved.clip = node.clip ? intersection(inherited_clip, border) : inherited_clip;
        resolved.visible = node.visible && resolved.clip.w > 0.0f && resolved.clip.h > 0.0f;
        if (!resolved.visible)
            return;
        layout_children(index, resolved.content, resolved.clip);
    }

    void layout_children(NodeIndex parent_index, Rect content, Rect clip) {
        const CompiledNode& parent = graph_.nodes[parent_index];
        if (parent.child_count == 0)
            return;
        if (parent.container == ContainerKind::Row || parent.container == ContainerKind::Column) {
            layout_linear(parent, content, clip, parent.container == ContainerKind::Row);
            return;
        }
        if (parent.container == ContainerKind::Grid) {
            layout_grid(parent, content, clip);
            return;
        }
        for (std::uint32_t offset = 0; offset < parent.child_count; ++offset) {
            const NodeIndex child_index = graph_.children[parent.first_child + offset];
            const CompiledNode& child = graph_.nodes[child_index];
            Rect border = content;
            if (parent.container == ContainerKind::Absolute) {
                border = map_rect(content, child.absolute_rect);
                const Size size = resolve_size(child, input_, content, border.w, border.h);
                border.w = size.width;
                border.h = size.height;
            } else {
                const Size size = resolve_size(child, input_, content, content.w, content.h);
                border = align_in(content, size, child.align);
            }
            place_node(child_index, border, clip);
        }
    }

    Rect align_in(Rect area, Size size, Align align) const {
        if (align == Align::Stretch)
            return area;
        float x = area.x;
        float y = area.y;
        if (align == Align::Center) {
            x += (area.w - size.width) * 0.5f;
            y += (area.h - size.height) * 0.5f;
        } else if (align == Align::End) {
            x += area.w - size.width;
            y += area.h - size.height;
        }
        return Rect{x, y, size.width, size.height};
    }

    void layout_linear(const CompiledNode& parent, Rect content, Rect clip, bool horizontal) {
        const float main_available = horizontal ? content.w : content.h;
        const float cross_available = horizontal ? content.h : content.w;
        const float total_gap = parent.gap * static_cast<float>(parent.child_count - 1);
        float fixed = 0.0f;
        std::uint32_t fills = 0;
        std::vector<Size> sizes(parent.child_count);
        for (std::uint32_t offset = 0; offset < parent.child_count; ++offset) {
            const NodeIndex child_index = graph_.children[parent.first_child + offset];
            const CompiledNode& child = graph_.nodes[child_index];
            const Length main_rule = horizontal ? child.size.width : child.size.height;
            if (main_rule.kind == LengthKind::Fill) {
                ++fills;
                continue;
            }
            sizes[offset] = resolve_size(child, input_, content, 0.0f, cross_available);
            fixed += horizontal ? sizes[offset].width : sizes[offset].height;
        }
        const float fill_size = fills > 0
                                    ? std::max(0.0f, main_available - fixed - total_gap) /
                                          static_cast<float>(fills)
                                    : 0.0f;
        float used = fixed + total_gap + fill_size * static_cast<float>(fills);
        float cursor = horizontal ? content.x : content.y;
        float gap = parent.gap;
        if (parent.distribution == Distribution::Center)
            cursor += (main_available - used) * 0.5f;
        else if (parent.distribution == Distribution::End)
            cursor += main_available - used;
        else if (parent.distribution == Distribution::SpaceBetween && parent.child_count > 1)
            gap += std::max(0.0f, main_available - used) /
                   static_cast<float>(parent.child_count - 1);

        for (std::uint32_t offset = 0; offset < parent.child_count; ++offset) {
            const NodeIndex child_index = graph_.children[parent.first_child + offset];
            const CompiledNode& child = graph_.nodes[child_index];
            if ((horizontal ? child.size.width.kind : child.size.height.kind) == LengthKind::Fill)
                sizes[offset] = resolve_size(child, input_, content,
                                             horizontal ? fill_size : cross_available,
                                             horizontal ? cross_available : fill_size);
            Size size = sizes[offset];
            float cross = horizontal ? size.height : size.width;
            float cross_pos = horizontal ? content.y : content.x;
            if (child.align == Align::Stretch)
                cross = cross_available;
            else if (child.align == Align::Center)
                cross_pos += (cross_available - cross) * 0.5f;
            else if (child.align == Align::End)
                cross_pos += cross_available - cross;
            const float main = horizontal ? size.width : size.height;
            const Rect border = horizontal ? Rect{cursor, cross_pos, main, cross}
                                           : Rect{cross_pos, cursor, cross, main};
            place_node(child_index, border, clip);
            cursor += main + gap;
        }
    }

    void layout_grid(const CompiledNode& parent, Rect content, Rect clip) {
        const int columns = std::max(1, parent.columns);
        const int count = static_cast<int>(parent.child_count);
        const int rows = std::max(1, (count + columns - 1) / columns);
        const float cell_width = std::max(0.0f, content.w - parent.gap * static_cast<float>(columns - 1)) /
                                 static_cast<float>(columns);
        const float cell_height = std::max(0.0f, content.h - parent.gap * static_cast<float>(rows - 1)) /
                                  static_cast<float>(rows);
        for (int item = 0; item < count; ++item) {
            const NodeIndex child_index = graph_.children[parent.first_child + item];
            Rect cell{content.x + static_cast<float>(item % columns) * (cell_width + parent.gap),
                      content.y + static_cast<float>(item / columns) * (cell_height + parent.gap),
                      cell_width, cell_height};
            const CompiledNode& child = graph_.nodes[child_index];
            const Size size = resolve_size(child, input_, cell, cell.w, cell.h);
            place_node(child_index, align_in(cell, size, child.align), clip);
        }
    }

    void apply_anchors() {
        for (std::size_t pass = 0; pass < graph_.nodes.size(); ++pass) {
            bool changed = false;
            for (NodeIndex index = 0; index < graph_.nodes.size(); ++index) {
                const CompiledNode& node = graph_.nodes[index];
                for (std::uint32_t offset = 0; offset < node.anchor_count; ++offset) {
                    const CompiledAnchor& anchor = graph_.anchors[node.first_anchor + offset];
                    if (anchor.target == invalid_node_index)
                        continue;
                    Rect before = output_[index].border;
                    place_edge(output_[index].border, anchor.own_edge,
                               edge_value(output_[anchor.target].border, anchor.target_edge) +
                                   anchor.offset);
                    const float dx = output_[index].border.x - before.x;
                    const float dy = output_[index].border.y - before.y;
                    if (dx != 0.0f || dy != 0.0f) {
                        shift_children(index, dx, dy);
                        changed = true;
                    }
                }
            }
            if (!changed)
                break;
        }
    }

    void shift_children(NodeIndex index, float dx, float dy) {
        const CompiledNode& node = graph_.nodes[index];
        for (std::uint32_t offset = 0; offset < node.child_count; ++offset) {
            const NodeIndex child = graph_.children[node.first_child + offset];
            output_[child].border.x += dx;
            output_[child].border.y += dy;
            shift_children(child, dx, dy);
        }
    }

    void refresh_contents_and_clips(NodeIndex index, Rect inherited_clip) {
        const CompiledNode& node = graph_.nodes[index];
        ResolvedNode& resolved = output_[index];
        resolved.content = inset_rect(resolved.border, node.padding);
        resolved.clip = node.clip ? intersection(inherited_clip, resolved.border) : inherited_clip;
        resolved.visible = node.visible && resolved.clip.w > 0.0f && resolved.clip.h > 0.0f;
        resolved.content_extent = Size{resolved.content.w, resolved.content.h};
        for (std::uint32_t offset = 0; offset < node.child_count; ++offset) {
            const NodeIndex child = graph_.children[node.first_child + offset];
            refresh_contents_and_clips(child, resolved.clip);
            const Rect child_rect = output_[child].border;
            resolved.content_extent.width =
                std::max(resolved.content_extent.width, child_rect.x + child_rect.w - resolved.content.x);
            resolved.content_extent.height =
                std::max(resolved.content_extent.height, child_rect.y + child_rect.h - resolved.content.y);
        }
    }

    const CompiledGraph& graph_;
    const ResolveInput& input_;
    std::vector<ResolvedNode>& output_;
};

bool same_input(const ResolveInput& left, const ResolveInput& right) {
    return left.viewport.x == right.viewport.x && left.viewport.y == right.viewport.y &&
           left.viewport.w == right.viewport.w && left.viewport.h == right.viewport.h &&
           left.safe_area.left == right.safe_area.left && left.safe_area.top == right.safe_area.top &&
           left.safe_area.right == right.safe_area.right &&
           left.safe_area.bottom == right.safe_area.bottom && left.measure == right.measure &&
           left.measure_user_data == right.measure_user_data;
}

} // namespace

// Owns the compiled graph and reuses clean geometry across frames.
GraphRuntime::GraphRuntime(CompiledGraph graph) {
    reset(std::move(graph));
}

void GraphRuntime::reset(CompiledGraph graph) {
    graph_ = std::move(graph);
    resolved_.assign(graph_.nodes.size(), {});
    dirty_ = true;
    has_previous_input_ = false;
}

void GraphRuntime::invalidate() {
    dirty_ = true;
}

bool GraphRuntime::resolve(const ResolveInput& input) {
    if (!dirty_ && has_previous_input_ && same_input(input, previous_input_)) {
        ++stats_.clean_reuses;
        return false;
    }
    resolved_.assign(graph_.nodes.size(), {});
    Resolver(graph_, input, resolved_).run();
    previous_input_ = input;
    has_previous_input_ = true;
    dirty_ = false;
    ++stats_.generation;
    ++stats_.layout_passes;
    stats_.resolved_nodes = static_cast<std::uint32_t>(resolved_.size());
    return true;
}

const CompiledGraph& GraphRuntime::graph() const { return graph_; }
const std::vector<ResolvedNode>& GraphRuntime::nodes() const { return resolved_; }

const ResolvedNode* GraphRuntime::find(std::string_view id) const {
    const auto found = graph_.indices.find(std::string(id));
    return found == graph_.indices.end() ? nullptr : &resolved_[found->second];
}

const ResolveStats& GraphRuntime::stats() const { return stats_; }

} // namespace glayout
