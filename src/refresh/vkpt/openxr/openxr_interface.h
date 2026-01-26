//--------------------------------------------------------------------------------------
// Copyright (c) 2026 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef OPENXR_INTERFACE_H
#define OPENXR_INTERFACE_H

#include "defines.h"

#if SUPPORT_OPENXR

#include <stdint.h>
typedef signed char        schar;
typedef long long          slong;
typedef unsigned char      uchar;
typedef unsigned short     ushort;
typedef unsigned int	   uint;
typedef unsigned long long ulong;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#include <array>
#include <map>
#include <list>
#include <vector>
#include <set>
#include <string>

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

#include "openxr_swapchain.h"
#include "openxr_command_buffer.h"
#include "openxr_input.h"

#if ENABLE_OPENXR_FB_BODY_TRACKING
#include <openxr/meta_body_tracking_calibration.h>
#include <openxr/meta_body_tracking_fidelity.h>
#include <openxr/meta_body_tracking_full_body.h>
#endif

#if ENABLE_OPENXR_FB_FACE_TRACKING
//#include <openxr/fb_face_tracking.h>
#endif

#if ENABLE_OPENXR_FB_SIMULTANEOUS_HANDS_AND_CONTROLLERS
//#include <openxr/meta_simultaneous_hands_and_controllers.h>
#endif

#if 0
struct XrMatrix4x4f;

float4x4 tv_convert_xr_to_float4x4(const XrMatrix4x4f& input);
XrMatrix4x4f tv_convert_to_xr(const float4x4& input);

float3 tv_convert(const XrVector3f& input);
XrVector3f tv_convert_to_xr(const float3& input);

Quat tv_convert(const XrQuaternionf& input);
XrQuaternionf tv_convert_to_xr(const Quat& input);

Pose tv_convert_xr_pose_to_pose(const XrPosef& input);
XRPose tv_convert(const XrPosef& input);
XrPosef tv_convert_to_xr(const Pose& input);
#endif


struct XRPose
{
	//Quat rotation_;
	//float3 position_;
};

struct XRView
{
	XRPose xr_pose_;
	//FOVAngles fov_;
};

struct HMDView
{
	XRView eye_poses_[NUM_EYES];
	//const float3 get_head_position_LS() const;
	const XRPose get_head_xr_pose_LS() const;
};

constexpr bool is_xr_pose_valid(XrSpaceLocationFlags locationFlags);

class OpenXR
{
public:
	friend class XRInputState;

	VkInstance vk_instance_ = nullptr;
	VkPhysicalDevice vk_physical_device_ = nullptr;
	VkDevice vk_logical_device_ = nullptr;

	OpenXR();
	virtual ~OpenXR();

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

	int xr_command_buffer_index_ = 0;
	CommandBuffer xr_command_buffers_[NUM_COMMAND_BUFFERS];

	bool is_session_running() const;

	bool override_predicted_display_time_ = false;
	int display_time_offset_us_ = 0;

	XrTime get_predicted_display_time() const;
	
	HMDView& get_hmd_view();
	const HMDView& get_hmd_view() const;

	bool update_views(XrTime predicted_display_time);
	bool update_controller_poses(XrTime predicted_display_time);

#if ENABLE_OPENXR_FB_EYE_TRACKING_SOCIAL
	void set_social_eye_tracking_supported(const bool social_eye_tracking_supported);
	bool is_social_eye_tracking_supported() const;
#endif

#if ENABLE_EXT_EYE_TRACKING
	void set_EXT_eye_tracking_supported(const bool EXT_eye_tracking_supported);
	bool is_EXT_eye_tracking_supported() const;
#endif

#if ENABLE_EYE_TRACKING
	bool is_eye_tracking_supported() const;

	void set_eye_tracking_enabled(const bool enabled);
	bool is_eye_tracking_enabled() const;
	void toggle_eye_tracking();

	void update_gaze_pose(const uint eye, const XrPosef& gaze_pose);
	bool get_gaze_pose(const uint eye, Pose& gaze_pose);
	bool get_gaze_UV(const uint eye, float2& gaze_uv);
#endif

#if ENABLE_BODY_TRACKING
	bool is_body_tracking_supported() const;
	bool is_body_tracking_enabled() const;
	void set_body_tracking_enabled(const bool enabled);
	void toggle_body_tracking();
#endif

#if ENABLE_WAIST_TRACKING
	bool get_waist_pose_LS(Pose& waist_pose_LS) const;
	void update_waist_pose_LS(const XrPosef& xr_waist_pose_LS);
#endif

	void update_thumbstick(const uint hand, const XrVector2f& stick_value);
	void update_thumbstick_x(const uint hand, const float x_axis_value);
	void update_thumbstick_y(const uint hand, const float y_axis_value);

	float get_stick_x_value(const uint hand) const;
	float get_stick_y_value(const uint hand) const;

	bool is_hand_gripping(const uint hand) const;
	void set_hand_gripping(const uint hand, const bool hand_gripping);
	bool hand_gripping_[2] = { false, false };

	bool is_trigger_squeezed(const uint hand) const;
	void set_trigger_squeezed(const uint hand, const bool trigger_squeezed);
	bool trigger_squeezed_[2] = { false, false };

	void update_trigger(const uint hand, const float trigger_value);
	float get_trigger(const uint hand) const;

	void update_grip(const uint hand, const float grip_value);
	float get_grip(const uint hand) const;

	bool render_into_xr_swapchain_ = true;

#if ENABLE_EXT_EYE_TRACKING
	bool EXT_eye_tracking_supported_ = false;
	bool EXT_eye_tracking_enabled_ = false;
#endif

#if ENABLE_EYE_TRACKING
	Pose gaze_poses_[NUM_EYES] = {};
	float2 gaze_UVs_[NUM_EYES] = { float2_half, float2_half };
#endif

	int m_queueFamilyIndex = INVALID_INDEX;
	int m_queueFamilyIndexCompute = INVALID_INDEX;
	int m_queueFamilyIndexTransfer = INVALID_INDEX;

	bool supports_depth_layer_ = false;

#if ENABLE_OPENXR_HAND_TRACKING
	bool supports_hand_tracking_ = false;
#endif

#if ENABLE_OPENXR_FB_EYE_TRACKING_SOCIAL
	bool supports_eye_tracking_social_ = false;
#endif

#if ENABLE_EXT_EYE_TRACKING
	bool supports_ext_eye_tracking_ = false;
#endif

#if ENABLE_OPENXR_FB_FACE_TRACKING
	bool supports_face_tracking_ = false;
#endif

#if ENABLE_OPENXR_FB_BODY_TRACKING
	bool supports_fb_body_tracking_ = false;
#endif


#if ENABLE_OPENXR_META_BODY_TRACKING_FIDELITY
	bool supports_meta_body_tracking_fidelity_ = false;
#endif

#if ENABLE_OPENXR_META_FULL_BODY_TRACKING
	bool supports_meta_full_body_tracking_ = false;
#endif

#if ENABLE_OPENXR_FB_SIMULTANEOUS_HANDS_AND_CONTROLLERS
	bool supports_simultaneous_hands_and_controllers_ = false;
#endif

#if ENABLE_VIVE_TRACKERS
	bool supports_HTCX_vive_tracker_interaction_ = false;
	//Pose local_waist_pose_from_VUT;
#endif

#if USE_RECENTERING
	bool recenter_event_occurred_ = false;
#endif

	bool create_instance(const VkInstanceCreateInfo& instance_create_info);
	void destroy_instance();

	bool init_system();
	void destroy_system();

	bool create_device(const VkDeviceCreateInfo& device_create_info);
	void destroy_device();

	bool create_swapchain();
	void destroy_swapchain();

	bool get_system_properties();

private:
	bool initialized_ = false;

	//HMDView hmd_view_;
	//float2 thumbsticks_[NUM_SIDES] = { float2_zero, float2_zero };

	float triggers_[NUM_SIDES] = { 0.0f, 0.0f };
	float grips_[NUM_SIDES] = { 0.0f, 0.0f };

	XrInstance xr_instance_{ XR_NULL_HANDLE };
	XrSession xr_session_{ XR_NULL_HANDLE };
	XrSpace xr_app_space_{ XR_NULL_HANDLE };
	XrSystemId xr_system_id_{ XR_NULL_SYSTEM_ID };

	std::vector<XrViewConfigurationView> xr_config_views_;
	std::vector<XRSwapchain> xr_swapchains_;
	std::map<XrSwapchain, std::vector<XrSwapchainImageBaseHeader*>> xr_swapchain_images_;
	std::vector<XrView> xr_views_;

	XRMemoryAllocator xr_memory_allocator_ = {};

	int64_t xr_colour_swapchain_format_ = -1;

	std::vector<XrSpace> xr_visualized_spaces_;

	XrSessionState xr_session_state_ = XR_SESSION_STATE_UNKNOWN;
	bool is_xr_session_running_ = false;

	XrEventDataBuffer xr_event_data_buffer_{};
	XrFrameState xr_frame_state_{ XR_TYPE_FRAME_STATE };

	XRInputState xr_input_;

	const std::set<XrEnvironmentBlendMode> m_acceptableBlendModes;

	XrGraphicsBindingVulkan2KHR xr_graphics_binding_{ XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR };

	const XrBaseInStructure* get_graphics_binding() const
	{
		return reinterpret_cast<const XrBaseInStructure*>(&xr_graphics_binding_);
	}

	int64_t select_swap_format(const std::vector<int64_t>& runtimeFormats) const;
	inline XrReferenceSpaceCreateInfo GetXrReferenceSpaceCreateInfo(const std::string& referenceSpaceTypeStr);
	std::vector<XrSwapchainImageBaseHeader*> AllocateSwapchainImageStructs(uint32_t capacity, const XrSwapchainCreateInfo& swapchainCreateInfo);

	std::list<XRSwapchainImageContext> m_swapchainImageContexts;
	std::map<const XrSwapchainImageBaseHeader*, XRSwapchainImageContext*> m_swapchainImageContextMap;

	bool render_composition_layer(std::vector<XrCompositionLayerProjectionView>& projection_layer_views, XrCompositionLayerProjection& composition_layer);
	void render_projection_layer_view(const XrCompositionLayerProjectionView& projection_layer_view, const XrSwapchainImageBaseHeader* swapchain_image, int64_t swapchain_format, int view_id);

	virtual XrStructureType GetGraphicsBindingType() const { return XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR; }
	virtual XrStructureType GetSwapchainImageType() const { return XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR; }

	XrGraphicsRequirementsVulkan2KHR xr_graphics_requirements_{ XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR };

	virtual XrResult CreateVulkanInstanceKHR(XrInstance instance, const XrVulkanInstanceCreateInfoKHR* createInfo, VkInstance* vulkanInstance, VkResult* vulkanResult);
	virtual XrResult CreateVulkanDeviceKHR(XrInstance instance, const XrVulkanDeviceCreateInfoKHR* createInfo, VkDevice* vulkanDevice, VkResult* vulkanResult);
	virtual XrResult GetVulkanGraphicsDevice2KHR(XrInstance instance, const XrVulkanGraphicsDeviceGetInfoKHR* getInfo, VkPhysicalDevice* vulkanPhysicalDevice);
	virtual XrResult GetVulkanGraphicsRequirements2KHR(XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsVulkan2KHR* graphicsRequirements);

#if ENABLE_EXT_EYE_TRACKING
	XrSystemEyeGazeInteractionPropertiesEXT ext_gaze_interaction_properties_{ XR_TYPE_SYSTEM_EYE_GAZE_INTERACTION_PROPERTIES_EXT };
#endif

#if ENABLE_OPENXR_FB_EYE_TRACKING_SOCIAL
	XrSystemEyeTrackingPropertiesFB eye_tracking_social_properties_{ XR_TYPE_SYSTEM_EYE_TRACKING_PROPERTIES_FB };
#endif

	XrSystemProperties xr_system_properties_{ XR_TYPE_SYSTEM_PROPERTIES };
	bool xr_system_properties_initialized_ = false;

	const XrEventDataBaseHeader* read_next_event();
	void handle_sesssion_state_changed_event(const XrEventDataSessionStateChanged& stateChangedEvent, bool* exitRenderLoop, bool* requestRestart);
	void poll_events(bool* exitRenderLoop, bool* requestRestart);

	void log_layers_and_extensions();
	void log_instance_info();

	std::vector<std::string> GetPlatformInstanceExtensions() const;
	std::vector<std::string> GetGraphicsInstanceExtensions() const;

#if ENABLE_OPENXR_FB_REFRESH_RATE
	bool supports_refresh_rate_ = false;
	std::vector<float> supported_refresh_rates_;
	float current_refresh_rate_ = 0.0f;
	float max_refresh_rate_ = 0.0f;

	PFN_xrGetDisplayRefreshRateFB xrGetDisplayRefreshRateFB = nullptr;
	PFN_xrEnumerateDisplayRefreshRatesFB xrEnumerateDisplayRefreshRatesFB = nullptr;
	PFN_xrRequestDisplayRefreshRateFB xrRequestDisplayRefreshRateFB = nullptr;

	const std::vector<float>& GetSupportedRefreshRates() const;
	float GetCurrentRefreshRate();
	float GetMaxRefreshRate();
	bool IsRefreshRateSupported(const float refresh_rate);
	void SetRefreshRate(const float refresh_rate); // 0.0 = system default
#endif

#if ENABLE_OPENXR_FB_EYE_TRACKING_SOCIAL
	PFN_xrCreateEyeTrackerFB xrCreateEyeTrackerFB = nullptr;
	PFN_xrDestroyEyeTrackerFB xrDestroyEyeTrackerFB = nullptr;
	PFN_xrGetEyeGazesFB xrGetEyeGazesFB = nullptr;

	bool social_eye_tracking_supported_ = false;
	bool social_eye_tracking_enabled_ = false;
	XrEyeTrackerFB social_eye_tracker_ = nullptr;
	XrEyeGazesFB social_eye_gazes_{ XR_TYPE_EYE_GAZES_FB, nullptr };

	void UpdateSocialEyeTracker(const XrTime predicted_display_time);

	bool GetGazePoseSocial(const int eye, XrPosef& gaze_pose);
	void SetSocialEyeTrackerEnabled(const bool enabled);
	void CreateSocialEyeTracker();
	void DestroySocialEyeTracker();
	void UpdateSocialEyeTrackerGazes(const XrTime predicted_display_time);
#endif

#if ENABLE_EXT_EYE_TRACKING
	bool ext_eye_tracking_enabled_ = false;
	XrPosef ext_gaze_pose_;
	bool ext_gaze_pose_valid_ = false;

	XrEyeGazeSampleTimeEXT last_ext_gaze_pose_time_{ XR_TYPE_EYE_GAZE_SAMPLE_TIME_EXT };

	bool GetGazePoseEXT(XrPosef& gaze_pose);
	void CreateEXTEyeTracking();
	void DestroyEXTEeyeTracking();
	void SetEXTEyeTrackerEnabled(const bool enabled);
	void UpdateEXTEyeTrackerGaze(XrTime predicted_display_time);
#endif

#if ENABLE_WAIST_TRACKING
	Pose waist_pose_LS_ = {};
#endif

#if ENABLE_OPENXR_FB_BODY_TRACKING
	bool fb_body_tracking_supported_ = false;
	bool fb_body_tracking_enabled_ = true;

	PFN_xrCreateBodyTrackerFB xrCreateBodyTrackerFB = nullptr;
	PFN_xrDestroyBodyTrackerFB xrDestroyBodyTrackerFB = nullptr;
	PFN_xrLocateBodyJointsFB xrLocateBodyJointsFB = nullptr;

	XrBodyTrackerFB body_tracker_ = {};
	XrBodyJointLocationFB body_joints_[XR_BODY_JOINT_COUNT_FB] = {};
	XrBodyJointLocationsFB body_joint_locations_{ XR_TYPE_BODY_JOINT_LOCATIONS_FB, nullptr };

	void CreateFBBodyTracker();
	void DestroyFBBodyTracker();
	void UpdateFBBodyTrackerLocations(const XrTime& predicted_display_time);
#endif

#if ENABLE_OPENXR_META_BODY_TRACKING_FIDELITY
	PFN_xrRequestBodyTrackingFidelityMETA xrRequestBodyTrackingFidelityMETA = nullptr;

	XrBodyTrackingFidelityMETA current_fidelity_ = XR_BODY_TRACKING_FIDELITY_LOW_META;

	XrBodyTrackingFidelityStatusMETA body_tracker_fidelity_status_{ XR_TYPE_BODY_TRACKING_FIDELITY_STATUS_META };
	XrSystemPropertiesBodyTrackingFidelityMETA body_tracker_fidelity_properties_{ XR_TYPE_SYSTEM_PROPERTIES_BODY_TRACKING_FIDELITY_META };

	void RequestMetaFidelityBodyTracker(bool high_fidelity);
#endif

#if ENABLE_OPENXR_META_FULL_BODY_TRACKING
	XrBodyJointLocationFB full_body_joints_[XR_FULL_BODY_JOINT_COUNT_META] = {};
#endif

#if ENABLE_BODY_TRACKING
	void CreateBodyTracker()
	{
#if ENABLE_OPENXR_FB_BODY_TRACKING
		CreateFBBodyTracker();
#endif

#if ENABLE_OPENXR_META_BODY_TRACKING_FIDELITY
		RequestMetaFidelityBodyTracker(true);
#endif
	}

	void DestroyBodyTracker()
	{
#if ENABLE_OPENXR_META_BODY_TRACKING_FIDELITY
		RequestMetaFidelityBodyTracker(false);
#endif

#if ENABLE_OPENXR_FB_BODY_TRACKING
		DestroyFBBodyTracker();
#endif
	}
#endif

#if ENABLE_OPENXR_FB_SIMULTANEOUS_HANDS_AND_CONTROLLERS
	PFN_xrResumeSimultaneousHandsAndControllersTrackingMETA xrResumeSimultaneousHandsAndControllersTrackingMETA = nullptr;
	PFN_xrPauseSimultaneousHandsAndControllersTrackingMETA xrPauseSimultaneousHandsAndControllersTrackingMETA = nullptr;

	bool simultaneous_hands_and_controllers_enabled_ = false;

	bool AreSimultaneousHandsAndControllersSupported() const;
	bool AreSimultaneousHandsAndControllersEnabled() const;
	void SetSimultaneousHandsAndControllersEnabled(const bool enabled);
#endif

};

#endif // SUPPORT_OPENXR

#endif // OPENXR_INTERFACE_H