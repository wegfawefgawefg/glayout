#pragma once

#include "glayout/layout.hpp"

#include <vector>

namespace glayout {

struct Viewport {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

struct EditorInput {
    float mouse_x = 0.0f;
    float mouse_y = 0.0f;
    bool left_down = false;
    bool ctrl = false;
    bool shift = false;
    bool key_save = false;
    bool key_undo = false;
    bool key_redo = false;
    bool key_delete = false;
    bool key_copy = false;
    bool key_paste = false;
};

enum class Handle {
    None,
    Center,
    Left,
    Right,
    Top,
    Bottom,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
};

struct HitResult {
    int object_index = -1;
    Handle handle = Handle::None;
};

struct EditorFrameResult {
    bool changed = false;
    bool save_requested = false;
    bool selection_changed = false;
};

struct EditorState {
    bool dirty = false;
    bool save_requested = false;
    bool snap_enabled = true;
    float grid_step = 0.05f;
    std::vector<int> selection;
    int primary = -1;

    bool mouse_was_down = false;
    bool dragging = false;
    bool drag_changed = false;
    Handle drag_handle = Handle::None;
    float drag_start_x = 0.0f;
    float drag_start_y = 0.0f;
    std::vector<Object> drag_start_objects;
    std::vector<int> drag_start_selection;
    std::vector<Object> clipboard;

    std::vector<Layout> undo_stack;
    std::vector<Layout> redo_stack;
};

bool editor_hit_test(const Layout& layout,
                     Viewport viewport,
                     float mouse_x,
                     float mouse_y,
                     const std::vector<int>& selection,
                     HitResult& out_hit);

EditorFrameResult editor_begin_frame(EditorState& editor,
                                     Layout& layout,
                                     const EditorInput& input,
                                     Viewport viewport);

void editor_clear_selection(EditorState& editor);
void editor_select_single(EditorState& editor, int object_index);
void editor_add_to_selection(EditorState& editor, int object_index);
void editor_remove_from_selection(EditorState& editor, int object_index);
bool editor_is_selected(const EditorState& editor, int object_index);

void editor_mark_saved(EditorState& editor);
void editor_commit_undo(EditorState& editor, const Layout& layout);
bool editor_undo(EditorState& editor, Layout& layout);
bool editor_redo(EditorState& editor, Layout& layout);

} // namespace glayout
