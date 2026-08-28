#pragma once

#include "glayout/graph_types.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace glayout {

struct GraphParseResult {
    bool ok = false;
    std::vector<GraphLayout> layouts;
    std::vector<Diagnostic> diagnostics;
};

GraphParseResult parse_graphs(std::string_view text);
std::string write_graphs(const std::vector<GraphLayout>& layouts);
GraphParseResult load_graph_file(const std::filesystem::path& path);
bool save_graph_file(const std::filesystem::path& path,
                     const std::vector<GraphLayout>& layouts);

} // namespace glayout
