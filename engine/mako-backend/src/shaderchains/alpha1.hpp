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
    /// alpha shaderchain
    class Alpha1 {
    public:
        /// create a alpha shaderchain
        /// @param ctx context
        /// @param temporal temporal count
        /// @param sourceImages source images
        Alpha1(const Ctx& ctx, size_t temporal,
            const std::vector<vk::Image>& sourceImages);

        /// prepare the shaderchain initially
        /// @param images vector to fill with image handles
        void prepare(std::vector<VkImage>& images) const;

        /// Describe the selected history slot for grouped recording.
        [[nodiscard]] ManagedDispatch getDispatch(size_t index) const {
            return {std::cref(this->sets.at(index % this->sets.size())), this->dispatchExtent};
        }

        /// get the generated images
        /// @return vector of images
        [[nodiscard]] const auto& getImages() const { return this->images; }
    private:
        std::vector<std::vector<vk::Image>> images;

        std::vector<ManagedShader> sets;
        VkExtent2D dispatchExtent{};
    };
}
