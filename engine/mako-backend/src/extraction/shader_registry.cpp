/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "shader_registry.hpp"
#include "spirv_image_format.hpp"
#include "spirv_output_split.hpp"
#include "../shaders/color_conversion_spirv.hpp"
#include "model_resource_validation.hpp"
#include "lsfg_shader_set.hpp"
#include "mako-common/helpers/errors.hpp"
#include "mako-common/vulkan/shader.hpp"
#include "mako-common/vulkan/vulkan.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>
#include <utility>

using namespace mako;
using namespace mako::backend;

namespace {
    bool splitOutputPipelinesSupported(const vk::Vulkan& vk) {
        VkPhysicalDeviceProperties properties{};
        vk.fi().GetPhysicalDeviceProperties(vk.physdev(), &properties);
        if (properties.vendorID != 0x1002 || properties.apiVersion < VK_API_VERSION_1_2)
            return false;
        VkPhysicalDeviceDriverProperties driver{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES,
        };
        VkPhysicalDeviceProperties2 extended{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = &driver,
        };
        vk.fi().GetPhysicalDeviceProperties2(vk.physdev(), &extended);
        // Register allocation and dead-code elimination are compiler-specific.
        // Other drivers keep the original path until separately qualified.
        return driver.driverID == VK_DRIVER_ID_MESA_RADV;
    }

    /// get the source code for a shader
    const std::vector<uint8_t>& getShaderSource(uint32_t id, bool perf,
            const DllResourceArchive& archive,
            const LsfgShaderSet& selection) {
        return selection.resource(archive, id, perf);
    }

    [[nodiscard]] const mako::backend::detail::LsfgShaderSpec& shaderSpec(
            const uint32_t id, const bool perf) {
        const auto specs = mako::backend::detail::lsfgShaderSpecs(perf);
        const auto found = std::ranges::find(
            specs, id, &mako::backend::detail::LsfgShaderSpec::logicalId
        );
        if (found == specs.end())
            throw ls::error("unknown LSFG shader contract: " + std::to_string(id));
        return *found;
    }

    [[nodiscard]] vk::Shader makeShader(
            const vk::Vulkan& vk, const uint32_t id, const bool perf,
            const DllResourceArchive& archive,
            const LsfgShaderSet& selection, const bool splitOutputs) {
        const auto& contract = shaderSpec(id, perf).contract;
        // These five Quality convolutions have four independent outputs. Their
        // chain owners bind disjoint RGBA scratch banks, retaining the original
        // dependency before and after the pair of dispatches.
        if (splitOutputs && !perf && selection.fp16 && !selection.translated &&
                (id == 270 || id == 260 || id == 261 || id == 264 || id == 265)) {
            if (auto split = detail::splitIndependentImageWrites(
                    getShaderSource(id, perf, archive, selection))) {
                return vk::Shader(vk, split->at(0), contract.sampledImages,
                    contract.storageImages, contract.uniformBuffers,
                    contract.samplers, split->at(1));
            }
        }
        return vk::Shader(
            vk, getShaderSource(id, perf, archive, selection),
            contract.sampledImages, contract.storageImages,
            contract.uniformBuffers, contract.samplers
        );
    }

}

ShaderRegistry backend::buildShaderRegistry(const vk::Vulkan& vk, bool fp16,
        const DllResourceArchive& archive, const std::filesystem::path& dll) {
    const auto selection = loadLsfgShaderSet(archive, dll, fp16);
    const bool splitOutputs = selection.fp16 && !selection.translated &&
        splitOutputPipelinesSupported(vk);
    if (selection.convertedFp16Stages)
        std::clog << "MAKO Renderer: LSFG experimental_fp16=forced; quality=unqualified; "
            << "converted_stages=" << selection.convertedFp16Stages << '\n';
    // patch the generate shader
    std::vector<uint8_t> generate_data = getShaderSource(256, false, archive, selection);
    std::vector<uint8_t> generate_data_hdr = generate_data;
    std::vector<uint8_t> generate_data_packed = generate_data;
    detail::patchStorageImageFormat(generate_data, 4); // Rgba8
    detail::patchStorageImageFormat(generate_data_hdr, 2); // Rgba16f
    detail::patchStorageImageFormat(generate_data_packed, 11); // Rgb10A2

    // load all other shaders
#define SHADER(id) makeShader(vk, id, PERF, archive, selection, splitOutputs)

    return {
#define PERF false
        .mipmaps = SHADER(255),
        .generate = vk::Shader(vk, generate_data,
            shaderSpec(256, PERF).contract.sampledImages,
            shaderSpec(256, PERF).contract.storageImages,
            shaderSpec(256, PERF).contract.uniformBuffers,
            shaderSpec(256, PERF).contract.samplers),
        .generate_hdr = vk::Shader(vk, generate_data_hdr,
            shaderSpec(256, PERF).contract.sampledImages,
            shaderSpec(256, PERF).contract.storageImages,
            shaderSpec(256, PERF).contract.uniformBuffers,
            shaderSpec(256, PERF).contract.samplers),
        .generate_pq_packed_source = std::move(generate_data_packed),
        .hdr10_pq_to_scrgb = vk::Shader(
            vk, embedded::hdr10PqToScRgbSpirv, 1, 1, 0, 1
        ),
        .scrgb_to_hdr10_pq = vk::Shader(
            vk, embedded::scRgbToHdr10PqSpirv, 1, 1, 0, 1
        ),
        .scrgb_to_hdr10_pq_packed =
            vk.supportsStorageImageExtendedFormats()
                ? std::optional<vk::Shader>(std::in_place,
                    vk, embedded::scRgbToHdr10PqPackedSpirv, 1, 1, 0, 1)
                : std::nullopt,
        .quality = {
            .alpha = {
                SHADER(267), SHADER(268), SHADER(269), SHADER(270)
            },
            .beta = {
                SHADER(275), SHADER(276), SHADER(277), SHADER(278),
                SHADER(279)
            },
            .gamma = {
                SHADER(257), SHADER(259), SHADER(260), SHADER(261),
                SHADER(262)
            },
            .delta = {
                SHADER(257), SHADER(263), SHADER(264), SHADER(265),
                SHADER(266), SHADER(258), SHADER(271), SHADER(272),
                SHADER(273), SHADER(274)
            }
        },
#undef PERF
#define PERF true
        .performance = {
            .alpha = {
                SHADER(267), SHADER(268), SHADER(269), SHADER(270)
            },
            .beta = {
                SHADER(275), SHADER(276), SHADER(277), SHADER(278),
                SHADER(279)
            },
            .gamma = {
                SHADER(257), SHADER(259), SHADER(260), SHADER(261),
                SHADER(262)
            },
            .delta = {
                SHADER(257), SHADER(263), SHADER(264), SHADER(265),
                SHADER(266), SHADER(258), SHADER(271), SHADER(272),
                SHADER(273), SHADER(274)
            }
        },
#undef PERF
        .is_fp16 = selection.fp16
    };

#undef SHADER
}

vk::Shader ShaderRegistry::packedPqShader(const vk::Vulkan& vk) const {
    const auto& contract = shaderSpec(256, false).contract;
    return vk::Shader(vk, this->generate_pq_packed_source, contract.sampledImages,
        contract.storageImages, contract.uniformBuffers, contract.samplers);
}
