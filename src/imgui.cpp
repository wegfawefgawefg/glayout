#include "glayout/imgui.hpp"

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <string>

namespace glayout::imgui {
namespace {

const char* form_factor_label(FormFactor form_factor) {
    switch (form_factor) {
        case FormFactor::Desktop: return "Desktop";
        case FormFactor::Tablet: return "Tablet";
        case FormFactor::Phone: return "Phone";
    }
    return "Desktop";
}

} // namespace

void render_layout_browser(const std::vector<Layout>& layouts) {
    if (!ImGui::Begin("glayout: Layouts")) {
        ImGui::End();
        return;
    }

    if (layouts.empty()) {
        ImGui::TextUnformatted("No layouts loaded.");
        ImGui::End();
        return;
    }

    for (const Layout& layout : layouts) {
        std::string title = layout.label + " #" + std::to_string(layout.id) + " " +
                            std::to_string(layout.width) + "x" +
                            std::to_string(layout.height) + " " +
                            std::string(form_factor_label(layout.form_factor));
        if (!ImGui::TreeNode(title.c_str()))
            continue;

        ImGui::Text("Objects: %zu", layout.objects.size());
        if (ImGui::BeginTable("objects", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("ID");
            ImGui::TableSetupColumn("Label");
            ImGui::TableSetupColumn("X/Y");
            ImGui::TableSetupColumn("W/H");
            ImGui::TableSetupColumn("Index");
            ImGui::TableHeadersRow();

            for (std::size_t i = 0; i < layout.objects.size(); ++i) {
                const Object& object = layout.objects[i];
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", object.id);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(object.label.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.3f, %.3f",
                            static_cast<double>(object.rect.x),
                            static_cast<double>(object.rect.y));
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%.3f, %.3f",
                            static_cast<double>(object.rect.w),
                            static_cast<double>(object.rect.h));
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%d", static_cast<int>(i));
            }

            ImGui::EndTable();
        }

        ImGui::TreePop();
    }

    ImGui::End();
}

bool render_editor_panel(EditorState& editor, Layout& layout) {
    bool changed = false;

    if (!ImGui::Begin("glayout: Editor")) {
        ImGui::End();
        return false;
    }

    ImGui::Text("Layout #%d: %s", layout.id, layout.label.c_str());
    ImGui::Text("%dx%d %s", layout.width, layout.height, form_factor_label(layout.form_factor));
    ImGui::Checkbox("Snap", &editor.snap_enabled);
    ImGui::DragFloat("Grid", &editor.grid_step, 0.005f, 0.001f, 1.0f, "%.3f");

    if (ImGui::Button("Save")) {
        editor.save_requested = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Undo")) {
        changed = editor_undo(editor, layout) || changed;
    }
    ImGui::SameLine();
    if (ImGui::Button("Redo")) {
        changed = editor_redo(editor, layout) || changed;
    }

    if (editor.dirty)
        ImGui::TextColored(ImVec4(1.0f, 0.72f, 0.25f, 1.0f), "Dirty");

    if (ImGui::Button("Add object")) {
        editor_commit_undo(editor, layout);
        Object object;
        object.id = generate_object_id(layout);
        object.label = "object_" + std::to_string(object.id);
        object.rect = Rect{0.4f, 0.4f, 0.2f, 0.1f};
        layout.objects.push_back(object);
        editor_select_single(editor, static_cast<int>(layout.objects.size()) - 1);
        editor.dirty = true;
        changed = true;
    }

    if (ImGui::BeginListBox("Objects")) {
        for (int i = 0; i < static_cast<int>(layout.objects.size()); ++i) {
            const Object& object = layout.objects[static_cast<std::size_t>(i)];
            std::string label = object.label + " #" + std::to_string(object.id);
            if (ImGui::Selectable(label.c_str(), editor_is_selected(editor, i)))
                editor_select_single(editor, i);
        }
        ImGui::EndListBox();
    }

    if (editor.primary >= 0 && editor.primary < static_cast<int>(layout.objects.size())) {
        Object& object = layout.objects[static_cast<std::size_t>(editor.primary)];
        ImGui::SeparatorText("Selected object");
        ImGui::Text("ID: %d", object.id);
        char label[128]{};
        std::snprintf(label, sizeof(label), "%s", object.label.c_str());
        if (ImGui::InputText("Label", label, sizeof(label))) {
            object.label = label;
            editor.dirty = true;
            changed = true;
        }
        float rect[4]{object.rect.x, object.rect.y, object.rect.w, object.rect.h};
        if (ImGui::DragFloat4("Rect", rect, 0.005f, -2.0f, 2.0f, "%.3f")) {
            object.rect = Rect{rect[0], rect[1], rect[2], rect[3]};
            editor.dirty = true;
            changed = true;
        }
    }

    ImGui::End();
    return changed;
}

bool render_layout_pool_editor(EditorState& editor,
                               std::vector<Layout>& layouts,
                               int& selected_layout_index) {
    bool changed = false;

    if (!ImGui::Begin("glayout: Layout Pool")) {
        ImGui::End();
        return false;
    }

    if (layouts.empty()) {
        ImGui::TextUnformatted("No layouts loaded.");
        if (ImGui::Button("New layout")) {
            Layout layout;
            layout.id = generate_layout_id(layouts);
            layout.label = "Layout_" + std::to_string(layout.id);
            layout.width = 1920;
            layout.height = 1080;
            layout.form_factor = FormFactor::Desktop;
            layouts.push_back(layout);
            selected_layout_index = 0;
            changed = true;
        }
        ImGui::End();
        return changed;
    }

    selected_layout_index =
        std::clamp(selected_layout_index, 0, static_cast<int>(layouts.size()) - 1);

    if (ImGui::BeginListBox("Layouts")) {
        for (int i = 0; i < static_cast<int>(layouts.size()); ++i) {
            const Layout& layout = layouts[static_cast<std::size_t>(i)];
            std::string label = layout.label + " #" + std::to_string(layout.id) + " " +
                                std::to_string(layout.width) + "x" +
                                std::to_string(layout.height) + " " +
                                std::string(form_factor_label(layout.form_factor));
            if (ImGui::Selectable(label.c_str(), selected_layout_index == i)) {
                selected_layout_index = i;
                editor_clear_selection(editor);
            }
        }
        ImGui::EndListBox();
    }

    Layout& selected = layouts[static_cast<std::size_t>(selected_layout_index)];
    if (ImGui::Button("Duplicate layout")) {
        Layout copy = selected;
        copy.id = generate_layout_id(layouts);
        copy.label += "_copy";
        layouts.push_back(copy);
        selected_layout_index = static_cast<int>(layouts.size()) - 1;
        editor_clear_selection(editor);
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("New layout")) {
        Layout layout;
        layout.id = generate_layout_id(layouts);
        layout.label = "Layout_" + std::to_string(layout.id);
        layout.width = selected.width;
        layout.height = selected.height;
        layout.form_factor = selected.form_factor;
        layouts.push_back(layout);
        selected_layout_index = static_cast<int>(layouts.size()) - 1;
        editor_clear_selection(editor);
        changed = true;
    }

    ImGui::End();

    if (selected_layout_index >= 0 && selected_layout_index < static_cast<int>(layouts.size())) {
        changed =
            render_editor_panel(editor, layouts[static_cast<std::size_t>(selected_layout_index)]) ||
            changed;
    }

    return changed;
}

} // namespace glayout::imgui
