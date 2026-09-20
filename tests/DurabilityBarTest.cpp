// Focused checks for the pure durability-overlay math used by the preview.
// Built with `xmake build LaminaPeekTests`, run with `xmake run LaminaPeekTests`.

#include "mod/render/DurabilityBar.h"

#include <cmath>
#include <cstdio>

using lamina_peek::render::DurabilityRgb;
using lamina_peek::render::Rect;
using lamina_peek::render::durabilityBackground;
using lamina_peek::render::durabilityColor;
using lamina_peek::render::durabilityForeground;
using lamina_peek::render::durabilityRatio;
using lamina_peek::render::shouldShowDurabilityBar;

namespace {

int gFailures = 0;

bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

bool nearRgb(DurabilityRgb const& actual, float r, float g, float b) {
    return near(actual.r, r) && near(actual.g, g) && near(actual.b, b);
}

void check(bool ok, char const* what, int line) {
    if (!ok) {
        ++gFailures;
        std::printf("FAIL (line %d): %s\n", line, what);
    }
}

#define CHECK(expr) check((expr), #expr, __LINE__)

void testRatioTracksRemainingDurability() {
    // A diamond pickaxe (max 1561): undamaged is full, half-used is half,
    // one use left is nearly zero, fully used is zero.
    CHECK(near(durabilityRatio(0, 1561), 1.0f));
    CHECK(near(durabilityRatio(781, 1562), 0.5f));
    CHECK(durabilityRatio(1560, 1561) > 0.0f);
    CHECK(near(durabilityRatio(1561, 1561), 0.0f));
}

void testRatioClampsAndGuards() {
    CHECK(near(durabilityRatio(-5, 100), 1.0f));   // over-repaired clamps to full
    CHECK(near(durabilityRatio(150, 100), 0.0f));  // over-damaged clamps to empty
    CHECK(near(durabilityRatio(5, 0), 1.0f));      // degenerate max: no bar path divides by zero
    CHECK(near(durabilityRatio(5, -10), 1.0f));
}

void testVisibilityMatchesVanillaRule() {
    CHECK(!shouldShowDurabilityBar(true, 0, 1561));    // undamaged: no bar
    CHECK(shouldShowDurabilityBar(true, 1, 1561));     // first hit: bar appears
    CHECK(shouldShowDurabilityBar(true, 1561, 1561));  // fully used: bar still shown
    CHECK(!shouldShowDurabilityBar(false, 0, 0));      // non-damageable (dirt): no bar
    CHECK(!shouldShowDurabilityBar(false, 10, 0));     // damage value without max: no bar
    CHECK(!shouldShowDurabilityBar(true, 5, 0));       // degenerate max: no bar
}

void testColorRampIsGreenYellowRed() {
    CHECK(nearRgb(durabilityColor(1.0f), 0.0f, 1.0f, 0.0f)); // full: green
    CHECK(nearRgb(durabilityColor(0.5f), 1.0f, 1.0f, 0.0f)); // half: yellow
    CHECK(nearRgb(durabilityColor(0.0f), 1.0f, 0.0f, 0.0f)); // empty: red
    // Nearly-broken must stay red-dominant (distinguishable warning color).
    DurabilityRgb const low = durabilityColor(0.05f);
    CHECK(low.r > 0.9f && low.g < 0.4f && near(low.b, 0.0f));
}

void testBarGeometrySitsAtIconBottom() {
    Rect const icon{10.0f, 20.0f, 26.0f, 36.0f}; // 16x16 icon
    Rect const bg = durabilityBackground(icon);
    CHECK(near(bg.height(), 2.0f));                             // 2 units tall
    CHECK(near(bg.x0, icon.x0 + 1.0f) && near(bg.x1, icon.x1 - 1.0f)); // 1-unit side margins
    CHECK(near(bg.y1, icon.y1 - 1.0f));                         // 1 unit above the icon bottom
    CHECK(near(bg.width(), 14.0f));
}

void testForegroundWidthTracksRatio() {
    Rect const bg{0.0f, 0.0f, 14.0f, 2.0f};
    Rect const full = durabilityForeground(bg, 1.0f);
    CHECK(near(full.width(), bg.width())); // undamaged width == full strip (visibility gates this path)
    Rect const half = durabilityForeground(bg, 0.5f);
    CHECK(near(half.width(), bg.width() * 0.5f));
    CHECK(near(half.x0, bg.x0)); // left-aligned
    CHECK(near(half.y0, bg.y0) && near(half.y1, bg.y1));
    // A nearly-broken item keeps a visible sliver instead of vanishing.
    Rect const sliver = durabilityForeground(bg, 0.001f);
    CHECK(near(sliver.width(), 1.0f));
    CHECK(near(sliver.x0, bg.x0));
    // Zero remaining collapses to nothing (vanilla shows only the dark strip).
    Rect const gone = durabilityForeground(bg, 0.0f);
    CHECK(near(gone.width(), 0.0f));
    CHECK(near(gone.x0, bg.x0));
}

} // namespace

int runDurabilityBarTests() {
    testRatioTracksRemainingDurability();
    testRatioClampsAndGuards();
    testVisibilityMatchesVanillaRule();
    testColorRampIsGreenYellowRed();
    testBarGeometrySitsAtIconBottom();
    testForegroundWidthTracksRatio();
    if (gFailures == 0) {
        std::printf("DurabilityBar tests: all passed\n");
    } else {
        std::printf("DurabilityBar tests: %d failure(s)\n", gFailures);
    }
    return gFailures;
}
