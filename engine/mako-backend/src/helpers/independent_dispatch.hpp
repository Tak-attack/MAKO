/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <vulkan/vulkan_core.h>

namespace mako::backend {
    /// A batch may share read-only images, but no image written by another
    /// dispatch. Treat the whole image as one resource, conservatively.
    inline bool independentComputeDispatches(
            std::span<const std::span<const VkImageMemoryBarrier>> accesses) {
        for (size_t i = 0; i < accesses.size(); ++i) {
            for (const auto& a : accesses[i]) {
                if (a.image == VK_NULL_HANDLE ||
                    (a.dstAccessMask != VK_ACCESS_SHADER_READ_BIT &&
                     a.dstAccessMask != VK_ACCESS_SHADER_WRITE_BIT) ||
                    a.oldLayout != VK_IMAGE_LAYOUT_GENERAL ||
                    a.newLayout != VK_IMAGE_LAYOUT_GENERAL ||
                    a.srcQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED ||
                    a.dstQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED)
                    return false;
                for (size_t j = 0; j < i; ++j)
                    for (const auto& b : accesses[j])
                        if (a.image == b.image &&
                            ((a.dstAccessMask | b.dstAccessMask) & VK_ACCESS_SHADER_WRITE_BIT))
                            return false;
            }
        }
        return true;
    }

    /// The owner supplies dead, same-format prepass images. Odd pyramid sizes
    /// can have different rounding, in which case retain the serial graph.
    template<typename Image>
    bool parallelScratchCompatible(std::span<const Image> first,
            std::span<const Image> second, size_t count, VkExtent2D extent,
            std::span<const VkImage> forbidden) {
        if (count == 0 || first.size() != count || second.size() != count)
            return false;
        const std::array banks{first, second};
        for (size_t b = 0; b < 2; ++b) {
            for (size_t i = 0; i < count; ++i) {
                const auto& image = banks.at(b)[i];
                const auto size = image.getExtent();
                if (image.handle() == VK_NULL_HANDLE || size.width != extent.width ||
                    size.height != extent.height)
                    return false;
                for (const auto live : forbidden)
                    if (image.handle() == live) return false;
                for (size_t priorBank = 0; priorBank <= b; ++priorBank)
                    for (size_t j = 0; j < (priorBank == b ? i : count); ++j)
                        if (image.handle() == banks.at(priorBank)[j].handle()) return false;
            }
        }
        return true;
    }
}
