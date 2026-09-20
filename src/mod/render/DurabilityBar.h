#pragma once

#include "mod/render/PreviewLayout.h"

// Pure durability-overlay math for the preview grid. Deliberately free of
// game types so it can be unit-tested without LeviLamina or the game.
//
// The bar mirrors the vanilla slot durability overlay: a dark background
// strip anchored to the bottom of the 16x16 icon with a coloured foreground
// whose width tracks remaining durability (green -> yellow -> red, the same
// hue ramp vanilla uses: full = green, half = yellow, empty = red).
namespace lamina_peek::render {

/// Remaining durability in [0, 1]: 1 = undamaged, 0 = no uses left.
/// Non-positive maxDamage cannot happen for a damageable item; treat it as
/// full so no caller can divide by zero.
[[nodiscard]] constexpr float durabilityRatio(int damageValue, int maxDamage) {
    if (maxDamage <= 0) {
        return 1.0f;
    }
    float const ratio = static_cast<float>(maxDamage - damageValue) / static_cast<float>(maxDamage);
    if (ratio <= 0.0f) {
        return 0.0f;
    }
    if (ratio >= 1.0f) {
        return 1.0f;
    }
    return ratio;
}

/// Vanilla shows the bar only once the item has taken damage. Undamaged
/// items (and non-damageables) get no bar.
[[nodiscard]] constexpr bool shouldShowDurabilityBar(bool damageable, int damageValue, int maxDamage) {
    return damageable && maxDamage > 0 && damageValue > 0;
}

struct DurabilityRgb {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
};

/// Vanilla hue ramp: hue = ratio / 3 (red at 0, green at full), full
/// saturation and value. Implemented directly so the result is constexpr.
[[nodiscard]] constexpr DurabilityRgb durabilityColor(float ratio) {
    float h = ratio / 3.0f;
    if (h <= 0.0f) {
        return DurabilityRgb{1.0f, 0.0f, 0.0f};
    }
    if (h >= 1.0f / 3.0f) {
        return DurabilityRgb{0.0f, 1.0f, 0.0f};
    }
    float const h6 = h * 6.0f;
    int const   i  = static_cast<int>(h6);
    float const f  = h6 - static_cast<float>(i);
    // s = v = 1, so p = 0, q = 1 - f, t = f.
    if (i == 0) {
        return DurabilityRgb{1.0f, f, 0.0f};
    }
    if (i == 1) {
        return DurabilityRgb{1.0f - f, 1.0f, 0.0f};
    }
    return DurabilityRgb{0.0f, 1.0f, f};
}

// Bar shape, in the same GUI units as PreviewLayout: 2 units tall with a
// 1-unit margin on the left, right and bottom of the icon, matching the
// vanilla overlay which sits at the bottom of the item sprite.
constexpr float kDurabilityBarHeight      = 2.0f;
constexpr float kDurabilityBarSideMargin  = 1.0f;
constexpr float kDurabilityBarBottomMargin = 1.0f;
constexpr float kDurabilityBarMinWidth    = 1.0f; // nearly-broken items keep a visible red sliver

/// Full-width dark strip the coloured bar is drawn over.
[[nodiscard]] constexpr Rect durabilityBackground(Rect icon) {
    return Rect{
        icon.x0 + kDurabilityBarSideMargin,
        icon.y1 - kDurabilityBarBottomMargin - kDurabilityBarHeight,
        icon.x1 - kDurabilityBarSideMargin,
        icon.y1 - kDurabilityBarBottomMargin
    };
}

/// Left-aligned coloured portion for `ratio` remaining durability. A
/// positive ratio always yields at least a sliver so a nearly-broken item
/// stays distinguishable from one with no uses left.
[[nodiscard]] constexpr Rect durabilityForeground(Rect background, float ratio) {
    if (ratio <= 0.0f) {
        return Rect{background.x0, background.y0, background.x0, background.y1};
    }
    float width = background.width() * ratio;
    if (width < kDurabilityBarMinWidth) {
        width = kDurabilityBarMinWidth;
    }
    if (width > background.width()) {
        width = background.width();
    }
    return Rect{background.x0, background.y0, background.x0 + width, background.y1};
}

} // namespace lamina_peek::render
