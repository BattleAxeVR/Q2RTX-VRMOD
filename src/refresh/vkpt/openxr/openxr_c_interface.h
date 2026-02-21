//--------------------------------------------------------------------------------------
// Copyright (c) 2026 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OPENXR_C_INTERFACE_H
#define OPENXR_C_INTERFACE_H

#include "defines.h"

#if SUPPORT_OPENXR

#include "vr_controllers.h"

#include <openxr/openxr.h>
#include <vulkan/vulkan.h>

VkResult CreateVulkanOpenXRInstance(const VkInstanceCreateInfo* instance_create_info, VkInstance* vk_instance);
VkResult CreateVulkanOpenXRDevice(const VkDeviceCreateInfo* device_create_info, VkPhysicalDevice* vk_physical_device, VkDevice* vk_logical_device);

void OpenXR_Update();
void OpenXR_Shutdown();
void OpenXR_Endframe(VkCommandBuffer* external_command_buffer, VkExtent2D input_extent, int image_index, bool waterwarp);

bool Is_OpenXR_Session_Running();

float GetIPD();
bool GetFov(const int view_id, XrFovf* fov_ptr);

bool GetPitch(const int view_id, const bool in_degrees, float* pitch_ptr);
bool GetYaw(const int view_id, const bool in_degrees, float* yaw_ptr);
bool GetRoll(const int view_id, const bool in_degrees, float* roll_ptr);

bool GetViewMatrix(const int view_id, float* view_origin_ptr, float* view_angles_ptr, float* view_matrix_ptr, float* inv_view_matrix_ptr);
bool GetHandMatrix(const int hand_id, float* view_origin_ptr, float* view_angles_ptr, const float* scale_ptr, float* hand_matrix_ptr, float* gun_offsets_ptr);
bool GetVRControllerState(const int hand_id, const bool update, VRControllerState* vr_controller_state_ptr);

bool ApplyHeadOrientedLocomotion(const int view_id, float* walk_forward_ptr, float* strafe_ptr);
bool ApplyWaistOrientedLocomotion(const int view_id, float* walk_forward_ptr, float* strafe_ptr);

#endif // SUPPORT_OPENXR

#endif // OPENXR_C_INTERFACE_H