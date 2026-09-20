#include "mod/LaminaPeek.h"

#include "mod/hover/HoverTracker.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/render/UIRenderEvent.h"
#include "ll/api/io/FileSink.h"
#include "ll/api/io/LogLevel.h"
#include "ll/api/io/PatternFormatter.h"
#include "ll/api/mod/RegisterHelper.h"

#include "mc/client/gui/screens/ScreenController.h"
#include "mc/client/gui/screens/ScreenView.h"

namespace lamina_peek {

LaminaPeek& LaminaPeek::getInstance() {
    static LaminaPeek instance;
    return instance;
}

bool LaminaPeek::load() {
#ifdef LAMINAPEEK_TRACE
    // Trace builds mirror every line, flushed immediately, into the mod
    // directory so the diagnostics can be followed while the game runs.
    getSelf().getLogger().setLevel(ll::io::LogLevel::Debug);
    auto sink = std::make_shared<ll::io::FileSink>(
        getSelf().getModDir() / "trace.log",
        ll::makePolymorphic<ll::io::PatternFormatter>("[{3:.3%F %T.} {2}][{1}] {0}", false),
        std::ios::app
    );
    sink->setFlushLevel(ll::io::LogLevel::Debug);
    getSelf().getLogger().addSink(std::move(sink));
    getSelf().getLogger().info("Trace build: debug logging enabled, mirrored to trace.log");
#endif
    getSelf().getLogger().debug("Loading...");
    return true;
}

bool LaminaPeek::enable() {
    getSelf().getLogger().debug("Enabling...");
    hover::HoverTracker::getInstance().install();
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
    hover::HoverTracker::getInstance().uninstall();
    mPreviewCache.clear();
    return true;
}

void LaminaPeek::onAfterUIRender(ll::event::AfterUIRenderEvent& event) {
    // Every ScreenView on the stack renders each frame; only the container
    // screen that owns the hovered slot is interesting.
    auto* controller = event.screenView().mController.get();
    if (!controller || !controller->_isContainerScreen()) {
        return;
    }
    auto const* preview = mPreviewCache.resolve(*controller);
    if (!preview) {
        return;
    }
    mPreviewRenderer.render(event.screenView(), event.uiRenderContext(), *preview);
}

} // namespace lamina_peek

LL_REGISTER_MOD(lamina_peek::LaminaPeek, lamina_peek::LaminaPeek::getInstance());
