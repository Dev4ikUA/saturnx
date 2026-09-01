#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR

#include <Windows.h>
#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace SaturnX
{
    class VulkanRenderer
    {
    public:
        bool initialize(HWND window);
        bool uploadFrame(
            const void* data,
            unsigned width,
            unsigned height,
            size_t pitch);

        void drawFrame();
        void shutdown();

    private:
        bool createInstance();
        bool createSurface();
        bool pickPhysicalDevice();
        bool createLogicalDevice();
        bool createSwapchain();
        bool createImageViews();
        bool createRenderPass();
        bool createFramebuffers();
        bool createCommandPool();
        bool createCommandBuffers();
        bool createSyncObjects();

        uint32_t findGraphicsPresentQueue() const;
        uint32_t findMemoryType(
            uint32_t typeFilter,
            VkMemoryPropertyFlags properties) const;

        bool createFrameImage(
            unsigned width,
            unsigned height);

        void destroyFrameImage();

        bool createStagingBuffer(
            VkDeviceSize size);

        void destroyStagingBuffer();

    private:
        HWND window_ = nullptr;

        VkInstance instance_ = VK_NULL_HANDLE;
        VkSurfaceKHR surface_ = VK_NULL_HANDLE;

        VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
        VkDevice device_ = VK_NULL_HANDLE;

        VkQueue graphicsQueue_ = VK_NULL_HANDLE;
        uint32_t graphicsQueueFamily_ = UINT32_MAX;

        VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
        VkFormat swapchainFormat_ = VK_FORMAT_UNDEFINED;
        VkExtent2D swapchainExtent_{};

        std::vector<VkImage> swapchainImages_;
        std::vector<VkImageView> swapchainImageViews_;
        std::vector<VkFramebuffer> framebuffers_;

        VkRenderPass renderPass_ = VK_NULL_HANDLE;

        VkCommandPool commandPool_ = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> commandBuffers_;

        VkSemaphore imageAvailableSemaphore_ =
            VK_NULL_HANDLE;

        VkSemaphore renderFinishedSemaphore_ =
            VK_NULL_HANDLE;

        VkFence inFlightFence_ =
            VK_NULL_HANDLE;

        // Saturn framebuffer
        VkImage frameImage_ =
            VK_NULL_HANDLE;

        VkDeviceMemory frameMemory_ =
            VK_NULL_HANDLE;

        VkImageView frameView_ =
            VK_NULL_HANDLE;

        VkBuffer stagingBuffer_ =
            VK_NULL_HANDLE;

        VkDeviceMemory stagingMemory_ =
            VK_NULL_HANDLE;

        unsigned frameWidth_ = 0;
        unsigned frameHeight_ = 0;

        bool frameReady_ = false;
    };
}