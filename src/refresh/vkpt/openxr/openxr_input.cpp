//--------------------------------------------------------------------------------------
// Copyright (c) 2026 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#include "openxr_interface.h"
#include "openxr_input.h"

#if SUPPORT_OPENXR

#include "assert.h"

namespace BVR
{

XRInputState::XRInputState(OpenXR& openxr) : openxr_(openxr)
{
}

bool XRInputState::init()
{
	if (!openxr_.is_session_running())
	{
		return false;
	}

	if (is_initialized())
	{
		return false;
	}

	assert(xr_action_set_);
	const bool initialized_ok = init_controllers();

	set_initialized(initialized_ok);

	return initialized_ok;
}

bool XRInputState::init_actions()
{
	assert(openxr_.xr_instance_);
	assert(!xr_action_set_);

	XrActionSetCreateInfo action_set_info{ XR_TYPE_ACTION_SET_CREATE_INFO };
	strcpy_s(action_set_info.actionSetName, "gameplay");
	strcpy_s(action_set_info.localizedActionSetName, "Gameplay");
	action_set_info.priority = 0;
	xrCreateActionSet(openxr_.xr_instance_, &action_set_info, &xr_action_set_);

	xrStringToPath(openxr_.xr_instance_, "/user/hand/left", &hand_subaction_path_[LEFT]);
	xrStringToPath(openxr_.xr_instance_, "/user/hand/right", &hand_subaction_path_[RIGHT]);

	XrActionCreateInfo actionInfo{ XR_TYPE_ACTION_CREATE_INFO };

#if SUPPORT_SELECT
	xrStringToPath(openxr_.xr_instance_, "/user/hand/left/input/select/click", &selectPath[LEFT]);
	xrStringToPath(openxr_.xr_instance_, "/user/hand/right/input/select/click", &selectPath[RIGHT]);
#endif

#if SUPPORT_HAPTICS
	xrStringToPath(openxr_.xr_instance_, "/user/hand/left/output/haptic", &hapticPath[LEFT]);
	xrStringToPath(openxr_.xr_instance_, "/user/hand/right/output/haptic", &hapticPath[RIGHT]);
#endif

#if SUPPORT_MENU
	xrStringToPath(openxr_.xr_instance_, "/user/hand/left/input/menu/click", &menuClickPath[LEFT]);
	xrStringToPath(openxr_.xr_instance_, "/user/hand/right/input/menu/click", &menuClickPath[RIGHT]);
#endif

#if SUPPORT_TRIGGERS
	xrStringToPath(openxr_.xr_instance_, "/user/hand/left/input/trigger/value", &triggerValuePath[LEFT]);
	xrStringToPath(openxr_.xr_instance_, "/user/hand/right/input/trigger/value", &triggerValuePath[RIGHT]);
#endif

#if SUPPORT_FACE_BUTTONS
	xrStringToPath(openxr_.xr_instance_, "/user/hand/left/input/x/touch", &XA_TouchPath[LEFT]);
	xrStringToPath(openxr_.xr_instance_, "/user/hand/right/input/a/touch", &XA_TouchPath[RIGHT]);

	xrStringToPath(openxr_.xr_instance_, "/user/hand/left/input/x/click", &XA_ClickPath[LEFT]);
	xrStringToPath(openxr_.xr_instance_, "/user/hand/right/input/a/click", &XA_ClickPath[RIGHT]);

	xrStringToPath(openxr_.xr_instance_, "/user/hand/left/input/y/touch", &YB_TouchPath[LEFT]);
	xrStringToPath(openxr_.xr_instance_, "/user/hand/right/input/b/touch", &YB_TouchPath[RIGHT]);

	xrStringToPath(openxr_.xr_instance_, "/user/hand/left/input/y/click", &YB_ClickPath[LEFT]);
	xrStringToPath(openxr_.xr_instance_, "/user/hand/right/input/b/click", &YB_ClickPath[RIGHT]);
#endif

#if SUPPORT_GRIP
	xrStringToPath(openxr_.xr_instance_, "/user/hand/left/input/squeeze/value", &squeezeValuePath[LEFT]);
	xrStringToPath(openxr_.xr_instance_, "/user/hand/right/input/squeeze/value", &squeezeValuePath[RIGHT]);

	xrStringToPath(openxr_.xr_instance_, "/user/hand/left/input/squeeze/force", &squeezeForcePath[LEFT]);
	xrStringToPath(openxr_.xr_instance_, "/user/hand/right/input/squeeze/force", &squeezeForcePath[RIGHT]);

	xrStringToPath(openxr_.xr_instance_, "/user/hand/left/input/squeeze/click", &squeezeClickPath[LEFT]);
	xrStringToPath(openxr_.xr_instance_, "/user/hand/right/input/squeeze/click", &squeezeClickPath[RIGHT]);

	actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
	strcpy_s(actionInfo.actionName, "grab_object");
	strcpy_s(actionInfo.localizedActionName, "Grab Object");
	actionInfo.countSubactionPaths = uint32_t(hand_subaction_path_.size());
	actionInfo.subactionPaths = hand_subaction_path_.data();
	xrCreateAction(xr_action_set_, &actionInfo, &grabAction);
#endif

#if SUPPORT_TRIGGERS
	actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
	strcpy_s(actionInfo.actionName, "trigger_object");
	strcpy_s(actionInfo.localizedActionName, "Pull Trigger");
	actionInfo.countSubactionPaths = uint32_t(hand_subaction_path_.size());
	actionInfo.subactionPaths = hand_subaction_path_.data();
	xrCreateAction(xr_action_set_, &actionInfo, &triggerPullAction);
#endif

#if SUPPORT_GRIP_POSE
	xrStringToPath(openxr_.xr_instance_, "/user/hand/left/input/grip/pose", &gripPath[LEFT]);
	xrStringToPath(openxr_.xr_instance_, "/user/hand/right/input/grip/pose", &gripPath[RIGHT]);

	// Create an input action getting the left and right hand poses.
	actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
	strcpy_s(actionInfo.actionName, "grip_pose");
	strcpy_s(actionInfo.localizedActionName, "Grip Pose");
	actionInfo.countSubactionPaths = uint32_t(hand_subaction_path_.size());
	actionInfo.subactionPaths = hand_subaction_path_.data();
	xrCreateAction(xr_action_set_, &actionInfo, &gripPoseAction);
#endif

#if SUPPORT_AIM_POSE
	xrStringToPath(openxr_.xr_instance_, "/user/hand/left/input/aim/pose", &aimPath[LEFT]);
	xrStringToPath(openxr_.xr_instance_, "/user/hand/right/input/aim/pose", &aimPath[RIGHT]);

	actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
	strcpy_s(actionInfo.actionName, "aim_pose");
	strcpy_s(actionInfo.localizedActionName, "Aim Pose");
	actionInfo.countSubactionPaths = uint32_t(hand_subaction_path_.size());
	actionInfo.subactionPaths = hand_subaction_path_.data();
	xrCreateAction(xr_action_set_, &actionInfo, &aimPoseAction);
#endif

#if SUPPORT_THUMBSTICKS
	xrStringToPath(openxr_.xr_instance_, "/user/hand/left/input/thumbstick/touch", &stickTouchPath[LEFT]);
	xrStringToPath(openxr_.xr_instance_, "/user/hand/right/input/thumbstick/touch", &stickTouchPath[RIGHT]);

	xrStringToPath(openxr_.xr_instance_, "/user/hand/left/input/thumbstick/click", &stickClickPath[LEFT]);
	xrStringToPath(openxr_.xr_instance_, "/user/hand/right/input/thumbstick/click", &stickClickPath[RIGHT]);

#if SEPARATE_X_Y_AXES_FOR_THUMBSTICKS
	xrStringToPath(openxr_.xr_instance_, "/user/hand/left/input/thumbstick/x", &stickXPath[LEFT]);
	xrStringToPath(openxr_.xr_instance_, "/user/hand/right/input/thumbstick/x", &stickXPath[RIGHT]);

	xrStringToPath(openxr_.xr_instance_, "/user/hand/left/input/thumbstick/y", &stickYPath[LEFT]);
	xrStringToPath(openxr_.xr_instance_, "/user/hand/right/input/thumbstick/y", &stickYPath[RIGHT]);

	actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
	strcpy_s(actionInfo.actionName, "thumbstick_x");
	strcpy_s(actionInfo.localizedActionName, "Thumbstick X");
	xrCreateAction(xr_action_set_, &actionInfo, &thumbstickXAction);

	actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
	strcpy_s(actionInfo.actionName, "thumbstick_y");
	strcpy_s(actionInfo.localizedActionName, "Thumbstick Y");
	xrCreateAction(xr_action_set_, &actionInfo, &thumbstickYAction);
#else
	xrStringToPath(openxr_.xr_instance_, "/user/hand/left/input/thumbstick", &thumbstickPath[LEFT]);
	xrStringToPath(openxr_.xr_instance_, "/user/hand/right/input/thumbstick", &thumbstickPath[RIGHT]);

	actionInfo.actionType = XR_ACTION_TYPE_VECTOR2F_INPUT;
	strcpy_s(actionInfo.actionName, "thumbstick");
	strcpy_s(actionInfo.localizedActionName, "Thumbstick");
	actionInfo.countSubactionPaths = uint32_t(hand_subaction_path_.size());
	actionInfo.subactionPaths = hand_subaction_path_.data();
	xrCreateAction(xr_action_set_, &actionInfo, &thumbstickAction);
#endif
	actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
	strcpy(actionInfo.actionName, "thumbstick_touch");
	strcpy(actionInfo.localizedActionName, "Thumbstick Touch");
	xrCreateAction(xr_action_set_, &actionInfo, &thumbstickTouchAction);

	actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
	strcpy(actionInfo.actionName, "thumbstick_click");
	strcpy(actionInfo.localizedActionName, "Thumbstick Click");
	xrCreateAction(xr_action_set_, &actionInfo, &thumbstickClickAction);
#endif

#if SUPPORT_HAPTICS
	// Create output actions for vibrating the left and right controller.
	actionInfo.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
	strcpy_s(actionInfo.actionName, "vibrate_hand");
	strcpy_s(actionInfo.localizedActionName, "Vibrate Hand");
	actionInfo.countSubactionPaths = uint32_t(hand_subaction_path_.size());
	actionInfo.subactionPaths = hand_subaction_path_.data();
	xrCreateAction(xr_action_set_, &actionInfo, &vibrateAction);
#endif

#if SUPPORT_FACE_BUTTONS
	actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
	strcpy_s(actionInfo.actionName, "button_xa_touch");
	strcpy_s(actionInfo.localizedActionName, "Button X/A Touch");
	actionInfo.countSubactionPaths = uint32_t(hand_subaction_path_.size());
	actionInfo.subactionPaths = hand_subaction_path_.data();
	xrCreateAction(xr_action_set_, &actionInfo, &button_XA_Touch_Action);

	actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
	strcpy_s(actionInfo.actionName, "button_xa_click");
	strcpy_s(actionInfo.localizedActionName, "Button X/A Click");
	actionInfo.countSubactionPaths = uint32_t(hand_subaction_path_.size());
	actionInfo.subactionPaths = hand_subaction_path_.data();
	xrCreateAction(xr_action_set_, &actionInfo, &button_XA_Click_Action);

	actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
	strcpy_s(actionInfo.actionName, "button_yb_touch");
	strcpy_s(actionInfo.localizedActionName, "Button Y/B Touch");
	actionInfo.countSubactionPaths = uint32_t(hand_subaction_path_.size());
	actionInfo.subactionPaths = hand_subaction_path_.data();
	xrCreateAction(xr_action_set_, &actionInfo, &button_YB_Touch_Action);

	actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
	strcpy_s(actionInfo.actionName, "button_yb_click");
	strcpy_s(actionInfo.localizedActionName, "Button Y/B Click");
	actionInfo.countSubactionPaths = uint32_t(hand_subaction_path_.size());
	actionInfo.subactionPaths = hand_subaction_path_.data();
	xrCreateAction(xr_action_set_, &actionInfo, &button_YB_Click_Action);
#endif

#if SUPPORT_MENU
	actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
	strcpy_s(actionInfo.actionName, "menu");
	strcpy_s(actionInfo.localizedActionName, "Menu Button");
	//actionInfo.countSubactionPaths = 0;
	//actionInfo.subactionPaths = nullptr;
	actionInfo.countSubactionPaths = uint32_t(hand_subaction_path_.size());
	actionInfo.subactionPaths = hand_subaction_path_.data();
	xrCreateAction(xr_action_set_, &actionInfo, &menuAction);
#endif

#if ENABLE_EXT_EYE_TRACKING
	openxr_.CreateEXTEyeTracking();
#endif

	return true;
}


bool XRInputState::init_controllers()
{
	assert(openxr_.xr_instance_);
	assert(openxr_.xr_session_);
	assert(xr_action_set_);

#if SUPPORT_KHR_SIMPLE_CONTROLLERS
	// Suggest bindings for KHR Simple.
	{
		XrPath khrSimpleInteractionProfilePath;
		xrStringToPath(openxr_.xr_instance_, "/interaction_profiles/khr/simple_controller", &khrSimpleInteractionProfilePath);

		std::vector<XrActionSuggestedBinding> bindings{ {// Fall back to a click input for the grab action.
			{grabAction, selectPath[LEFT]},
			{grabAction, selectPath[RIGHT]},
#if SUPPORT_GRIP_POSE
			{gripPoseAction, gripPath[LEFT]},
			{gripPoseAction, gripPath[RIGHT]},
#endif
#if SUPPORT_AIM_POSE
			{aimPoseAction, aimPath[LEFT]},
			{aimPoseAction, aimPath[RIGHT]},
#endif
#if SUPPORT_MENU
			{menuAction, menuClickPath[LEFT]},
			{menuAction, menuClickPath[RIGHT]},
#endif
#if SUPPORT_HAPTICS
			{vibrateAction, hapticPath[LEFT]},
			{vibrateAction, hapticPath[RIGHT]}
#endif
			} };

		XrInteractionProfileSuggestedBinding suggestedBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
		suggestedBindings.interactionProfile = khrSimpleInteractionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		xrSuggestInteractionProfileBindings(openxr_.xr_instance_, &suggestedBindings);
	}
#endif

#if SUPPORT_TOUCH_CONTROLLERS
	// Suggest bindings for the Oculus Touch.
	{
		XrPath oculusTouchInteractionProfilePath;
		xrStringToPath(openxr_.xr_instance_, "/interaction_profiles/oculus/touch_controller", &oculusTouchInteractionProfilePath);

		std::vector<XrActionSuggestedBinding> bindings
		{
			{
#if SUPPORT_GRIP
				{grabAction, squeezeValuePath[LEFT]},
			{grabAction, squeezeValuePath[RIGHT]},
#endif

#if SUPPORT_TRIGGERS
			{triggerPullAction, triggerValuePath[LEFT]},
			{triggerPullAction, triggerValuePath[RIGHT]},
#endif

#if SUPPORT_GRIP_POSE
			{gripPoseAction, gripPath[LEFT]},
			{gripPoseAction, gripPath[RIGHT]},
#endif

#if SUPPORT_AIM_POSE
			{aimPoseAction, aimPath[LEFT]},
			{aimPoseAction, aimPath[RIGHT]},
#endif

#if SUPPORT_THUMBSTICKS

#if SEPARATE_X_Y_AXES_FOR_THUMBSTICKS
			{thumbstickXAction, stickXPath[LEFT]},
			{thumbstickXAction, stickXPath[RIGHT]},
			{thumbstickYAction, stickYPath[LEFT]},
			{thumbstickYAction, stickYPath[RIGHT]},
#else
			{thumbstickAction, thumbstickPath[LEFT]},
			{thumbstickAction, thumbstickPath[RIGHT]},
#endif

			{thumbstickTouchAction, stickTouchPath[LEFT]},
			{thumbstickTouchAction, stickTouchPath[RIGHT]},
			{thumbstickClickAction, stickClickPath[LEFT]},
			{thumbstickClickAction, stickClickPath[RIGHT]},
#endif

#if SUPPORT_MENU
			{menuAction, menuClickPath[LEFT]},
			{menuAction, menuClickPath[RIGHT]},
#endif

#if SUPPORT_FACE_BUTTONS
			{button_XA_Touch_Action, XA_TouchPath[LEFT]},
			{button_XA_Click_Action, XA_ClickPath[LEFT]},

			{button_XA_Touch_Action, XA_TouchPath[RIGHT]},
			{button_XA_Click_Action, XA_ClickPath[RIGHT]},

			{button_YB_Touch_Action, YB_TouchPath[LEFT]},
			{button_YB_Click_Action, YB_ClickPath[LEFT]},

			{button_YB_Touch_Action, YB_TouchPath[RIGHT]},
			{button_YB_Click_Action, YB_ClickPath[RIGHT]},
#endif

#if SUPPORT_HAPTICS
			{vibrateAction, hapticPath[LEFT]},
			{vibrateAction, hapticPath[RIGHT]},
#endif
			}
		};

		XrInteractionProfileSuggestedBinding suggestedBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
		suggestedBindings.interactionProfile = oculusTouchInteractionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();

		xrSuggestInteractionProfileBindings(openxr_.xr_instance_, &suggestedBindings);
	}
#endif

#if SUPPORT_KHR_GENERIC_CONTROLLERS
	// Suggest bindings for XR_KHR_generic_controller 
	// https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html#XR_KHR_generic_controller
	{
		XrPath khrGenericInteractionProfilePath;
		xrStringToPath(openxr_.xr_instance_, "/interaction_profiles/khr/generic_controller", &khrGenericInteractionProfilePath);

		std::vector<XrActionSuggestedBinding> bindings{ {{grabAction, squeezeValuePath[LEFT]},
			{grabAction, squeezeValuePath[RIGHT]},
#if SUPPORT_GRIP_POSE
			{gripPoseAction, posePath[LEFT]},
			{gripPoseAction, posePath[RIGHT]},
#endif
#if SUPPORT_AIM_POSE
			{aimPoseAction, aimPath[LEFT]},
			{aimPoseAction, aimPath[RIGHT]},
#endif
#if SUPPORT_THUMBSTICKS

#if SEPARATE_X_Y_AXES_FOR_THUMBSTICKS
			{thumbstickXAction, stickXPath[LEFT]},
			{thumbstickXAction, stickXPath[RIGHT]},
			{thumbstickYAction, stickYPath[LEFT]},
			{thumbstickYAction, stickYPath[RIGHT]},
#else
			{thumbstickAction, thumbstickPath[LEFT]},
			{thumbstickAction, thumbstickPath[RIGHT]},
#endif

			{thumbstickTouchAction, stickTouchPath[LEFT]},
			{thumbstickTouchAction, stickTouchPath[RIGHT]},
			{thumbstickClickAction, stickClickPath[LEFT]},
			{thumbstickClickAction, stickClickPath[RIGHT]},
#endif
#if SUPPORT_TRIGGERS
			{triggerClickAction, triggerValuePath[LEFT]},
			{triggerClickAction, triggerValuePath[RIGHT]},
			{triggerPullAction, triggerValuePath[LEFT]},
			{triggerPullAction, triggerValuePath[RIGHT]},
#endif
#if SUPPORT_FACE_BUTTONS
			{button_XA_Touch_Action, XA_TouchPath[LEFT]},
			{button_XA_Click_Action, XA_ClickPath[LEFT]},

			{button_XA_Touch_Action, XA_TouchPath[RIGHT]},
			{button_XA_Click_Action, XA_ClickPath[RIGHT]},

			{button_YB_Touch_Action, YB_TouchPath[LEFT]},
			{button_YB_Click_Action, YB_ClickPath[LEFT]},

			{button_YB_Touch_Action, YB_TouchPath[RIGHT]},
			{button_YB_Click_Action, YB_ClickPath[RIGHT]},
#endif
#if SUPPORT_MENU
			{menuAction, menuClickPath[LEFT]},
#endif
#if SUPPORT_HAPTICS
			{vibrateAction, hapticPath[LEFT]},
			{vibrateAction, hapticPath[RIGHT]}
#endif
			}
		};

		XrInteractionProfileSuggestedBinding suggestedBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
		suggestedBindings.interactionProfile = khrGenericInteractionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		xrSuggestInteractionProfileBindings(openxr_.xr_instance_, &suggestedBindings);
	}
#endif

#if SUPPORT_VIVE_CONTROLLERS
	// Suggest bindings for the Vive Controller.
	{
		XrPath viveControllerInteractionProfilePath;
		xrStringToPath(openxr_.xr_instance_, "/interaction_profiles/htc/vive_controller", &viveControllerInteractionProfilePath);

		std::vector<XrActionSuggestedBinding> bindings{ {{grabAction, triggerValuePath[LEFT]},
			{grabAction, triggerValuePath[RIGHT]},
			{gripPoseAction, gripPath[LEFT]},
			{gripPoseAction, gripPath[RIGHT]},
#if SUPPORT_MENU
			{menuAction, menuClickPath[LEFT]},
			{menuAction, menuClickPath[RIGHT]},
#endif
#if SUPPORT_HAPTICS
			{vibrateAction, hapticPath[LEFT]},
			{vibrateAction, hapticPath[RIGHT]}
#endif
			} };

		XrInteractionProfileSuggestedBinding suggestedBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
		suggestedBindings.interactionProfile = viveControllerInteractionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		xrSuggestInteractionProfileBindings(openxr_.xr_instance_, &suggestedBindings);
	}
#endif

#if SUPPORT_VALVE_INDEX_CONTROLLERS
	// Suggest bindings for the Valve Index Controller.
	{
		XrPath indexControllerInteractionProfilePath;
		xrStringToPath(openxr_.xr_instance_, "/interaction_profiles/valve/index_controller", &indexControllerInteractionProfilePath);

		std::vector<XrActionSuggestedBinding> bindings{ {{grabAction, squeezeForcePath[LEFT]},
			{grabAction, squeezeForcePath[RIGHT]},
			{gripPoseAction, gripPath[LEFT]},
			{gripPoseAction, gripPath[RIGHT]},
#if SUPPORT_MENU
			{menuAction, menuClickPath[LEFT]},
			{menuAction, menuClickPath[RIGHT]},
#endif
#if SUPPORT_HAPTICS
			{vibrateAction, hapticPath[LEFT]},
			{vibrateAction, hapticPath[RIGHT]}
#endif
			} };

		XrInteractionProfileSuggestedBinding suggestedBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
		suggestedBindings.interactionProfile = indexControllerInteractionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		xrSuggestInteractionProfileBindings(openxr_.xr_instance_, &suggestedBindings);
	}
#endif

#if SUPPORT_WMR_CONTROLLERS
	// Suggest bindings for the Microsoft Mixed Reality Motion Controller.
	{
		XrPath microsoftMixedRealityInteractionProfilePath;
		xrStringToPath(openxr_.xr_instance_, "/interaction_profiles/microsoft/motion_controller", &microsoftMixedRealityInteractionProfilePath);

		std::vector<XrActionSuggestedBinding> bindings{ {{grabAction, squeezeClickPath[LEFT]},
			{grabAction, squeezeClickPath[RIGHT]},
			{gripPoseAction, gripPath[LEFT]},
			{gripPoseAction, gripPath[RIGHT]},
#if SUPPORT_MENU
			{menuAction, menuClickPath[LEFT]},
			{menuAction, menuClickPath[RIGHT]},
#endif
#if SUPPORT_HAPTICS
			{vibrateAction, hapticPath[LEFT]},
			{vibrateAction, hapticPath[RIGHT]}
#endif
			} };

		XrInteractionProfileSuggestedBinding suggestedBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
		suggestedBindings.interactionProfile = microsoftMixedRealityInteractionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		xrSuggestInteractionProfileBindings(openxr_.xr_instance_, &suggestedBindings);
	}
#endif

#if ENABLE_VIVE_TRACKERS
	if(openxr_.supports_HTCX_vive_tracker_interaction_)
	{
		// From https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_HTCX_vive_tracker_interaction
		XrPath viveTrackerInteractionProfilePath;

		xrStringToPath(openxr_.xr_instance_, "/interaction_profiles/htc/vive_tracker_htcx",	&viveTrackerInteractionProfilePath);

		// NB Can use xrEnumerateViveTrackerPathsHTCX instead of these hardcoded paths. This'll do for now...

#if ENABLE_VIVE_HANDHELD_OBJECTS
		// TODO: Need sub path per hand
		// 
		{
			ViveTrackerInfo tracker_info;
			tracker_info.subaction = "/user/vive_tracker_htcx/role/handheld_object";
			tracker_info.actionName = "left_handheld_object_pose";
			tracker_info.localizedActionName = "Left Handheld Object Pose";
			tracker_info.bindingPath = "/user/vive_tracker_htcx/role/left_foot/input/grip/pose";

			tracker_infos_.push_back(tracker_info);
		}

		{
			ViveTrackerInfo tracker_info;
			tracker_info.subaction = "/user/vive_tracker_htcx/role/handheld_object";
			tracker_info.actionName = "right_handheld_object_pose";
			tracker_info.localizedActionName = "Right Handheld Object Pose";
			tracker_info.bindingPath = "/user/vive_tracker_htcx/role/handheld_object/input/grip/pose";

			tracker_infos_.push_back(tracker_info);
		}
#endif

#if ENABLE_VIVE_FEET
		{
			ViveTrackerInfo tracker_info;
			tracker_info.subaction = "/user/vive_tracker_htcx/role/left_foot";
			tracker_info.actionName = "left_foot_pose";
			tracker_info.localizedActionName = "Left Foot Pose";
			tracker_info.bindingPath = "/user/vive_tracker_htcx/role/left_foot/input/grip/pose";

			tracker_infos_.push_back(tracker_info);
		}

		{
			ViveTrackerInfo tracker_info;
			tracker_info.subaction = "/user/vive_tracker_htcx/role/right_foot";
			tracker_info.actionName = "right_foot_pose";
			tracker_info.localizedActionName = "Right Foot Pose";
			tracker_info.bindingPath = "/user/vive_tracker_htcx/role/right_foot/input/grip/pose";

			tracker_infos_.push_back(tracker_info);
		}
#endif


#if ENABLE_VIVE_SHOULDERS
		{
			ViveTrackerInfo tracker_info;
			tracker_info.subaction = "/user/vive_tracker_htcx/role/left_shoulder";
			tracker_info.actionName = "left_shoulder_pose";
			tracker_info.localizedActionName = "Left Shoulder Pose";
			tracker_info.bindingPath = "/user/vive_tracker_htcx/role/left_shoulder/input/grip/pose";

			tracker_infos_.push_back(tracker_info);
		}

		{
			ViveTrackerInfo tracker_info;
			tracker_info.subaction = "/user/vive_tracker_htcx/role/right_shoulder";
			tracker_info.actionName = "right_shoulder_pose";
			tracker_info.localizedActionName = "Right Shoulder Pose";
			tracker_info.bindingPath = "/user/vive_tracker_htcx/role/right_shoulder/input/grip/pose";

			tracker_infos_.push_back(tracker_info);
		}
#endif

#if ENABLE_VIVE_ELBOWS
		{
			ViveTrackerInfo tracker_info;
			tracker_info.subaction = "/user/vive_tracker_htcx/role/left_elbow";
			tracker_info.actionName = "left_elbow_pose";
			tracker_info.localizedActionName = "Left Elbow Pose";
			tracker_info.bindingPath = "/user/vive_tracker_htcx/role/left_elbow/input/grip/pose";

			tracker_infos_.push_back(tracker_info);
		}

		{
			ViveTrackerInfo tracker_info;
			tracker_info.subaction = "/user/vive_tracker_htcx/role/right_elbow";
			tracker_info.actionName = "right_elbow_pose";
			tracker_info.localizedActionName = "Right Elbow Pose";
			tracker_info.bindingPath = "/user/vive_tracker_htcx/role/right_elbow/input/grip/pose";

			tracker_infos_.push_back(tracker_info);
		}
#endif

#if ENABLE_VIVE_KNEES
		{
			ViveTrackerInfo tracker_info;
			tracker_info.subaction = "/user/vive_tracker_htcx/role/left_knee";
			tracker_info.actionName = "left_knee_pose";
			tracker_info.localizedActionName = "Left Knee Pose";
			tracker_info.bindingPath = "/user/vive_tracker_htcx/role/left_knee/input/grip/pose";

			tracker_infos_.push_back(tracker_info);
		}

		{
			ViveTrackerInfo tracker_info;
			tracker_info.subaction = "/user/vive_tracker_htcx/role/right_knee";
			tracker_info.actionName = "right_knee_pose";
			tracker_info.localizedActionName = "Right Knee Pose";
			tracker_info.bindingPath = "/user/vive_tracker_htcx/role/right_knee/input/grip/pose";

			tracker_infos_.push_back(tracker_info);
		}
#endif

#if ENABLE_VIVE_WRISTS
		{
			ViveTrackerInfo tracker_info;
			tracker_info.subaction = "/user/vive_tracker_htcx/role/left_wrist";
			tracker_info.actionName = "left_wrist_pose";
			tracker_info.localizedActionName = "Left Wrist Pose";
			tracker_info.bindingPath = "/user/vive_tracker_htcx/role/left_wrist/input/grip/pose";

			tracker_infos_.push_back(tracker_info);
		}

		{
			ViveTrackerInfo tracker_info;
			tracker_info.subaction = "/user/vive_tracker_htcx/role/right_wrist";
			tracker_info.actionName = "right_wrist_pose";
			tracker_info.localizedActionName = "Right Wrist Pose";
			tracker_info.bindingPath = "/user/vive_tracker_htcx/role/right_wrist/input/grip/pose";

			tracker_infos_.push_back(tracker_info);
		}
#endif

#if ENABLE_VIVE_ANKLES
		{
			ViveTrackerInfo tracker_info;
			tracker_info.subaction = "/user/vive_tracker_htcx/role/left_ankle";
			tracker_info.actionName = "left_ankle_pose";
			tracker_info.localizedActionName = "Left Ankle Pose";
			tracker_info.bindingPath = "/user/vive_tracker_htcx/role/left_ankle/input/grip/pose";

			tracker_infos_.push_back(tracker_info);
		}

		{
			ViveTrackerInfo tracker_info;
			tracker_info.subaction = "/user/vive_tracker_htcx/role/right_ankle";
			tracker_info.actionName = "right_ankle_pose";
			tracker_info.localizedActionName = "Right Ankle Pose";
			tracker_info.bindingPath = "/user/vive_tracker_htcx/role/right_ankle/input/grip/pose";

			tracker_infos_.push_back(tracker_info);
		}
#endif

#if ENABLE_VIVE_WAIST
		{
			ViveTrackerInfo tracker_info;
			tracker_info.subaction = "/user/vive_tracker_htcx/role/waist";
			tracker_info.actionName = "waist_pose";
			tracker_info.localizedActionName = "Waist Pose";
			tracker_info.bindingPath = "/user/vive_tracker_htcx/role/waist/input/grip/pose";

			tracker_infos_.push_back(tracker_info);
		}
#endif

#if ENABLE_VIVE_CHEST
		{
			ViveTrackerInfo tracker_info;
			tracker_info.subaction = "/user/vive_tracker_htcx/role/chest";
			tracker_info.actionName = "chest_pose";
			tracker_info.localizedActionName = "Chest Pose";
			tracker_info.bindingPath = "/user/vive_tracker_htcx/role/chest/input/grip/pose";

			tracker_infos_.push_back(tracker_info);
		}
#endif

#if ENABLE_VIVE_CAMERA
		{
			ViveTrackerInfo tracker_info;
			tracker_info.subaction = "/user/vive_tracker_htcx/role/camera";
			tracker_info.actionName = "camera_pose";
			tracker_info.localizedActionName = "Camera Pose";
			tracker_info.bindingPath = "/user/vive_tracker_htcx/role/chest/input/camera/pose";

			tracker_infos_.push_back(tracker_info);
		}
#endif

#if ENABLE_VIVE_KEYBOARD
		{
			ViveTrackerInfo tracker_info;
			tracker_info.subaction = "/user/vive_tracker_htcx/role/keyboard";
			tracker_info.actionName = "keyboard_pose";
			tracker_info.localizedActionName = "Keyboard Pose";
			tracker_info.bindingPath = "/user/vive_tracker_htcx/role/chest/input/keyboard/pose";

			tracker_infos_.push_back(tracker_info);
		}
#endif

		std::vector<XrActionSuggestedBinding> actionSuggBindings;

		const int num_trackers = (int)tracker_infos_.size();

		for(int tracker_index = 0; tracker_index < num_trackers; tracker_index++)
		{
			ViveTrackerInfo& tracker_info = tracker_infos_[tracker_index];
			xrStringToPath(openxr_.xr_instance_, tracker_info.subaction.c_str(), &tracker_info.tracker_role_path);

			XrActionCreateInfo actionInfo{ XR_TYPE_ACTION_CREATE_INFO };
			actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
			strcpy_s(actionInfo.actionName, tracker_info.actionName.c_str());
			strcpy_s(actionInfo.localizedActionName, tracker_info.localizedActionName.c_str());
			actionInfo.countSubactionPaths = 1;
			actionInfo.subactionPaths = &tracker_info.tracker_role_path;
			xrCreateAction(xr_action_set_, &actionInfo, &tracker_info.tracker_pose_action);

			// Describe a suggested binding for that action and subaction path.
			XrPath suggestedBindingPath;
			xrStringToPath(openxr_.xr_instance_, tracker_info.bindingPath.c_str(), &suggestedBindingPath);

			XrActionSuggestedBinding actionSuggBinding;
			actionSuggBinding.action = tracker_info.tracker_pose_action;
			actionSuggBinding.binding = suggestedBindingPath;
			actionSuggBindings.push_back(actionSuggBinding);

			// Create action space for locating tracker
			XrActionSpaceCreateInfo actionSpaceInfo{ XR_TYPE_ACTION_SPACE_CREATE_INFO };
			actionSpaceInfo.action = tracker_info.tracker_pose_action;
			actionSpaceInfo.subactionPath = tracker_info.tracker_role_path;
			xrCreateActionSpace(openxr_.xr_session_, &actionSpaceInfo, &tracker_info.tracker_pose_space);
		}

		XrInteractionProfileSuggestedBinding profileSuggBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };

		profileSuggBindings.interactionProfile = viveTrackerInteractionProfilePath;
		profileSuggBindings.suggestedBindings = actionSuggBindings.data();
		profileSuggBindings.countSuggestedBindings = (uint32_t)actionSuggBindings.size();

		xrSuggestInteractionProfileBindings(openxr_.xr_instance_, &profileSuggBindings);
	}
#endif // ENABLE_VIVE_TRACKERS

#if ENABLE_EXT_EYE_TRACKING
	if(openxr_.supports_ext_eye_tracking_)
	{
		XrPath eyeGazeInteractionProfilePath;
		xrStringToPath(openxr_.xr_instance_, "/interaction_profiles/ext/eye_gaze_interaction", &eyeGazeInteractionProfilePath);

		XrPath gazePosePath;
		xrStringToPath(openxr_.xr_instance_, "/user/eyes_ext/input/gaze_ext/pose", &gazePosePath);

		XrActionSuggestedBinding bindings;
		bindings.action = gazeAction;
		bindings.binding = gazePosePath;

		XrInteractionProfileSuggestedBinding suggestedBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
		suggestedBindings.interactionProfile = eyeGazeInteractionProfilePath;
		suggestedBindings.suggestedBindings = &bindings;
		suggestedBindings.countSuggestedBindings = 1;
		xrSuggestInteractionProfileBindings(openxr_.xr_instance_, &suggestedBindings);
	}
#endif

#if SUPPORT_GRIP_POSE
	{
		XrActionSpaceCreateInfo actionSpaceInfo{ XR_TYPE_ACTION_SPACE_CREATE_INFO };
		actionSpaceInfo.action = gripPoseAction;
		actionSpaceInfo.poseInActionSpace.orientation.w = 1.f;
		actionSpaceInfo.subactionPath = hand_subaction_path_[LEFT];

		xrCreateActionSpace(openxr_.xr_session_, &actionSpaceInfo, &gripSpace[LEFT]);

		actionSpaceInfo.subactionPath = hand_subaction_path_[RIGHT];
		xrCreateActionSpace(openxr_.xr_session_, &actionSpaceInfo, &gripSpace[RIGHT]);
	}
#endif

#if SUPPORT_AIM_POSE
	{
		XrActionSpaceCreateInfo actionSpaceInfo{ XR_TYPE_ACTION_SPACE_CREATE_INFO };
		actionSpaceInfo.action = aimPoseAction;

		actionSpaceInfo.subactionPath = hand_subaction_path_[LEFT];
		xrCreateActionSpace(openxr_.xr_session_, &actionSpaceInfo, &aimSpace[LEFT]);

		actionSpaceInfo.subactionPath = hand_subaction_path_[RIGHT];
		xrCreateActionSpace(openxr_.xr_session_, &actionSpaceInfo, &aimSpace[RIGHT]);
	}
#endif

	XrSessionActionSetsAttachInfo attachInfo{ XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
	attachInfo.countActionSets = 1;
	attachInfo.actionSets = &xr_action_set_;
	xrAttachSessionActionSets(openxr_.xr_session_, &attachInfo);

	return true;
}

bool XRInputState::update()
{
	if(!is_initialized() || !openxr_.is_session_running())
	{
		return false;
	}

	const XrActiveActionSet activeActionSet{ xr_action_set_, XR_NULL_PATH };
	XrActionsSyncInfo syncInfo{ XR_TYPE_ACTIONS_SYNC_INFO };
	syncInfo.countActiveActionSets = 1;
	syncInfo.activeActionSets = &activeActionSet;
	xrSyncActions(openxr_.xr_session_, &syncInfo);

	for(int hand : { LEFT, RIGHT })
	{
#if SUPPORT_GRIP
		{
			XrActionStateGetInfo get_grab_info{ XR_TYPE_ACTION_STATE_GET_INFO };
			get_grab_info.action = grabAction;
			get_grab_info.subactionPath = hand_subaction_path_[hand];

			XrActionStateFloat grab_value{ XR_TYPE_ACTION_STATE_FLOAT };
			xrGetActionStateFloat(openxr_.xr_session_, &get_grab_info, &grab_value);

			if(grab_value.isActive == XR_TRUE)
			{
				if(grab_value.currentState > 0.1f)
				{
#if SUPPORT_HAPTICS
					XrHapticVibration vibration{ XR_TYPE_HAPTIC_VIBRATION };
					vibration.amplitude = grab_value.currentState;
					vibration.duration = XR_MIN_HAPTIC_DURATION;
					vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

					XrHapticActionInfo hapticActionInfo{ XR_TYPE_HAPTIC_ACTION_INFO };
					hapticActionInfo.action = vibrateAction;
					hapticActionInfo.subactionPath = hand_subaction_path_[hand];

					xrApplyHapticFeedback(openxr_.xr_session_, &hapticActionInfo, (XrHapticBaseHeader*)&vibration);

#endif // SUPPORT_HAPTICS					
					openxr_.set_hand_gripping((uint)hand, true);
					openxr_.update_grip((uint)hand, grab_value.currentState);

				}
				else
				{
					openxr_.set_hand_gripping((uint)hand, false);
				}
			}
			else
			{
				openxr_.set_hand_gripping((uint)hand, false);
			}
		}
#endif

#if SUPPORT_TRIGGERS
		{
			XrActionStateGetInfo get_trigger_info{ XR_TYPE_ACTION_STATE_GET_INFO };
			get_trigger_info.action = triggerPullAction;
			get_trigger_info.subactionPath = hand_subaction_path_[hand];

			XrActionStateFloat trigger_value{ XR_TYPE_ACTION_STATE_FLOAT };
			xrGetActionStateFloat(openxr_.xr_session_, &get_trigger_info, &trigger_value);

			if(trigger_value.isActive == XR_TRUE)
			{
				if(trigger_value.currentState > 0.1f)
				{
					openxr_.set_trigger_squeezed((uint)hand, true);
					openxr_.update_trigger((uint)hand, trigger_value.currentState);

#if SUPPORT_HAPTICS
					XrHapticVibration vibration{ XR_TYPE_HAPTIC_VIBRATION };
					vibration.amplitude = trigger_value.currentState;
					vibration.duration = XR_MIN_HAPTIC_DURATION;
					vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

					XrHapticActionInfo hapticActionInfo{ XR_TYPE_HAPTIC_ACTION_INFO };
					hapticActionInfo.action = vibrateAction;
					hapticActionInfo.subactionPath = hand_subaction_path_[hand];
					xrApplyHapticFeedback(openxr_.xr_session_, &hapticActionInfo, (XrHapticBaseHeader*)&vibration);
#endif

				}
				else
				{
					openxr_.set_trigger_squeezed((uint)hand, false);
				}
			}
			else
			{
				openxr_.set_trigger_squeezed((uint)hand, false);
			}
		}
#endif

#if SUPPORT_GRIP_POSE
		{
			XrActionStateGetInfo get_pose_info{ XR_TYPE_ACTION_STATE_GET_INFO };
			get_pose_info.action = gripPoseAction;
			get_pose_info.subactionPath = hand_subaction_path_[hand];
			XrActionStatePose poseState{ XR_TYPE_ACTION_STATE_POSE };
			xrGetActionStatePose(openxr_.xr_session_, &get_pose_info, &poseState);
			//handActive[hand] = poseState.isActive;
		}
#endif

#if SUPPORT_THUMBSTICKS
		{
			// Thumbstick Analog Action
#if SEPARATE_X_Y_AXES_FOR_THUMBSTICKS
			// X axis
			{
				XrActionStateGetInfo action_get_info = { XR_TYPE_ACTION_STATE_GET_INFO };
				action_get_info.action = thumbstickXAction;
				action_get_info.subactionPath = hand_subaction_path_[hand];

				XrActionStateFloat axis_state_x = { XR_TYPE_ACTION_STATE_FLOAT };
				xrGetActionStateFloat(openxr_.xr_session_, &action_get_info, &axis_state_x);

				if(axis_state_x.isActive)
				{
					openxr_.update_thumbstick_x((uint)hand, axis_state_x.currentState);
				}
				else
				{
					openxr_.update_thumbstick_x((uint)hand, 0.0f);
				}
			}

			// Y axis
			{
				XrActionStateGetInfo action_get_info = { XR_TYPE_ACTION_STATE_GET_INFO };
				action_get_info.action = thumbstickYAction;
				action_get_info.subactionPath = hand_subaction_path_[hand];

				XrActionStateFloat axis_state_y = { XR_TYPE_ACTION_STATE_FLOAT };
				xrGetActionStateFloat(openxr_.xr_session_, &action_get_info, &axis_state_y);

				if(axis_state_y.isActive)
				{
					openxr_.update_thumbstick_y((uint)hand, axis_state_y.currentState);
				}
				else
				{
					openxr_.update_thumbstick_y((uint)hand, 0.0f);
				}
			}
#else

			XrActionStateGetInfo get_joystick_info{ XR_TYPE_ACTION_STATE_GET_INFO };
			get_joystick_info.action = thumbstickAction;
			get_joystick_info.subactionPath = hand_subaction_path_[hand];

			XrActionStateVector2f thumbstickValue{ XR_TYPE_ACTION_STATE_VECTOR2F };
			xrGetActionStateVector2f(openxr_.xr_session_, &get_joystick_info, &thumbstickValue);

			if(thumbstickValue.isActive == XR_TRUE)
			{
				update_thumbstick((uint)hand, thumbstickValue.currentState);
			}
			else
			{
				update_thumbstick((uint)hand, float2_zero);
			}
#endif
		}

		{
			// Thumbstick Click Action
			XrActionStateGetInfo get_button_info{ XR_TYPE_ACTION_STATE_GET_INFO };
			get_button_info.action = thumbstickClickAction;
			get_button_info.subactionPath = hand_subaction_path_[hand];

			XrActionStateBoolean button_value{ XR_TYPE_ACTION_STATE_BOOLEAN };
			xrGetActionStateBoolean(openxr_.xr_session_, &get_button_info, &button_value);

			if(button_value.isActive == XR_TRUE)
			{
				//vr_manager.vr_controllers_[hand].digital_[tv::VRButtonID::VRDigitalButton_Joystick_Click].set_state(button_value.currentState);
			}
		}

#endif // SUPPORT_THUMBSTICKS

#if SUPPORT_MENU
		{
			XrActionStateGetInfo menu_info{ XR_TYPE_ACTION_STATE_GET_INFO };
			menu_info.action = menuAction;
			menu_info.subactionPath = hand_subaction_path_[hand];

			XrActionStateBoolean menu_button_value{ XR_TYPE_ACTION_STATE_BOOLEAN };
			xrGetActionStateBoolean(openxr_.xr_session_, &menu_info, &menu_button_value);

			if(menu_button_value.isActive == XR_TRUE)
			{
				//vr_manager.vr_controllers_[hand].digital_[tv::VRButtonID::VRDigitalButton_ApplicationMenu].set_state(menu_button_value.currentState);
			}
		}
#endif

#if SUPPORT_FACE_BUTTONS
		{
			//Button X/A Touch
			XrActionStateGetInfo get_button_info{ XR_TYPE_ACTION_STATE_GET_INFO };
			get_button_info.action = button_XA_Touch_Action;
			get_button_info.subactionPath = hand_subaction_path_[hand];

			XrActionStateBoolean button_value{ XR_TYPE_ACTION_STATE_BOOLEAN };
			xrGetActionStateBoolean(openxr_.xr_session_, &get_button_info, &button_value);

			if(button_value.isActive == XR_TRUE)
			{
				//vr_manager.vr_controllers_[hand].digital_[tv::VRButtonID::VRDigitalButton_XA_Touch].set_state(button_value.currentState);
			}
		}

		{
			//Button X/A Click
			XrActionStateGetInfo get_button_info{ XR_TYPE_ACTION_STATE_GET_INFO };
			get_button_info.action = button_XA_Click_Action;
			get_button_info.subactionPath = hand_subaction_path_[hand];

			XrActionStateBoolean button_value{ XR_TYPE_ACTION_STATE_BOOLEAN };
			xrGetActionStateBoolean(openxr_.xr_session_, &get_button_info, &button_value);

			if(button_value.isActive == XR_TRUE)
			{
				//vr_manager.vr_controllers_[hand].digital_[tv::VRButtonID::VRDigitalButton_XA_Click].set_state(button_value.currentState);
			}
		}

		{
			//Button Y/B Touch
			XrActionStateGetInfo get_button_info{ XR_TYPE_ACTION_STATE_GET_INFO };
			get_button_info.action = button_YB_Touch_Action;
			get_button_info.subactionPath = hand_subaction_path_[hand];

			XrActionStateBoolean button_value{ XR_TYPE_ACTION_STATE_BOOLEAN };
			xrGetActionStateBoolean(openxr_.xr_session_, &get_button_info, &button_value);

			if(button_value.isActive == XR_TRUE)
			{
				//vr_manager.vr_controllers_[hand].digital_[tv::VRButtonID::VRDigitalButton_YB_Touch].set_state(button_value.currentState);
			}
		}

		{
			//Button Y/B Click
			XrActionStateGetInfo get_button_info{ XR_TYPE_ACTION_STATE_GET_INFO };
			get_button_info.action = button_YB_Click_Action;
			get_button_info.subactionPath = hand_subaction_path_[hand];

			XrActionStateBoolean button_value{ XR_TYPE_ACTION_STATE_BOOLEAN };
			xrGetActionStateBoolean(openxr_.xr_session_, &get_button_info, &button_value);

			if(button_value.isActive == XR_TRUE)
			{
				//vr_manager.vr_controllers_[hand].digital_[tv::VRButtonID::VRDigitalButton_YB_Click].set_state(button_value.currentState);
			}
		}
#endif

	}

	return true;
}

bool XRInputState::shutdown()
{
	if (!is_initialized())
	{
		return false;
	}

	if(xr_action_set_ != XR_NULL_HANDLE)
	{
		for(auto hand : { LEFT, RIGHT })
		{
#if SUPPORT_GRIP_POSE
			if(gripSpace[hand])
			{
				xrDestroySpace(gripSpace[hand]);
				gripSpace[hand] = nullptr;
			}
#endif

#if SUPPORT_AIM_POSE
			if(aimSpace[hand])
			{
				xrDestroySpace(aimSpace[hand]);
				aimSpace[hand] = nullptr;
			}
#endif
		}
		xrDestroyActionSet(xr_action_set_);
		xr_action_set_ = XR_NULL_HANDLE;
	}

	set_initialized(false);
	return true;
}

} // BVR

#endif // SUPPORT_OPENXR