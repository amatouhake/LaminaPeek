#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace lamina_peek::preview {

// Pure, game-independent Bundle helpers: item-family recognition, content
// fingerprinting and dynamic-grid layout. Deliberately free of game types so
// the unit tests exercise the production functions directly.

/// True when `typeName` is a Bundle of any colour: exactly "minecraft:bundle"
/// or "<namespace>:<colour>_bundle" ("undyed_bundle" included). A plain
/// `ends_with("bundle")` would also match unrelated future items, so the '_'
/// (or ':') boundary before "bundle" is required.
[[nodiscard]] inline bool isBundleTypeName(std::string const& typeName) {
    constexpr std::string_view kSuffix = "bundle";
    if (!typeName.ends_with(kSuffix)) {
        return false;
    }
    if (typeName.size() == kSuffix.size()) {
        return true;
    }
    char const boundary = typeName[typeName.size() - kSuffix.size() - 1];
    return boundary == '_' || boundary == ':';
}

/// Bounded dynamic grid for the packed Bundle entries.
///
/// Shulker Boxes mirror the real container (fixed 9x3 with gaps); Bundles
/// store a packed list instead, so the grid is derived from the filled count:
/// rows grow only as needed, columns widen for large Bundles (8x8 at 64
/// entries still fits an ordinary full-HD inventory screen at vanilla GUI
/// sizes), and the heuristic prefers wider shapes (fewer rows) so the preview
/// stays short.
struct BundleGrid {
    /// Maximum preview slots. Bedrock storage items support up to 64 dynamic
    /// slots (`max_slots: 64` in the vanilla Bundle definition); every real
    /// non-empty entry is retained, never discarded.
    static constexpr int kMaxSlots = 64;
    /// Narrowest grid: keeps single entries readable instead of a 1-wide strip.
    static constexpr int kMinColumns = 3;
    /// Widest grid: 8 columns of 18-unit cells plus padding is 152 GUI units.
    /// Ordinary full-HD inventory screens are ~300+ units wide, so even a
    /// full 8x8 grid fits; `PreviewLayout::anchored` flips/clamps at edges.
    static constexpr int kMaxColumns = 8;
    /// Empty Bundle frame (drawn only when `bundle.showEmpty` is set): the
    /// narrowest grid, one row, so an empty Bundle is unobtrusive.
    static constexpr int kEmptyColumns = 3;
    static constexpr int kEmptyRows    = 1;

    int columns{0};
    int rows{0};

    /// Smallest grid holding `filled` entries within the column band, wider
    /// shapes preferred: columns grow before rows do. `filled <= 0` yields an
    /// empty (0x0) shape; the caller decides whether to draw it.
    [[nodiscard]] static constexpr BundleGrid shapeFor(int filled) {
        if (filled <= 0) {
            return BundleGrid{0, 0};
        }
        if (filled > kMaxSlots) {
            filled = kMaxSlots;
        }
        for (int columns = kMinColumns; columns <= kMaxColumns; ++columns) {
            if (filled <= columns * columns) {
                int const rows = (filled + columns - 1) / columns;
                return BundleGrid{columns, rows};
            }
        }
        // More entries than an 8x8 square: keep 8 columns and add rows.
        int const rows = (filled + kMaxColumns - 1) / kMaxColumns;
        return BundleGrid{kMaxColumns, rows};
    }
};

/// Cheap content fingerprint for cache invalidation: FNV-1a over the
/// authoritative per-entry bytes (slot index, item id/aux/count and the
/// entry's NBT hash). Order-sensitive (slot order is part of the preview),
/// allocation-free, and game-independent so tests pin it directly.
[[nodiscard]] inline uint64_t fingerprintBundleEntries(
    uint64_t seed,
    int      slot,
    short    id,
    short    aux,
    uint8_t  count,
    uint64_t entryHash
) {
    // FNV-1a 64: mix each field byte-wise. `seed` chains entries; start from
    // the FNV offset basis for the first entry.
    uint64_t hash = seed == 0 ? 14695981039346656037ULL : seed;
    auto     mix  = [&hash](uint64_t value, int bytes) {
        for (int i = 0; i < bytes; ++i) {
            hash ^= static_cast<uint8_t>(value >> (i * 8));
            hash *= 1099511628211ULL;
        }
    };
    mix(static_cast<uint64_t>(static_cast<uint32_t>(slot)), 4);
    mix(static_cast<uint64_t>(static_cast<uint16_t>(id)), 2);
    mix(static_cast<uint64_t>(static_cast<uint16_t>(aux)), 2);
    mix(static_cast<uint64_t>(count), 1);
    mix(entryHash, 8);
    return hash;
}

} // namespace lamina_peek::preview
