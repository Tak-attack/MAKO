/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "../helpers/managed_shader.hpp"
#include "../helpers/utils.hpp"
#include "mako-common/helpers/pointers.hpp"
#include "mako-common/vulkan/command_buffer.hpp"
#include "mako-common/vulkan/image.hpp"
#include "mako-common/vulkan/vulkan.hpp"

#include <memory>
#include <span>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace ctx { struct Ctx; }

namespace mako::backend {
    /// delta refinement shaderchain
    class Delta1 {
    public:
        /// create a delta refinement shaderchain
        /// @param ctx context
        /// @param idx generated frame index
        /// @param sourceImages0 source images
        /// @param sourceImages1 source images
        /// @param temporaryImages0 same-extent temporary scratch from Gamma1
        /// @param additionalInput0 additional input image
        /// @param additionalInput1 additional input image
        /// @param additionalInput2 additional input image
        /// @param sharedScratch same-context first-output resources, or null to own them
        /// @param parallelScratch0 dead coarser Alpha0 scratch, or empty for serial execution
        /// @param parallelScratch1 second disjoint bank with the same format and extent
        Delta1(const Ctx& ctx, size_t idx,
            const std::vector<vk::Image>& sourceImages0,
            const std::vector<vk::Image>& sourceImages1,
            const std::vector<vk::Image>& temporaryImages0,
            const vk::Image& additionalInput0,
            const vk::Image& additionalInput1,
            const vk::Image& additionalInput2,
            const Delta1* sharedScratch = nullptr,
            std::span<const vk::Image> parallelScratch0 = {},
            std::span<const vk::Image> parallelScratch1 = {});

        /// prepare the shaderchain initially
        /// @param images vector to fill with image handles
        void prepare(std::vector<VkImage>& images) const;

        /// render the delta refinement shaderchain
        /// @param vk the vulkan instance
        /// @param cmd command buffer
        void render(const vk::Vulkan& vk, const vk::CommandBuffer& cmd) const;

        /// get the first generated image
        /// @return image
        [[nodiscard]] const auto& getImage0() const { return *this->image0; }

        /// get the second generated image
        /// @return image
        [[nodiscard]] const auto& getImage1() const { return *this->image1; }
    private:
        std::shared_ptr<vk::Image> image0;
        std::shared_ptr<vk::Image> image1;

        bool parallelBranches{false};
        std::vector<ManagedShader> sets;
        VkExtent2D dispatchExtent{};
    };
}
