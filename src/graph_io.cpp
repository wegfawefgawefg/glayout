#include "glayout/graph_io.hpp"

#include <fstream>
#include <gsexp/sexp.hpp>
#include <optional>
#include <sstream>

namespace glayout {
namespace {

void add_error(GraphParseResult& result, std::string message, int line = 1, int column = 1) {
    result.diagnostics.push_back(
        Diagnostic{DiagnosticSeverity::Error, std::move(message), line, column});
}

void add_warning(GraphParseResult& result, std::string message) {
    result.diagnostics.push_back(Diagnostic{DiagnosticSeverity::Warning, std::move(message), 1, 1});
}

bool has_errors(const GraphParseResult& result) {
    for (const Diagnostic& diagnostic : result.diagnostics) {
        if (diagnostic.severity == DiagnosticSeverity::Error)
            return true;
    }
    return false;
}

std::optional<std::string> scalar(gsexp::FormView form, std::string_view key) {
    gsexp::Node value = form.find_arg(key, 0);
    if (!value.valid() || value.is_list())
        return std::nullopt;
    return std::string(value.text());
}

bool boolean(gsexp::FormView form, std::string_view key, bool fallback) {
    const std::optional<std::string> value = scalar(form, key);
    if (!value)
        return fallback;
    return *value == "true" || *value == "yes" || *value == "1" || *value == "on";
}

Length parse_length(gsexp::Node value, Length fallback = {}) {
    if (!value.valid())
        return fallback;
    if (!value.is_list())
        return Length{length_kind_from_string(value.text()), 0.0f};
    gsexp::FormView form(value);
    gsexp::Node kind = form.arg(0);
    gsexp::Node amount = form.arg(1);
    Length result{kind.valid() ? length_kind_from_string(kind.text()) : LengthKind::Auto, 0.0f};
    if (amount.valid() && gsexp::looks_like_float(amount.text()))
        result.value = std::stof(std::string(amount.text()));
    return result;
}

Insets parse_insets(gsexp::Node node) {
    if (!node.valid())
        return {};
    gsexp::FormView form(node);
    return Insets{form.get_float("left").value_or(0.0f), form.get_float("top").value_or(0.0f),
                  form.get_float("right").value_or(0.0f),
                  form.get_float("bottom").value_or(0.0f)};
}

Rect parse_rect(gsexp::Node node) {
    if (!node.valid())
        return Rect{0.0f, 0.0f, 1.0f, 1.0f};
    gsexp::FormView form(node);
    return Rect{form.get_float("x").value_or(0.0f), form.get_float("y").value_or(0.0f),
                form.get_float("w").value_or(1.0f), form.get_float("h").value_or(1.0f)};
}

bool parse_node(gsexp::Node source, GraphNode& output, GraphParseResult& result) {
    gsexp::FormView form(source);
    const std::optional<std::string> id = form.get_string("id");
    if (!id) {
        add_warning(result, "skipping graph node without an id");
        return false;
    }
    output.id = *id;
    output.label = form.get_string("label").value_or("");
    output.measure_key = form.get_string("measure").value_or("");
    output.container = container_kind_from_string(scalar(form, "container").value_or("absolute"));
    output.padding = parse_insets(form.find("padding"));
    output.absolute_rect = parse_rect(form.find("rect"));
    output.gap = form.get_float("gap").value_or(0.0f);
    output.columns = form.get_int("columns").value_or(1);
    output.align = align_from_string(scalar(form, "align").value_or("stretch"));
    output.distribution =
        distribution_from_string(scalar(form, "distribution").value_or("start"));
    output.clip = boolean(form, "clip", false);
    output.visible = boolean(form, "visible", true);
    output.mask = form.get_string("mask").value_or("");

    gsexp::Node size_node = form.find("size");
    if (size_node.valid()) {
        gsexp::FormView size(size_node);
        output.size.width = parse_length(size.find("width"));
        output.size.height = parse_length(size.find("height"));
        output.size.min_width = size.get_float("min_width").value_or(0.0f);
        output.size.min_height = size.get_float("min_height").value_or(0.0f);
        output.size.max_width = size.get_float("max_width").value_or(output.size.max_width);
        output.size.max_height = size.get_float("max_height").value_or(output.size.max_height);
        output.size.aspect_ratio = size.get_float("aspect").value_or(0.0f);
    }

    gsexp::Node anchors = form.find("anchors");
    if (anchors.valid()) {
        for (gsexp::Node entry : anchors.children()) {
            if (!entry.is_list() || !entry.head().is_atom("anchor"))
                continue;
            gsexp::FormView anchor(entry);
            output.anchors.push_back(AnchorRule{
                edge_from_string(scalar(anchor, "own").value_or("left")),
                anchor.get_string("target").value_or(""),
                edge_from_string(scalar(anchor, "edge").value_or("left")),
                anchor.get_float("offset").value_or(0.0f),
            });
        }
    }

    gsexp::Node children = form.find("children");
    if (children.valid()) {
        for (gsexp::Node entry : children.children()) {
            if (!entry.is_list() || !entry.head().is_atom("node"))
                continue;
            GraphNode child;
            if (parse_node(entry, child, result))
                output.children.push_back(std::move(child));
        }
    }
    return true;
}

bool parse_graph(gsexp::Node source, GraphLayout& output, GraphParseResult& result) {
    gsexp::FormView form(source);
    const std::optional<std::string> id = form.get_string("id");
    gsexp::Node root = form.find("root");
    if (!id || !root.valid() || root.child_count() < 2) {
        add_warning(result, "skipping malformed graph");
        return false;
    }
    output.id = *id;
    output.label = form.get_string("label").value_or("");
    output.width = form.get_int("width").value_or(0);
    output.height = form.get_int("height").value_or(0);
    output.dpi_scale = form.get_float("dpi").value_or(1.0f);
    output.form_factor =
        form_factor_from_string(scalar(form, "form_factor").value_or("desktop"));
    return parse_node(root.child_at(1), output.root, result);
}

void write_length(std::ostringstream& out, std::string_view name, Length length) {
    out << '(' << name << ' ' << to_string(length.kind);
    if (length.kind == LengthKind::Pixels || length.kind == LengthKind::Percent ||
        length.kind == LengthKind::Fill)
        out << ' ' << length.value;
    out << ')';
}

void write_node(std::ostringstream& out, const GraphNode& node, int depth) {
    const std::string pad(static_cast<std::size_t>(depth * 2), ' ');
    out << pad << "(node\n" << pad << "  (id " << gsexp::quote_string(node.id) << ")\n";
    if (!node.label.empty()) out << pad << "  (label " << gsexp::quote_string(node.label) << ")\n";
    if (!node.measure_key.empty())
        out << pad << "  (measure " << gsexp::quote_string(node.measure_key) << ")\n";
    out << pad << "  (container " << to_string(node.container) << ")\n";
    out << pad << "  (size ";
    write_length(out, "width", node.size.width);
    out << ' ';
    write_length(out, "height", node.size.height);
    out << " (min_width " << node.size.min_width << ") (min_height " << node.size.min_height
        << ") (max_width " << node.size.max_width << ") (max_height " << node.size.max_height
        << ") (aspect " << node.size.aspect_ratio << "))\n";
    out << pad << "  (padding (left " << node.padding.left << ") (top " << node.padding.top
        << ") (right " << node.padding.right << ") (bottom " << node.padding.bottom << "))\n";
    out << pad << "  (rect (x " << node.absolute_rect.x << ") (y " << node.absolute_rect.y
        << ") (w " << node.absolute_rect.w << ") (h " << node.absolute_rect.h << "))\n";
    out << pad << "  (gap " << node.gap << ") (columns " << node.columns << ") (align "
        << to_string(node.align) << ") (distribution " << to_string(node.distribution) << ")\n";
    out << pad << "  (clip " << (node.clip ? "true" : "false") << ") (visible "
        << (node.visible ? "true" : "false") << ")\n";
    if (!node.mask.empty()) out << pad << "  (mask " << gsexp::quote_string(node.mask) << ")\n";
    if (!node.anchors.empty()) {
        out << pad << "  (anchors\n";
        for (const AnchorRule& anchor : node.anchors)
            out << pad << "    (anchor (own " << to_string(anchor.own_edge) << ") (target "
                << gsexp::quote_string(anchor.target) << ") (edge " << to_string(anchor.target_edge)
                << ") (offset " << anchor.offset << "))\n";
        out << pad << "  )\n";
    }
    if (!node.children.empty()) {
        out << pad << "  (children\n";
        for (const GraphNode& child : node.children) write_node(out, child, depth + 2);
        out << pad << "  )\n";
    }
    out << pad << ")\n";
}

} // namespace

// Parses graph variants while retaining useful diagnostics for live editors.
GraphParseResult parse_graphs(std::string_view text) {
    GraphParseResult result;
    const gsexp::ParseResult parsed = gsexp::parse(text);
    for (const gsexp::Diagnostic& diagnostic : parsed.diagnostics) {
        result.diagnostics.push_back(Diagnostic{
            diagnostic.severity == gsexp::DiagnosticSeverity::Warning
                ? DiagnosticSeverity::Warning
                : DiagnosticSeverity::Error,
            diagnostic.message, diagnostic.line, diagnostic.column});
    }
    if (!parsed.ok)
        return result;
    gsexp::Node root;
    for (std::size_t index = 0; index < parsed.root_count(); ++index) {
        if (parsed.root(index).is_list() && parsed.root(index).head().is_atom("ui_graphs")) {
            root = parsed.root(index);
            break;
        }
    }
    if (!root.valid()) {
        add_error(result, "missing ui_graphs root");
        return result;
    }
    for (gsexp::Node entry : root.children()) {
        if (!entry.is_list() || !entry.head().is_atom("graph"))
            continue;
        GraphLayout layout;
        if (parse_graph(entry, layout, result))
            result.layouts.push_back(std::move(layout));
    }
    result.ok = !has_errors(result);
    return result;
}

std::string write_graphs(const std::vector<GraphLayout>& layouts) {
    std::ostringstream out;
    out << "(ui_graphs\n";
    for (const GraphLayout& layout : layouts) {
        out << "  (graph\n    (id " << gsexp::quote_string(layout.id) << ")\n    (label "
            << gsexp::quote_string(layout.label) << ")\n    (width " << layout.width
            << ") (height " << layout.height << ") (dpi " << layout.dpi_scale
            << ") (form_factor " << to_string(layout.form_factor) << ")\n    (root\n";
        write_node(out, layout.root, 3);
        out << "    )\n  )\n";
    }
    out << ")\n";
    return out.str();
}

GraphParseResult load_graph_file(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        GraphParseResult result;
        add_error(result, "failed to open graph file: " + path.string());
        return result;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return parse_graphs(buffer.str());
}

bool save_graph_file(const std::filesystem::path& path, const std::vector<GraphLayout>& layouts) {
    std::error_code error;
    if (path.has_parent_path())
        std::filesystem::create_directories(path.parent_path(), error);
    if (error)
        return false;
    std::ofstream file(path);
    file << write_graphs(layouts);
    return file.good();
}

} // namespace glayout
