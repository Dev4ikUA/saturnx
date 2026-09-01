#include "vulkan_renderer.h"

#include <Windows.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>
#include <vector>

namespace SaturnX
{
    namespace
    {
        const char* g_instanceExtensions[] =
        {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
        };

        const char* g_deviceExtensions[] =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
    }

    bool VulkanRenderer::initialize(HWND window)
    {
        window_ = window;

        if (!createInstance())
            return false;

        if (!createSurface())
            return false;

        if (!pickPhysicalDevice())
            return false;

        if (!createLogicalDevice())
            return false;

        if (!createSwapchain())
            return false;

        if (!createImageViews())
            return false;

        if (!createRenderPass())
            return false;

        if (!createFramebuffers())
            return false;

        if (!createCommandPool())
            return false;

        if (!createCommandBuffers())
            return false;

        if (!createSyncObjects())
            return false;

        std::cout << "Vulkan initialized.\n";

        return true;
    }

    bool VulkanRenderer::createInstance()
    {
        VkApplicationInfo appInfo{};
        appInfo.sType =
            VK_STRUCTURE_TYPE_APPLICATION_INFO;

        appInfo.pApplicationName =
            "SaturnX";

        appInfo.applicationVersion =
            VK_MAKE_VERSION(0, 1, 0);

        appInfo.pEngineName =
            "SaturnX";

        appInfo.engineVersion =
            VK_MAKE_VERSION(0, 1, 0);

        appInfo.apiVersion =
            VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType =
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        createInfo.pApplicationInfo =
            &appInfo;

        createInfo.enabledExtensionCount =
            static_cast<uint32_t>(
                sizeof(g_instanceExtensions) /
                sizeof(g_instanceExtensions[0]));

        createInfo.ppEnabledExtensionNames =
            g_instanceExtensions;

        VkResult result =
            vkCreateInstance(
                &createInfo,
                nullptr,
                &instance_);

        if (result != VK_SUCCESS)
        {
            std::cerr
                << "vkCreateInstance failed: "
                << result
                << '\n';

            return false;
        }

        return true;
    }

    bool VulkanRenderer::createSurface()
    {
        VkWin32SurfaceCreateInfoKHR createInfo{};

        createInfo.sType =
            VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;

        createInfo.hinstance =
            GetModuleHandleW(nullptr);

        createInfo.hwnd =
            window_;

        VkResult result =
            vkCreateWin32SurfaceKHR(
                instance_,
                &createInfo,
                nullptr,
                &surface_);

        if (result != VK_SUCCESS)
        {
            std::cerr
                << "vkCreateWin32SurfaceKHR failed: "
                << result
                << '\n';

            return false;
        }

        return true;
    }

    uint32_t VulkanRenderer::findGraphicsPresentQueue() const
    {
        uint32_t count = 0;

        vkGetPhysicalDeviceQueueFamilyProperties(
            physicalDevice_,
            &count,
            nullptr);

        if (count == 0)
            return UINT32_MAX;

        std::vector<VkQueueFamilyProperties> families(
            count);

        vkGetPhysicalDeviceQueueFamilyProperties(
            physicalDevice_,
            &count,
            families.data());

        for (uint32_t i = 0; i < count; ++i)
        {
            if ((families[i].queueFlags &
                VK_QUEUE_GRAPHICS_BIT) == 0)
            {
                continue;
            }

            VkBool32 presentSupport =
                VK_FALSE;

            if (vkGetPhysicalDeviceSurfaceSupportKHR(
                physicalDevice_,
                i,
                surface_,
                &presentSupport) != VK_SUCCESS)
            {
                continue;
            }

            if (presentSupport)
                return i;
        }

        return UINT32_MAX;
    }

    bool VulkanRenderer::pickPhysicalDevice()
    {
        uint32_t count = 0;

        if (vkEnumeratePhysicalDevices(
            instance_,
            &count,
            nullptr) != VK_SUCCESS)
        {
            return false;
        }

        if (count == 0)
        {
            std::cerr
                << "No Vulkan physical devices found.\n";

            return false;
        }

        std::vector<VkPhysicalDevice> devices(
            count);

        if (vkEnumeratePhysicalDevices(
            instance_,
            &count,
            devices.data()) != VK_SUCCESS)
        {
            return false;
        }

        for (VkPhysicalDevice candidate : devices)
        {
            physicalDevice_ =
                candidate;

            graphicsQueueFamily_ =
                findGraphicsPresentQueue();

            if (graphicsQueueFamily_ != UINT32_MAX)
                break;

            physicalDevice_ =
                VK_NULL_HANDLE;
        }

        if (physicalDevice_ == VK_NULL_HANDLE)
        {
            std::cerr
                << "No suitable Vulkan GPU found.\n";

            return false;
        }

        VkPhysicalDeviceProperties properties{};

        vkGetPhysicalDeviceProperties(
            physicalDevice_,
            &properties);

        std::cout
            << "Vulkan GPU: "
            << properties.deviceName
            << '\n';

        return true;
    }

    bool VulkanRenderer::createLogicalDevice()
    {
        float queuePriority = 1.0f;

        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType =
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

        queueInfo.queueFamilyIndex =
            graphicsQueueFamily_;

        queueInfo.queueCount = 1;

        queueInfo.pQueuePriorities =
            &queuePriority;

        VkPhysicalDeviceFeatures features{};

        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType =
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        deviceInfo.pEnabledFeatures =
            &features;

        deviceInfo.queueCreateInfoCount = 1;

        deviceInfo.pQueueCreateInfos =
            &queueInfo;

        deviceInfo.enabledExtensionCount =
            static_cast<uint32_t>(
                sizeof(g_deviceExtensions) /
                sizeof(g_deviceExtensions[0]));

        deviceInfo.ppEnabledExtensionNames =
            g_deviceExtensions;

        VkResult result =
            vkCreateDevice(
                physicalDevice_,
                &deviceInfo,
                nullptr,
                &device_);

        if (result != VK_SUCCESS)
        {
            std::cerr
                << "vkCreateDevice failed: "
                << result
                << '\n';

            return false;
        }

        vkGetDeviceQueue(
            device_,
            graphicsQueueFamily_,
            0,
            &graphicsQueue_);

        return true;
    }

    bool VulkanRenderer::createSwapchain()
    {
        VkSurfaceCapabilitiesKHR capabilities{};

        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            physicalDevice_,
            surface_,
            &capabilities) != VK_SUCCESS)
        {
            return false;
        }

        uint32_t formatCount = 0;

        if (vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice_,
            surface_,
            &formatCount,
            nullptr) != VK_SUCCESS)
        {
            return false;
        }

        if (formatCount == 0)
            return false;

        std::vector<VkSurfaceFormatKHR> formats(
            formatCount);

        if (vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice_,
            surface_,
            &formatCount,
            formats.data()) != VK_SUCCESS)
        {
            return false;
        }

        VkSurfaceFormatKHR surfaceFormat =
            formats[0];

        for (const VkSurfaceFormatKHR& format : formats)
        {
            if (format.format ==
                VK_FORMAT_B8G8R8A8_UNORM &&
                format.colorSpace ==
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                surfaceFormat = format;
                break;
            }
        }

        uint32_t presentModeCount = 0;

        if (vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice_,
            surface_,
            &presentModeCount,
            nullptr) != VK_SUCCESS)
        {
            return false;
        }

        std::vector<VkPresentModeKHR> presentModes;

        if (presentModeCount > 0)
        {
            presentModes.resize(
                presentModeCount);

            if (vkGetPhysicalDeviceSurfacePresentModesKHR(
                physicalDevice_,
                surface_,
                &presentModeCount,
                presentModes.data()) != VK_SUCCESS)
            {
                return false;
            }
        }

        VkPresentModeKHR presentMode =
            VK_PRESENT_MODE_FIFO_KHR;

        for (VkPresentModeKHR mode : presentModes)
        {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                presentMode = mode;
                break;
            }
        }

        VkExtent2D extent{};

        if (capabilities.currentExtent.width !=
            std::numeric_limits<uint32_t>::max())
        {
            extent =
                capabilities.currentExtent;
        }
        else
        {
            RECT rect{};

            GetClientRect(
                window_,
                &rect);

            uint32_t width =
                static_cast<uint32_t>(
                    rect.right - rect.left);

            uint32_t height =
                static_cast<uint32_t>(
                    rect.bottom - rect.top);

            extent.width =
                std::clamp(
                    width,
                    capabilities.minImageExtent.width,
                    capabilities.maxImageExtent.width);

            extent.height =
                std::clamp(
                    height,
                    capabilities.minImageExtent.height,
                    capabilities.maxImageExtent.height);
        }

        uint32_t imageCount =
            capabilities.minImageCount + 1;

        if (capabilities.maxImageCount != 0 &&
            imageCount > capabilities.maxImageCount)
        {
            imageCount =
                capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};

        createInfo.sType =
            VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

        createInfo.surface =
            surface_;

        createInfo.minImageCount =
            imageCount;

        createInfo.imageFormat =
            surfaceFormat.format;

        createInfo.imageColorSpace =
            surfaceFormat.colorSpace;

        createInfo.imageExtent =
            extent;

        createInfo.imageArrayLayers = 1;

        /*
         * We copy the Saturn framebuffer into the
         * swapchain image using a transfer operation.
         */
        createInfo.imageUsage =
            VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        createInfo.imageSharingMode =
            VK_SHARING_MODE_EXCLUSIVE;

        createInfo.preTransform =
            capabilities.currentTransform;

        createInfo.compositeAlpha =
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        createInfo.presentMode =
            presentMode;

        createInfo.clipped =
            VK_TRUE;

        VkResult result =
            vkCreateSwapchainKHR(
                device_,
                &createInfo,
                nullptr,
                &swapchain_);

        if (result != VK_SUCCESS)
        {
            std::cerr
                << "vkCreateSwapchainKHR failed: "
                << result
                << '\n';

            return false;
        }

        swapchainFormat_ =
            surfaceFormat.format;

        swapchainExtent_ =
            extent;

        uint32_t swapchainImageCount = 0;

        vkGetSwapchainImagesKHR(
            device_,
            swapchain_,
            &swapchainImageCount,
            nullptr);

        swapchainImages_.resize(
            swapchainImageCount);

        vkGetSwapchainImagesKHR(
            device_,
            swapchain_,
            &swapchainImageCount,
            swapchainImages_.data());

        return true;
    }

    bool VulkanRenderer::createImageViews()
    {
        /*
         * The current renderer copies directly into
         * swapchain images, so image views are not needed.
         */
        return true;
    }

    bool VulkanRenderer::createRenderPass()
    {
        /*
         * No render pass is required for the current
         * transfer/blit presentation path.
         */
        return true;
    }

    bool VulkanRenderer::createFramebuffers()
    {
        /*
         * No framebuffers are required for the current
         * transfer/blit presentation path.
         */
        return true;
    }

    bool VulkanRenderer::createCommandPool()
    {
        VkCommandPoolCreateInfo createInfo{};

        createInfo.sType =
            VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

        createInfo.queueFamilyIndex =
            graphicsQueueFamily_;

        createInfo.flags =
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkResult result =
            vkCreateCommandPool(
                device_,
                &createInfo,
                nullptr,
                &commandPool_);

        if (result != VK_SUCCESS)
        {
            std::cerr
                << "vkCreateCommandPool failed: "
                << result
                << '\n';

            return false;
        }

        return true;
    }

    bool VulkanRenderer::createCommandBuffers()
    {
        commandBuffers_.resize(1);

        VkCommandBufferAllocateInfo allocateInfo{};

        allocateInfo.sType =
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

        allocateInfo.commandPool =
            commandPool_;

        allocateInfo.level =
            VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        allocateInfo.commandBufferCount = 1;

        VkResult result =
            vkAllocateCommandBuffers(
                device_,
                &allocateInfo,
                commandBuffers_.data());

        if (result != VK_SUCCESS)
        {
            std::cerr
                << "vkAllocateCommandBuffers failed: "
                << result
                << '\n';

            return false;
        }

        return true;
    }

    bool VulkanRenderer::createSyncObjects()
    {
        VkSemaphoreCreateInfo semaphoreInfo{};

        semaphoreInfo.sType =
            VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};

        fenceInfo.sType =
            VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        fenceInfo.flags =
            VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(
            device_,
            &semaphoreInfo,
            nullptr,
            &imageAvailableSemaphore_) != VK_SUCCESS)
        {
            return false;
        }

        if (vkCreateSemaphore(
            device_,
            &semaphoreInfo,
            nullptr,
            &renderFinishedSemaphore_) != VK_SUCCESS)
        {
            return false;
        }

        if (vkCreateFence(
            device_,
            &fenceInfo,
            nullptr,
            &inFlightFence_) != VK_SUCCESS)
        {
            return false;
        }

        return true;
    }

    uint32_t VulkanRenderer::findMemoryType(
        uint32_t typeFilter,
        VkMemoryPropertyFlags properties) const
    {
        VkPhysicalDeviceMemoryProperties memoryProperties{};

        vkGetPhysicalDeviceMemoryProperties(
            physicalDevice_,
            &memoryProperties);

        for (uint32_t i = 0;
            i < memoryProperties.memoryTypeCount;
            ++i)
        {
            if ((typeFilter & (1u << i)) != 0 &&
                (memoryProperties.memoryTypes[i].propertyFlags &
                    properties) == properties)
            {
                return i;
            }
        }

        return UINT32_MAX;
    }

    void VulkanRenderer::destroyStagingBuffer()
    {
        if (stagingBuffer_ != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(
                device_,
                stagingBuffer_,
                nullptr);

            stagingBuffer_ =
                VK_NULL_HANDLE;
        }

        if (stagingMemory_ != VK_NULL_HANDLE)
        {
            vkFreeMemory(
                device_,
                stagingMemory_,
                nullptr);

            stagingMemory_ =
                VK_NULL_HANDLE;
        }
    }

    void VulkanRenderer::destroyFrameImage()
    {
        if (frameView_ != VK_NULL_HANDLE)
        {
            vkDestroyImageView(
                device_,
                frameView_,
                nullptr);

            frameView_ =
                VK_NULL_HANDLE;
        }

        if (frameImage_ != VK_NULL_HANDLE)
        {
            vkDestroyImage(
                device_,
                frameImage_,
                nullptr);

            frameImage_ =
                VK_NULL_HANDLE;
        }

        if (frameMemory_ != VK_NULL_HANDLE)
        {
            vkFreeMemory(
                device_,
                frameMemory_,
                nullptr);

            frameMemory_ =
                VK_NULL_HANDLE;
        }

        frameWidth_ = 0;
        frameHeight_ = 0;
    }

    bool VulkanRenderer::createFrameImage(
        unsigned width,
        unsigned height)
    {
        destroyFrameImage();

        VkImageCreateInfo imageInfo{};

        imageInfo.sType =
            VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

        imageInfo.imageType =
            VK_IMAGE_TYPE_2D;

        imageInfo.extent =
        {
            width,
            height,
            1
        };

        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;

        imageInfo.format =
            VK_FORMAT_B8G8R8A8_UNORM;

        imageInfo.tiling =
            VK_IMAGE_TILING_OPTIMAL;

        imageInfo.initialLayout =
            VK_IMAGE_LAYOUT_UNDEFINED;

        imageInfo.usage =
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        imageInfo.samples =
            VK_SAMPLE_COUNT_1_BIT;

        imageInfo.sharingMode =
            VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(
            device_,
            &imageInfo,
            nullptr,
            &frameImage_) != VK_SUCCESS)
        {
            return false;
        }

        VkMemoryRequirements requirements{};

        vkGetImageMemoryRequirements(
            device_,
            frameImage_,
            &requirements);

        uint32_t memoryType =
            findMemoryType(
                requirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (memoryType == UINT32_MAX)
            return false;

        VkMemoryAllocateInfo allocateInfo{};

        allocateInfo.sType =
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

        allocateInfo.allocationSize =
            requirements.size;

        allocateInfo.memoryTypeIndex =
            memoryType;

        if (vkAllocateMemory(
            device_,
            &allocateInfo,
            nullptr,
            &frameMemory_) != VK_SUCCESS)
        {
            return false;
        }

        if (vkBindImageMemory(
            device_,
            frameImage_,
            frameMemory_,
            0) != VK_SUCCESS)
        {
            return false;
        }

        frameWidth_ =
            width;

        frameHeight_ =
            height;

        return true;
    }

    bool VulkanRenderer::createStagingBuffer(
        VkDeviceSize size)
    {
        destroyStagingBuffer();

        VkBufferCreateInfo bufferInfo{};

        bufferInfo.sType =
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

        bufferInfo.size =
            size;

        bufferInfo.usage =
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        bufferInfo.sharingMode =
            VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(
            device_,
            &bufferInfo,
            nullptr,
            &stagingBuffer_) != VK_SUCCESS)
        {
            return false;
        }

        VkMemoryRequirements requirements{};

        vkGetBufferMemoryRequirements(
            device_,
            stagingBuffer_,
            &requirements);

        uint32_t memoryType =
            findMemoryType(
                requirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (memoryType == UINT32_MAX)
            return false;

        VkMemoryAllocateInfo allocateInfo{};

        allocateInfo.sType =
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

        allocateInfo.allocationSize =
            requirements.size;

        allocateInfo.memoryTypeIndex =
            memoryType;

        if (vkAllocateMemory(
            device_,
            &allocateInfo,
            nullptr,
            &stagingMemory_) != VK_SUCCESS)
        {
            return false;
        }

        if (vkBindBufferMemory(
            device_,
            stagingBuffer_,
            stagingMemory_,
            0) != VK_SUCCESS)
        {
            return false;
        }

        return true;
    }

    bool VulkanRenderer::uploadFrame(
        const void* data,
        unsigned width,
        unsigned height,
        size_t pitch)
    {
        if (!data ||
            width == 0 ||
            height == 0)
        {
            return false;
        }

        const VkDeviceSize requiredSize =
            static_cast<VkDeviceSize>(width) *
            static_cast<VkDeviceSize>(height) *
            4;

        if (frameImage_ == VK_NULL_HANDLE ||
            frameWidth_ != width ||
            frameHeight_ != height)
        {
            if (vkDeviceWaitIdle(device_) != VK_SUCCESS)
                return false;

            if (!createFrameImage(
                width,
                height))
            {
                return false;
            }

            if (!createStagingBuffer(
                requiredSize))
            {
                return false;
            }
        }

        void* mapped = nullptr;

        if (vkMapMemory(
            device_,
            stagingMemory_,
            0,
            requiredSize,
            0,
            &mapped) != VK_SUCCESS)
        {
            return false;
        }

        const uint8_t* source =
            static_cast<const uint8_t*>(data);

        uint8_t* destination =
            static_cast<uint8_t*>(mapped);

        const size_t destinationPitch =
            static_cast<size_t>(width) * 4;

        for (unsigned y = 0;
            y < height;
            ++y)
        {
            std::memcpy(
                destination +
                static_cast<size_t>(y) *
                destinationPitch,

                source +
                static_cast<size_t>(y) *
                pitch,

                destinationPitch);
        }

        vkUnmapMemory(
            device_,
            stagingMemory_);

        VkCommandBuffer commandBuffer =
            commandBuffers_[0];

        vkResetCommandBuffer(
            commandBuffer,
            0);

        VkCommandBufferBeginInfo beginInfo{};

        beginInfo.sType =
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        beginInfo.flags =
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(
            commandBuffer,
            &beginInfo) != VK_SUCCESS)
        {
            return false;
        }

        VkImageMemoryBarrier barrier{};

        barrier.sType =
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

        barrier.srcAccessMask = 0;

        barrier.dstAccessMask =
            VK_ACCESS_TRANSFER_WRITE_BIT;

        barrier.oldLayout =
            VK_IMAGE_LAYOUT_UNDEFINED;

        barrier.newLayout =
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        barrier.image =
            frameImage_;

        barrier.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_COLOR_BIT;

        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);

        VkBufferImageCopy region{};

        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask =
            VK_IMAGE_ASPECT_COLOR_BIT;

        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset =
        {
            0,
            0,
            0
        };

        region.imageExtent =
        {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(
            commandBuffer,
            stagingBuffer_,
            frameImage_,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);

        barrier.srcAccessMask =
            VK_ACCESS_TRANSFER_WRITE_BIT;

        barrier.dstAccessMask =
            VK_ACCESS_TRANSFER_READ_BIT;

        barrier.oldLayout =
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        barrier.newLayout =
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);

        if (vkEndCommandBuffer(
            commandBuffer) != VK_SUCCESS)
        {
            return false;
        }

        VkSubmitInfo submitInfo{};

        submitInfo.sType =
            VK_STRUCTURE_TYPE_SUBMIT_INFO;

        submitInfo.commandBufferCount = 1;

        submitInfo.pCommandBuffers =
            &commandBuffer;

        if (vkQueueSubmit(
            graphicsQueue_,
            1,
            &submitInfo,
            VK_NULL_HANDLE) != VK_SUCCESS)
        {
            return false;
        }

        if (vkQueueWaitIdle(
            graphicsQueue_) != VK_SUCCESS)
        {
            return false;
        }

        frameReady_ = true;

        return true;
    }

    void VulkanRenderer::drawFrame()
    {
        if (!frameReady_)
            return;

        if (vkWaitForFences(
            device_,
            1,
            &inFlightFence_,
            VK_TRUE,
            UINT64_MAX) != VK_SUCCESS)
        {
            return;
        }

        if (vkResetFences(
            device_,
            1,
            &inFlightFence_) != VK_SUCCESS)
        {
            return;
        }

        uint32_t imageIndex = 0;

        VkResult acquireResult =
            vkAcquireNextImageKHR(
                device_,
                swapchain_,
                UINT64_MAX,
                imageAvailableSemaphore_,
                VK_NULL_HANDLE,
                &imageIndex);

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
            return;

        if (acquireResult != VK_SUCCESS &&
            acquireResult != VK_SUBOPTIMAL_KHR)
        {
            return;
        }

        VkCommandBuffer commandBuffer =
            commandBuffers_[0];

        vkResetCommandBuffer(
            commandBuffer,
            0);

        VkCommandBufferBeginInfo beginInfo{};

        beginInfo.sType =
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        beginInfo.flags =
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(
            commandBuffer,
            &beginInfo) != VK_SUCCESS)
        {
            return;
        }

        VkImageMemoryBarrier swapBarrier{};

        swapBarrier.sType =
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

        swapBarrier.srcAccessMask = 0;

        swapBarrier.dstAccessMask =
            VK_ACCESS_TRANSFER_WRITE_BIT;

        swapBarrier.oldLayout =
            VK_IMAGE_LAYOUT_UNDEFINED;

        swapBarrier.newLayout =
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        swapBarrier.image =
            swapchainImages_[imageIndex];

        swapBarrier.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_COLOR_BIT;

        swapBarrier.subresourceRange.baseMipLevel = 0;
        swapBarrier.subresourceRange.levelCount = 1;
        swapBarrier.subresourceRange.baseArrayLayer = 0;
        swapBarrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &swapBarrier);

        VkImageBlit blit{};

        blit.srcSubresource.aspectMask =
            VK_IMAGE_ASPECT_COLOR_BIT;

        blit.srcSubresource.mipLevel = 0;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;

        blit.srcOffsets[0] =
        {
            0,
            0,
            0
        };

        blit.srcOffsets[1] =
        {
            static_cast<int32_t>(frameWidth_),
            static_cast<int32_t>(frameHeight_),
            1
        };

        blit.dstSubresource.aspectMask =
            VK_IMAGE_ASPECT_COLOR_BIT;

        blit.dstSubresource.mipLevel = 0;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        blit.dstOffsets[0] =
        {
            0,
            0,
            0
        };

        blit.dstOffsets[1] =
        {
            static_cast<int32_t>(
                swapchainExtent_.width),

            static_cast<int32_t>(
                swapchainExtent_.height),

            1
        };

        vkCmdBlitImage(
            commandBuffer,
            frameImage_,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            swapchainImages_[imageIndex],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &blit,
            VK_FILTER_NEAREST);

        swapBarrier.srcAccessMask =
            VK_ACCESS_TRANSFER_WRITE_BIT;

        swapBarrier.dstAccessMask = 0;

        swapBarrier.oldLayout =
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        swapBarrier.newLayout =
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &swapBarrier);

        if (vkEndCommandBuffer(
            commandBuffer) != VK_SUCCESS)
        {
            return;
        }

        VkPipelineStageFlags waitStage =
            VK_PIPELINE_STAGE_TRANSFER_BIT;

        VkSubmitInfo submitInfo{};

        submitInfo.sType =
            VK_STRUCTURE_TYPE_SUBMIT_INFO;

        submitInfo.waitSemaphoreCount = 1;

        submitInfo.pWaitSemaphores =
            &imageAvailableSemaphore_;

        submitInfo.pWaitDstStageMask =
            &waitStage;

        submitInfo.commandBufferCount = 1;

        submitInfo.pCommandBuffers =
            &commandBuffer;

        submitInfo.signalSemaphoreCount = 1;

        submitInfo.pSignalSemaphores =
            &renderFinishedSemaphore_;

        if (vkQueueSubmit(
            graphicsQueue_,
            1,
            &submitInfo,
            inFlightFence_) != VK_SUCCESS)
        {
            return;
        }

        VkPresentInfoKHR presentInfo{};

        presentInfo.sType =
            VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;

        presentInfo.pWaitSemaphores =
            &renderFinishedSemaphore_;

        presentInfo.swapchainCount = 1;

        presentInfo.pSwapchains =
            &swapchain_;

        presentInfo.pImageIndices =
            &imageIndex;

        vkQueuePresentKHR(
            graphicsQueue_,
            &presentInfo);
    }

    void VulkanRenderer::shutdown()
    {
        if (device_ != VK_NULL_HANDLE)
            vkDeviceWaitIdle(device_);

        destroyStagingBuffer();
        destroyFrameImage();

        if (inFlightFence_ != VK_NULL_HANDLE)
        {
            vkDestroyFence(
                device_,
                inFlightFence_,
                nullptr);

            inFlightFence_ =
                VK_NULL_HANDLE;
        }

        if (renderFinishedSemaphore_ != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(
                device_,
                renderFinishedSemaphore_,
                nullptr);

            renderFinishedSemaphore_ =
                VK_NULL_HANDLE;
        }

        if (imageAvailableSemaphore_ != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(
                device_,
                imageAvailableSemaphore_,
                nullptr);

            imageAvailableSemaphore_ =
                VK_NULL_HANDLE;
        }

        if (commandPool_ != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(
                device_,
                commandPool_,
                nullptr);

            commandPool_ =
                VK_NULL_HANDLE;
        }

        if (swapchain_ != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(
                device_,
                swapchain_,
                nullptr);

            swapchain_ =
                VK_NULL_HANDLE;
        }

        if (device_ != VK_NULL_HANDLE)
        {
            vkDestroyDevice(
                device_,
                nullptr);

            device_ =
                VK_NULL_HANDLE;
        }

        if (surface_ != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(
                instance_,
                surface_,
                nullptr);

            surface_ =
                VK_NULL_HANDLE;
        }

        if (instance_ != VK_NULL_HANDLE)
        {
            vkDestroyInstance(
                instance_,
                nullptr);

            instance_ =
                VK_NULL_HANDLE;
        }

        physicalDevice_ =
            VK_NULL_HANDLE;

        graphicsQueue_ =
            VK_NULL_HANDLE;

        graphicsQueueFamily_ =
            UINT32_MAX;

        swapchainImages_.clear();
        swapchainImageViews_.clear();
        framebuffers_.clear();
        commandBuffers_.clear();

        frameReady_ =
            false;
    }
}