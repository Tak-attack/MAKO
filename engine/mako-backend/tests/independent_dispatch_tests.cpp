/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "helpers/independent_dispatch.hpp"
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <span>
#include <type_traits>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace {
    void require(bool result, const char* message) {
        if (!result) throw std::runtime_error(message);
    }
    template<typename T> T handle(uintptr_t value) {
        if constexpr (std::is_pointer_v<T>) return reinterpret_cast<T>(value);
        else return static_cast<T>(value);
    }
    VkImageMemoryBarrier access(uintptr_t image, VkAccessFlags role) {
        return {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .dstAccessMask = role,
            .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = handle<VkImage>(image),
        };
    }
    bool independent(const std::vector<std::vector<VkImageMemoryBarrier>>& groups) {
        std::vector<std::span<const VkImageMemoryBarrier>> spans;
        for (const auto& group : groups) spans.emplace_back(group);
        return mako::backend::independentComputeDispatches(spans);
    }
    void testResourceDependencies() {
        const auto read = access(1, VK_ACCESS_SHADER_READ_BIT);
        const auto write = access(1, VK_ACCESS_SHADER_WRITE_BIT);
        const auto otherWrite = access(2, VK_ACCESS_SHADER_WRITE_BIT);
        require(independent({}), "empty group rejected");
        require(independent({{read}, {read}}), "shared immutable input rejected");
        require(independent({{write}, {otherWrite}}), "disjoint outputs rejected");
        require(!independent({{write}, {read}}), "read-after-write dependency admitted");
        require(!independent({{read}, {write}}), "write-after-read dependency admitted");
        require(!independent({{write}, {write}}), "write-after-write dependency admitted");
        require(!independent({{write}, {otherWrite}, {read}}), "non-adjacent dependency missed");
        require(!independent({{access(0, VK_ACCESS_SHADER_READ_BIT)}}), "null image admitted");
        require(!independent({{access(1, 0)}}), "unknown access admitted");
        auto layout = read;
        layout.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        require(!independent({{layout}, {read}}), "layout transition admitted");
        auto ownership = read;
        ownership.srcQueueFamilyIndex = 0;
        require(!independent({{ownership}}), "ownership transition admitted");
    }
    struct Image {
        VkImage image;
        VkExtent2D extent;
        VkImage handle() const { return image; }
        VkExtent2D getExtent() const { return extent; }
    };
    Image image(uintptr_t id, VkExtent2D extent = {320, 180}) {
        return {handle<VkImage>(id), extent};
    }
    void testScratchOwnershipAndRounding() {
        using mako::backend::parallelScratchCompatible;
        for (size_t count : {1U, 2U}) {
            std::vector<Image> first, second;
            for (size_t i = 0; i < count; ++i) {
                first.push_back(image(10 + i));
                second.push_back(image(20 + i));
            }
            const std::array live{handle<VkImage>(99)};
            const auto valid = [&] {
                return parallelScratchCompatible<Image>(first, second, count, {320, 180}, live);
            };
            require(valid(), "valid dead banks rejected");
            second[0].image = first[0].image;
            require(!valid(), "cross-bank alias admitted");
            second[0] = image(20);
            first[0].image = live[0];
            require(!valid(), "live input overwritten by scratch");
            first[0] = image(10);
            second[0].extent.height = 181;
            require(!valid(), "pyramid rounding mismatch admitted");
            second[0] = image(20);
            if (count == 2) {
                second[1].image = second[0].image;
                require(!valid(), "duplicate image inside bank admitted");
                second[1] = image(21);
            }
            second.pop_back();
            require(!valid(), "incomplete scratch bank admitted");
        }
        const std::array<Image, 1> rounded{image(10, {160, 120})};
        const std::array<Image, 1> other{image(20, {160, 120})};
        require(!parallelScratchCompatible<Image>(rounded, other, 1, {161, 121}, {}),
            "641x481 finer-level rounding must retain serial dispatch");
        require(parallelScratchCompatible<Image>(rounded, other, 1, {160, 120}, {}),
            "matching coarser half-resolution bank rejected");
        require(!parallelScratchCompatible<Image>({}, {}, 0, {1, 1}, {}),
            "empty scratch selection admitted");
    }
}

int main() {
    testResourceDependencies();
    testScratchOwnershipAndRounding();
    std::cout << "Independent dispatch hazards and scratch fallback checks passed\n";
}
