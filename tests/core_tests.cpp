#include "glayout/layout.hpp"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool nearly_equal(float a, float b) {
    return std::fabs(a - b) < 0.0001f;
}

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << "\n";
        std::exit(1);
    }
}

void test_rect_mapping() {
    glayout::Rect parent{10.0f, 20.0f, 200.0f, 100.0f};
    glayout::Rect child{0.25f, 0.5f, 0.5f, 0.25f};
    glayout::Rect mapped = glayout::map_rect(parent, child);

    require(nearly_equal(mapped.x, 60.0f), "map_rect x");
    require(nearly_equal(mapped.y, 70.0f), "map_rect y");
    require(nearly_equal(mapped.w, 100.0f), "map_rect w");
    require(nearly_equal(mapped.h, 25.0f), "map_rect h");

    glayout::Rect clipped = glayout::intersection(glayout::Rect{0.0f, 0.0f, 10.0f, 10.0f},
                                                  glayout::Rect{5.0f, 5.0f, 10.0f, 10.0f});
    require(glayout::intersects(glayout::Rect{0.0f, 0.0f, 10.0f, 10.0f},
                                glayout::Rect{5.0f, 5.0f, 10.0f, 10.0f}),
            "intersects overlapping rects");
    require(nearly_equal(clipped.x, 5.0f), "intersection x");
    require(nearly_equal(clipped.y, 5.0f), "intersection y");
    require(nearly_equal(clipped.w, 5.0f), "intersection w");
    require(nearly_equal(clipped.h, 5.0f), "intersection h");
}

void test_matching() {
    std::vector<glayout::Layout> layouts{
        glayout::Layout{100, "Title", 1280, 720, glayout::FormFactor::Desktop, {}},
        glayout::Layout{100, "Title", 1920, 1080, glayout::FormFactor::Desktop, {}},
        glayout::Layout{100, "Title", 1080, 1920, glayout::FormFactor::Phone, {}},
    };

    const glayout::Layout* desktop =
        glayout::find_best_layout(layouts, 100, 1920, 1080, glayout::FormFactor::Desktop);
    require(desktop != nullptr, "desktop match exists");
    require(desktop->width == 1920 && desktop->height == 1080, "desktop picks exact 1080p");

    const glayout::Layout* phone =
        glayout::find_best_layout(layouts, 100, 1080, 1920, glayout::FormFactor::Phone);
    require(phone != nullptr, "phone match exists");
    require(phone->form_factor == glayout::FormFactor::Phone, "phone form factor preferred");

    const glayout::Layout* fallback =
        glayout::find_best_layout(layouts, 100, 1920, 1080, glayout::FormFactor::Tablet);
    require(fallback != nullptr, "tablet fallback exists");
    require(fallback->width == 1920 && fallback->height == 1080, "tablet fallback uses best score");
}

void test_parse_write() {
    const std::string text = R"(
(ui_layouts
  (layout
    (id 100)
    (label "Title")
    (resolution (width 1920) (height 1080))
    (form_factor desktop)
    (objects
      (object (id 1) (label "play_button") (x 0.4) (y 0.45) (w 0.2) (h 0.08))
    )
  )
  (layout
    (id 200)
    (label "Phone")
    (resolution (width 1080) (height 1920))
    (form_factor weird)
    (objects)
  )
  (layout
    (id 100)
    (label "Title Duplicate")
    (resolution (width 1920) (height 1080))
    (form_factor desktop)
    (objects)
  )
)
)";

    glayout::ParseResult result = glayout::parse_layouts(text);
    require(result.ok, "parse_layouts ok with warnings");
    require(result.layouts.size() == 3, "parse_layouts layout count");
    require(result.layouts[0].objects.size() == 1, "parse_layouts object count");
    require(result.layouts[0].objects[0].label == "play_button", "parse_layouts object label");
    require(result.layouts[1].form_factor == glayout::FormFactor::Desktop,
            "unknown form factor defaults desktop");
    require(!result.diagnostics.empty(), "warnings are reported");

    std::string written = glayout::write_layouts(result.layouts);
    glayout::ParseResult round_trip = glayout::parse_layouts(written);
    require(round_trip.ok, "round-trip parse ok");
    require(round_trip.layouts.size() == result.layouts.size(), "round-trip layout count");
}

void test_replace_helpers() {
    glayout::Layout layout;
    layout.id = 1;
    layout.label = "A";
    layout.width = 100;
    layout.height = 100;

    glayout::Object a{10, "first", glayout::Rect{0.0f, 0.0f, 0.1f, 0.1f}};
    glayout::Object b{10, "second", glayout::Rect{0.5f, 0.5f, 0.1f, 0.1f}};
    glayout::add_or_replace_object(layout, a);
    glayout::add_or_replace_object(layout, b);

    require(layout.objects.size() == 1, "object replace by id");
    require(layout.objects[0].label == "second", "object replace label");

    std::vector<glayout::Layout> layouts;
    glayout::add_or_replace_layout(layouts, layout);
    layout.label = "B";
    glayout::add_or_replace_layout(layouts, layout);

    require(layouts.size() == 1, "layout replace by variant key");
    require(layouts[0].label == "B", "layout replace label");
}

} // namespace

int main() {
    test_rect_mapping();
    test_matching();
    test_parse_write();
    test_replace_helpers();

    std::cout << "glayout_core_tests passed\n";
    return 0;
}
