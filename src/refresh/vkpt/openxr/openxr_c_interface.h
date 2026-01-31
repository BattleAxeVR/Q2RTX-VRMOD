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

bool GetEyePosition(const int view_id, float* eye_pos_vec3, float* tracking_to_world_matrix);
bool GetViewMatrix(const int view_id, const bool append, float* eye_pos_vec3, float yaw_deg, float* matrix_ptr);
bool GetFov(const int view_id, XrFovf* fov_ptr);

bool GetHandPosition(const int hand_id, float* hand_pos_vec3, float* tracking_to_world_matrix);
bool GetHandMatrix(const int hand_id, const bool append, float* matrix_ptr);

#endif // SUPPORT_OPENXR

#endif // OPENXR_C_INTERFACE_H