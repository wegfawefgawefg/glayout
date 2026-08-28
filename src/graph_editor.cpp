#include "glayout/graph_editor.hpp"

#include <algorithm>
#include <unordered_map>
#include <utility>

namespace glayout {
namespace {

GraphNode* find_node(GraphNode& node, std::string_view id) {
    if (node.id == id)
        return &node;
    for (GraphNode& child : node.children) {
        if (GraphNode* found = find_node(child, id))
            return found;
    }
    return nullptr;
}

const GraphNode* find_node(const GraphNode& node, std::string_view id) {
    if (node.id == id)
        return &node;
    for (const GraphNode& child : node.children) {
        if (const GraphNode* found = find_node(child, id))
            return found;
    }
    return nullptr;
}

GraphNode* find_parent(GraphNode& node, std::string_view id) {
    for (GraphNode& child : node.children) {
        if (child.id == id)
            return &node;
        if (GraphNode* found = find_parent(child, id))
            return found;
    }
    return nullptr;
}

bool take_child(GraphNode& parent, std::string_view id, GraphNode& output) {
    const auto direct = std::find_if(parent.children.begin(), parent.children.end(),
                                     [&](const GraphNode& child) { return child.id == id; });
    if (direct != parent.children.end()) {
        output = std::move(*direct);
        parent.children.erase(direct);
        return true;
    }
    for (GraphNode& child : parent.children) {
        if (take_child(child, id, output))
            return true;
    }
    return false;
}

void rename_copy(GraphNode& node, std::string_view root_id, std::string_view new_root,
                 std::unordered_map<std::string, std::string>& mapping) {
    const std::string old_id = node.id;
    node.id = old_id == root_id ? std::string(new_root) : std::string(new_root) + "/" + old_id;
    mapping.emplace(old_id, node.id);
    for (GraphNode& child : node.children)
        rename_copy(child, root_id, new_root, mapping);
}

void repair_copy_anchors(GraphNode& node,
                         const std::unordered_map<std::string, std::string>& mapping) {
    for (AnchorRule& anchor : node.anchors) {
        const auto replacement = mapping.find(anchor.target);
        if (replacement != mapping.end())
            anchor.target = replacement->second;
    }
    for (GraphNode& child : node.children)
        repair_copy_anchors(child, mapping);
}

} // namespace

// Finds authored identities without exposing storage traversal to editors.
GraphNode* find_graph_node(GraphLayout& layout, std::string_view id) {
    return find_node(layout.root, id);
}

const GraphNode* find_graph_node(const GraphLayout& layout, std::string_view id) {
    return find_node(layout.root, id);
}

GraphNode* find_graph_parent(GraphLayout& layout, std::string_view id) {
    return find_parent(layout.root, id);
}

// Applies structural edits while preserving stable identity invariants.
bool graph_add_child(GraphLayout& layout, std::string_view parent_id, GraphNode child) {
    if (child.id.empty() || find_graph_node(layout, child.id))
        return false;
    GraphNode* parent = find_graph_node(layout, parent_id);
    if (!parent)
        return false;
    parent->children.push_back(std::move(child));
    return true;
}

bool graph_remove_node(GraphLayout& layout, std::string_view id) {
    if (layout.root.id == id)
        return false;
    GraphNode removed;
    return take_child(layout.root, id, removed);
}

bool graph_reparent_node(GraphLayout& layout, std::string_view id, std::string_view parent_id) {
    if (id == layout.root.id || id == parent_id)
        return false;
    GraphNode* source = find_graph_node(layout, id);
    GraphNode* target = find_graph_node(layout, parent_id);
    if (!source || !target || find_node(*source, parent_id))
        return false;
    GraphNode moved;
    if (!take_child(layout.root, id, moved))
        return false;
    target = find_graph_node(layout, parent_id);
    target->children.push_back(std::move(moved));
    return true;
}

bool graph_duplicate_node(GraphLayout& layout, std::string_view id, std::string new_id) {
    if (new_id.empty() || find_graph_node(layout, new_id))
        return false;
    const GraphNode* source = find_graph_node(layout, id);
    GraphNode* parent = find_graph_parent(layout, id);
    if (!source || !parent)
        return false;
    GraphNode copy = *source;
    std::unordered_map<std::string, std::string> mapping;
    rename_copy(copy, id, new_id, mapping);
    repair_copy_anchors(copy, mapping);
    parent->children.push_back(std::move(copy));
    return true;
}

// Stores authoring-only snapshots so runtime structures stay allocation free.
void graph_editor_commit(GraphEditorState& editor, const GraphLayout& before) {
    editor.undo_stack.push_back(before);
    editor.redo_stack.clear();
    editor.dirty = true;
}

bool graph_editor_undo(GraphEditorState& editor, GraphLayout& layout) {
    if (editor.undo_stack.empty())
        return false;
    editor.redo_stack.push_back(layout);
    layout = std::move(editor.undo_stack.back());
    editor.undo_stack.pop_back();
    editor.dirty = true;
    return true;
}

bool graph_editor_redo(GraphEditorState& editor, GraphLayout& layout) {
    if (editor.redo_stack.empty())
        return false;
    editor.undo_stack.push_back(layout);
    layout = std::move(editor.redo_stack.back());
    editor.redo_stack.pop_back();
    editor.dirty = true;
    return true;
}

void graph_editor_mark_saved(GraphEditorState& editor) {
    editor.dirty = false;
}

} // namespace glayout
