#pragma once

#include "mc/world/item/ItemStack.h"

#include <vector>

namespace lamina_peek::preview {

/// The data needed to draw a preview of a container-like item: a fixed grid of
/// slots, each holding the item stored there (or a null stack when empty).
///
/// The layout mirrors the real container so that slot `row * columns + col`
/// is rendered at that grid position, including gaps left by empty slots.
struct ContainerPreview {
    int                    columns{0};
    int                    rows{0};
    std::vector<ItemStack> slots;               // size == rows * columns; null stacks for empty slots
    int                    skippedSlotCount{0}; // entries that could not be decoded and were left empty

    [[nodiscard]] int slotCount() const { return rows * columns; }

    /// Number of slots holding an item; 0 for a completely empty container.
    [[nodiscard]] int filledSlotCount() const {
        int filled = 0;
        for (auto const& slot : slots) {
            if (!slot.isNull()) ++filled;
        }
        return filled;
    }

    [[nodiscard]] static ContainerPreview empty(int columns, int rows) {
        ContainerPreview preview;
        preview.columns = columns;
        preview.rows    = rows;
        preview.slots.resize(static_cast<size_t>(columns) * static_cast<size_t>(rows));
        return preview;
    }
};

} // namespace lamina_peek::preview
