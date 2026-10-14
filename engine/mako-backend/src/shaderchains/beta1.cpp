/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "beta1.hpp"
#include "../helpers/image_prefix.hpp"
#include "../helpers/utils.hpp"
#include "mako-common/helpers/pointers.hpp"
#include "mako-common/vulkan/command_buffer.hpp"
#include "mako-common/vulkan/image.hpp"
#include "mako-common/vulkan/vulkan.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace mako::backend;

Beta1::Beta1(const Ctx& ctx,
        const std::vector<vk::Image>& sourceImages,
        const std::vector<vk::Image>& prepassScratch)
        : tempImages0(backend::requiredPrefix(prepassScratch, 2, "Beta1 Alpha0 scratch")),
          tempImages1(backend::requiredPrefix(sourceImages, 2, "Beta1 Beta0 scratch")) {
    const VkExtent2D extent = sourceImages.at(0).getExtent();

    // Alpha1 consumed Alpha0 before Beta1 runs. Its first dispatch consumes
    // Beta0, so the next dispatch can reuse those images as the second bank.
    // Compute dependencies and the context completion fence preserve ordering.
    for (const auto& image : this->tempImages0) {
        const auto scratchExtent = image.getExtent();
        if (scratchExtent.width != extent.width || scratchExtent.height != extent.height)
            throw std::invalid_argument("Beta1 prepass scratch extent does not match");
    }

    this->images.reserve(6);
    for (uint32_t i = 0; i < 6; i++)
        this->images.emplace_back(ctx.vk,
            backend::shift_extent(extent, i), ctx.imageMemoryPool,
            VK_FORMAT_R8_UNORM);

    // create descriptor sets
    const auto& shaders = (ctx.perf ?
        ctx.shaders.get().performance : ctx.shaders.get().quality).beta;
    this->sets.reserve(4);
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(sourceImages)
        .storages(this->tempImages0)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, ctx.pool, shaders.at(1)));
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(this->tempImages0)
        .storages(this->tempImages1)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, ctx.pool, shaders.at(2)));
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(this->tempImages1)
        .storages(this->tempImages0)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, ctx.pool, shaders.at(3)));
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(this->tempImages0)
        .storages(this->images)
        .sampler(ctx.bnbSampler)
        .buffer(ctx.constantBuffer)
        .build(ctx.vk, ctx.pool, shaders.at(4)));

    // store dispatch extents
    this->dispatchExtent0 = backend::add_shift_extent(extent, 7, 3);
    this->dispatchExtent1 = backend::add_shift_extent(extent, 31, 5);
}

void Beta1::prepare(std::vector<VkImage>& images) const {
    // Borrowed scratch is initialized by Alpha0/Beta0.
    for (const auto& img : this->images)
        images.push_back(img.handle());
}

void Beta1::render(const vk::Vulkan& vk, const vk::CommandBuffer& cmd) const {
    this->sets.at(0).dispatch(vk, cmd, this->dispatchExtent0);
    this->sets.at(1).dispatch(vk, cmd, this->dispatchExtent0);
    this->sets.at(2).dispatch(vk, cmd, this->dispatchExtent0);
    this->sets.at(3).dispatch(vk, cmd, this->dispatchExtent1);
}
