#include "glayout/editor.hpp"

#include <array>

namespace glayout {
namespace {

constexpr float kHandlePixels = 8.0f;

Rect object_to_screen(const Object& object, Viewport viewport) {
    return map_rect(Rect{viewport.x, viewport.y, viewport.w, viewport.h}, object.rect);
}

Rect handle_rect(float cx, float cy) {
    float half = kHandlePixels * 0.5f;
    return Rect{cx - half, cy - half, kHandlePixels, kHandlePixels};
}

std::array<OverlayHandle, 8> handles_for_rect(int object_index, Rect rect) {
    return {{
        OverlayHandle{object_index, Handle::TopLeft, handle_rect(rect.x, rect.y)},
        OverlayHandle{object_index, Handle::TopRight, handle_rect(rect.x + rect.w, rect.y)},
        OverlayHandle{object_index, Handle::BottomLeft, handle_rect(rect.x, rect.y + rect.h)},
        OverlayHandle{
            object_index,
            Handle::BottomRight,
            handle_rect(rect.x + rect.w, rect.y + rect.h),
        },
        OverlayHandle{
            object_index,
            Handle::Left,
            Rect{rect.x - kHandlePixels * 0.5f, rect.y, kHandlePixels, rect.h},
        },
        OverlayHandle{
            object_index,
            Handle::Right,
            Rect{rect.x + rect.w - kHandlePixels * 0.5f, rect.y, kHandlePixels, rect.h},
        },
        OverlayHandle{
            object_index,
            Handle::Top,
            Rect{rect.x, rect.y - kHandlePixels * 0.5f, rect.w, kHandlePixels},
        },
        OverlayHandle{
            object_index,
            Handle::Bottom,
            Rect{rect.x, rect.y + rect.h - kHandlePixels * 0.5f, rect.w, kHandlePixels},
        },
    }};
}

} // namespace

std::vector<OverlayObject> editor_collect_overlay_objects(const EditorState& editor,
                                                          const Layout& layout,
                                                          Viewport viewport) {
    std::vector<OverlayObject> objects;
    objects.reserve(layout.objects.size());

    for (int i = 0; i < static_cast<int>(layout.objects.size()); ++i) {
        const Object& object = layout.objects[static_cast<std::size_t>(i)];
        objects.push_back(OverlayObject{
            i,
            object_to_screen(object, viewport),
            editor_is_selected(editor, i),
            editor.primary == i,
        });
    }

    return objects;
}

std::vector<OverlayHandle> editor_collect_overlay_handles(const EditorState& editor,
                                                          const Layout& layout,
                                                          Viewport viewport) {
    std::vector<OverlayHandle> handles;

    for (int index : editor.selection) {
        if (index < 0 || index >= static_cast<int>(layout.objects.size()))
            continue;

        Rect rect = object_to_screen(layout.objects[static_cast<std::size_t>(index)], viewport);
        auto object_handles = handles_for_rect(index, rect);
        handles.insert(handles.end(), object_handles.begin(), object_handles.end());
    }

    return handles;
}

} // namespace glayout
