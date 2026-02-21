//--------------------------------------------------------------------------------------
// Copyright (c) 2026 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef VR_CONTROLLERS_H
#define VR_CONTROLLERS_H

#include "defines.h"

#if SUPPORT_OPENXR

typedef struct
{
	bool is_down_;
	bool was_pressed_;
	bool was_released_;

	bool has_analog_value_;
	float analog_value_;

} DigitalButton;

typedef struct
{
	float thumbstick_values_[2];

	DigitalButton XA_button_;
	DigitalButton BY_button_;

	DigitalButton joystick_button_;

	DigitalButton trigger_;
	DigitalButton grip_;

} VRControllerState;


#endif // VR_CONTROLLERS_H

#endif // OPENXR_INPUT_H

