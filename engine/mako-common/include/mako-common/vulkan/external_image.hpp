/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "vulkan.hpp"

#include <vulkan/vulkan_core.h>

namespace vk {
    // Images shared with the private backend cross Vulkan instance ownership.
    // GENERAL is the layout at that boundary; a semaphore orders the release
    // and acquire, but does not replace either ownership operation.
    [[nodiscard]] inline VkImageMemoryBarrier externalImageAcquire(
            const VkImage image, const uint32_t family,
            const VkAccessFlags access) {
        return {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .dstAccessMask = access,
            .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
            .dstQueueFamilyIndex = family,
            .image = image,
            .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        };
    }

    [[nodiscard]] inline VkImageMemoryBarrier externalImageRelease(
            const VkImage image, const uint32_t family,
            const VkAccessFlags access,
            const VkImageLayout layout = VK_IMAGE_LAYOUT_GENERAL) {
        return {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = access,
            .oldLayout = layout,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = family,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL,
            .image = image,
            .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        };
    }

    inline void acquireExternalImage(const Vulkan& vk, const VkCommandBuffer command,
            const VkImage image, const VkAccessFlags access,
            const VkPipelineStageFlags stage) {
        const auto barrier = externalImageAcquire(image, vk.queueFamilyIndex(), access);
        vk.df().CmdPipelineBarrier(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    inline void releaseExternalImage(const Vulkan& vk, const VkCommandBuffer command,
            const VkImage image, const VkAccessFlags access,
            const VkPipelineStageFlags stage,
            const VkImageLayout layout = VK_IMAGE_LAYOUT_GENERAL) {
        const auto barrier = externalImageRelease(image, vk.queueFamilyIndex(), access, layout);
        vk.df().CmdPipelineBarrier(command, stage, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
    }
}
