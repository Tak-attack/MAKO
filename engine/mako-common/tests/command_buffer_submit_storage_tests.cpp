/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "vulkan/command_buffer_submit_storage.hpp"
#include "mako-common/vulkan/command_buffer.hpp"
#include "mako-common/helpers/errors.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace {
    void require(const bool condition, const std::string& message) {
        if (!condition)
            throw std::runtime_error(message);
    }

    template<typename Handle>
    Handle fakeHandle(const uintptr_t value) {
        if constexpr (std::is_pointer_v<Handle>) {
            return reinterpret_cast<Handle>(value);
        } else {
            return static_cast<Handle>(value);
        }
    }

    void testInlineTimelineComposition() {
        const std::array waits{
            fakeHandle<VkSemaphore>(1), fakeHandle<VkSemaphore>(2)
        };
        const std::array signals{fakeHandle<VkSemaphore>(4)};
        const vk::detail::CommandBufferSubmitStorage storage{
            waits, fakeHandle<VkSemaphore>(3), 17,
            signals, fakeHandle<VkSemaphore>(5), 23
        };

        require(storage.waits().size() == 3,
            "inline wait count is wrong");
        require(storage.waits()[0] == waits[0] &&
                storage.waits()[1] == waits[1] &&
                storage.waits()[2] == fakeHandle<VkSemaphore>(3),
            "timeline wait was not appended after binary waits");
        require(storage.waitValues()[0] == 0 &&
                storage.waitValues()[1] == 0 &&
                storage.waitValues()[2] == 17,
            "inline wait values are not aligned with semaphores");
        require(storage.signals().size() == 2 &&
                storage.signals()[0] == signals[0] &&
                storage.signals()[1] == fakeHandle<VkSemaphore>(5),
            "timeline signal was not appended after binary signals");
        require(storage.signalValues()[0] == 0 &&
                storage.signalValues()[1] == 23,
            "inline signal values are not aligned with semaphores");
        require(storage.usesTimelineSemaphores(),
            "timeline submission did not request timeline submit info");
        for (const auto stage : storage.stages()) {
            require(stage == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                "wait stage changed");
        }
    }

    void testOverflowAndEmptyComposition() {
        const std::array waits{
            fakeHandle<VkSemaphore>(10), fakeHandle<VkSemaphore>(11),
            fakeHandle<VkSemaphore>(12), fakeHandle<VkSemaphore>(13),
            fakeHandle<VkSemaphore>(14)
        };
        const vk::detail::CommandBufferSubmitStorage overflow{
            waits, fakeHandle<VkSemaphore>(15), 29,
            {}, VK_NULL_HANDLE, 0
        };
        require(overflow.waits().size() == waits.size() + 1,
            "overflow wait count is wrong");
        for (size_t i = 0; i < waits.size(); ++i) {
            require(overflow.waits()[i] == waits[i],
                "overflow storage changed a binary wait semaphore");
            require(overflow.waitValues()[i] == 0,
                "overflow binary wait received a timeline value");
        }
        require(overflow.waits().back() == fakeHandle<VkSemaphore>(15) &&
                overflow.waitValues().back() == 29,
            "overflow storage lost the appended timeline wait");
        require(overflow.stages().size() == overflow.waits().size(),
            "overflow wait stage count is misaligned");
        for (const auto stage : overflow.stages()) {
            require(stage == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                "overflow wait stage changed");
        }
        require(overflow.signals().empty() &&
                overflow.signalValues().empty(),
            "empty signals unexpectedly created storage entries");

        const vk::detail::CommandBufferSubmitStorage empty{
            {}, VK_NULL_HANDLE, 0, {}, VK_NULL_HANDLE, 0
        };
        require(empty.waits().empty() && empty.waitValues().empty() &&
                empty.stages().empty() && empty.signals().empty() &&
                empty.signalValues().empty(),
            "empty submission storage is not empty");
        require(!empty.usesTimelineSemaphores(),
            "empty submission requested timeline submit info");

        const std::array binarySignals{fakeHandle<VkSemaphore>(16)};
        const vk::detail::CommandBufferSubmitStorage binaryOnly{
            waits, VK_NULL_HANDLE, 0,
            binarySignals, VK_NULL_HANDLE, 0
        };
        require(!binaryOnly.usesTimelineSemaphores(),
            "binary-only submission requested timeline submit info");
    }

    void testTimelineOnlyComposition() {
        const auto wait = fakeHandle<VkSemaphore>(20);
        const auto signal = fakeHandle<VkSemaphore>(21);
        const vk::detail::CommandBufferSubmitStorage storage{
            {}, wait, 31, {}, signal, 37
        };

        require(storage.waits().size() == 1 && storage.waits()[0] == wait &&
                storage.waitValues()[0] == 31 &&
                storage.stages()[0] == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            "timeline-only wait composition is wrong");
        require(storage.signals().size() == 1 &&
                storage.signals()[0] == signal &&
                storage.signalValues()[0] == 37,
            "timeline-only signal composition is wrong");
        require(storage.usesTimelineSemaphores(),
            "timeline-only submission did not request submit info");
    }

    struct ExpectedSubmission {
        std::span<const VkCommandBuffer> commands;
        std::span<const VkSemaphore> waits;
        VkSemaphore waitTimeline{};
        uint64_t waitValue{};
        std::span<const VkSemaphore> signals;
        VkSemaphore signalTimeline{};
        uint64_t signalValue{};
    } expected;
    size_t queueCalls{};
    VkResult submitResult{};
    VkQueue expectedQueue{};
    VkFence expectedFence{};

    vk::Vulkan makeVulkan() {
        vk::VulkanInstanceFuncs fi{};
        fi.GetPhysicalDeviceQueueFamilyProperties = [](VkPhysicalDevice,
                uint32_t* count, VkQueueFamilyProperties* properties) {
            *count = 1;
            if (properties) properties[0].queueFlags = VK_QUEUE_COMPUTE_BIT;
        };
        vk::VulkanDeviceFuncs df{};
        df.GetDeviceQueue = [](VkDevice, uint32_t, uint32_t, VkQueue* queue) {
            *queue = fakeHandle<VkQueue>(10);
        };
        df.CreateCommandPool = [](VkDevice, const VkCommandPoolCreateInfo*,
                const VkAllocationCallbacks*, VkCommandPool* pool) {
            *pool = fakeHandle<VkCommandPool>(1); return VK_SUCCESS;
        };
        df.DestroyCommandPool = [](VkDevice, VkCommandPool, const VkAllocationCallbacks*) {};
        df.CreatePipelineCache = [](VkDevice, const VkPipelineCacheCreateInfo*,
                const VkAllocationCallbacks*, VkPipelineCache* cache) {
            *cache = fakeHandle<VkPipelineCache>(1); return VK_SUCCESS;
        };
        df.DestroyPipelineCache = [](VkDevice, VkPipelineCache, const VkAllocationCallbacks*) {};
        df.QueueSubmit = [](VkQueue queue, uint32_t count, const VkSubmitInfo* infos, VkFence fence) {
            ++queueCalls;
            require(queue == expectedQueue && fence == expectedFence,
                "submission changed queue or completion fence");
            require(count == 1, "commands did not share one submission boundary");
            const auto& got = *infos;
            require(got.sType == VK_STRUCTURE_TYPE_SUBMIT_INFO &&
                    got.commandBufferCount == expected.commands.size(), "command count changed");
            const std::span commands{got.pCommandBuffers, got.commandBufferCount};
            for (size_t i = 0; i < commands.size(); ++i)
                require(commands[i] == expected.commands[i], "command order changed");
            const vk::detail::CommandBufferSubmitStorage sync{
                expected.waits, expected.waitTimeline, expected.waitValue,
                expected.signals, expected.signalTimeline, expected.signalValue};
            require(got.waitSemaphoreCount == sync.waits().size() &&
                    got.signalSemaphoreCount == sync.signals().size(), "semaphore counts changed");
            const std::span waits{got.pWaitSemaphores, got.waitSemaphoreCount};
            const std::span stages{got.pWaitDstStageMask, got.waitSemaphoreCount};
            const std::span signals{got.pSignalSemaphores, got.signalSemaphoreCount};
            for (size_t i = 0; i < waits.size(); ++i)
                require(waits[i] == sync.waits()[i] && stages[i] == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    "wait semaphore or stage changed");
            for (size_t i = 0; i < signals.size(); ++i)
                require(signals[i] == sync.signals()[i], "signal semaphore changed");
            if (sync.usesTimelineSemaphores()) {
                require(got.pNext, "timeline metadata is missing");
                const auto& timeline = *static_cast<const VkTimelineSemaphoreSubmitInfo*>(got.pNext);
                require(timeline.sType == VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO &&
                        timeline.waitSemaphoreValueCount == waits.size() &&
                        timeline.signalSemaphoreValueCount == signals.size(), "timeline counts changed");
                const std::span waitValues{timeline.pWaitSemaphoreValues, timeline.waitSemaphoreValueCount};
                const std::span signalValues{timeline.pSignalSemaphoreValues, timeline.signalSemaphoreValueCount};
                for (size_t i = 0; i < waitValues.size(); ++i)
                    require(waitValues[i] == sync.waitValues()[i], "timeline wait value changed");
                for (size_t i = 0; i < signalValues.size(); ++i)
                    require(signalValues[i] == sync.signalValues()[i], "timeline signal value changed");
            } else require(!got.pNext, "binary submission acquired timeline metadata");
            return submitResult;
        };
        return {fakeHandle<VkInstance>(1), fakeHandle<VkDevice>(1),
            fakeHandle<VkPhysicalDevice>(1), fi, df, 0, true};
    }

    void testOrderedCommandSubmission() {
        const auto vk = makeVulkan();
        const std::array commands{fakeHandle<VkCommandBuffer>(1),
            fakeHandle<VkCommandBuffer>(2), fakeHandle<VkCommandBuffer>(3)};
        const std::array binary{fakeHandle<VkSemaphore>(20), fakeHandle<VkSemaphore>(21),
            fakeHandle<VkSemaphore>(22), fakeHandle<VkSemaphore>(23), fakeHandle<VkSemaphore>(24)};
        for (const size_t commandCount : {1U, 3U}) {
            for (const size_t binaryCount : {0U, 1U, 5U}) {
                for (const bool timeline : {false, true}) {
                    for (const bool overrideQueue : {false, true}) {
                        expected = {std::span(commands).first(commandCount),
                            std::span(binary).first(binaryCount), timeline ? fakeHandle<VkSemaphore>(40) : VK_NULL_HANDLE, 71,
                            std::span(binary).first(binaryCount), timeline ? fakeHandle<VkSemaphore>(41) : VK_NULL_HANDLE, 72};
                        expectedQueue = fakeHandle<VkQueue>(overrideQueue ? 11 : 10);
                        expectedFence = overrideQueue ? fakeHandle<VkFence>(12) : VK_NULL_HANDLE;
                        for (const auto result : {VK_SUCCESS, VK_ERROR_OUT_OF_DEVICE_MEMORY, VK_ERROR_DEVICE_LOST}) {
                            queueCalls = 0;
                            submitResult = result;
                            bool failed = false;
                            try {
                                vk::CommandBuffer::submitCommands(vk, expected.commands,
                                    expected.waits, expected.waitTimeline, expected.waitValue,
                                    expected.signals, expected.signalTimeline, expected.signalValue,
                                    expectedFence, overrideQueue ? expectedQueue : VK_NULL_HANDLE);
                            } catch (const ls::vulkan_error&) { failed = true; }
                            require(queueCalls == 1 && failed == (result != VK_SUCCESS),
                                "submission failure was ignored or retried");
                        }
                    }
                }
            }
        }
        queueCalls = 0;
        bool rejected = false;
        try { vk::CommandBuffer::submitCommands(vk, {}, {}, VK_NULL_HANDLE, 0, {}, VK_NULL_HANDLE, 0); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected && queueCalls == 0, "empty command sequence touched the queue");
    }

}

int main() {
    try {
        testInlineTimelineComposition();
        testOverflowAndEmptyComposition();
        testTimelineOnlyComposition();
        testOrderedCommandSubmission();
    } catch (const std::exception& error) {
        std::cerr << "Command submit storage test failed: "
                  << error.what() << '\n';
        return 1;
    }
    return 0;
}
