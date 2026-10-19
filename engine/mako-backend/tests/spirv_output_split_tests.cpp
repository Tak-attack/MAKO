/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "extraction/spirv_output_split.hpp"

#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
    using Words = std::vector<uint32_t>;
    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }
    void instruction(Words& words, uint32_t opcode, std::initializer_list<uint32_t> operands) {
        words.push_back((static_cast<uint32_t>(operands.size() + 1) << 16) | opcode);
        words.insert(words.end(), operands);
    }
    std::vector<uint8_t> bytes(const Words& words) {
        std::vector<uint8_t> result(words.size() * sizeof(uint32_t));
        if (!result.empty()) std::memcpy(result.data(), words.data(), result.size());
        return result;
    }
    Words words(const std::vector<uint8_t>& bytes) {
        Words result(bytes.size() / sizeof(uint32_t));
        std::memcpy(result.data(), bytes.data(), bytes.size());
        return result;
    }
    // A synthetic module with the real descriptor interface but no licensed
    // constants, weights or instructions. Every output stores a zero vector.
    Words fixture() {
        Words w{0x07230203, 0x00010000, 0, 128, 0};
        instruction(w, 17, {1}); // Shader
        instruction(w, 14, {0, 1}); // Logical GLSL450
        instruction(w, 15, {5, 100, 0x6e69616d, 0, 81}); // Compute main
        instruction(w, 16, {100, 17, 8, 8, 1});
        instruction(w, 5, {110, 0x006f}); // Name a loaded output handle.
        for (uint32_t index = 0; index < 4; ++index) {
            instruction(w, 71, {40 + index, 33, 32 + index});
            instruction(w, 71, {40 + index, 34, 0});
            instruction(w, 71, {70 + index, 33, 48 + index});
            instruction(w, 71, {70 + index, 34, 0});
        }
        instruction(w, 71, {80, 33, 16});
        instruction(w, 71, {80, 34, 0});
        instruction(w, 71, {81, 11, 28}); // GlobalInvocationId
        instruction(w, 19, {1});
        instruction(w, 33, {2, 1});
        instruction(w, 22, {3, 32});
        instruction(w, 21, {4, 32, 0});
        instruction(w, 23, {5, 4, 2});
        instruction(w, 23, {6, 3, 4});
        instruction(w, 25, {7, 3, 1, 0, 0, 0, 1, 0});
        instruction(w, 25, {8, 3, 1, 0, 0, 0, 2, 4});
        instruction(w, 26, {9});
        instruction(w, 32, {11, 0, 7});
        instruction(w, 32, {12, 0, 8});
        instruction(w, 32, {13, 0, 9});
        instruction(w, 23, {14, 4, 3});
        instruction(w, 32, {15, 1, 14});
        instruction(w, 43, {4, 16, 0});
        instruction(w, 44, {5, 17, 16, 16});
        instruction(w, 43, {3, 18, 0});
        instruction(w, 44, {6, 19, 18, 18, 18, 18});
        for (uint32_t index = 0; index < 4; ++index) {
            instruction(w, 59, {11, 40 + index, 0});
            instruction(w, 59, {12, 70 + index, 0});
        }
        instruction(w, 59, {13, 80, 0});
        instruction(w, 59, {15, 81, 1});
        instruction(w, 54, {1, 100, 0, 2});
        instruction(w, 248, {101});
        instruction(w, 129, {6, 120, 19, 19});
        for (uint32_t index = 0; index < 4; ++index) {
            instruction(w, 61, {8, 110 + index, 70 + index});
            instruction(w, 99, {110 + index, 17, 120, 0});
        }
        instruction(w, 253, {});
        instruction(w, 56, {});
        return w;
    }
    size_t find(const Words& w, uint32_t opcode, size_t occurrence = 0) {
        for (size_t i = 5; i < w.size(); i += w[i] >> 16)
            if ((w[i] & 0xffffU) == opcode && occurrence-- == 0) return i;
        throw std::runtime_error("missing fixture instruction");
    }
    void rejected(const Words& w) {
        require(!mako::backend::detail::splitIndependentImageWrites(bytes(w)),
            "unsafe or unsupported module was split");
    }
    void testPartitionPreservesCalculations() {
        const auto source = fixture();
        const auto input = bytes(source);
        const auto split = mako::backend::detail::splitIndependentImageWrites(input);
        require(split.has_value(), "independent four-output shader rejected");
        require(input == bytes(source), "input module was mutated");
        for (size_t group = 0; group < 2; ++group) {
            Words expected(source.begin(), source.begin() + 5);
            for (size_t i = 5; i < source.size();) {
                const auto count = source[i] >> 16;
                const auto opcode = source[i] & 0xffffU;
                const bool otherWrite = opcode == 99 && (source[i + 1] - 110) / 2 != group;
                const bool otherHandle = opcode == 61 && (source[i + 2] - 110) / 2 != group;
                const bool otherName = opcode == 5 && (source[i + 1] - 110) / 2 != group;
                if (!otherWrite && !otherHandle && !otherName)
                    expected.insert(expected.end(), source.begin() + i, source.begin() + i + count);
                i += count;
            }
            require(words(split->at(group)) == expected,
                "split changed arithmetic, coordinates, types or an output assignment");
        }
    }
    void testUnknownAndDependentGraphsStayUnmodified() {
        auto unknownDepth = fixture();
        unknownDepth[find(unknownDepth, 25) + 4] = 2;
        unknownDepth[find(unknownDepth, 25, 1) + 4] = 2;
        require(mako::backend::detail::splitIndependentImageWrites(bytes(unknownDepth)).has_value(),
            "ordinary images with unknown depth declaration rejected");
        for (uint32_t opcode : {12, 57, 62, 65, 83, 98, 224, 227, 245, 249, 333, 65535}) {
            auto w = fixture();
            const auto index = find(w, 253);
            w.insert(w.begin() + index, (1U << 16) | opcode);
            rejected(w);
        }
        for (uint32_t opcode : {86, 103}) {
            auto w = fixture();
            Words read;
            instruction(read, opcode, {7, 121, 110, 16});
            w.insert(w.begin() + find(w, 253), read.begin(), read.end());
            rejected(w); // Attempt to sample/query an output.
        }
        auto w = fixture();
        w[find(w, 99, 1) + 1] = 110;
        rejected(w); // Duplicate output and missing second binding.
        w = fixture();
        w[find(w, 99) + 1] = 127;
        rejected(w); // Unresolved output alias.
        w = fixture();
        const auto write = find(w, 99);
        w.erase(w.begin() + write, w.begin() + write + 5);
        rejected(w);
        w = fixture();
        w[find(w, 99) + 4] = 2048; // VolatileTexel.
        rejected(w);
        w = fixture();
        w[find(w, 59) + 3] = 4; // Workgroup storage.
        rejected(w);
        w = fixture();
        w[find(w, 32) + 2] = 12; // StorageBuffer pointer.
        rejected(w);
        w = fixture();
        w[find(w, 16) + 3] = 16;
        rejected(w);
        w = fixture();
        w[find(w, 14) + 2] = 3; // Vulkan memory model.
        rejected(w);
        w = fixture();
        w[find(w, 71, 2) + 3] = 52; // Different storage binding.
        rejected(w);
        w = fixture();
        w[find(w, 71, 3) + 3] = 1; // Different descriptor set.
        rejected(w);
        w = fixture();
        w[find(w, 71, 18) + 3] = 41; // SubgroupLocalInvocationId.
        rejected(w);
        w = fixture();
        instruction(w, 248, {122});
        rejected(w); // More than one basic block.
    }
    void testMalformedInputs() {
        require(!mako::backend::detail::splitIndependentImageWrites({}), "empty module accepted");
        auto raw = bytes(fixture());
        raw.pop_back();
        require(!mako::backend::detail::splitIndependentImageWrites(raw), "unaligned module accepted");
        for (uint32_t badHeader : {0U, 0xffffffffU}) {
            auto w = fixture();
            w[0] = badHeader;
            rejected(w);
        }
        auto w = fixture();
        w[5] = 17; // Zero word count.
        rejected(w);
        w = fixture();
        w[5] = (65535U << 16) | 17;
        rejected(w);
        // Every truncation must either fail closed or retain a complete module.
        for (size_t size = 0; size < fixture().size(); ++size) {
            w = fixture();
            w.resize(size);
            rejected(w);
        }
    }
}

int main() {
    try {
        testPartitionPreservesCalculations();
        testUnknownAndDependentGraphsStayUnmodified();
        testMalformedInputs();
    } catch (const std::exception& error) {
        std::cerr << "Output split test failed: " << error.what() << '\n';
        return 1;
    }
}
