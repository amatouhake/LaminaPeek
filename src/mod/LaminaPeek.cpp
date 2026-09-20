#include "mod/LaminaPeek.h"

#include "mod/hover/HoverTracker.h"
#include "mod/preview/ShulkerTooltipSuppressor.h"

#include "ll/api/Config.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/render/UIRenderEvent.h"
#include "ll/api/io/FileSink.h"
#include "ll/api/io/LogLevel.h"
#include "ll/api/io/PatternFormatter.h"
#include "ll/api/io/RotatePolicy.h"
#include "ll/api/mod/RegisterHelper.h"

#include "mc/client/gui/screens/ScreenController.h"
#include "mc/client/gui/screens/ScreenView.h"

#include <algorithm>
#include <chrono>
#include <optional>
#include <vector>

namespace lamina_peek {

LaminaPeek& LaminaPeek::getInstance() {
    static LaminaPeek instance;
    return instance;
}

bool LaminaPeek::load() {
#ifdef LAMINAPEEK_TRACE
    // Trace builds mirror every line, flushed immediately, into the mod
    // directory so the diagnostics can be followed while the game runs.
    // Rotation is disabled so the file is simply appended to across runs.
    getSelf().getLogger().setLevel(ll::io::LogLevel::Debug);
    auto sink = std::make_shared<ll::io::FileSink>(
        getSelf().getModDir() / "trace.log",
        ll::makePolymorphic<ll::io::PatternFormatter>("[{3:.3%F %T.} {2}][{1}] {0}", false),
        ll::io::RotatePolicy::disabled()
    );
    sink->setFlushLevel(ll::io::LogLevel::Debug);
    getSelf().getLogger().addSink(std::move(sink));
    getSelf().getLogger().info("Trace build: debug logging enabled, mirrored to trace.log");
#endif
    getSelf().getLogger().debug("Loading...");

    // Missing file -> written with defaults; unknown/old version -> merged
    // with defaults and rewritten, so the file always reflects the schema.
    auto const configPath = getSelf().getConfigDir() / "config.json";
    try {
        if (!ll::config::loadConfig(mConfig, configPath)) {
            ll::config::saveConfig(mConfig, configPath);
        }
    } catch (std::exception const& e) {
        getSelf().getLogger().error("Failed to load {}: {}; using defaults", configPath.string(), e.what());
        mConfig = Config{};
    }
    getSelf().getLogger().debug(
        "Config: shulker.enabled={} showEmpty={} disableVanillaContentsPreview={} bundle.enabled={} showEmpty={}",
        mConfig.shulker.enabled,
        mConfig.shulker.showEmpty,
        mConfig.shulker.disableVanillaContentsPreview,
        mConfig.bundle.enabled,
        mConfig.bundle.showEmpty
    );
    return true;
}

bool LaminaPeek::enable() {
    getSelf().getLogger().debug("Enabling...");
    hover::HoverTracker::getInstance().install();
    preview::ShulkerTooltipSuppressor::getInstance().install();
    mUIRenderListener = ll::event::EventBus::getInstance().emplaceListener<ll::event::AfterUIRenderEvent>(
        [this](ll::event::AfterUIRenderEvent& event) { onAfterUIRender(event); }
    );
    getSelf().getLogger().debug(
        "Enabled: hover hooks installed, UI render listener {}",
        mUIRenderListener ? "registered" : "FAILED to register"
    );
    return true;
}

bool LaminaPeek::disable() {
    getSelf().getLogger().debug("Disabling...");
    if (mUIRenderListener) {
        ll::event::EventBus::getInstance().removeListener(mUIRenderListener);
        mUIRenderListener.reset();
    }
    preview::ShulkerTooltipSuppressor::getInstance().uninstall();
    hover::HoverTracker::getInstance().uninstall();
    mPreviewCache.clear();
    return true;
}

#ifdef LAMINAPEEK_TRACE
namespace {
// Aggregated timing for the container screen's render callback: the interval
// between consecutive callbacks approximates the frame time seen by the
// render thread (it includes waiting for present), and the render duration is
// CPU time spent inside PreviewRenderer::render. Reported every 5 seconds.
struct PerfStats {
    using clock = std::chrono::steady_clock;
    std::vector<double>              frameMs;
    std::vector<double>              renderUs;
    std::optional<clock::time_point> lastFrame;
    clock::time_point                lastReport = clock::now();
    ScreenController const*          controller{nullptr};

    static double percentile(std::vector<double>& v, double p) {
        if (v.empty()) return 0.0;
        std::sort(v.begin(), v.end());
        size_t const idx = std::min(v.size() - 1, static_cast<size_t>(p * static_cast<double>(v.size())));
        return v[idx];
    }
    static double mean(std::vector<double> const& v) {
        if (v.empty()) return 0.0;
        double sum = 0;
        for (double x : v) sum += x;
        return sum / static_cast<double>(v.size());
    }

    void frame(ScreenController const& ctrl, double renderDurationUs, bool rendered) {
        auto const now = clock::now();
        if (controller != &ctrl) {
            controller = &ctrl;
            lastFrame.reset();
        }
        if (lastFrame) {
            frameMs.push_back(std::chrono::duration<double, std::milli>(now - *lastFrame).count());
        }
        lastFrame = now;
        if (rendered) renderUs.push_back(renderDurationUs);
        if (now - lastReport >= std::chrono::seconds(5) && !frameMs.empty()) {
            auto fm = frameMs;
            auto ru = renderUs;
            LaminaPeek::getInstance().getSelf().getLogger().debug(
                "perf: frames={} frame avg={:.2f}ms p99={:.2f}ms max={:.2f}ms ({:.0f} fps) | render n={} avg={:.0f}us "
                "p99={:.0f}us max={:.0f}us",
                fm.size(),
                mean(fm),
                percentile(fm, 0.99),
                percentile(fm, 1.0),
                1000.0 / std::max(mean(fm), 1e-6),
                ru.size(),
                mean(ru),
                percentile(ru, 0.99),
                percentile(ru, 1.0)
            );
            frameMs.clear();
            renderUs.clear();
            lastReport = now;
        }
    }
};
PerfStats gPerf;
} // namespace
#endif

void LaminaPeek::onAfterUIRender(ll::event::AfterUIRenderEvent& event) {
    // Every ScreenView on the stack renders each frame; only the container
    // screen that owns the hovered slot is interesting.
    auto* controller = event.screenView().mController.get();
    if (!controller || !controller->_isContainerScreen()) {
        return;
    }
#ifdef LAMINAPEEK_TRACE
    auto const perfStart = std::chrono::steady_clock::now();
    bool       rendered  = false;
    struct PerfScope {
        ScreenController const&               ctrl;
        std::chrono::steady_clock::time_point start;
        bool&                                 rendered;
        ~PerfScope() {
            auto const us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count();
            gPerf.frame(ctrl, us, rendered);
        }
    } perfScope{*controller, perfStart, rendered};
#endif
    auto const* preview = mPreviewCache.resolve(*controller);
    if (!preview) {
        return;
    }
    // The cache may hold a Shulker or a Bundle preview; each family has its
    // own master switch and empty-grid setting. Gating here (rather than in
    // the providers) keeps extraction read-only and lets hover-switching fall
    // through to the next frame's re-extraction with no stale preview: a
    // disabled family simply never draws. The Bundle grid is dynamic (never
    // 9x3), so grid shape identifies the family without re-reading the item.
    bool showEmpty = false;
    if (preview->columns == 9 && preview->rows == 3) {
        if (!mConfig.shulker.enabled) {
            return;
        }
        showEmpty = mConfig.shulker.showEmpty;
    } else {
        if (!mConfig.bundle.enabled) {
            return;
        }
        showEmpty = mConfig.bundle.showEmpty;
    }
    // An empty container has nothing useful to show unless the user asked
    // for the grid anyway.
    if (preview->filledSlotCount() == 0 && !showEmpty) {
        return;
    }
#ifdef LAMINAPEEK_TRACE
    rendered = true;
#endif
    mPreviewRenderer.render(event.screenView(), event.uiRenderContext(), *preview);
}

} // namespace lamina_peek

LL_REGISTER_MOD(lamina_peek::LaminaPeek, lamina_peek::LaminaPeek::getInstance());
