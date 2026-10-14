/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "swapchain/spatial_command_recording.hpp"
#include <iostream>
#include <stdexcept>
#include <type_traits>

namespace {
    void require(bool condition) {
        if (!condition) throw std::runtime_error("spatial command recording contract failed");
    }
    template<typename T> T handle(size_t value) {
        if constexpr (std::is_pointer_v<T>) return reinterpret_cast<T>(value);
        else return static_cast<T>(value);
    }
}
int main() {
    mako::layer::SpatialCommandRecording recording;
    const auto app = handle<VkImage>(1);
    const auto source = handle<VkImage>(2);
    size_t calls = 0;
    auto record = [&] { ++calls; };
    for (size_t i = 0; i < 1000; ++i) recording.update(app, VK_NULL_HANDLE, record);
    require(calls == 1);
    recording.update(app, source, record);
    recording.update(app, source, record);
    require(calls == 2);
    recording.update(app, VK_NULL_HANDLE, record);
    require(calls == 3);
    recording.update(source, VK_NULL_HANDLE, record);
    require(calls == 4);
    recording.invalidate(); // Same handles can name a replacement resource graph.
    recording.update(source, VK_NULL_HANDLE, record);
    require(calls == 5);
    bool failed = false;
    try { recording.update(app, source, [] { throw std::runtime_error("record failed"); }); }
    catch (const std::runtime_error&) { failed = true; }
    require(failed);
    // A failed new recording also invalidates the previous successful key.
    recording.update(source, VK_NULL_HANDLE, record);
    require(calls == 6);
    recording.update(app, source, record);
    require(calls == 7);
    mako::layer::SpatialCommandRecording anotherPass;
    anotherPass.update(app, source, record);
    require(calls == 8);
    std::cout << "Spatial command reuse, invalidation, and failed-record retry passed\n";
}
