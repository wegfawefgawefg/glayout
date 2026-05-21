# glayout SDL Demo

This demo is a normal SDL3 consumer of `glayout_core` and `glayout_editor`.
It loads `data/layouts.lisp`, picks the closest layout for the active page and
preview form factor, and draws the resulting rectangles with SDL.

## Controls

- `1`, `2`, `3`: switch to Title, Settings, Credits.
- `Tab`: cycle pages.
- `P`: cycle desktop/tablet/phone preview.
- Arrow keys: move demo focus.
- `Enter`: activate focused demo item.
- `E`: toggle layout edit mode.
- Mouse drag in edit mode: move or resize rectangles.
- `S`: save layouts.
- `Z`, `Y`: undo, redo.
- `C`, `V`: copy, paste selected object.
- `Delete`/`Backspace`: delete selected object.
- `Esc`: quit.

The demo intentionally treats buttons, panels, and labels as demo-only concepts.
`glayout` itself only knows about rectangles.

Optional Dear ImGui helpers live in `glayout_imgui`. If CMake is configured with
`GLAYOUT_WITH_IMGUI=ON` and `GLAYOUT_IMGUI_SOURCE_DIR` points at an ImGui source
tree that contains the official SDL3 renderer backends, this demo initializes
those backends and shows the ImGui layout browser/editor while edit mode is on.
If those backends are not available, the SDL rectangle editor still works.
