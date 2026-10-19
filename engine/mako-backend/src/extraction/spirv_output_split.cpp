/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "spirv_output_split.hpp"
#include "model_resource_validation.hpp"
#include "mako-common/helpers/errors.hpp"

#include <algorithm>
#include <cstring>
#include <map>

namespace {
    // Deliberately smaller than SPIR-V: no calls, control flow, pointer
    // arithmetic, stores, atomics, barriers, extension instructions or storage
    // image reads. Such shaders retain the unmodified pipeline.
    bool supportedInstruction(const uint32_t opcode) {
        switch (opcode) {
            case 3: case 5: // Source, Name
            case 14: case 15: case 16: case 17: // Module declarations
            case 19: case 21: case 22: case 23: case 24: // Numeric types
            case 25: case 26: case 27: case 32: case 33: // Image/pointer/function types
            case 43: case 44: // Constant, ConstantComposite
            case 54: case 56: case 59: case 61: case 71: // Function, Variable, Load, Decorate
            case 79: case 80: case 81: // VectorShuffle, CompositeConstruct/Extract
            case 86: case 88: case 99: case 103: // SampledImage, Sample, Write, QuerySizeLod
            case 112: case 115: // ConvertUToF, FConvert
            case 129: case 131: case 133: case 136: case 145: // Pure arithmetic
            case 248: case 253: // Label, Return
                return true;
            default: return false;
        }
    }
}

std::optional<std::array<std::vector<uint8_t>, 2>>
mako::backend::detail::splitIndependentImageWrites(const std::span<const uint8_t> source) {
    // Reuse the model loader's interface/structure owner. Fingerprints are not
    // compatibility gates: equivalent relocated or revised resources can qualify.
    try {
        if (!validateSpirvComputeShader(source, {4, 4, 0, 1}, "output split").exactBindings)
            return std::nullopt;
    } catch (const ls::error&) {
        return std::nullopt;
    }
    std::vector<uint32_t> words(source.size() / sizeof(uint32_t));
    std::memcpy(words.data(), source.data(), source.size());
    std::map<uint32_t, uint32_t> bindings, imageTypes, variableTypes, loadedTypes;
    std::map<uint32_t, uint32_t> outputLoads;
    std::array<size_t, 4> writes{};
    size_t functions{}, functionEnds{}, labels{}, returns{}, entries{}, localSizes{};
    for (size_t i = 5; i < words.size();) {
        const auto count = words[i] >> 16;
        const auto opcode = words[i] & 0xffffU;
        // The canonical validator has already checked all instruction bounds.
        if (!supportedInstruction(opcode)) return std::nullopt;
        switch (opcode) {
            case 14: // Logical addressing and GLSL450 memory model only.
                if (count != 3 || words[i + 1] != 0 || words[i + 2] != 1)
                    return std::nullopt;
                break;
            case 15: ++entries; break;
            case 16: // Fixed 8x8x1 workgroups; no other execution modes.
                if (count != 6 || words[i + 2] != 17 || words[i + 3] != 8 ||
                        words[i + 4] != 8 || words[i + 5] != 1) return std::nullopt;
                ++localSizes;
                break;
            case 25: // Only ordinary 2D sampled/storage images.
                if (count != 9 || words[i + 3] != 1 ||
                        (words[i + 4] != 0 && words[i + 4] != 2) ||
                        words[i + 5] != 0 || words[i + 6] != 0 ||
                        (words[i + 7] != 1 && words[i + 7] != 2)) return std::nullopt;
                imageTypes.emplace(words[i + 1], words[i + 7]);
                break;
            case 32: // No writable memory except the four storage images.
                if (count != 4 || words[i + 2] > 1) return std::nullopt;
                break;
            case 54: ++functions; break;
            case 56: ++functionEnds; break;
            case 59:
                if (count != 4 || words[i + 3] > 1) return std::nullopt;
                variableTypes.emplace(words[i + 2], words[i + 1]);
                break;
            case 71:
                if (count < 3) return std::nullopt;
                // Binding, DescriptorSet, BuiltIn, RelaxedPrecision and
                // NoContraction carry no extra memory or invocation semantics.
                if (words[i + 2] != 33 && words[i + 2] != 34 && words[i + 2] != 11 &&
                        words[i + 2] != 0 && words[i + 2] != 42) return std::nullopt;
                // Geometry built-ins are stable across the two dispatches;
                // subgroup identities and other execution-dependent inputs are not.
                if (words[i + 2] == 11 &&
                        (count != 4 || words[i + 3] < 24 || words[i + 3] > 28))
                    return std::nullopt;
                if (words[i + 2] == 33) {
                    if (count != 4) return std::nullopt;
                    bindings.emplace(words[i + 1], words[i + 3]);
                }
                break;
            case 61: {
                if (count != 4 || !variableTypes.contains(words[i + 3])) return std::nullopt;
                loadedTypes.emplace(words[i + 2], words[i + 1]);
                const auto type = imageTypes.find(words[i + 1]);
                if (type != imageTypes.end() && type->second == 2) {
                    const auto binding = bindings.find(words[i + 3]);
                    if (binding == bindings.end() || binding->second < 48 || binding->second > 51)
                        return std::nullopt;
                    outputLoads.emplace(words[i + 2], binding->second - 48);
                }
                break;
            }
            case 86: case 103: { // Sampling/querying must never read an output.
                if (count != 5) return std::nullopt;
                const auto loaded = loadedTypes.find(words[i + 3]);
                if (loaded == loadedTypes.end()) return std::nullopt;
                const auto type = imageTypes.find(loaded->second);
                if (type == imageTypes.end() || type->second != 1) return std::nullopt;
                break;
            }
            case 99: {
                // Explicit ImageOperands None is equivalent to omitting it.
                // Memory/visibility qualifiers would need separate analysis.
                if (count != 4 && (count != 5 || words[i + 4] != 0)) return std::nullopt;
                const auto output = outputLoads.find(words[i + 1]);
                if (output == outputLoads.end() || ++writes[output->second] != 1)
                    return std::nullopt;
                break;
            }
            case 248: ++labels; break;
            case 253: ++returns; break;
            default: break;
        }
        i += count;
    }
    if (functions != 1 || functionEnds != 1 || labels != 1 || returns != 1 ||
            entries != 1 || localSizes != 1 ||
            !std::ranges::all_of(writes, [](const auto count) { return count == 1; }))
        return std::nullopt;

    std::array<std::vector<uint8_t>, 2> result;
    for (size_t group = 0; group < result.size(); ++group) {
        auto& bytes = result[group];
        bytes.reserve(source.size());
        bytes.insert(bytes.end(), source.begin(), source.begin() + 5 * sizeof(uint32_t));
        for (size_t i = 5; i < words.size();) {
            const auto count = words[i] >> 16;
            const auto opcode = words[i] & 0xffffU;
            bool retain = opcode != 99 || outputLoads.at(words[i + 1]) / 2 == group;
            if (opcode == 61) {
                // A storage-image handle load is itself a static descriptor
                // use. Remove the other pair's unused loads too, so Vulkan
                // sees disjoint access without relying on driver DCE.
                const auto output = outputLoads.find(words[i + 2]);
                if (output != outputLoads.end() && output->second / 2 != group)
                    retain = false;
            }
            if (opcode == 5 || opcode == 71) {
                // Debug names/decorations must not reference a removed handle.
                if (count < 2) return std::nullopt;
                const auto output = outputLoads.find(words[i + 1]);
                if (output != outputLoads.end() && output->second / 2 != group)
                    retain = false;
            }
            if (retain)
                bytes.insert(bytes.end(), source.begin() + i * sizeof(uint32_t),
                    source.begin() + (i + count) * sizeof(uint32_t));
            i += count;
        }
    }
    return result;
}
