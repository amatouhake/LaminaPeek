#pragma once

#include "ll/api/event/ListenerBase.h"
#include "ll/api/mod/NativeMod.h"

#include "mod/Config.h"
#include "mod/preview/HoveredPreviewCache.h"
#include "mod/render/PreviewRenderer.h"

namespace ll::event::inline render {
class AfterUIRenderEvent;
}

namespace lamina_peek {

class LaminaPeek {

public:
    static LaminaPeek& getInstance();

    LaminaPeek() : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    [[nodiscard]] Config const& getConfig() const { return mConfig; }

    /// @return True if the mod is loaded successfully.
    bool load();

    /// @return True if the mod is enabled successfully.
    bool enable();

    /// @return True if the mod is disabled successfully.
    bool disable();

private:
    void onAfterUIRender(ll::event::AfterUIRenderEvent& event);

    ll::mod::NativeMod&          mSelf;
    Config                       mConfig;
    ll::event::ListenerPtr       mUIRenderListener;
    preview::HoveredPreviewCache mPreviewCache;
    render::PreviewRenderer      mPreviewRenderer;
};

} // namespace lamina_peek
