#pragma once

#include "glayout/graph_compile.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace glayout {

struct MeasureRequest {
    std::string_view key;
    float available_width = 0.0f;
    float available_height = 0.0f;
};

using MeasureFunction = Size (*)(const MeasureRequest& request, void* user_data);

struct ResolveInput {
    Rect viewport;
    Insets safe_area;
    MeasureFunction measure = nullptr;
    void* measure_user_data = nullptr;
};

struct ResolvedNode {
    Rect border;
    Rect content;
    Rect clip;
    Size content_extent;
    bool visible = false;
};

struct ResolveStats {
    std::uint64_t generation = 0;
    std::uint64_t layout_passes = 0;
    std::uint64_t clean_reuses = 0;
    std::uint32_t resolved_nodes = 0;
};

class GraphRuntime {
  public:
    GraphRuntime() = default;
    explicit GraphRuntime(CompiledGraph graph);

    void reset(CompiledGraph graph);
    void invalidate();
    bool resolve(const ResolveInput& input);

    const CompiledGraph& graph() const;
    const std::vector<ResolvedNode>& nodes() const;
    const ResolvedNode* find(std::string_view id) const;
    const ResolveStats& stats() const;

  private:
    CompiledGraph graph_;
    std::vector<ResolvedNode> resolved_;
    ResolveInput previous_input_;
    ResolveStats stats_;
    bool dirty_ = true;
    bool has_previous_input_ = false;
};

} // namespace glayout
