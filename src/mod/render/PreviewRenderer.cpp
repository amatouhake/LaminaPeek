#include "mod/render/PreviewRenderer.h"

#include "mod/LaminaPeek.h"
#include "mod/preview/ContainerPreview.h"
#include "mod/render/PreviewLayout.h"

#include "mc/client/game/IClientInstance.h"
#include "mc/client/gui/CaretMeasureData.h"
#include "mc/client/gui/Font.h"
#include "mc/client/gui/FontHandle.h"
#include "mc/client/gui/TextAlignment.h"
#include "mc/client/gui/TextMeasureData.h"
#include "mc/client/gui/screens/ScreenView.h"
#include "mc/client/renderer/BaseActorRenderContext.h"
#include "mc/client/renderer/actor/ItemRenderer.h"
#include "mc/client/renderer/screen/MinecraftUIRenderContext.h"
#include "mc/deps/core/math/Color.h"
#include "mc/deps/core/string/HashedString.h"
#include "mc/deps/input/RectangleArea.h"

#include <chrono>
#include <optional>
#include <string>

namespace lamina_peek::render {

namespace {

// Material the UI uses for plain coloured quads (fillRectangle/drawRectangle).
HashedString const kFillMaterial{"ui_fillColor"};

constexpr mce::Color kFrameBackground{0.10f, 0.10f, 0.10f, 1.0f};
constexpr mce::Color kFrameBorder{0.55f, 0.35f, 0.70f, 1.0f};
constexpr mce::Color kSlotBackground{0.23f, 0.23f, 0.23f, 1.0f};
constexpr mce::Color kCountText{1.0f, 1.0f, 1.0f, 1.0f};
constexpr mce::Color kWhite{1.0f, 1.0f, 1.0f, 1.0f};

constexpr float kFrameAlpha = 0.92f;
constexpr float kSlotAlpha  = 1.0f;
constexpr float kCountFont  = 1.0f;
constexpr float kCountLineH = 10.0f; // approx. glyph height at font scale 1
constexpr int   kItemZOrder = 17;

// The glint overlay from renderGuiItemNew(foil = true) blends additively and,
// measured against an enchanted item in a vanilla slot, tints roughly a third
// as strongly as the UI's own glint pass; three overlay passes match vanilla.
constexpr int kGlintPasses = 3;
// The glint texture scrolls per animation frame; vanilla advances it on the
// UI's 20 Hz animation clock.
constexpr auto kGlintFramePeriod = std::chrono::milliseconds(50);


RectangleArea toArea(Rect const& r) { return RectangleArea{r.x0, r.x1, r.y0, r.y1}; }

} // namespace

void PreviewRenderer::render(
    ScreenView&                      view,
    MinecraftUIRenderContext&        context,
    preview::ContainerPreview const& preview
) {
    if (preview.columns <= 0 || preview.rows <= 0 || preview.slots.size() < static_cast<size_t>(preview.slotCount())) {
        return;
    }

    glm::vec2 const pointer = view.mPointerLocationPrevious;
    glm::vec2 const screen  = view.mSize;
    auto const      layout =
        PreviewLayout::anchored(preview.columns, preview.rows, pointer.x, pointer.y, screen.x, screen.y);

    static bool sLoggedFirstRender = false;
    if (!sLoggedFirstRender) {
        sLoggedFirstRender = true;
        LaminaPeek::getInstance().getSelf().getLogger().debug(
            "First preview render: pointer=({}, {}) screen=({}, {}) frame=({}, {})-({}, {})",
            pointer.x,
            pointer.y,
            screen.x,
            screen.y,
            layout.frame.x0,
            layout.frame.y0,
            layout.frame.x1,
            layout.frame.y1
        );
    }

    // 1. Frame and slot backgrounds. These are batched by the context, so
    //    flush them before drawing anything that must appear on top. Each
    //    slot only fills its 16x16 icon area so the 2-unit gaps between cells
    //    keep the grid visible even when the box is empty.
    context.fillRectangle(toArea(layout.frame), kFrameBackground, kFrameAlpha);
    for (int slot = 0; slot < preview.slotCount(); ++slot) {
        context.fillRectangle(toArea(layout.icon(slot)), kSlotBackground, kSlotAlpha);
    }
    context.drawRectangle(toArea(layout.frame), kFrameBorder, 1.0f, 1);
    context.flushImages(kWhite, 1.0f, kFillMaterial);

    // 2. Item icons, drawn immediately by the game's item renderer.
    IClientInstance& client       = context.mClient;
    ItemRenderer*    itemRenderer = client.getItemRenderer();
    if (itemRenderer) {
        int const glintFrame =
            static_cast<int>(std::chrono::steady_clock::now().time_since_epoch() / kGlintFramePeriod);
        BaseActorRenderContext renderContext(context.mScreenContext, client, client.getMinecraftGame_DEPRECATED());
        for (int slot = 0; slot < preview.slotCount(); ++slot) {
            ItemStack const& stack = preview.slots[static_cast<size_t>(slot)];
            if (stack.isNull()) {
                continue;
            }
            Rect const icon = layout.icon(slot);
            // renderEnchantmentFoil selects the pass: false draws the item
            // icon itself, true draws only the additive glint overlay (vanilla
            // draws the icon in one UI pass and the glint in another).
            itemRenderer
                ->renderGuiItemNew(renderContext, stack, 0, icon.x0, icon.y0, false, 1.0f, 1.0f, 1.0f, kItemZOrder);
            if (stack.isEnchanted()) {
                for (int pass = 0; pass < kGlintPasses; ++pass) {
                    itemRenderer->renderGuiItemNew(
                        renderContext,
                        stack,
                        glintFrame,
                        icon.x0,
                        icon.y0,
                        true,
                        1.0f,
                        1.0f,
                        1.0f,
                        kItemZOrder
                    );
                }
            }
        }
    }

    // 3. Stack counts, bottom-right of the cell like vanilla slots.
    auto const& fontHandle = client.getFontHandle();
    Font&       font       = fontHandle.getFont();
    bool        anyText    = false;
    for (int slot = 0; slot < preview.slotCount(); ++slot) {
        ItemStack const& stack = preview.slots[static_cast<size_t>(slot)];
        if (stack.isNull() || stack.mCount <= 1) {
            continue;
        }
        Rect const cell = layout.cell(slot);
        Rect const textRect{cell.x0, cell.y1 - kCountLineH, cell.x1 - 1.0f, cell.y1};
        context.drawText(
            font,
            toArea(textRect),
            std::to_string(static_cast<int>(stack.mCount)),
            kCountText,
            1.0f,
            ui::TextAlignment::Right,
            TextMeasureData{kCountFont, 0.0f, true, false, false, ui::TextAlignment::Right},
            CaretMeasureData{-1, false}
        );
        anyText = true;
    }
    if (anyText) {
        context.flushText(0.0f, std::nullopt);
    }
}

} // namespace lamina_peek::render
