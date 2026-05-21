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

Optional Dear ImGui helpers live in `glayout_imgui`, but this SDL demo does not
vendor or initialize Dear ImGui backends. A host that already owns ImGui can link
`glayout_imgui` and call the helper panels from its existing ImGui frame.
