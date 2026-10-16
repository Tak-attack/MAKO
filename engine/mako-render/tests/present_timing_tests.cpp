/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "swapchain/present_timing.hpp"

#include <cstdlib>
#include <iostream>
#include <vector>

using namespace mako::layer;

namespace {
    void expect(const bool condition, const char* message) {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            std::exit(1);
        }
    }
#if defined(VK_EXT_present_timing) && defined(VK_KHR_present_id2)
    using Clock = OwnedPresentTiming::Clock;
    uint64_t nowNs() {
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
            Clock::now().time_since_epoch()).count());
    }
    uint32_t queueSize{}, pastCalls{}, domainCalls{};
    uint64_t duration{16'666'667}, interval{8'333'333}, propertiesCounter{1}, domainCounter{1};
    uint64_t deviation{}, clockOffset{9'000'000'000};
    VkResult pastStatus{VK_SUCCESS}, sizeStatus{VK_SUCCESS}, calibrationStatus{VK_SUCCESS};
    VkResult propertiesStatus{VK_SUCCESS};
    bool availableDomain{true}, complete{true};
    struct Record { uint64_t id, time, domainId{7}; };
    std::vector<Record> ready;
    VkResult VKAPI_CALL setSize(VkDevice, VkSwapchainKHR, uint32_t size) {
        queueSize = size;
        return sizeStatus;
    }
    VkResult VKAPI_CALL properties(VkDevice, VkSwapchainKHR, VkSwapchainTimingPropertiesEXT* out, uint64_t* counter) {
        if (propertiesStatus != VK_SUCCESS)
            return propertiesStatus;
        out->refreshDuration = duration;
        out->refreshInterval = interval;
        *counter = propertiesCounter;
        return VK_SUCCESS;
    }
    VkResult VKAPI_CALL domains(VkDevice, VkSwapchainKHR, VkSwapchainTimeDomainPropertiesEXT* out, uint64_t* counter) {
        ++domainCalls;
        *counter = domainCounter;
        out->timeDomainCount = 1;
        out->pTimeDomains[0] = availableDomain ? VK_TIME_DOMAIN_PRESENT_STAGE_LOCAL_EXT : VK_TIME_DOMAIN_DEVICE_KHR;
        out->pTimeDomainIds[0] = 7;
        return VK_SUCCESS;
    }
    VkResult VKAPI_CALL calibrate(VkDevice, uint32_t count, const VkCalibratedTimestampInfoKHR* info,
            uint64_t* timestamps, uint64_t* maxDeviation) {
        expect(count == 1 && info->timeDomain == VK_TIME_DOMAIN_PRESENT_STAGE_LOCAL_EXT,
            "calibrate the selected presentation clock");
        const auto* local = static_cast<const VkSwapchainCalibratedTimestampInfoEXT*>(info->pNext);
        expect(local && local->presentStage == OwnedPresentTiming::queueStage && local->timeDomainId == 7,
            "calibration identifies this swapchain stage and domain");
        timestamps[0] = nowNs() + clockOffset;
        *maxDeviation = deviation;
        return calibrationStatus;
    }
    VkResult VKAPI_CALL past(VkDevice, const VkPastPresentationTimingInfoEXT* info,
            VkPastPresentationTimingPropertiesEXT* out) {
        ++pastCalls;
        expect(info->flags == VK_PAST_PRESENTATION_TIMING_ALLOW_OUT_OF_ORDER_RESULTS_BIT_EXT,
            "completed records may arrive out of order; partial results are not requested");
        expect(out->presentationTimingCount == OwnedPresentTiming::capacity,
            "queries have bounded capacity");
        out->timingPropertiesCounter = propertiesCounter;
        out->timeDomainsCounter = domainCounter;
        out->presentationTimingCount = static_cast<uint32_t>(ready.size());
        for (size_t i = 0; i < ready.size(); ++i) {
            auto& record = out->pPresentationTimings[i];
            expect(record.sType == VK_STRUCTURE_TYPE_PAST_PRESENTATION_TIMING_EXT && record.presentStageCount == 1,
                "initialize output structs and stage capacity");
            record.presentId = ready[i].id;
            record.timeDomain = VK_TIME_DOMAIN_PRESENT_STAGE_LOCAL_EXT;
            record.timeDomainId = ready[i].domainId;
            record.reportComplete = complete;
            record.pPresentStages[0] = {OwnedPresentTiming::queueStage, ready[i].time};
        }
        ready.clear();
        return pastStatus;
    }
    const OwnedPresentTiming::Functions functions{setSize, properties, domains, past, calibrate};
#endif
}

int main() {
#if defined(VK_EXT_present_timing) && defined(VK_KHR_present_id2)
    VkDeviceCreateInfo device{.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    expect(OwnedPresentTiming::ownsNamespace(device), "unused device timing can be owned");
    for (const char* name : {"VK_EXT_present_timing", "VK_GOOGLE_display_timing", "VK_KHR_present_id2", "VK_KHR_present_id"}) {
        device.enabledExtensionCount = 1;
        device.ppEnabledExtensionNames = &name;
        expect(!OwnedPresentTiming::ownsNamespace(device), "preserve application timing and ID namespaces");
    }
    device.enabledExtensionCount = 0;
    VkPhysicalDevicePresentWaitFeaturesKHR waits{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR, .presentWait = VK_TRUE};
    device.pNext = &waits;
    expect(!OwnedPresentTiming::ownsNamespace(device), "application completion waits retain ownership");
    VkPhysicalDevicePresentId2FeaturesKHR ids{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_2_FEATURES_KHR};
    device.pNext = &ids;
    expect(!OwnedPresentTiming::ownsNamespace(device), "even disabled caller nodes are not duplicated");
    VkPresentTimingSurfaceCapabilitiesEXT caps{
        .sType = VK_STRUCTURE_TYPE_PRESENT_TIMING_SURFACE_CAPABILITIES_EXT,
        .presentTimingSupported = VK_TRUE, .presentAtAbsoluteTimeSupported = VK_TRUE,
        .presentStageQueries = OwnedPresentTiming::queueStage};
    VkSurfaceCapabilitiesPresentId2KHR surfaceIds{
        .sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_PRESENT_ID_2_KHR, .presentId2Supported = VK_TRUE};
    expect(OwnedPresentTiming::surfaceSupported(caps, surfaceIds), "surface must support the complete contract");
    caps.presentStageQueries = VK_PRESENT_STAGE_IMAGE_FIRST_PIXEL_OUT_BIT_EXT;
    expect(!OwnedPresentTiming::surfaceSupported(caps, surfaceIds), "predicted display time cannot replace observed queue completion");

    OwnedPresentTiming timing;
    expect(!timing.initialize(VK_NULL_HANDLE, VK_NULL_HANDLE, {}), "missing commands retain existing pacing");
    availableDomain = false;
    expect(!timing.initialize(VK_NULL_HANDLE, VK_NULL_HANDLE, functions), "unsupported clocks retain existing pacing");
    availableDomain = true;
    deviation = 2'000'000;
    expect(!timing.initialize(VK_NULL_HANDLE, VK_NULL_HANDLE, functions), "imprecise calibration is rejected");
    deviation = 0;
    sizeStatus = VK_ERROR_OUT_OF_HOST_MEMORY;
    expect(!timing.initialize(VK_NULL_HANDLE, VK_NULL_HANDLE, functions), "feedback storage failure is optional");
    sizeStatus = VK_SUCCESS;
    expect(timing.initialize(VK_NULL_HANDLE, VK_NULL_HANDLE, functions) && queueSize == 64,
        "initialize bounded timing storage");
    VkPresentModeKHR mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    VkSwapchainPresentModeInfoEXT modes{.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODE_INFO_EXT,
        .swapchainCount = 1, .pPresentModes = &mode};
    timing.observePresentMode(&modes);
    timing.observePresentMode(nullptr);
    expect(timing.active() && !timing.controlsOutput(), "dynamic non-FIFO mode persists and disables timing targets");
    mode = VK_PRESENT_MODE_FIFO_KHR;
    timing.observePresentMode(&modes);
    expect(timing.controlsOutput(), "explicit FIFO return resumes supported targets");
    OrderedPresentTimeline unknownRefreshTimeline;
    duration = interval = 0;
    ++propertiesCounter;
    timing.update(unknownRefreshTimeline, 0);
    expect(timing.active() && !timing.controlsOutput() && timing.controlsOutput(120),
        "unknown timing properties retain direct pacing unless an external refresh clock is known");
    duration = 16'666'667;
    interval = 8'333'333;
    ++propertiesCounter;
    timing.update(unknownRefreshTimeline, 1);
    expect(timing.controlsOutput(), "available refresh properties can take pacing ownership");
    expect(timing.outputRate(120) > 59.9 && timing.outputRate(120) < 60.1 && timing.refreshRate(0) > 119.9,
        "compositor cap and fixed panel refresh remain separate");
    OrderedPresentTimeline timeline;
    VkPresentTimingInfoEXT request{};
    uint64_t id{};
    const auto target = Clock::now() + std::chrono::milliseconds(30);
    timeline.observeSourceBatch(Clock::now(), 1);
    static_cast<void>(timeline.schedule(Clock::now(), 60, 120, 2));
    expect(timing.prepare(target, timeline, request, id) && id == 1 &&
            request.presentStageQueries == OwnedPresentTiming::queueStage && request.flags == 0,
        "request absolute timing and only the supported observed stage");
    const auto targetNs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(target.time_since_epoch()).count());
    expect(request.targetTime > targetNs + clockOffset - 1'000'000 &&
            request.targetTime < targetNs + clockOffset + 1'000'000,
        "translate targets into the calibrated domain");
    for (uint32_t i = 1; i < OwnedPresentTiming::capacity; ++i)
        expect(timing.prepare(target, timeline, request, id) && request.presentStageQueries,
            "reserve the remaining bounded slots");
    expect(timing.prepare(target, timeline, request, id) && !request.presentStageQueries && id == 65,
        "missing feedback does not cause queue-full errors or unbounded storage");
    ready = {{2, nowNs() + clockOffset}, {2, nowNs() + clockOffset}, {999, 0}, {1, 0}};
    timing.update(timeline, 2);
    expect(timing.outstanding() == 62, "out-of-order completion and drops retire once; unknown IDs do not");
    const auto calls = pastCalls;
    timing.update(timeline, 2);
    expect(pastCalls == calls, "only one feedback query per source batch");
    expect(timing.prepare(target, timeline, request, id) && request.presentStageQueries && id == 66,
        "feedback releases capacity without reusing IDs");
    complete = false;
    ready = {{3, nowNs() + clockOffset}};
    timing.update(timeline, 3);
    expect(timing.outstanding() == 63, "partial feedback cannot release reservations");
    complete = true;
    ready = {{3, nowNs() + clockOffset, 99}};
    interval = UINT64_MAX;
    duration = 11'111'111;
    ++propertiesCounter;
    timing.update(timeline, 4);
    expect(timing.outstanding() == 62 && timing.outputRate(120) > 89.9 && timing.refreshRate(120) < 90.1,
        "invalid clock records retire without training; VRR properties update the effective ceiling");
    ++domainCounter;
    const auto domainsBefore = domainCalls;
    timing.update(timeline, 5);
    expect(domainCalls == domainsBefore + 1 && timing.active(), "domain changes trigger bounded recalibration");
    pastStatus = VK_ERROR_SURFACE_LOST_KHR;
    timing.update(timeline, 6);
    expect(!timing.active() && !timing.prepare(target, timeline, request, id),
        "query failures return control to existing pacing");
    const auto fallbackNow = Clock::now() + std::chrono::seconds(1);
    OrderedPresentTimeline legacy;
    expect(timeline.schedule(fallbackNow, 120, 120, 2)->presentAt ==
            legacy.schedule(fallbackNow, 120, 120, 2)->presentAt,
        "query failure restores legacy preparation lead, not observed-timing startup lead");

    pastStatus = VK_SUCCESS;
    OwnedPresentTiming clockFailure;
    OrderedPresentTimeline clockTimeline;
    expect(clockFailure.initialize(VK_NULL_HANDLE, VK_NULL_HANDLE, functions),
        "initialize the clock-loss scenario");
    clockTimeline.observeSourceBatch(Clock::now(), 1);
    const auto reserved = clockTimeline.schedule(Clock::now(), 60, 120, 2)->presentAt;
    ++domainCounter;
    calibrationStatus = VK_ERROR_UNKNOWN;
    clockFailure.update(clockTimeline, 2);
    expect(!clockFailure.active(), "loss of calibrated clock disables optional timing");
    expect(clockTimeline.schedule(Clock::now(), 120, 120, 2)->presentAt > reserved,
        "clock-loss fallback preserves already reserved output ordering");
    OrderedPresentTimeline legacyClock;
    expect(clockTimeline.schedule(fallbackNow, 120, 120, 2)->presentAt ==
            legacyClock.schedule(fallbackNow, 120, 120, 2)->presentAt,
        "clock-loss fallback also restores legacy preparation lead");

    calibrationStatus = VK_SUCCESS;
    OwnedPresentTiming propertiesFailure;
    propertiesStatus = VK_ERROR_UNKNOWN;
    expect(!propertiesFailure.initialize(VK_NULL_HANDLE, VK_NULL_HANDLE, functions),
        "unreadable initial timing properties retain existing pacing");
    propertiesStatus = VK_SUCCESS;
    expect(propertiesFailure.initialize(VK_NULL_HANDLE, VK_NULL_HANDLE, functions),
        "initialize the timing-property failure scenario");
    OrderedPresentTimeline propertiesTimeline;
    propertiesTimeline.observeSourceBatch(Clock::now(), 1);
    static_cast<void>(propertiesTimeline.schedule(Clock::now(), 60, 120, 2));
    propertiesStatus = VK_ERROR_UNKNOWN;
    ++propertiesCounter;
    propertiesFailure.update(propertiesTimeline, 2);
    const auto failedPropertiesCalls = pastCalls;
    propertiesFailure.update(propertiesTimeline, 3);
    expect(!propertiesFailure.controlsOutput(120) && !propertiesFailure.active() &&
            pastCalls == failedPropertiesCalls,
        "unreadable changed properties cannot keep stale compositor limits or retry every frame");
    OrderedPresentTimeline legacyProperties;
    expect(propertiesTimeline.schedule(fallbackNow, 120, 120, 2)->presentAt ==
            legacyProperties.schedule(fallbackNow, 120, 120, 2)->presentAt,
        "timing-property failure restores legacy preparation lead");

    OwnedPresentTiming delayedProperties;
    OrderedPresentTimeline delayedTimeline;
    propertiesStatus = VK_NOT_READY;
    expect(delayedProperties.initialize(VK_NULL_HANDLE, VK_NULL_HANDLE, functions) &&
            !delayedProperties.controlsOutput() && delayedProperties.controlsOutput(120),
        "delayed startup properties preserve the transport and use only a known external clock");
    propertiesStatus = VK_SUCCESS;
    duration = 16'666'667;
    interval = 8'333'333;
    delayedProperties.update(delayedTimeline, 1);
    expect(delayedProperties.controlsOutput() && delayedProperties.outputRate(120) < 60.1,
        "startup properties become usable without requiring a counter change");
    propertiesStatus = VK_NOT_READY;
    ++propertiesCounter;
    delayedProperties.update(delayedTimeline, 2);
    expect(delayedProperties.active() && !delayedProperties.controlsOutput() &&
            delayedProperties.outputRate(120) == 120,
        "temporarily unavailable changed properties discard the old compositor limit");
    propertiesStatus = VK_SUCCESS;
    duration = 11'111'111;
    delayedProperties.update(delayedTimeline, 3);
    expect(delayedProperties.controlsOutput() && delayedProperties.outputRate(120) > 89.9 &&
            delayedProperties.outputRate(120) < 90.1,
        "changed properties recover on the next batch even with the same counter");
#else
    expect(!OwnedPresentTiming{}.active(), "older manual-build headers keep timing disabled");
#endif
    std::cout << "Owned presentation timing tests passed\n";
}
