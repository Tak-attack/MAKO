/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "../helpers/managed_shader.hpp"
#include "../helpers/utils.hpp"
#include "mako-common/vulkan/command_buffer.hpp"
#include "mako-common/vulkan/image.hpp"
#include "mako-common/vulkan/vulkan.hpp"

#include <functional>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace ctx { struct Ctx; }

namespace mako::backend {
    /// pre-alpha shaderchain
    class Alpha0 {
    public:
        /// create a pre-alpha shaderchain
        /// @param ctx context
        /// @param sourceImage source image
        Alpha0(const Ctx& ctx,
            const vk::Image& sourceImage);

        /// prepare the shaderchain initially
        /// @param images vector to fill with image handles
        void prepare(std::vector<VkImage>& images) const;

        /// Describe one independent pyramid stage for grouped recording.
        [[nodiscard]] ManagedDispatch getDispatch(size_t index) const {
            return {std::cref(this->sets.at(index)), index == 2 ? this->dispatchExtent1 : this->dispatchExtent0};
        }

        /// Dead after the prepass; matching coarser-level images can serve Delta1.
        [[nodiscard]] const auto& getTemporaryImages0() const { return this->tempImages0; }
        [[nodiscard]] const auto& getTemporaryImages1() const { return this->tempImages1; }

        /// get the generated images
        /// @return vector of images
        [[nodiscard]] const auto& getImages() const { return this->images; }
    private:
        std::vector<vk::Image> tempImages0;
        std::vector<vk::Image> tempImages1;
        std::vector<vk::Image> images;

        std::vector<ManagedShader> sets;
        VkExtent2D dispatchExtent0{};
        VkExtent2D dispatchExtent1{};
    };
}
