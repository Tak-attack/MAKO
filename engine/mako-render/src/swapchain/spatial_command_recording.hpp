/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once

#include <optional>
#include <utility>
#include <vulkan/vulkan_core.h>

namespace mako::layer {
    class SpatialCommandRecording {
    public:
        // Call only after the owning pass's completion fence is satisfied.
        template<typename Record>
        void update(const VkImage application, const VkImage source, Record&& record) {
            const std::pair images{application, source};
            if (this->recordedImages == images)
                return;
            this->invalidate();
            std::forward<Record>(record)();
            this->recordedImages = images;
        }
        void invalidate() noexcept { this->recordedImages.reset(); }
    private:
        std::optional<std::pair<VkImage, VkImage>> recordedImages;
    };
}
