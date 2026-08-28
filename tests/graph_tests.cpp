#include "glayout/graph.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void require(bool condition, const char* message) {
    if (condition)
        return;
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
}

bool near(float left, float right) {
    return std::fabs(left - right) < 0.01f;
}

glayout::GraphNode fixed(std::string id, float width, float height) {
    glayout::GraphNode node;
    node.id = std::move(id);
    node.size.width = {glayout::LengthKind::Pixels, width};
    node.size.height = {glayout::LengthKind::Pixels, height};
    node.align = glayout::Align::Start;
    return node;
}

glayout::GraphLayout sample_graph() {
    glayout::GraphLayout layout;
    layout.id = "settings";
    layout.label = "Settings";
    layout.width = 1280;
    layout.height = 720;
    layout.root.id = "root";
    layout.root.container = glayout::ContainerKind::Row;
    layout.root.padding = {20.0f, 10.0f, 20.0f, 10.0f};
    layout.root.gap = 10.0f;

    glayout::GraphNode rail = fixed("rail", 240.0f, 700.0f);
    rail.container = glayout::ContainerKind::Column;
    rail.children.push_back(fixed("play", 240.0f, 50.0f));
    rail.children.push_back(fixed("settings_button", 240.0f, 50.0f));

    glayout::GraphNode body;
    body.id = "body";
    body.container = glayout::ContainerKind::Grid;
    body.columns = 2;
    body.gap = 12.0f;
    body.size.width = {glayout::LengthKind::Fill, 1.0f};
    body.size.height = {glayout::LengthKind::Fill, 1.0f};
    body.children.push_back(fixed("list", 0.0f, 0.0f));
    body.children.back().align = glayout::Align::Stretch;
    body.children.push_back(fixed("detail", 0.0f, 0.0f));
    body.children.back().align = glayout::Align::Stretch;

    layout.root.children.push_back(std::move(rail));
    layout.root.children.push_back(std::move(body));
    return layout;
}

// Verifies nested direct-child identity survives dense compilation.
void test_compile_tree() {
    const glayout::GraphCompileResult compiled = glayout::compile_graph(sample_graph());
    require(compiled.ok, "graph compiles");
    require(compiled.graph.nodes.size() == 7, "all nested nodes compile");
    const glayout::CompiledNode& root = compiled.graph.nodes[0];
    require(root.child_count == 2, "root has two direct children");
    require(compiled.graph.nodes[compiled.graph.children[root.first_child]].id == "rail",
            "first direct child remains rail");
    require(compiled.graph.nodes[compiled.graph.children[root.first_child + 1]].id == "body",
            "second direct child remains body");
}

// Verifies row, fill, grid, padding, and clean-frame reuse behavior.
void test_resolve() {
    glayout::GraphRuntime runtime(glayout::compile_graph(sample_graph()).graph);
    const glayout::ResolveInput input{glayout::Rect{0.0f, 0.0f, 1280.0f, 720.0f}, {}, nullptr,
                                      nullptr};
    require(runtime.resolve(input), "first resolve performs layout");
    require(!runtime.resolve(input), "second resolve reuses clean geometry");
    const glayout::ResolvedNode* rail = runtime.find("rail");
    const glayout::ResolvedNode* body = runtime.find("body");
    const glayout::ResolvedNode* list = runtime.find("list");
    require(rail && body && list, "resolved identities are queryable");
    require(near(rail->border.x, 20.0f) && near(rail->border.w, 240.0f), "fixed rail resolves");
    require(near(body->border.x, 270.0f) && near(body->border.w, 990.0f), "fill body resolves");
    require(near(list->border.w, 489.0f), "grid subtracts its gap");
    require(runtime.stats().clean_reuses == 1, "clean reuse is counted");
}

// Verifies authored graphs survive the public S-expression persistence path.
void test_round_trip() {
    const std::string text = glayout::write_graphs({sample_graph()});
    const glayout::GraphParseResult parsed = glayout::parse_graphs(text);
    if (!parsed.ok) {
        for (const glayout::Diagnostic& diagnostic : parsed.diagnostics)
            std::cerr << diagnostic.message << '\n';
    }
    require(parsed.ok, "written graph parses");
    require(parsed.layouts.size() == 1, "one graph round trips");
    require(parsed.layouts[0].root.children.size() == 2, "nested children round trip");
    require(parsed.layouts[0].root.children[0].children.size() == 2,
            "deep children round trip");
}

// Verifies invalid references fail before reaching the runtime.
void test_anchor_validation() {
    glayout::GraphLayout layout = sample_graph();
    layout.root.children[0].anchors.push_back(
        glayout::AnchorRule{glayout::Edge::Left, "missing", glayout::Edge::Right, 4.0f});
    require(!glayout::compile_graph(layout).ok, "missing anchor target is rejected");
}

// Verifies structural authoring operations remain transactional and cycle safe.
void test_graph_editor() {
    glayout::GraphLayout layout = sample_graph();
    glayout::GraphEditorState editor;
    const glayout::GraphLayout before = layout;
    require(glayout::graph_reparent_node(layout, "settings_button", "body"),
            "node can be reparented");
    require(!glayout::graph_reparent_node(layout, "body", "settings_button"),
            "node cannot be reparented beneath its descendant");
    require(glayout::graph_duplicate_node(layout, "rail", "rail_copy"),
            "nested subtree can be duplicated");
    require(glayout::find_graph_node(layout, "rail_copy/play"),
            "duplicated descendants receive unique identities");
    glayout::GraphNode item = layout.root.children[0];
    require(glayout::graph_repeat_children(layout, "body", item, {"profile/moss", "profile/vega"}),
            "runtime-sized collections repeat nested geometry templates");
    require(glayout::find_graph_node(layout, "profile/moss/play"),
            "repeated template descendants retain stable item identity");
    glayout::graph_editor_commit(editor, before);
    require(glayout::graph_editor_undo(editor, layout), "undo restores prior graph");
    require(!glayout::find_graph_node(layout, "rail_copy"), "undo removes duplicated subtree");
    require(glayout::graph_editor_redo(editor, layout), "redo restores edit");
    require(glayout::find_graph_node(layout, "rail_copy"), "redo restores duplicated subtree");
}

// Verifies native-canvas movement maps pixels through parent content geometry.
void test_graph_canvas_absolute_move() {
    glayout::GraphLayout layout;
    layout.id = "canvas";
    layout.root.id = "root";
    layout.root.container = glayout::ContainerKind::Absolute;
    layout.root.padding = {20.0f, 10.0f, 20.0f, 10.0f};
    glayout::GraphNode child = fixed("child", 100.0f, 80.0f);
    child.absolute_rect = {0.1f, 0.2f, 0.25f, 0.25f};
    layout.root.children.push_back(child);

    glayout::GraphRuntime runtime(glayout::compile_graph(layout).graph);
    runtime.resolve({{0.0f, 0.0f, 500.0f, 300.0f}, {}, nullptr, nullptr});
    glayout::GraphCanvasState canvas;
    canvas.selection = {"root"};
    canvas.primary = "root";
    const glayout::Rect before = runtime.find("child")->border;
    require(glayout::graph_canvas_press(layout, runtime.graph(), runtime.nodes(), canvas,
                                        before.x + 20.0f, before.y + 20.0f, false)
                .transaction_started,
            "canvas press begins direct manipulation");
    require(canvas.primary == "child", "selected container does not intercept a deeper child");
    require(glayout::graph_canvas_drag(layout, runtime.graph(), runtime.nodes(), canvas,
                                       before.x + 36.0f, before.y + 20.0f)
                .changed,
            "canvas drag mutates absolute geometry");
    require(near(layout.root.children[0].absolute_rect.x, 0.1f + 16.0f / 460.0f),
            "absolute movement uses padded parent content width");
    require(glayout::graph_canvas_release(layout, runtime.graph(), runtime.nodes(), canvas,
                                          before.x + 36.0f, before.y + 20.0f)
                .transaction_finished,
            "canvas release closes transaction");
}

// Verifies multi-selection bounds and structural row reordering remain deterministic.
void test_graph_canvas_selection_and_reorder() {
    glayout::GraphLayout layout = sample_graph();
    glayout::GraphRuntime runtime(glayout::compile_graph(layout).graph);
    runtime.resolve({{0.0f, 0.0f, 1280.0f, 720.0f}, {}, nullptr, nullptr});
    glayout::GraphCanvasState canvas;
    canvas.selection = {"play", "settings_button"};
    canvas.primary = "settings_button";
    glayout::Rect bounds;
    require(glayout::graph_canvas_selection_bounds(runtime.graph(), runtime.nodes(), canvas,
                                                    bounds),
            "multi-selection bounds resolve");
    require(near(bounds.y, 10.0f) && near(bounds.h, 100.0f),
            "multi-selection bounds union keeps original edges");

    canvas.selection = {"rail"};
    canvas.primary = "rail";
    const glayout::Rect rail = runtime.find("rail")->border;
    glayout::graph_canvas_press(layout, runtime.graph(), runtime.nodes(), canvas,
                               rail.x + 30.0f, rail.y + 180.0f, false);
    const glayout::Rect body = runtime.find("body")->border;
    glayout::graph_canvas_release(layout, runtime.graph(), runtime.nodes(), canvas,
                                 body.x + body.w, body.y);
    require(layout.root.children.back().id == "rail", "row node can reorder to final position");
}

// Verifies keyboard nudging never turns a flow child into an accidental reorder.
void test_graph_canvas_flow_nudge() {
    glayout::GraphLayout layout = sample_graph();
    glayout::GraphRuntime runtime(glayout::compile_graph(layout).graph);
    runtime.resolve({{0.0f, 0.0f, 1280.0f, 720.0f}, {}, nullptr, nullptr});
    glayout::GraphCanvasState canvas;
    canvas.selection = {"rail"};
    canvas.primary = "rail";
    require(!glayout::graph_canvas_nudge(layout, runtime.graph(), runtime.nodes(), canvas,
                                         16.0f, 0.0f),
            "flow child ignores position nudge without anchors");
    require(layout.root.children.front().id == "rail",
            "flow nudge preserves structural ordering");
    require(!canvas.dragging && canvas.drag_nodes.empty(),
            "nudge closes its direct-manipulation state");
}

} // namespace

int main() {
    test_compile_tree();
    test_resolve();
    test_round_trip();
    test_anchor_validation();
    test_graph_editor();
    test_graph_canvas_absolute_move();
    test_graph_canvas_selection_and_reorder();
    test_graph_canvas_flow_nudge();
    std::cout << "glayout graph tests passed\n";
    return 0;
}
