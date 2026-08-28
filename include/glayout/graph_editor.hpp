#pragma once

#include "glayout/graph_types.hpp"

#include <string_view>
#include <vector>

namespace glayout {

struct GraphEditorState {
    std::string selection;
    bool dirty = false;
    std::vector<GraphLayout> undo_stack;
    std::vector<GraphLayout> redo_stack;
};

GraphNode* find_graph_node(GraphLayout& layout, std::string_view id);
const GraphNode* find_graph_node(const GraphLayout& layout, std::string_view id);
GraphNode* find_graph_parent(GraphLayout& layout, std::string_view id);

bool graph_add_child(GraphLayout& layout, std::string_view parent_id, GraphNode child);
bool graph_remove_node(GraphLayout& layout, std::string_view id);
bool graph_reparent_node(GraphLayout& layout, std::string_view id, std::string_view parent_id);
bool graph_duplicate_node(GraphLayout& layout, std::string_view id, std::string new_id);

void graph_editor_commit(GraphEditorState& editor, const GraphLayout& before);
bool graph_editor_undo(GraphEditorState& editor, GraphLayout& layout);
bool graph_editor_redo(GraphEditorState& editor, GraphLayout& layout);
void graph_editor_mark_saved(GraphEditorState& editor);

} // namespace glayout
