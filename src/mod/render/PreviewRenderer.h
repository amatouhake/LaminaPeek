#pragma once

class MinecraftUIRenderContext;
class ScreenView;

namespace lamina_peek::preview {
struct ContainerPreview;
}

namespace lamina_peek::render {

/// Draws a ContainerPreview as an overlay on top of a container screen.
///
/// Must be called from the AfterUIRenderEvent of the ScreenView that owns the
/// hovered slot: at that point the engine has already flushed its own UI
/// batches, so everything drawn here is flushed explicitly.
class PreviewRenderer {
public:
    void render(ScreenView& view, MinecraftUIRenderContext& context, preview::ContainerPreview const& preview);
};

} // namespace lamina_peek::render
