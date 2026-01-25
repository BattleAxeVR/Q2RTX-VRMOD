//--------------------------------------------------------------------------------------
// Copyright (c) 2026 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OPENXR_INPUT_H
#define OPENXR_INPUT_H

#include "defines.h"

#if 0//SUPPORT_OPENXR

#include <array>
#include <map>
#include <list>

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

#if ENABLE_VIVE_TRACKERS
struct ViveTrackerInfo
{
	std::string subaction;
	std::string actionName;
	std::string localizedActionName;
	std::string bindingPath;

	XrPath tracker_role_path;
	XrSpace tracker_pose_space{ XR_NULL_HANDLE };
	XrAction tracker_pose_action{ XR_NULL_HANDLE };
};
#endif

class OpenXR;

class XRInputState
{
public:
	XRInputState(OpenXR& openxr);

	virtual bool init();
	virtual bool update();
	virtual bool shutdown();

	bool init_actions();
	bool init_controllers();

	XrActionSet xr_action_set_{ XR_NULL_HANDLE };

	std::array<XrPath, NUM_SIDES> hand_subaction_path_;

#if SUPPORT_FACE_BUTTONS
	std::array<XrPath, NUM_SIDES> XA_TouchPath;
	std::array<XrPath, NUM_SIDES> XA_ClickPath;
	std::array<XrPath, NUM_SIDES> YB_TouchPath;
	std::array<XrPath, NUM_SIDES> YB_ClickPath;

	XrAction button_XA_Touch_Action{ XR_NULL_HANDLE };
	XrAction button_XA_Click_Action{ XR_NULL_HANDLE };

	XrAction button_YB_Touch_Action{ XR_NULL_HANDLE };
	XrAction button_YB_Click_Action{ XR_NULL_HANDLE };
#endif

#if SUPPORT_SELECT
	std::array<XrPath, NUM_SIDES> selectPath;
#endif

#if SUPPORT_GRIP
	std::array<XrPath, NUM_SIDES> squeezeValuePath;
	std::array<XrPath, NUM_SIDES> squeezeForcePath;
	std::array<XrPath, NUM_SIDES> squeezeClickPath;

	XrAction grabAction{ XR_NULL_HANDLE };
#endif

#if SUPPORT_GRIP_POSE
	std::array<XrPath, NUM_SIDES> gripPath;
	XrAction gripPoseAction{ XR_NULL_HANDLE };
	std::array<XrSpace, NUM_SIDES> gripSpace = { XR_NULL_HANDLE, XR_NULL_HANDLE };
#endif

#if SUPPORT_AIM_POSE
	std::array<XrPath, NUM_SIDES> aimPath;
	XrAction aimPoseAction{ XR_NULL_HANDLE };
	std::array<XrSpace, NUM_SIDES> aimSpace = { XR_NULL_HANDLE, XR_NULL_HANDLE };
#endif

#if SUPPORT_THUMBSTICKS

#if SEPARATE_X_Y_AXES_FOR_THUMBSTICKS
	std::array<XrPath, NUM_SIDES> stickXPath;
	std::array<XrPath, NUM_SIDES> stickYPath;

	XrAction thumbstickXAction{ XR_NULL_HANDLE };
	XrAction thumbstickYAction{ XR_NULL_HANDLE };
#else
	std::array<XrPath, NUM_SIDES> thumbstickPath;
	XrAction thumbstickAction{ XR_NULL_HANDLE };
#endif

	std::array<XrPath, NUM_SIDES> stickTouchPath;
	std::array<XrPath, NUM_SIDES> stickClickPath;

	XrAction thumbstickTouchAction{ XR_NULL_HANDLE };
	XrAction thumbstickClickAction{ XR_NULL_HANDLE };
#endif

#if SUPPORT_HAPTICS
	std::array<XrPath, NUM_SIDES> hapticPath;
	XrAction vibrateAction{ XR_NULL_HANDLE };
#endif

#if SUPPORT_MENU
	std::array<XrPath, NUM_SIDES> menuClickPath;
	XrAction menuAction{ XR_NULL_HANDLE };
#endif

#if SUPPORT_TRIGGERS
	std::array<XrPath, NUM_SIDES> triggerValuePath;

	XrAction triggerPullAction{ XR_NULL_HANDLE };
	XrAction triggerTouchAction{ XR_NULL_HANDLE };
	XrAction triggerClickAction{ XR_NULL_HANDLE };
#endif


#if ENABLE_EXT_EYE_TRACKING
	XrAction gazeAction{ XR_NULL_HANDLE };
	XrSpace gazeActionSpace{ XR_NULL_HANDLE };
	//XrSpace localReferenceSpace{ XR_NULL_HANDLE };
	XrBool32 gazeActive;
#endif

#if ENABLE_VIVE_TRACKERS
	std::vector<ViveTrackerInfo> tracker_infos_;
#endif

private:
	OpenXR& openxr_;
};

#endif // SUPPORT_OPENXR

#endif // OPENXR_INPUT_H

