#include "glayout/editor.hpp"

#include <algorithm>
#include <cmath>

namespace glayout {
namespace {

constexpr float kMinSize = 0.01f;
constexpr float kHandlePixels = 8.0f;
constexpr float kPasteNudge = 0.02f;

bool contains(Rect rect, float x, float y) {
    return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
}

float snap(float value, float step) {
    if (step <= 0.0f)
        return value;
    return std::round(value / step) * step;
}

float maybe_snap(const EditorState& editor, float value) {
    if (!editor.snap_enabled)
        return value;
    return snap(value, editor.grid_step);
}

float clamp_unit_position(float value, float size) {
    float max_position = std::max(0.0f, 1.0f - size);
    return std::clamp(value, 0.0f, max_position);
}

Rect object_to_screen(const Object& object, Viewport viewport) {
    return map_rect(Rect{viewport.x, viewport.y, viewport.w, viewport.h}, object.rect);
}

Rect handle_rect(float cx, float cy, float size) {
    float half = size * 0.5f;
    return Rect{cx - half, cy - half, size, size};
}

Handle hit_handle(Rect rect, float mouse_x, float mouse_y) {
    if (contains(handle_rect(rect.x, rect.y, kHandlePixels), mouse_x, mouse_y))
        return Handle::TopLeft;
    if (contains(handle_rect(rect.x + rect.w, rect.y, kHandlePixels), mouse_x, mouse_y))
        return Handle::TopRight;
    if (contains(handle_rect(rect.x, rect.y + rect.h, kHandlePixels), mouse_x, mouse_y))
        return Handle::BottomLeft;
    if (contains(handle_rect(rect.x + rect.w, rect.y + rect.h, kHandlePixels), mouse_x, mouse_y))
        return Handle::BottomRight;
    if (contains(Rect{rect.x - kHandlePixels * 0.5f, rect.y, kHandlePixels, rect.h},
                 mouse_x,
                 mouse_y)) {
        return Handle::Left;
    }
    if (contains(Rect{rect.x + rect.w - kHandlePixels * 0.5f, rect.y, kHandlePixels, rect.h},
                 mouse_x,
                 mouse_y)) {
        return Handle::Right;
    }
    if (contains(Rect{rect.x, rect.y - kHandlePixels * 0.5f, rect.w, kHandlePixels},
                 mouse_x,
                 mouse_y)) {
        return Handle::Top;
    }
    if (contains(Rect{rect.x, rect.y + rect.h - kHandlePixels * 0.5f, rect.w, kHandlePixels},
                 mouse_x,
                 mouse_y)) {
        return Handle::Bottom;
    }
    if (contains(rect, mouse_x, mouse_y))
        return Handle::Center;
    return Handle::None;
}

void sync_primary(EditorState& editor) {
    if (editor.selection.empty()) {
        editor.primary = -1;
        return;
    }
    if (!editor_is_selected(editor, editor.primary))
        editor.primary = editor.selection.back();
}

bool valid_object_index(const Layout& layout, int index) {
    return index >= 0 && index < static_cast<int>(layout.objects.size());
}

void prune_selection(EditorState& editor, const Layout& layout) {
    auto it = std::remove_if(editor.selection.begin(),
                             editor.selection.end(),
                             [&](int index) {
                                 return !valid_object_index(layout, index);
                             });
    editor.selection.erase(it, editor.selection.end());
    sync_primary(editor);
}

void begin_drag(EditorState& editor,
                const Layout& layout,
                const HitResult& hit,
                float local_x,
                float local_y) {
    editor.dragging = hit.handle != Handle::None;
    editor.drag_changed = false;
    editor.drag_handle = hit.handle;
    editor.drag_start_x = local_x;
    editor.drag_start_y = local_y;
    editor.drag_start_selection = editor.selection;
    editor.drag_start_objects.clear();
    editor.drag_start_objects.reserve(editor.selection.size());

    for (int index : editor.selection) {
        if (valid_object_index(layout, index))
            editor.drag_start_objects.push_back(layout.objects[static_cast<std::size_t>(index)]);
    }
}

void translate_selection(EditorState& editor, Layout& layout, float dx, float dy) {
    for (std::size_t i = 0; i < editor.drag_start_selection.size(); ++i) {
        int index = editor.drag_start_selection[i];
        if (!valid_object_index(layout, index) || i >= editor.drag_start_objects.size())
            continue;

        const Object& start = editor.drag_start_objects[i];
        Object& object = layout.objects[static_cast<std::size_t>(index)];
        object.rect.x = clamp_unit_position(maybe_snap(editor, start.rect.x + dx), object.rect.w);
        object.rect.y = clamp_unit_position(maybe_snap(editor, start.rect.y + dy), object.rect.h);
    }
}

void resize_primary(EditorState& editor, Layout& layout, float dx, float dy) {
    if (editor.drag_start_objects.empty() || editor.primary < 0)
        return;
    if (!valid_object_index(layout, editor.primary))
        return;

    const Object* start_object = nullptr;
    for (std::size_t i = 0; i < editor.drag_start_selection.size(); ++i) {
        if (editor.drag_start_selection[i] == editor.primary && i < editor.drag_start_objects.size()) {
            start_object = &editor.drag_start_objects[i];
            break;
        }
    }
    if (!start_object)
        return;

    const Object& start = *start_object;
    Rect rect = start.rect;

    switch (editor.drag_handle) {
        case Handle::Left:
        case Handle::TopLeft:
        case Handle::BottomLeft:
            rect.x = start.rect.x + dx;
            rect.w = start.rect.w - dx;
            break;
        default: break;
    }

    switch (editor.drag_handle) {
        case Handle::Right:
        case Handle::TopRight:
        case Handle::BottomRight:
            rect.w = start.rect.w + dx;
            break;
        default: break;
    }

    switch (editor.drag_handle) {
        case Handle::Top:
        case Handle::TopLeft:
        case Handle::TopRight:
            rect.y = start.rect.y + dy;
            rect.h = start.rect.h - dy;
            break;
        default: break;
    }

    switch (editor.drag_handle) {
        case Handle::Bottom:
        case Handle::BottomLeft:
        case Handle::BottomRight:
            rect.h = start.rect.h + dy;
            break;
        default: break;
    }

    rect.x = maybe_snap(editor, rect.x);
    rect.y = maybe_snap(editor, rect.y);
    rect.w = maybe_snap(editor, rect.w);
    rect.h = maybe_snap(editor, rect.h);

    rect.w = std::clamp(rect.w, kMinSize, 1.0f);
    rect.h = std::clamp(rect.h, kMinSize, 1.0f);
    rect.x = clamp_unit_position(rect.x, rect.w);
    rect.y = clamp_unit_position(rect.y, rect.h);

    layout.objects[static_cast<std::size_t>(editor.primary)].rect = rect;
}

bool layout_changed(const Layout& a, const Layout& b) {
    if (a.id != b.id || a.label != b.label || a.width != b.width || a.height != b.height ||
        a.form_factor != b.form_factor || a.objects.size() != b.objects.size()) {
        return true;
    }

    for (std::size_t i = 0; i < a.objects.size(); ++i) {
        const Object& left = a.objects[i];
        const Object& right = b.objects[i];
        if (left.id != right.id || left.label != right.label)
            return true;
        if (left.rect.x != right.rect.x || left.rect.y != right.rect.y ||
            left.rect.w != right.rect.w || left.rect.h != right.rect.h) {
            return true;
        }
    }

    return false;
}

} // namespace

bool editor_hit_test(const Layout& layout,
                     Viewport viewport,
                     float mouse_x,
                     float mouse_y,
                     const std::vector<int>& selection,
                     HitResult& out_hit) {
    out_hit = HitResult{};
    if (viewport.w <= 0.0f || viewport.h <= 0.0f)
        return false;

    for (int index : selection) {
        if (!valid_object_index(layout, index))
            continue;
        Rect rect = object_to_screen(layout.objects[static_cast<std::size_t>(index)], viewport);
        Handle handle = hit_handle(rect, mouse_x, mouse_y);
        if (handle != Handle::None) {
            out_hit.object_index = index;
            out_hit.handle = handle;
            return true;
        }
    }

    for (int index = static_cast<int>(layout.objects.size()) - 1; index >= 0; --index) {
        Rect rect = object_to_screen(layout.objects[static_cast<std::size_t>(index)], viewport);
        Handle handle = hit_handle(rect, mouse_x, mouse_y);
        if (handle != Handle::None) {
            out_hit.object_index = index;
            out_hit.handle = handle;
            return true;
        }
    }

    return false;
}

EditorFrameResult editor_begin_frame(EditorState& editor,
                                     Layout& layout,
                                     const EditorInput& input,
                                     Viewport viewport) {
    EditorFrameResult result;
    prune_selection(editor, layout);

    if (input.key_save) {
        editor.save_requested = true;
        result.save_requested = true;
    }
    if (input.key_undo && editor_undo(editor, layout)) {
        result.changed = true;
        editor.dirty = true;
    }
    if (input.key_redo && editor_redo(editor, layout)) {
        result.changed = true;
        editor.dirty = true;
    }
    if (input.key_delete && !editor.selection.empty()) {
        editor_commit_undo(editor, layout);
        std::vector<int> to_remove = editor.selection;
        std::sort(to_remove.begin(), to_remove.end());
        for (auto it = to_remove.rbegin(); it != to_remove.rend(); ++it) {
            if (valid_object_index(layout, *it))
                layout.objects.erase(layout.objects.begin() + *it);
        }
        editor_clear_selection(editor);
        editor.dirty = true;
        result.changed = true;
        result.selection_changed = true;
    }
    if (input.key_copy && !editor.selection.empty()) {
        editor.clipboard.clear();
        for (int index : editor.selection) {
            if (valid_object_index(layout, index))
                editor.clipboard.push_back(layout.objects[static_cast<std::size_t>(index)]);
        }
    }
    if (input.key_paste && !editor.clipboard.empty()) {
        editor_commit_undo(editor, layout);
        editor_clear_selection(editor);
        for (const Object& object : editor.clipboard) {
            Object copy = object;
            copy.id = generate_object_id(layout);
            copy.rect.x = clamp_unit_position(copy.rect.x + kPasteNudge, copy.rect.w);
            copy.rect.y = clamp_unit_position(copy.rect.y + kPasteNudge, copy.rect.h);
            layout.objects.push_back(copy);
            editor_add_to_selection(editor, static_cast<int>(layout.objects.size()) - 1);
        }
        editor.dirty = true;
        result.changed = true;
        result.selection_changed = true;
    }

    float local_x = viewport.w > 0.0f ? (input.mouse_x - viewport.x) / viewport.w : 0.0f;
    float local_y = viewport.h > 0.0f ? (input.mouse_y - viewport.y) / viewport.h : 0.0f;

    if (input.left_down && !editor.mouse_was_down) {
        HitResult hit;
        if (editor_hit_test(layout, viewport, input.mouse_x, input.mouse_y, editor.selection, hit)) {
            if (input.ctrl || input.shift) {
                if (editor_is_selected(editor, hit.object_index))
                    editor_remove_from_selection(editor, hit.object_index);
                else
                    editor_add_to_selection(editor, hit.object_index);
                result.selection_changed = true;
            } else if (!editor_is_selected(editor, hit.object_index)) {
                editor_select_single(editor, hit.object_index);
                result.selection_changed = true;
            }

            if (editor_is_selected(editor, hit.object_index)) {
                editor.primary = hit.object_index;
                begin_drag(editor, layout, hit, local_x, local_y);
            }
        } else if (!input.ctrl && !input.shift) {
            editor_clear_selection(editor);
            result.selection_changed = true;
        }
    }

    if (input.left_down && editor.dragging) {
        float dx = local_x - editor.drag_start_x;
        float dy = local_y - editor.drag_start_y;
        Layout before = layout;

        if (editor.drag_handle == Handle::Center) {
            translate_selection(editor, layout, dx, dy);
        } else {
            resize_primary(editor, layout, dx, dy);
        }

        if (layout_changed(before, layout)) {
            editor.drag_changed = true;
            editor.dirty = true;
            result.changed = true;
        }
    }

    if (!input.left_down && editor.mouse_was_down && editor.dragging) {
        if (editor.drag_changed) {
            Layout before_drag = layout;
            for (std::size_t i = 0; i < editor.drag_start_selection.size(); ++i) {
                int index = editor.drag_start_selection[i];
                if (valid_object_index(before_drag, index) && i < editor.drag_start_objects.size()) {
                    before_drag.objects[static_cast<std::size_t>(index)] =
                        editor.drag_start_objects[i];
                }
            }
            editor_commit_undo(editor, before_drag);
        }
        editor.dragging = false;
        editor.drag_changed = false;
        editor.drag_start_objects.clear();
        editor.drag_start_selection.clear();
    }

    editor.mouse_was_down = input.left_down;
    return result;
}

void editor_clear_selection(EditorState& editor) {
    editor.selection.clear();
    editor.primary = -1;
}

void editor_select_single(EditorState& editor, int object_index) {
    editor.selection.clear();
    if (object_index >= 0)
        editor.selection.push_back(object_index);
    editor.primary = object_index;
}

void editor_add_to_selection(EditorState& editor, int object_index) {
    if (object_index < 0)
        return;
    if (!editor_is_selected(editor, object_index)) {
        editor.selection.push_back(object_index);
        std::sort(editor.selection.begin(), editor.selection.end());
    }
    editor.primary = object_index;
}

void editor_remove_from_selection(EditorState& editor, int object_index) {
    auto it = std::remove(editor.selection.begin(), editor.selection.end(), object_index);
    editor.selection.erase(it, editor.selection.end());
    sync_primary(editor);
}

bool editor_is_selected(const EditorState& editor, int object_index) {
    return std::find(editor.selection.begin(), editor.selection.end(), object_index) !=
           editor.selection.end();
}

void editor_mark_saved(EditorState& editor) {
    editor.dirty = false;
    editor.save_requested = false;
}

void editor_commit_undo(EditorState& editor, const Layout& layout) {
    editor.undo_stack.push_back(layout);
    editor.redo_stack.clear();
}

bool editor_undo(EditorState& editor, Layout& layout) {
    if (editor.undo_stack.empty())
        return false;

    editor.redo_stack.push_back(layout);
    layout = editor.undo_stack.back();
    editor.undo_stack.pop_back();
    prune_selection(editor, layout);
    return true;
}

bool editor_redo(EditorState& editor, Layout& layout) {
    if (editor.redo_stack.empty())
        return false;

    editor.undo_stack.push_back(layout);
    layout = editor.redo_stack.back();
    editor.redo_stack.pop_back();
    prune_selection(editor, layout);
    return true;
}

} // namespace glayout
