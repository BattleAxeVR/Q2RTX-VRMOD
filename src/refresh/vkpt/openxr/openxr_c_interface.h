//--------------------------------------------------------------------------------------
// Copyright (c) 2026 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OPENXR_C_INTERFACE_H
#define OPENXR_C_INTERFACE_H

#include "defines.h"

#if SUPPORT_OPENXR

#include <vulkan/vulkan.h>

VkResult CreateVulkanOpenXRInstance(const VkInstanceCreateInfo* instance_create_info, VkInstance* vk_instance);
VkResult CreateVulkanOpenXRDevice(const VkDeviceCreateInfo* device_create_info, VkPhysicalDevice* vk_physical_device, VkDevice* vk_logical_device);

void OpenXR_Update();
void OpenXR_Shutdown();
void OpenXR_Endframe(VkCommandBuffer* external_command_buffer);

bool Is_OpenXR_Session_Running();

bool GetViewMatrix(const int view_id, float* matrix_ptr);
bool GetFov(const int view_id, XrFovf* fov_ptr);

#endif // SUPPORT_OPENXR

#endif // OPENXR_C_INTERFACE_H