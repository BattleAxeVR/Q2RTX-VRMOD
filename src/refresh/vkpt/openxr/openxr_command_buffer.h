//--------------------------------------------------------------------------------------
// Copyright (c) 2026 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OPENXR_COMMAND_BUFFER_H
#define OPENXR_COMMAND_BUFFER_H

#include "defines.h"

#if 0

enum class CommandBufferState
{
	Undefined,
	Initialized,
	Recording,
	Executable,
	Executing
};

struct CommandBuffer
{
	CommandBufferState state{ CommandBufferState::Undefined };
	VkCommandPool vk_command_pool_{ VK_NULL_HANDLE };
	VkCommandBuffer vk_command_buffer_{ VK_NULL_HANDLE };
	VkFence vk_fence_{ VK_NULL_HANDLE };

	CommandBuffer() = default;
	~CommandBuffer();

	bool Init(VkDevice device, uint32_t queue_family_index);
	void Destroy();

	bool Begin();
	bool End();
	bool Exec(VkQueue queue);
	bool Exec(VkQueue queue, VkSemaphore read_semaphore, VkSemaphore write_semaphore);
	bool Wait();
	bool Reset();

private:
	VkDevice vk_logical_device_{ VK_NULL_HANDLE };

	void SetState(CommandBufferState newState);
};

#endif


#endif // OPENXR_COMMAND_BUFFER_H

