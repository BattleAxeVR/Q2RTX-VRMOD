//--------------------------------------------------------------------------------------
// Copyright (c) 2026 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OPENXR_C_INTERFACE_H
#define OPENXR_C_INTERFACE_H

#include "defines.h"

#if SUPPORT_OPENXR

#include <vulkan/vulkan.h>

VkResult CreateVulkanOpenXRInstance(const VkInstanceCreateInfo* instance_create_info, VkInstance* vk_instance);
void OpenXR_Shutdown();

#endif // SUPPORT_OPENXR

#endif // OPENXR_C_INTERFACE_H