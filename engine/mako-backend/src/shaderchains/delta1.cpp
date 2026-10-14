/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "delta1.hpp"
#include "../helpers/image_prefix.hpp"
#include "../helpers/managed_shader.hpp"
#include "../helpers/independent_dispatch.hpp"
#include "../helpers/delta_scratch.hpp"
#include "../helpers/utils.hpp"
#include "mako-common/helpers/pointers.hpp"
#include "mako-common/vulkan/command_buffer.hpp"
#include "mako-common/vulkan/image.hpp"
#include "mako-common/vulkan/vulkan.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <span>
#include <vector>

#include <vulkan/vulkan_core.h>

using namespace mako::backend;

Delta1::Delta1(const Ctx& ctx, size_t idx,
        const std::vector<vk::Image>& sourceImages0,
        const std::vector<vk::Image>& sourceImages1,
        const std::vector<vk::Image>& temporaryImages0,
        const vk::Image& additionalInput0,
        const vk::Image& additionalInput1,
        const vk::Image& additionalInput2, const Delta1* sharedScratch,
        std::span<const vk::Image> parallelScratch0,
        std::span<const vk::Image> parallelScratch1) {
    const size_t m = ctx.perf ? 1 : 2; // multiplier
    const VkExtent2D extent = sourceImages0.at(0).getExtent();
    const auto source1 = backend::requiredPrefix(sourceImages1, m, "Delta1 source 1");
    const auto temporary0 = backend::requiredPrefix(
        temporaryImages0, 2 * m, "Delta1 temporary 0"
    );

    // The first dispatch consumes Gamma0's outputs into Gamma1 scratch. Later
    // dispatches can overwrite those dead outputs and Gamma1's unused tail.
    // Keep Gamma1's first m images intact for the second half of this chain.
    const auto scratch = backend::deltaScratch(sourceImages0, sourceImages1, m);
    this->image0 = sharedScratch ? sharedScratch->image0
        : std::make_shared<vk::Image>(ctx.vk, extent,
            ctx.imageMemoryPool, VK_FORMAT_R16G16B16A16_SFLOAT);
    this->image1 = sharedScratch ? sharedScratch->image1
        : std::make_shared<vk::Image>(ctx.vk, extent,
            ctx.imageMemoryPool, VK_FORMAT_R16G16B16A16_SFLOAT);

    // The coarser Alpha0 half-resolution banks are dead after preprocessing.
    // Borrow them only when their extent and ownership match; never allocate
    // extra images to enable concurrency or overwrite first-branch inputs.
    std::vector<VkImage> liveImages{
        additionalInput0.handle(), additionalInput1.handle(), additionalInput2.handle(),
        this->image0->handle(), this->image1->handle(),
    };
    for (const auto& image : sourceImages0) liveImages.push_back(image.handle());
    for (const auto& image : sourceImages1) liveImages.push_back(image.handle());
    for (const auto& image : temporaryImages0) liveImages.push_back(image.handle());
    this->parallelBranches = backend::parallelScratchCompatible(
        parallelScratch0, parallelScratch1, m, extent, liveImages);
    const auto second0 = this->parallelBranches ? parallelScratch0 : temporary0.first(m);
    const auto second1 = this->parallelBranches ? parallelScratch1 : scratch.primary.first(m);

    // create descriptor sets
    const auto& shaders = (ctx.perf ?
        ctx.shaders.get().performance : ctx.shaders.get().quality).delta;
    this->sets.reserve(4 + 4);

    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(sourceImages0)
        .storages(temporary0)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, ctx.pool, shaders.at(1)));
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(temporary0)
        .storages(scratch.primary)
        .storages(scratch.spare)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, ctx.pool, shaders.at(2)));
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(scratch.primary)
        .sampleds(scratch.spare)
        .storages(temporary0)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, ctx.pool, shaders.at(3)));
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(temporary0)
        .sampled(additionalInput0)
        .sampled(additionalInput1)
        .storage(*this->image0)
        .sampler(ctx.bnbSampler)
        .sampler(ctx.eabSampler)
        .buffer(ctx.constantBuffers.at(idx))
        .build(ctx.vk, ctx.pool, shaders.at(4)));

    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(source1)
        .storages(second0)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, ctx.pool, shaders.at(6)));
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(second0)
        .storages(second1)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, ctx.pool, shaders.at(7)));
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(second1)
        .storages(second0)
        .sampler(ctx.bnbSampler)
        .build(ctx.vk, ctx.pool, shaders.at(8)));
    this->sets.emplace_back(ManagedShaderBuilder()
        .sampleds(second0)
        .sampled(additionalInput2)
        .storage(*this->image1)
        .sampler(ctx.bnbSampler)
        .sampler(ctx.eabSampler)
        .buffer(ctx.constantBuffers.at(idx))
        .build(ctx.vk, ctx.pool, shaders.at(9)));

    // store dispatch extents
    this->dispatchExtent = backend::add_shift_extent(extent, 7, 3);
}

void Delta1::prepare(std::vector<VkImage>& images) const {
    // Reused scratch is initialized once by its Gamma0/Gamma1 owners.
    images.push_back(this->image0->handle());
    images.push_back(this->image1->handle());
}

void Delta1::render(const vk::Vulkan& vk, const vk::CommandBuffer& cmd) const {
    if (this->parallelBranches) {
        for (size_t stage = 0; stage < 4; ++stage) {
            const std::array batch{
                ManagedDispatch{std::cref(this->sets.at(stage)), dispatchExtent},
                ManagedDispatch{std::cref(this->sets.at(stage + 4)), dispatchExtent},
            };
            ManagedShader::dispatchIndependent(vk, cmd, batch);
        }
    } else {
        for (const auto& set : this->sets)
            set.dispatch(vk, cmd, dispatchExtent);
    }
}
