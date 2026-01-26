//--------------------------------------------------------------------------------------
// Copyright (c) 2026 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OPENXR_SWAPCHAIN_H
#define OPENXR_SWAPCHAIN_H

#include "defines.h"

#if SUPPORT_OPENXR

#include <array>
#include <map>
#include <list>
#include <vector>

#define XR_USE_GRAPHICS_API_VULKAN 1
#define XR_USE_PLATFORM_WIN32 1
#define OPENXR_HAVE_COMMON_CONFIG 1
#define XR_USE_TIMESPEC 1

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif  // !NOMINMAX

#include <windows.h>
#include <wrl/client.h>
#endif

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_reflection.h>

struct XRMemoryAllocator 
{
	static const VkFlags defaultFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	void Init(VkPhysicalDevice physicalDevice, VkDevice device);
	void Allocate(VkMemoryRequirements const& memReqs, VkDeviceMemory* mem, VkFlags flags, const void* pNext) const;

private:
	VkDevice vk_logical_device_{ VK_NULL_HANDLE };
	VkPhysicalDeviceMemoryProperties m_memProps{};
};

struct XRRenderTarget
{
	VkImage colorImage{ VK_NULL_HANDLE };
	VkImage depthImage{ VK_NULL_HANDLE };

	VkImageView colorView{ VK_NULL_HANDLE };
	VkImageView depthView{ VK_NULL_HANDLE };

	XRRenderTarget() = default;

	~XRRenderTarget();
	XRRenderTarget(XRRenderTarget&& other) noexcept;

	XRRenderTarget& operator=(XRRenderTarget&& other) noexcept;
	void Create(VkDevice device, VkImage aColorImage, VkFormat colour_format, VkImage aDepthImage, VkFormat depth_format, VkExtent2D size);
	XRRenderTarget(const XRRenderTarget&) = delete;
	XRRenderTarget& operator=(const XRRenderTarget&) = delete;

private:
	VkDevice vk_logical_device_{ VK_NULL_HANDLE };
};

struct XRDepthBuffer
{
	VkDeviceMemory depthMemory{ VK_NULL_HANDLE };
	VkImage depthImage{ VK_NULL_HANDLE };

	XRDepthBuffer() = default;

	~XRDepthBuffer();
	XRDepthBuffer(XRDepthBuffer&& other) noexcept;
	XRDepthBuffer& operator=(XRDepthBuffer&& other) noexcept;
	
	void Create(VkDevice device, XRMemoryAllocator* memAllocator, VkFormat depthFormat, const XrSwapchainCreateInfo& swapchainCreateInfo);
	void Destroy();
	void TransitionLayout(VkCommandBuffer command_buffer, VkImageLayout newLayout);

	XRDepthBuffer(const XRDepthBuffer&) = delete;
	XRDepthBuffer& operator=(const XRDepthBuffer&) = delete;

private:
	VkDevice vk_logical_device_{ VK_NULL_HANDLE };
	VkImageLayout m_vkLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};


struct XRSwapchainImageContext
{
	XRSwapchainImageContext(XrStructureType _swapchainImageType);

	std::vector<XrSwapchainImageVulkan2KHR> swapchainImages;
	std::vector<XRRenderTarget> renderTarget;

	VkExtent2D size{};
	VkFormat colorFormat = VK_FORMAT_UNDEFINED;
	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

	XRDepthBuffer depthBuffer{};

	XrStructureType swapchainImageType;

	XRSwapchainImageContext() = default;

	std::vector<XrSwapchainImageBaseHeader*> Create(VkDevice device, XRMemoryAllocator* memAllocator, uint32_t capacity, const XrSwapchainCreateInfo& swapchainCreateInfo);

	uint32_t ImageIndex(const XrSwapchainImageBaseHeader* swapchainImageHeader);
	void BindRenderTarget(uint32_t index, VkRenderPassBeginInfo* renderPassBeginInfo);

	VkImageView get_colour_view(uint32_t index);
	VkImageView get_depth_view(uint32_t index);

private:
	VkDevice vk_logical_device_{ VK_NULL_HANDLE };
};

struct XRSwapchain
{
	XrSwapchain handle = nullptr;
	int32_t width = 0;
	int32_t height = 0;
};

#endif // SUPPORT_OPENXR

#endif // OPENXR_SWAPCHAIN_H