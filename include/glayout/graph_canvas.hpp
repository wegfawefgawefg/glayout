#pragma once

#include "glayout/graph_editor.hpp"
#include "glayout/graph_layout.hpp"

#include <string>
#include <vector>

namespace glayout {

// Names native-canvas manipulation handles for hierarchical geometry.
enum class CanvasHandle {
    None,
    Center,
    Left,
    Right,
    Top,
    Bottom,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};

// Exposes snapping guides for renderer-specific native overlays.
struct CanvasGuide {
    bool vertical = true;
    float position = 0.0f;
};

// Stores one node at the beginning of a direct-manipulation transaction.
struct CanvasDragNode {
    std::string id;
    Rect resolved;
    Rect absolute;
    SizeRules size;
    std::vector<AnchorRule> anchors;
};

// Owns renderer-free native-canvas selection and drag state.
struct GraphCanvasState {
    std::vector<std::string> selection;
    std::string primary;
    bool dragging = false;
    bool changed = false;
    CanvasHandle handle = CanvasHandle::None;
    float start_x = 0.0f;
    float start_y = 0.0f;
    float grid_step = 16.0f;
    float snap_distance = 7.0f;
    bool snap_grid = true;
    bool snap_siblings = true;
    std::vector<CanvasDragNode> drag_nodes;
    std::vector<CanvasGuide> guides;
};

// Reports transaction boundaries so a composed editor can snapshot full views.
struct CanvasResult {
    bool selection_changed = false;
    bool transaction_started = false;
    bool changed = false;
    bool transaction_finished = false;
};

CanvasResult graph_canvas_press(GraphLayout& source, const CompiledGraph& compiled,
                                const std::vector<ResolvedNode>& resolved,
                                GraphCanvasState& state, float x, float y,
                                bool additive_selection);
CanvasResult graph_canvas_drag(GraphLayout& source, const CompiledGraph& compiled,
                               const std::vector<ResolvedNode>& resolved,
                               GraphCanvasState& state, float x, float y);
CanvasResult graph_canvas_release(GraphLayout& source, const CompiledGraph& compiled,
                                  const std::vector<ResolvedNode>& resolved,
                                  GraphCanvasState& state, float x, float y);
bool graph_canvas_nudge(GraphLayout& source, const CompiledGraph& compiled,
                        const std::vector<ResolvedNode>& resolved,
                        GraphCanvasState& state, float dx, float dy);
void graph_canvas_clear(GraphCanvasState& state);

CanvasHandle graph_canvas_hit_handle(Rect rect, float x, float y, float handle_size = 9.0f);
bool graph_canvas_selection_bounds(const CompiledGraph& compiled,
                                   const std::vector<ResolvedNode>& resolved,
                                   const GraphCanvasState& state, Rect& output);

} // namespace glayout
