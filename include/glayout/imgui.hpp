#pragma once

#include "glayout/editor.hpp"
#include "glayout/layout.hpp"

#include <vector>

namespace glayout::imgui {

void render_layout_browser(const std::vector<Layout>& layouts);
bool render_editor_panel(EditorState& editor, Layout& layout);

} // namespace glayout::imgui
