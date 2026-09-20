// Focused checks for the pure layout math used by the preview overlay.
// Built with `xmake build LaminaPeekTests`, run with `xmake run LaminaPeekTests`.

#include "mod/render/PreviewLayout.h"

#include <cmath>
#include <cstdio>

using lamina_peek::render::PreviewLayout;
using lamina_peek::render::Rect;

namespace {

int gFailures = 0;

bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

void check(bool ok, char const* what, int line) {
    if (!ok) {
        ++gFailures;
        std::printf("FAIL (line %d): %s\n", line, what);
    }
}

#define CHECK(expr) check((expr), #expr, __LINE__)
#define CHECK_RECT(rect, ex0, ey0, ex1, ey1)                                                                           \
    check(                                                                                                             \
        near((rect).x0, (ex0)) && near((rect).y0, (ey0)) && near((rect).x1, (ex1)) && near((rect).y1, (ey1)),          \
        #rect " == {" #ex0 ", " #ey0 ", " #ex1 ", " #ey1 "}",                                                          \
        __LINE__                                                                                                       \
    )

void testFrameSize() {
    CHECK(near(PreviewLayout::frameWidth(9), 9 * 18.0f + 8.0f));
    CHECK(near(PreviewLayout::frameHeight(3), 3 * 18.0f + 8.0f));
}

void testAnchorsAboveRightWhenThereIsRoom() {
    auto const layout = PreviewLayout::anchored(9, 3, 100.0f, 200.0f, 400.0f, 300.0f);
    // 8 units right of the pointer, and 8 units above it (bottom edge at 192).
    CHECK_RECT(layout.frame, 108.0f, 130.0f, 278.0f, 192.0f);
}

void testFallsBelowWhenNoRoomAbove() {
    auto const layout = PreviewLayout::anchored(9, 3, 100.0f, 30.0f, 400.0f, 300.0f);
    CHECK_RECT(layout.frame, 108.0f, 38.0f, 278.0f, 100.0f);
}

void testClampsToRightEdge() {
    auto const layout = PreviewLayout::anchored(9, 3, 390.0f, 200.0f, 400.0f, 300.0f);
    CHECK(near(layout.frame.x1, 400.0f));
    CHECK(near(layout.frame.x0, 230.0f));
}

void testClampsToLeftEdgeWhenScreenIsNarrow() {
    auto const layout = PreviewLayout::anchored(9, 3, 50.0f, 200.0f, 100.0f, 300.0f);
    CHECK(near(layout.frame.x0, 0.0f));
}

void testClampsToBottomEdge() {
    // No room above, and placing below would overflow: pin to the bottom.
    auto const layout = PreviewLayout::anchored(9, 3, 100.0f, 50.0f, 400.0f, 100.0f);
    CHECK(near(layout.frame.y1, 100.0f));
    CHECK(near(layout.frame.y0, 38.0f));
}

void testCellsFollowRowMajorSlotOrder() {
    auto const  layout = PreviewLayout::anchored(9, 3, 0.0f, 300.0f, 1000.0f, 1000.0f);
    float const x0     = layout.frame.x0 + PreviewLayout::kPadding;
    float const y0     = layout.frame.y0 + PreviewLayout::kPadding;

    CHECK_RECT(layout.cell(0), x0, y0, x0 + 18.0f, y0 + 18.0f);
    CHECK_RECT(layout.cell(10), x0 + 18.0f, y0 + 18.0f, x0 + 36.0f, y0 + 36.0f); // row 1, column 1
    CHECK_RECT(layout.cell(26), x0 + 8 * 18.0f, y0 + 2 * 18.0f, x0 + 9 * 18.0f, y0 + 3 * 18.0f);

    // The grid exactly fills the frame minus padding.
    CHECK(near(layout.cell(26).x1 + PreviewLayout::kPadding, layout.frame.x1));
    CHECK(near(layout.cell(26).y1 + PreviewLayout::kPadding, layout.frame.y1));
}

void testIconIsCentredInCell() {
    auto const layout = PreviewLayout::anchored(9, 3, 0.0f, 300.0f, 1000.0f, 1000.0f);
    Rect const cell   = layout.cell(5);
    Rect const icon   = layout.icon(5);
    CHECK(near(icon.width(), 16.0f));
    CHECK(near(icon.height(), 16.0f));
    CHECK(near(icon.x0 - cell.x0, 1.0f));
    CHECK(near(icon.y0 - cell.y0, 1.0f));
    CHECK(near(cell.x1 - icon.x1, 1.0f));
    CHECK(near(cell.y1 - icon.y1, 1.0f));
}

} // namespace

int main() {
    testFrameSize();
    testAnchorsAboveRightWhenThereIsRoom();
    testFallsBelowWhenNoRoomAbove();
    testClampsToRightEdge();
    testClampsToLeftEdgeWhenScreenIsNarrow();
    testClampsToBottomEdge();
    testCellsFollowRowMajorSlotOrder();
    testIconIsCentredInCell();

    if (gFailures == 0) {
        std::printf("PreviewLayout tests: all passed\n");
        return 0;
    }
    std::printf("PreviewLayout tests: %d failure(s)\n", gFailures);
    return 1;
}
