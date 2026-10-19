/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace mako::backend::detail {

    /// Split a validated, straight-line four-output convolution into two
    /// write-disjoint modules. All arithmetic, constants and precision survive
    /// byte-for-byte. Unused output writes and their handle loads are removed;
    /// the driver eliminates computations with no retained sink.
    /// Unsupported structure returns nullopt, preserving the original shader.
    /// Inputs must remain immutable and the four output images must not alias
    /// each other or an input for the duration of both dispatches.
    [[nodiscard]] std::optional<std::array<std::vector<uint8_t>, 2>>
    splitIndependentImageWrites(std::span<const uint8_t> source);

}
