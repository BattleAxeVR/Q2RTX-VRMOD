//--------------------------------------------------------------------------------------
// Copyright (c) 2026 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#include "openxr_command_buffer.h"

#if 0//SUPPORT_OPENXR

bool CommandBuffer::Init(VkDevice device, uint32_t queue_family_index)
{
	if(vk_logical_device_)
	{
		return true;
	}
	vk_logical_device_ = device;

	VkCommandPoolCreateInfo command_pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	command_pool_info.queueFamilyIndex = queue_family_index;
	vkCreateCommandPool(vk_logical_device_, &command_pool_info, nullptr, &vk_command_pool_);

	VkCommandBufferAllocateInfo command_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	command_info.commandPool = vk_command_pool_;
	command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_info.commandBufferCount = 1;
	vkAllocateCommandBuffers(vk_logical_device_, &command_info, &vk_command_buffer_);

	VkFenceCreateInfo fence_create_info{};
	fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_create_info.flags = 0;
	fence_create_info.pNext = nullptr;

	vkCreateFence(vk_logical_device_, &fence_create_info, nullptr, &vk_fence_);

	SetState(CommandBufferState::Initialized);
	return true;
}

void CommandBuffer::Destroy()
{
	SetState(CommandBufferState::Undefined);

	if(vk_logical_device_ != nullptr)
	{
		if(vk_command_buffer_ != VK_NULL_HANDLE)
		{
			vkFreeCommandBuffers(vk_logical_device_, vk_command_pool_, 1, &vk_command_buffer_);
		}

		if(vk_command_pool_ != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(vk_logical_device_, vk_command_pool_, nullptr);
		}

		if(vk_fence_ != VK_NULL_HANDLE)
		{
			vkDestroyFence(vk_logical_device_, vk_fence_, nullptr);
		}
	}
	vk_command_buffer_ = VK_NULL_HANDLE;
	vk_command_pool_ = VK_NULL_HANDLE;
	vk_fence_ = VK_NULL_HANDLE;
	vk_logical_device_ = nullptr;
}

CommandBuffer::~CommandBuffer()
{
	Destroy();
}

bool CommandBuffer::Begin()
{
	VkCommandBufferBeginInfo command_begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	vkBeginCommandBuffer(vk_command_buffer_, &command_begin_info);
	SetState(CommandBufferState::Recording);
	return true;
}

bool CommandBuffer::End()
{
	vkEndCommandBuffer(vk_command_buffer_);
	SetState(CommandBufferState::Executable);
	return true;
}

bool CommandBuffer::Exec(VkQueue queue)
{
	VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &vk_command_buffer_;
	vkQueueSubmit(queue, 1, &submit_info, vk_fence_);

	SetState(CommandBufferState::Executing);
	return true;
}

bool CommandBuffer::Exec(VkQueue queue, VkSemaphore read_semaphore, VkSemaphore write_semaphore)
{
	const VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pWaitDstStageMask = &waitStageMask;
	submit_info.pWaitSemaphores = &read_semaphore;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &write_semaphore;
	submit_info.signalSemaphoreCount = 1;

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &vk_command_buffer_;

	vkQueueSubmit(queue, 1, &submit_info, vk_fence_);

	SetState(CommandBufferState::Executing);
	return true;
}

bool CommandBuffer::Wait()
{
	if(state == CommandBufferState::Initialized)
	{
		return true;
	}

	const uint32_t timeout_ns = 1 * 1000 * 1000 * 1000;

	for(int i = 0; i < 5; ++i)
	{
		auto res = vkWaitForFences(vk_logical_device_, 1, &vk_fence_, VK_TRUE, timeout_ns);

		if(res == VK_SUCCESS)
		{
			SetState(CommandBufferState::Executable);
			return true;
		}
	}

	return false;
}

bool CommandBuffer::Reset()
{
	if(state != CommandBufferState::Initialized)
	{
		vkResetFences(vk_logical_device_, 1, &vk_fence_);
		vkResetCommandBuffer(vk_command_buffer_, 0);

		SetState(CommandBufferState::Initialized);
	}

	return true;
}

void CommandBuffer::SetState(CommandBufferState newState)
{
	state = newState;
}


#endif // SUPPORT_OPENXR

