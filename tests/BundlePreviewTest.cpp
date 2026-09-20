// Focused checks for the Bundle preview: provider recognition/extraction and
// the bounded dynamic grid shape. Game-independent: BundleGrid is constexpr
// and the pure name/shape logic is re-checked here without LeviLamina.
// Built with `xmake build LaminaPeekTests`, run with `xmake run LaminaPeekTests`.

#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>
namespace {

// Mirror of BundlePreviewProvider::isBundleTypeName (kept in sync by hand;
// the real one needs ItemStackBase). Any change to the matching rule must
// update both copies and these cases.
bool isBundleTypeName(std::string const& typeName) {
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

// Mirror of BundleGrid::shapeFor (constexpr, same algorithm).
struct Grid {
    int columns;
    int rows;
};

constexpr int kMaxSlots     = 16;
constexpr int kMinColumns   = 3;
constexpr int kMaxColumns   = 4;
constexpr int kEmptyColumns = 3;
constexpr int kEmptyRows    = 1;

// Empty Bundles report the minimal 3x1 frame (the render layer decides
// whether to draw it via `bundle.showEmpty`); this mirrors the provider.
constexpr Grid emptyShape() { return {kEmptyColumns, kEmptyRows}; }

constexpr Grid shapeFor(int filled) {
    if (filled <= 0) {
        return {0, 0};
    }
    if (filled > kMaxSlots) {
        filled = kMaxSlots;
    }
    for (int columns = kMinColumns; columns <= kMaxColumns; ++columns) {
        if (filled <= columns * columns) {
            return {columns, (filled + columns - 1) / columns};
        }
    }
    return {kMaxColumns, (filled + kMaxColumns - 1) / kMaxColumns};
}

int gFailures = 0;

void check(bool ok, char const* what, int line) {
    if (!ok) {
        ++gFailures;
        std::printf("FAIL line %d: %s\n", line, what);
    }
}

#define CHECK(expr) check((expr), #expr, __LINE__)

void testSupportsEveryBundleColour() {
    CHECK(isBundleTypeName("minecraft:bundle"));
    CHECK(isBundleTypeName("minecraft:black_bundle"));
    CHECK(isBundleTypeName("minecraft:white_bundle"));
    CHECK(isBundleTypeName("minecraft:undyed_bundle"));
    CHECK(isBundleTypeName("minecraft:light_blue_bundle"));
}

void testRejectsNonBundles() {
    CHECK(!isBundleTypeName("minecraft:shulker_box"));
    CHECK(!isBundleTypeName("minecraft:black_shulker_box"));
    CHECK(!isBundleTypeName("minecraft:stone"));
    CHECK(!isBundleTypeName(""));
    // A plain ends_with("bundle") would wrongly accept these.
    CHECK(!isBundleTypeName("minecraft:notabundle"));
    CHECK(!isBundleTypeName("minecraft:bundlelike"));
}

void testEmptyBundleReportsMinimalFrame() {
    // shapeFor(<=0) stays 0x0 (nothing to pack); the provider reports the
    // minimal 3x1 frame for empty Bundles so `bundle.showEmpty` draws it.
    Grid const g = shapeFor(0);
    CHECK(g.columns == 0 && g.rows == 0);
    Grid const neg = shapeFor(-3);
    CHECK(neg.columns == 0 && neg.rows == 0);
    Grid const empty = emptyShape();
    CHECK(empty.columns == kEmptyColumns && empty.rows == kEmptyRows);
}

void testSingleEntryStaysReadable() {
    Grid const g = shapeFor(1);
    CHECK(g.columns == 3 && g.rows == 1);
}

void testPrefersWiderShapes() {
    // 4 entries fit 3x2 (6 cells) as well as 2x2, but the wider 3-column
    // shape keeps the preview short.
    Grid const four = shapeFor(4);
    CHECK(four.columns == 3 && four.rows == 2);
    Grid const nine = shapeFor(9);
    CHECK(nine.columns == 3 && nine.rows == 3);
    // 10 entries need a 4th column rather than a 4th row.
    Grid const ten = shapeFor(10);
    CHECK(ten.columns == 4 && ten.rows == 3);
}

void testFullBundleStaysBounded() {
    Grid const full = shapeFor(16);
    CHECK(full.columns == 4 && full.rows == 4);
    // Past the cap: clamped, never drawn partially.
    Grid const over = shapeFor(64);
    CHECK(over.columns == 4 && over.rows == 4);
}

void testEveryShapeFitsOnScreen() {
    // At vanilla pitch (18-unit cells, 4-unit padding) even the largest grid
    // is 80x80 GUI units: on-screen at ordinary UI sizes.
    for (int filled = 1; filled <= kMaxSlots; ++filled) {
        Grid const g = shapeFor(filled);
        CHECK(g.columns >= kMinColumns && g.columns <= kMaxColumns);
        CHECK(g.rows >= 1 && g.rows <= 4);
        CHECK(g.columns * g.rows >= filled);
        float const w = g.columns * 18.0f + 8.0f;
        float const h = g.rows * 18.0f + 8.0f;
        CHECK(w <= 80.0f + 1e-4f && h <= 80.0f + 1e-4f);
    }
}

} // namespace

int runBundlePreviewTests() {
    testSupportsEveryBundleColour();
    testRejectsNonBundles();
    testEmptyBundleReportsMinimalFrame();
    testSingleEntryStaysReadable();
    testPrefersWiderShapes();
    testFullBundleStaysBounded();
    testEveryShapeFitsOnScreen();
    if (gFailures == 0) {
        std::printf("BundlePreview tests: all passed\n");
    } else {
        std::printf("BundlePreview tests: %d failure(s)\n", gFailures);
    }
    return gFailures;
}
