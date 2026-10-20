/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "mako-common/helpers/file_descriptors.hpp"
#include "mako-common/vulkan/image.hpp"
#include "mako-common/vulkan/buffer.hpp"
#include "mako-common/vulkan/command_buffer.hpp"
#include "mako-common/vulkan/external_image.hpp"
#include "mako-common/vulkan/semaphore.hpp"
#include "mako-common/vulkan/timeline_semaphore.hpp"

#include <array>
#include <cerrno>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace {
    void require(bool condition, const char* message) {
        if (!condition)
            throw std::runtime_error(message);
    }
    int descriptor() {
        const int fd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        require(fd >= 0, "could not create test descriptor");
        return fd;
    }
    bool isOpen(int fd) { return ::fcntl(fd, F_GETFD) != -1; }
    template<typename T> T handle() {
        if constexpr (std::is_pointer_v<T>)
            return reinterpret_cast<T>(1);
        else
            return static_cast<T>(1);
    }
    enum class Failure { None, Image, MemoryType, Allocate, Bind, View, Export,
        Semaphore, SemaphoreImport };
    struct RecordedBarrier {
        VkPipelineStageFlags source, destination;
        VkImageMemoryBarrier image;
    };
    std::vector<RecordedBarrier> recordedBarriers;
    std::vector<size_t> copyPositions;
    std::array<uint8_t, 4096> mappedBuffer{};
    struct Driver {
        Failure failure{Failure::None};
        int failImageIndex{0};
        int imageIndex{-1};
        int images{0}, memory{0}, views{0}, semaphores{0}, exports{0};
        std::vector<int> reused;
        bool fails(Failure stage) const {
            return failure == stage && imageIndex == failImageIndex;
        }
        void consume(int fd) {
            require(::close(fd) == 0, "driver received a closed descriptor");
            // Reuse the exact number immediately. A second close by the caller
            // would corrupt an unrelated live descriptor, not just return EBADF.
            const int replacement = descriptor();
            require(replacement == fd, "descriptor reuse precondition failed");
            reused.push_back(replacement);
        }
        void verify() {
            require(images == 0 && memory == 0 && views == 0 && semaphores == 0,
                "failed construction retained a Vulkan object");
            for (const int fd : reused) {
                require(isOpen(fd), "caller closed a descriptor already consumed by Vulkan");
                ::close(fd);
            }
            reused.clear();
        }
    } driver;

    vk::Vulkan makeVulkan() {
        vk::VulkanInstanceFuncs fi{};
        fi.GetPhysicalDeviceQueueFamilyProperties = [](VkPhysicalDevice, uint32_t* count, VkQueueFamilyProperties* props) {
            *count = 1;
            if (props) props[0].queueFlags = VK_QUEUE_COMPUTE_BIT;
        };
        fi.GetPhysicalDeviceMemoryProperties = [](VkPhysicalDevice, VkPhysicalDeviceMemoryProperties* props) {
            *props = {};
            props->memoryTypeCount = driver.fails(Failure::MemoryType) ? 0 : 1;
            props->memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        };
        vk::VulkanDeviceFuncs df{};
        df.GetDeviceQueue = [](VkDevice, uint32_t, uint32_t, VkQueue* queue) { *queue = handle<VkQueue>(); };
        df.CreateCommandPool = [](VkDevice, const VkCommandPoolCreateInfo*, const VkAllocationCallbacks*, VkCommandPool* pool) {
            *pool = handle<VkCommandPool>(); return VK_SUCCESS;
        };
        df.DestroyCommandPool = [](VkDevice, VkCommandPool, const VkAllocationCallbacks*) {};
        df.CreatePipelineCache = [](VkDevice, const VkPipelineCacheCreateInfo*, const VkAllocationCallbacks*, VkPipelineCache* cache) {
            *cache = handle<VkPipelineCache>(); return VK_SUCCESS;
        };
        df.DestroyPipelineCache = [](VkDevice, VkPipelineCache, const VkAllocationCallbacks*) {};
        df.CreateImage = [](VkDevice, const VkImageCreateInfo*, const VkAllocationCallbacks*, VkImage* image) {
            ++driver.imageIndex;
            if (driver.fails(Failure::Image)) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
            ++driver.images; *image = handle<VkImage>(); return VK_SUCCESS;
        };
        df.DestroyImage = [](VkDevice, VkImage, const VkAllocationCallbacks*) { --driver.images; };
        df.GetImageMemoryRequirements = [](VkDevice, VkImage, VkMemoryRequirements* reqs) { *reqs = {4096, 256, 1}; };
        df.AllocateMemory = [](VkDevice, const VkMemoryAllocateInfo* info, const VkAllocationCallbacks*, VkDeviceMemory* memory) {
            if (driver.fails(Failure::Allocate)) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
            const auto* imported = static_cast<const VkImportMemoryFdInfoKHR*>(info->pNext);
            if (imported && imported->sType == VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR)
                driver.consume(imported->fd);
            ++driver.memory; *memory = handle<VkDeviceMemory>(); return VK_SUCCESS;
        };
        df.FreeMemory = [](VkDevice, VkDeviceMemory, const VkAllocationCallbacks*) { --driver.memory; };
        df.BindImageMemory = [](VkDevice, VkImage, VkDeviceMemory, VkDeviceSize) {
            return driver.fails(Failure::Bind) ? VK_ERROR_OUT_OF_DEVICE_MEMORY : VK_SUCCESS;
        };
        df.CreateImageView = [](VkDevice, const VkImageViewCreateInfo*, const VkAllocationCallbacks*, VkImageView* view) {
            if (driver.fails(Failure::View)) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
            ++driver.views; *view = handle<VkImageView>(); return VK_SUCCESS;
        };
        df.DestroyImageView = [](VkDevice, VkImageView, const VkAllocationCallbacks*) { --driver.views; };
        df.GetMemoryFdKHR = [](VkDevice, const VkMemoryGetFdInfoKHR*, int* fd) {
            if (driver.fails(Failure::Export)) {
                *fd = 0; // Error outputs are undefined and must not be published.
                return VK_ERROR_TOO_MANY_OBJECTS;
            }
            ++driver.exports; *fd = descriptor(); return VK_SUCCESS;
        };
        df.CreateBuffer = [](VkDevice, const VkBufferCreateInfo*, const VkAllocationCallbacks*, VkBuffer* buffer) {
            *buffer = handle<VkBuffer>(); return VK_SUCCESS;
        };
        df.DestroyBuffer = [](VkDevice, VkBuffer, const VkAllocationCallbacks*) {};
        df.GetBufferMemoryRequirements = [](VkDevice, VkBuffer, VkMemoryRequirements* reqs) { *reqs = {4096, 256, 1}; };
        df.BindBufferMemory = [](VkDevice, VkBuffer, VkDeviceMemory, VkDeviceSize) { return VK_SUCCESS; };
        df.MapMemory = [](VkDevice, VkDeviceMemory, VkDeviceSize, VkDeviceSize, VkMemoryMapFlags, void** data) {
            *data = mappedBuffer.data(); return VK_SUCCESS;
        };
        df.UnmapMemory = [](VkDevice, VkDeviceMemory) {};
        df.AllocateCommandBuffers = [](VkDevice, const VkCommandBufferAllocateInfo*, VkCommandBuffer* command) {
            *command = handle<VkCommandBuffer>(); return VK_SUCCESS;
        };
        df.FreeCommandBuffers = [](VkDevice, VkCommandPool, uint32_t, const VkCommandBuffer*) {};
        df.CmdPipelineBarrier = [](VkCommandBuffer, VkPipelineStageFlags source, VkPipelineStageFlags destination,
                VkDependencyFlags, uint32_t, const VkMemoryBarrier*, uint32_t, const VkBufferMemoryBarrier*,
                uint32_t count, const VkImageMemoryBarrier* images) {
            for (uint32_t i = 0; i < count; ++i) recordedBarriers.push_back({source, destination, images[i]});
        };
        df.CmdCopyBufferToImage = [](VkCommandBuffer, VkBuffer, VkImage, VkImageLayout,
                uint32_t, const VkBufferImageCopy*) { copyPositions.push_back(recordedBarriers.size()); };
        df.CmdCopyImageToBuffer = [](VkCommandBuffer, VkImage, VkImageLayout, VkBuffer,
                uint32_t, const VkBufferImageCopy*) { copyPositions.push_back(recordedBarriers.size()); };
        df.CreateSemaphore = [](VkDevice, const VkSemaphoreCreateInfo*, const VkAllocationCallbacks*, VkSemaphore* semaphore) {
            if (driver.failure == Failure::Semaphore) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
            ++driver.semaphores; *semaphore = handle<VkSemaphore>(); return VK_SUCCESS;
        };
        df.DestroySemaphore = [](VkDevice, VkSemaphore, const VkAllocationCallbacks*) { --driver.semaphores; };
        df.ImportSemaphoreFdKHR = [](VkDevice, const VkImportSemaphoreFdInfoKHR* info) {
            if (driver.failure == Failure::SemaphoreImport) return VK_ERROR_INVALID_EXTERNAL_HANDLE;
            driver.consume(info->fd); return VK_SUCCESS;
        };
        return {handle<VkInstance>(), handle<VkDevice>(), handle<VkPhysicalDevice>(), fi, df, 0, true};
    }

    void testPartialExport(const vk::Vulkan& vk) {
        for (const auto stage : {Failure::Image, Failure::MemoryType, Failure::Allocate,
                Failure::Bind, Failure::View, Failure::Export, Failure::None}) {
            driver = {.failure = stage, .failImageIndex = 2};
            std::array<int, 4> fds{-1, -1, -1, -1};
            bool failed{false};
            try {
                ls::FileDescriptorScope exports{fds};
                std::vector<vk::Image> images;
                for (int& fd : fds)
                    images.emplace_back(vk, VkExtent2D{32, 32}, VK_FORMAT_R8G8B8A8_UNORM,
                        VK_IMAGE_USAGE_SAMPLED_BIT, std::nullopt, &fd);
            } catch (const std::exception&) { failed = true; }
            require(failed == (stage != Failure::None), "export failure was not exercised");
            require(driver.exports == (failed ? 2 : 4), "image exported before construction completed");
            for (int fd : fds) require(fd == -1 || !isOpen(fd), "partial export leaked a descriptor");
            require(!failed || fds[2] == -1, "failed image published an undefined descriptor");
            driver.verify();
        }
    }

    void testPartialImport(const vk::Vulkan& vk) {
        // Two source images, four destinations, and one timeline: fail every
        // image stage at every position, then each semaphore stage and success.
        for (const auto stage : {Failure::Image, Failure::MemoryType, Failure::Allocate,
                Failure::Bind, Failure::View, Failure::Semaphore, Failure::SemaphoreImport,
                Failure::None}) {
            for (int position = 0; position < 6; ++position) {
                driver = {.failure = stage, .failImageIndex = position};
                std::array<int, 7> fds{};
                for (int& fd : fds) fd = descriptor();
                bool failed{false};
                try {
                    ls::FileDescriptorScope incoming{fds};
                    std::vector<vk::Image> images;
                    images.reserve(6);
                    for (int i = 0; i < 6; ++i)
                        images.emplace_back(vk, VkExtent2D{32, 32}, VK_FORMAT_R8G8B8A8_UNORM,
                            VK_IMAGE_USAGE_SAMPLED_BIT, incoming.take());
                    for (const auto& image : images)
                        require(image.external(), "imported image lost external-memory identity");
                    const vk::TimelineSemaphore semaphore{vk, 0, incoming.take()};
                } catch (const std::exception&) { failed = true; }
                require(failed == (stage != Failure::None), "import failure was not exercised");
                for (size_t i = driver.reused.size(); i < fds.size(); ++i)
                    require(!isOpen(fds[i]), "unattempted or failed import leaked a descriptor");
                driver.verify();
            }
        }
    }

    void testScopeHandoffAndRejection() {
        const std::array fds{descriptor(), descriptor()};
        {
            ls::FileDescriptorScope caller{fds};
            // Models a consuming entry point rejecting the batch before any import.
            auto reject = [](std::span<const int> batch) { ls::FileDescriptorScope owner{batch}; };
            reject((caller.release(), std::span<const int>{fds}));
            driver.consume(descriptor());
        }
        // Reuse must survive both the callee and the released caller scope.
        driver.verify();
        ls::FileDescriptorScope empty{std::span<const int>{}};
    }

    void testBinarySemaphore(const vk::Vulkan& vk) {
        for (auto stage : {Failure::Semaphore, Failure::SemaphoreImport, Failure::None}) {
            driver = {.failure = stage};
            const int fd = descriptor();
            bool failed{false};
            try { const vk::Semaphore semaphore{vk, fd}; }
            catch (const std::exception&) { failed = true; }
            require(failed == (stage != Failure::None), "semaphore failure was not exercised");
            if (failed) require(!isOpen(fd), "binary semaphore failure leaked its descriptor");
            driver.verify();
        }
    }

    void testExternalImageCopies(const vk::Vulkan& vk) {
        for (const bool external : {false, true}) {
            driver = {};
            {
                int fd = -1;
                ls::FileDescriptorScope exported{{&fd, 1}};
                const vk::Image image(vk, {32, 32}, VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                    std::nullopt, external ? std::optional<int*>{&fd} : std::nullopt);
                require(image.external() == external, "image lost external-memory identity");
                const vk::Buffer buffer(vk, mappedBuffer);
                const vk::CommandBuffer command(vk);
                recordedBarriers.clear();
                copyPositions.clear();
                command.copyBufferToImage(vk, buffer, image);
                require(copyPositions == std::vector<size_t>{1}, "upload release must follow the copy");
                require(recordedBarriers.front().image.oldLayout ==
                    (external ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_GENERAL),
                    "external uploads must discard prior ownership; internal uploads retain layout");
                require(recordedBarriers.size() == (external ? 2 : 1), "unexpected upload handoff count");
                if (external) {
                    const auto& release = recordedBarriers.back();
                    require(release.image.srcQueueFamilyIndex == vk.queueFamilyIndex() &&
                        release.image.dstQueueFamilyIndex == VK_QUEUE_FAMILY_EXTERNAL &&
                        release.image.newLayout == VK_IMAGE_LAYOUT_GENERAL &&
                        release.image.srcAccessMask == VK_ACCESS_TRANSFER_WRITE_BIT &&
                        release.source == VK_PIPELINE_STAGE_TRANSFER_BIT,
                        "upload did not publish writes to the external GENERAL boundary");
                }
                recordedBarriers.clear();
                copyPositions.clear();
                command.copyImageToBuffer(vk, image, buffer);
                require(copyPositions == std::vector<size_t>{external ? 2U : 1U},
                    "readback must acquire external ownership before the copy");
                if (external) {
                    const auto& acquire = recordedBarriers.front();
                    require(acquire.image.srcQueueFamilyIndex == VK_QUEUE_FAMILY_EXTERNAL &&
                        acquire.image.dstQueueFamilyIndex == vk.queueFamilyIndex() &&
                        acquire.image.oldLayout == VK_IMAGE_LAYOUT_GENERAL &&
                        acquire.image.newLayout == VK_IMAGE_LAYOUT_GENERAL &&
                        acquire.image.dstAccessMask == VK_ACCESS_TRANSFER_READ_BIT &&
                        acquire.destination == VK_PIPELINE_STAGE_TRANSFER_BIT,
                        "readback did not acquire published pixels in GENERAL");
                } else {
                    for (const auto& barrier : recordedBarriers)
                        require(barrier.image.srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED &&
                            barrier.image.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED,
                            "private image unexpectedly crossed an ownership boundary");
                }
                for (const auto& barrier : recordedBarriers)
                    require(barrier.image.image == image.handle() &&
                        barrier.image.subresourceRange.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT &&
                        barrier.image.subresourceRange.levelCount == 1 &&
                        barrier.image.subresourceRange.layerCount == 1,
                        "handoff does not cover the copied image");
            }
            driver.verify();
        }
    }

    void testTransferOnlyImages(const vk::Vulkan& vk) {
        // A forbidden view call must never occur, even if the fake driver is
        // configured to fail it. Export still happens after binding completes.
        for (const auto usage : {VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                VkImageUsageFlagBits(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)}) {
            driver = {.failure = Failure::View};
            int fd = -1;
            {
                ls::FileDescriptorScope exportScope({&fd, 1});
                const vk::Image image(vk, {32, 32}, VK_FORMAT_R16G16B16A16_SFLOAT,
                    usage, std::nullopt, &fd);
                require(image.handle() != VK_NULL_HANDLE && image.imageview() == VK_NULL_HANDLE,
                    "transfer-only image must have no view");
                require(driver.views == 0 && driver.exports == 1 && isOpen(fd),
                    "transfer-only image must retain its export ownership contract");
            }
            require(!isOpen(fd), "transfer-only export leaked its descriptor");
            driver.verify();
        }
        for (const auto extra : {VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_USAGE_STORAGE_BIT,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT}) {
            driver = {};
            {
                const vk::Image image(vk, {32, 32}, VK_FORMAT_R16G16B16A16_SFLOAT,
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | extra);
                require(image.imageview() != VK_NULL_HANDLE && driver.views == 1,
                    "sampled/storage/attachment images must retain their views");
            }
            driver.verify();
        }
    }
}

int main() {
    try {
        const auto vk = makeVulkan();
        testPartialExport(vk);
        testPartialImport(vk);
        testScopeHandoffAndRejection();
        testBinarySemaphore(vk);
        testTransferOnlyImages(vk);
        testExternalImageCopies(vk);
    } catch (const std::exception& error) {
        std::cerr << "External FD ownership test failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
