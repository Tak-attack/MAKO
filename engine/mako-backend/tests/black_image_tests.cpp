/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "helpers/black_image.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <type_traits>

namespace {
    void require(bool result, const char* message) {
        if (!result) throw std::runtime_error(message);
    }
    template<typename T> T handle(uintptr_t value) {
        if constexpr (std::is_pointer_v<T>) return reinterpret_cast<T>(value);
        else return static_cast<T>(value);
    }
    const auto command = handle<VkCommandBuffer>(17);
    const auto image = handle<VkImage>(23);
    int step{};
    // Start with dirty contents so omitting the clear cannot pass.
    std::array<uint32_t, 4> pixels{1, 2, 3, 4};
    void checkRange(const VkImageSubresourceRange& range) {
        require(range.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT &&
            range.baseMipLevel == 0 && range.levelCount == 1 &&
            range.baseArrayLayer == 0 && range.layerCount == 1,
            "initialization does not cover the complete image");
    }
}

int main() {
    vk::VulkanDeviceFuncs functions{};
    functions.CmdPipelineBarrier = [](VkCommandBuffer cmd, VkPipelineStageFlags source,
            VkPipelineStageFlags destination, VkDependencyFlags,
            uint32_t, const VkMemoryBarrier*, uint32_t, const VkBufferMemoryBarrier*,
            uint32_t count, const VkImageMemoryBarrier* barriers) {
        require(cmd == command && count == 1 && barriers[0].image == image,
            "initialization targeted another image or command");
        const auto& barrier = barriers[0];
        checkRange(barrier.subresourceRange);
        require(barrier.srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED &&
            barrier.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED,
            "private black image must retain local ownership");
        if (step == 0) {
            require(source == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT &&
                destination == VK_PIPELINE_STAGE_TRANSFER_BIT &&
                barrier.srcAccessMask == 0 && barrier.dstAccessMask == VK_ACCESS_TRANSFER_WRITE_BIT &&
                barrier.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
                barrier.newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                "clear must discard undefined contents and establish transfer access");
        } else {
            require(step == 2 && pixels == std::array<uint32_t, 4>{} &&
                source == VK_PIPELINE_STAGE_TRANSFER_BIT && destination == VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT &&
                barrier.srcAccessMask == VK_ACCESS_TRANSFER_WRITE_BIT &&
                barrier.dstAccessMask == VK_ACCESS_SHADER_READ_BIT &&
                barrier.oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                barrier.newLayout == VK_IMAGE_LAYOUT_GENERAL,
                "shader reads must follow visible zero initialization");
        }
        ++step;
    };
    functions.CmdClearColorImage = [](VkCommandBuffer cmd, VkImage target, VkImageLayout layout,
            const VkClearColorValue* color, uint32_t count, const VkImageSubresourceRange* range) {
        require(step == 1 && cmd == command && target == image && count == 1 &&
            layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, "clear is missing its transfer dependency");
        checkRange(*range);
        pixels = std::bit_cast<std::array<uint32_t, 4>>(*color);
        ++step;
    };
    require((mako::backend::blackImageUsage &
        (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)) ==
        (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT),
        "black image must support clearing and sampling");
    mako::backend::initializeBlackImage(functions, command, image);
    require(step == 3, "black image initialization did not finish");
    std::cout << "Black image initialization checks passed\n";
}
