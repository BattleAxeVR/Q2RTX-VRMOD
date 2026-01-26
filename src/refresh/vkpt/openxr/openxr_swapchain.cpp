//--------------------------------------------------------------------------------------
// Copyright (c) 2026 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#include "openxr_swapchain.h"

#if SUPPORT_OPENXR

#include <algorithm>
#include "assert.h"

void XRMemoryAllocator::Init(VkPhysicalDevice physicalDevice, VkDevice device)
{
	vk_logical_device_ = device;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &m_memProps);
}

void XRMemoryAllocator::Allocate(VkMemoryRequirements const& memReqs, VkDeviceMemory* mem, VkFlags flags, const void* pNext) const
{
	for(uint32_t i = 0; i < m_memProps.memoryTypeCount; ++i)
	{
		if((memReqs.memoryTypeBits & (1 << i)) != 0u)
		{
			if((m_memProps.memoryTypes[i].propertyFlags & flags) == flags)
			{
				VkMemoryAllocateInfo memAlloc{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, pNext };
				memAlloc.allocationSize = memReqs.size;
				memAlloc.memoryTypeIndex = i;
				vkAllocateMemory(vk_logical_device_, &memAlloc, nullptr, mem);
				return;
			}
		}
	}
}

XRRenderTarget::XRRenderTarget(XRRenderTarget&& other) noexcept
{
	using std::swap;
	swap(colorImage, other.colorImage);
	swap(depthImage, other.depthImage);
	swap(colorView, other.colorView);
	swap(depthView, other.depthView);
	swap(vk_logical_device_, other.vk_logical_device_);
}

XRRenderTarget::~XRRenderTarget()
{
	if(vk_logical_device_ != nullptr)
	{
		if(colorView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(vk_logical_device_, colorView, nullptr);
		}
		if(depthView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(vk_logical_device_, depthView, nullptr);
		}
	}

	colorImage = VK_NULL_HANDLE;
	depthImage = VK_NULL_HANDLE;
	colorView = VK_NULL_HANDLE;
	depthView = VK_NULL_HANDLE;
	vk_logical_device_ = nullptr;
}


XRRenderTarget& XRRenderTarget::operator=(XRRenderTarget&& other) noexcept
{
	if(&other == this)
	{
		return *this;
	}
	this->~XRRenderTarget();
	using std::swap;
	swap(colorImage, other.colorImage);
	swap(depthImage, other.depthImage);
	swap(colorView, other.colorView);
	swap(depthView, other.depthView);
	swap(vk_logical_device_, other.vk_logical_device_);
	return *this;
}

void XRRenderTarget::Create(VkDevice device, VkImage aColorImage, VkFormat colour_format, VkImage aDepthImage, VkFormat depth_format, VkExtent2D size)
{
	vk_logical_device_ = device;

	colorImage = aColorImage;
	depthImage = aDepthImage;

	// Create color image view
	if(colorImage != VK_NULL_HANDLE)
	{
		VkImageViewCreateInfo colorViewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		colorViewInfo.image = colorImage;
		colorViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorViewInfo.format = colour_format;
		colorViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		colorViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		colorViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		colorViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		colorViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorViewInfo.subresourceRange.baseMipLevel = 0;
		colorViewInfo.subresourceRange.levelCount = 1;
		colorViewInfo.subresourceRange.baseArrayLayer = 0;
		colorViewInfo.subresourceRange.layerCount = 1;
		vkCreateImageView(vk_logical_device_, &colorViewInfo, nullptr, &colorView);
		assert(colorView);
	}

	// Create depth image view
	if(depthImage != VK_NULL_HANDLE)
	{
		VkImageViewCreateInfo depthViewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		depthViewInfo.image = depthImage;
		depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthViewInfo.format = depth_format;
		depthViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		depthViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		depthViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		depthViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		depthViewInfo.subresourceRange.baseMipLevel = 0;
		depthViewInfo.subresourceRange.levelCount = 1;
		depthViewInfo.subresourceRange.baseArrayLayer = 0;
		depthViewInfo.subresourceRange.layerCount = 1;
		vkCreateImageView(vk_logical_device_, &depthViewInfo, nullptr, &depthView);
		assert(depthView);
	}
}


XRDepthBuffer::XRDepthBuffer(XRDepthBuffer&& other) noexcept : XRDepthBuffer()
{
	using std::swap;

	swap(depthImage, other.depthImage);
	swap(depthMemory, other.depthMemory);
	swap(vk_logical_device_, other.vk_logical_device_);
}

XRDepthBuffer::~XRDepthBuffer()
{
	Destroy();
}


XRDepthBuffer& XRDepthBuffer::operator=(XRDepthBuffer&& other) noexcept
{
	if(&other == this)
	{
		return *this;
	}
	this->~XRDepthBuffer();
	using std::swap;

	swap(depthImage, other.depthImage);
	swap(depthMemory, other.depthMemory);
	swap(vk_logical_device_, other.vk_logical_device_);
	return *this;
}

void XRDepthBuffer::Create(VkDevice device, XRMemoryAllocator* memAllocator, VkFormat depthFormat, const XrSwapchainCreateInfo& swapchainCreateInfo)
{
	vk_logical_device_ = device;

	VkExtent2D size = { swapchainCreateInfo.width, swapchainCreateInfo.height };

	// Create a D32 depthbuffer
	VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = size.width;
	imageInfo.extent.height = size.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = depthFormat;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
#if USE_UNIFIED_IMAGE_LAYOUTS
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
#else
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
#endif
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.samples = (VkSampleCountFlagBits)swapchainCreateInfo.sampleCount;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vkCreateImage(device, &imageInfo, nullptr, &depthImage);

	VkMemoryRequirements memRequirements{};
	vkGetImageMemoryRequirements(device, depthImage, &memRequirements);
	memAllocator->Allocate(memRequirements, &depthMemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, nullptr);
	vkBindImageMemory(device, depthImage, depthMemory, 0);
}

void XRDepthBuffer::Destroy()
{
	if(vk_logical_device_ != nullptr)
	{
		if(depthImage != VK_NULL_HANDLE)
		{
			vkDestroyImage(vk_logical_device_, depthImage, nullptr);
			depthImage = VK_NULL_HANDLE;
		}

		if(depthMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(vk_logical_device_, depthMemory, nullptr);
			depthMemory = VK_NULL_HANDLE;
		}
	}
	depthImage = VK_NULL_HANDLE;
	depthMemory = VK_NULL_HANDLE;
	vk_logical_device_ = nullptr;
}

void XRDepthBuffer::TransitionLayout(VkCommandBuffer command_buffer, VkImageLayout newLayout)
{
	if(newLayout == m_vkLayout) 
	{
		return;
	}

	VkImageMemoryBarrier depthBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	depthBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	depthBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
	depthBarrier.oldLayout = m_vkLayout;
	depthBarrier.newLayout = newLayout;
	depthBarrier.image = depthImage;
	depthBarrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &depthBarrier);

	m_vkLayout = newLayout;
}

XRSwapchainImageContext::XRSwapchainImageContext(XrStructureType _swapchainImageType) : swapchainImageType(_swapchainImageType)
{

}

std::vector<XrSwapchainImageBaseHeader*> XRSwapchainImageContext::Create(VkDevice device, XRMemoryAllocator* memAllocator, uint32_t capacity, const XrSwapchainCreateInfo& swapchainCreateInfo)
{
	vk_logical_device_ = device;

	size = { swapchainCreateInfo.width, swapchainCreateInfo.height };
	colorFormat = (VkFormat)swapchainCreateInfo.format;
	depthFormat = VK_FORMAT_D32_SFLOAT;

	depthBuffer.Create(vk_logical_device_, memAllocator, depthFormat, swapchainCreateInfo);

	swapchainImages.resize(capacity);
	renderTarget.resize(capacity);
	std::vector<XrSwapchainImageBaseHeader*> bases(capacity);

	for(uint32_t i = 0; i < capacity; ++i)
	{
		swapchainImages[i] = { swapchainImageType };
		bases[i] = reinterpret_cast<XrSwapchainImageBaseHeader*>(&swapchainImages[i]);
	}

	return bases;
}

uint32_t XRSwapchainImageContext::ImageIndex(const XrSwapchainImageBaseHeader* swapchainImageHeader)
{
	auto p = reinterpret_cast<const XrSwapchainImageVulkan2KHR*>(swapchainImageHeader);
	return (uint32_t)(p - &swapchainImages[0]);
}

VkImageView XRSwapchainImageContext::get_colour_view(uint32_t index)
{
	if(renderTarget[index].colorView == VK_NULL_HANDLE)
	{
		renderTarget[index].Create(vk_logical_device_, swapchainImages[index].image, colorFormat, depthBuffer.depthImage, depthFormat, size);
	}

	return renderTarget[index].colorView;
}

VkImageView XRSwapchainImageContext::get_depth_view(uint32_t index)
{
	if(renderTarget[index].depthView == VK_NULL_HANDLE)
	{
		renderTarget[index].Create(vk_logical_device_, swapchainImages[index].image, colorFormat, depthBuffer.depthImage, depthFormat, size);
	}

	return renderTarget[index].depthView;
}

#endif // SUPPORT_OPENXR