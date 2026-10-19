/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "../helpers/pointers.hpp"
#include "vulkan.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace vk {
    /// vulkan shadermodule & pipeline wrapper
    class Shader {
    public:
        /// create a vulkan shader
        /// @param vk the vulkan instance
        /// @param code the SPIR-V bytecode
        /// @param sampledImages number of sampled images
        /// @param storageImages number of storage images
        /// @param buffers number of buffers
        /// @param samplers number of samplers
        /// @param independentCode optional second module with disjoint writes,
        /// identical layout, immutable shared inputs and no other side effects
        /// @throws ls::vulkan_error on failure
        Shader(const vk::Vulkan& vk, const std::vector<uint8_t>& code,
            size_t sampledImages, size_t storageImages,
            size_t buffers, size_t samplers,
            std::span<const uint8_t> independentCode = {});

        /// create a Vulkan shader directly from aligned SPIR-V words without
        /// constructing a process-startup byte vector
        Shader(const vk::Vulkan& vk, std::span<const uint32_t> code,
            size_t sampledImages, size_t storageImages,
            size_t buffers, size_t samplers);

        /// get the descriptor set layout
        /// @returns the descriptor set layout
        [[nodiscard]] const auto& descriptorlayout() const { return *this->descriptorLayout; }

        /// get the pipeline layout
        /// @returns the pipeline layout
        [[nodiscard]] const auto& pipelinelayout() const { return *this->pipelineLayout; }
        /// get the pipeline
        /// @returns the pipeline
        [[nodiscard]] const auto& pipeline() const { return *this->pipeline_; }
        /// Optional second dispatch, covered by the same external dependencies.
        [[nodiscard]] VkPipeline independentPipeline() const {
            return this->independentPipeline_ ? **this->independentPipeline_ : VK_NULL_HANDLE;
        }
    private:
        ls::owned_ptr<VkShaderModule> shaderModule;
        ls::owned_ptr<VkDescriptorSetLayout> descriptorLayout;

        ls::owned_ptr<VkPipelineLayout> pipelineLayout;
        ls::owned_ptr<VkPipeline> pipeline_;
        std::optional<ls::owned_ptr<VkPipeline>> independentPipeline_;
    };
}
