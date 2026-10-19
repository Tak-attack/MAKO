/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "mako-common/vulkan/shader.hpp"
#include "mako-common/vulkan/command_buffer.hpp"
#include "mako-common/vulkan/descriptor_pool.hpp"
#include "mako-common/vulkan/descriptor_set.hpp"
#include <iostream>
#include <stdexcept>
#include <type_traits>

namespace {
    template<typename T> T handle(uintptr_t value = 1) {
        if constexpr (std::is_pointer_v<T>) return reinterpret_cast<T>(value);
        else return static_cast<T>(value);
    }
    void require(bool value, const char* message) {
        if (!value) throw std::runtime_error(message);
    }
    struct Observed {
        uint32_t modules{}, moduleDestructions{}, pipelines{}, pipelineDestructions{};
        uint32_t layouts{}, layoutDestructions{}, descriptors{}, barriers{}, dispatches{};
        uint32_t failPipeline{};
        std::vector<VkPipeline> bound;
    } observed;
    vk::Vulkan makeVulkan() {
        vk::VulkanInstanceFuncs fi{};
        fi.GetPhysicalDeviceQueueFamilyProperties = [](VkPhysicalDevice, uint32_t* count, VkQueueFamilyProperties* properties) {
            *count = 1;
            if (properties) properties[0].queueFlags = VK_QUEUE_COMPUTE_BIT;
        };
        vk::VulkanDeviceFuncs df{};
        df.GetDeviceQueue = [](VkDevice, uint32_t, uint32_t, VkQueue* queue) { *queue = handle<VkQueue>(); };
        df.CreateCommandPool = [](VkDevice, const VkCommandPoolCreateInfo*, const VkAllocationCallbacks*, VkCommandPool* pool) {
            *pool = handle<VkCommandPool>(); return VK_SUCCESS;
        };
        df.DestroyCommandPool = [](VkDevice, VkCommandPool, const VkAllocationCallbacks*) {};
        df.CreatePipelineCache = [](VkDevice, const VkPipelineCacheCreateInfo*, const VkAllocationCallbacks*, VkPipelineCache* cache) {
            *cache = handle<VkPipelineCache>(); return VK_SUCCESS;
        };
        df.DestroyPipelineCache = [](VkDevice, VkPipelineCache, const VkAllocationCallbacks*) {};
        df.CreateShaderModule = [](VkDevice, const VkShaderModuleCreateInfo*, const VkAllocationCallbacks*, VkShaderModule* module) {
            *module = handle<VkShaderModule>(++observed.modules); return VK_SUCCESS;
        };
        df.DestroyShaderModule = [](VkDevice, VkShaderModule, const VkAllocationCallbacks*) { ++observed.moduleDestructions; };
        df.CreateDescriptorSetLayout = [](VkDevice, const VkDescriptorSetLayoutCreateInfo*, const VkAllocationCallbacks*, VkDescriptorSetLayout* layout) {
            *layout = handle<VkDescriptorSetLayout>(); return VK_SUCCESS;
        };
        df.DestroyDescriptorSetLayout = [](VkDevice, VkDescriptorSetLayout, const VkAllocationCallbacks*) {};
        df.CreatePipelineLayout = [](VkDevice, const VkPipelineLayoutCreateInfo*, const VkAllocationCallbacks*, VkPipelineLayout* layout) {
            *layout = handle<VkPipelineLayout>(++observed.layouts); return VK_SUCCESS;
        };
        df.DestroyPipelineLayout = [](VkDevice, VkPipelineLayout, const VkAllocationCallbacks*) {
            ++observed.layoutDestructions;
        };
        df.CreateComputePipelines = [](VkDevice, VkPipelineCache, uint32_t count, const VkComputePipelineCreateInfo* info, const VkAllocationCallbacks*, VkPipeline* pipeline) {
            require(count == 1 && info->layout == handle<VkPipelineLayout>(), "split pipeline changed layout");
            if (observed.failPipeline == observed.pipelines + 1) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
            *pipeline = handle<VkPipeline>(++observed.pipelines); return VK_SUCCESS;
        };
        df.DestroyPipeline = [](VkDevice, VkPipeline, const VkAllocationCallbacks*) {
            require(observed.layoutDestructions == 0, "layout destroyed before its pipelines");
            ++observed.pipelineDestructions;
        };
        df.CreateDescriptorPool = [](VkDevice, const VkDescriptorPoolCreateInfo*, const VkAllocationCallbacks*, VkDescriptorPool* pool) {
            *pool = handle<VkDescriptorPool>(); return VK_SUCCESS;
        };
        df.DestroyDescriptorPool = [](VkDevice, VkDescriptorPool, const VkAllocationCallbacks*) {};
        df.AllocateDescriptorSets = [](VkDevice, const VkDescriptorSetAllocateInfo*, VkDescriptorSet* set) {
            *set = handle<VkDescriptorSet>(); return VK_SUCCESS;
        };
        df.FreeDescriptorSets = [](VkDevice, VkDescriptorPool, uint32_t, const VkDescriptorSet*) { return VK_SUCCESS; };
        df.UpdateDescriptorSets = [](VkDevice, uint32_t, const VkWriteDescriptorSet*, uint32_t, const VkCopyDescriptorSet*) {};
        df.AllocateCommandBuffers = [](VkDevice, const VkCommandBufferAllocateInfo*, VkCommandBuffer* cmd) {
            *cmd = handle<VkCommandBuffer>(); return VK_SUCCESS;
        };
        df.FreeCommandBuffers = [](VkDevice, VkCommandPool, uint32_t, const VkCommandBuffer*) {};
        df.CmdPipelineBarrier = [](VkCommandBuffer, VkPipelineStageFlags, VkPipelineStageFlags, VkDependencyFlags,
                uint32_t, const VkMemoryBarrier*, uint32_t, const VkBufferMemoryBarrier*, uint32_t, const VkImageMemoryBarrier*) {
            ++observed.barriers;
        };
        df.CmdBindPipeline = [](VkCommandBuffer, VkPipelineBindPoint, VkPipeline pipeline) { observed.bound.push_back(pipeline); };
        df.CmdBindDescriptorSets = [](VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout layout, uint32_t,
                uint32_t, const VkDescriptorSet*, uint32_t, const uint32_t*) {
            require(layout == handle<VkPipelineLayout>(), "descriptor layout changed");
            ++observed.descriptors;
        };
        df.CmdDispatch = [](VkCommandBuffer, uint32_t x, uint32_t y, uint32_t z) {
            require(x == 80 && y == 45 && z == 1, "split changed workgroup geometry");
            ++observed.dispatches;
        };
        return {handle<VkInstance>(), handle<VkDevice>(), handle<VkPhysicalDevice>(), fi, df, 0, true};
    }
}

int main() {
    try {
        const auto vk = makeVulkan();
        const std::vector<uint8_t> code(4);
        for (bool split : {false, true}) {
            observed = {};
            {
                vk::Shader initial(vk, code, 0, 0, 0, 0,
                    split ? std::span<const uint8_t>(code) : std::span<const uint8_t>{});
                vk::Shader shader(std::move(initial));
                require(observed.modules == observed.moduleDestructions,
                    "shader modules retained after pipeline construction");
                const vk::DescriptorPool pool(vk, {.sets = 1});
                const vk::DescriptorSet set(vk, pool, shader, {}, {}, {}, {});
                const vk::CommandBuffer cmd(vk);
                cmd.dispatch(vk, shader, set, {}, 80, 45, 1);
                require(observed.dispatches == (split ? 2U : 1U) &&
                        observed.bound.size() == observed.dispatches,
                    "wrong number of pipeline dispatches");
                require(observed.bound[0] == shader.pipeline() &&
                        (!split || observed.bound[1] == shader.independentPipeline()),
                    "wrong dispatch pipeline order");
                require(observed.barriers == 1 && observed.descriptors == 1,
                    "split inserted a redundant dependency or descriptor bind");
            }
            require(observed.pipelines == observed.pipelineDestructions &&
                    observed.layouts == observed.layoutDestructions,
                "moved shader did not release every pipeline/layout exactly once");
        }
        for (uint32_t failure : {1U, 2U}) {
            observed = {.failPipeline = failure};
            bool failed{};
            try { const vk::Shader shader(vk, code, 0, 0, 0, 0, code); }
            catch (const std::exception&) { failed = true; }
            require(failed && observed.pipelines == observed.pipelineDestructions &&
                    observed.modules == observed.moduleDestructions &&
                    observed.layouts == observed.layoutDestructions,
                "partial pipeline construction leaked resources");
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
