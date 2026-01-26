//--------------------------------------------------------------------------------------
// Copyright (c) 2026 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OPENXR_C_INTERFACE_H
#define OPENXR_C_INTERFACE_H

#include "defines.h"

#if SUPPORT_OPENXR

struct OpenXR_CInterface
{
	//VkInstance vk_instance_ = nullptr;
	//VkPhysicalDevice vk_physical_device_ = nullptr;
	//VkDevice vk_logical_device_ = nullptr;
	int i;
#if 0
	bool init();
	bool shutdown();
	bool update();

	bool is_initialized() const
	{
		return initialized_;
	}

	void set_initialized(const bool initialized)
	{
		initialized_ = initialized;
	}

	bool wait_frame();
	bool begin_frame();
	bool end_frame();

	bool start_session();
	bool stop_session();

	bool is_session_running() const;

	bool initialized_ = false;
	void* openxr_interface_ = nullptr;
#endif
};

#endif // SUPPORT_OPENXR

#endif // OPENXR_C_INTERFACE_H