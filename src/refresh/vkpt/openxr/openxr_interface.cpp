//--------------------------------------------------------------------------------------
// Copyright (c) 2026 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

#include "openxr_interface.h"

#if SUPPORT_OPENXR

#include <algorithm>
#include "xr_linear.h"
#include <chrono>
#include <xlocale>

#include <iterator>

#ifndef XR_LOAD
#define XR_LOAD(instance, fn) xrGetInstanceProcAddr(instance, #fn, reinterpret_cast<PFN_xrVoidFunction*>(&fn))
#endif


#if defined(USE_GLSLANGVALIDATOR)
#define SPV_PREFIX {
#define SPV_SUFFIX }
#else
#define SPV_PREFIX
#define SPV_SUFFIX
#endif

#if 0
float4x4 tv_convert_xr_to_float4x4(const XrMatrix4x4f& input)
{
	float4x4 output = float4x4(&input.m[0]);
	return output;
}

XrMatrix4x4f tv_convert_to_xr(const float4x4& input)
{
	XrMatrix4x4f output;
	memcpy(&output, input.get_value(), sizeof(output));
	return output;
}

float3 tv_convert(const XrVector3f& input)
{
	float3 output;
	memcpy(&output, &input, sizeof(output));
	return output;
}

XrVector3f tv_convert_to_xr(const float3& input)
{
	XrVector3f output;
	memcpy(&output, &input, sizeof(output));
	return output;
}

Quat tv_convert(const XrQuaternionf& input)
{
	Quat output;
#if 1
	memcpy(&output, &input, sizeof(output));
#else
	output.val_.x = input.x;
	output.val_.y = input.y;
	output.val_.z = input.z;
	output.val_.w = input.w;
#endif
	return output;
}

XrQuaternionf tv_convert_to_xr(const Quat& input)
{
	XrQuaternionf output;
#if 1
	memcpy(&output, &input, sizeof(output));
#else
	output.x = input.val_.x;
	output.y = input.val_.y;
	output.z = input.val_.z;
	output.w = input.val_.w;
#endif
	return output;
}

Pose tv_convert_xr_pose_to_pose(const XrPosef& input)
{
	Pose output;
	output.position_ = tv_convert(input.position);
	output.rotation_ = tv_convert(input.orientation);
	output.is_valid_ = true;
	return output;
}

XRPose tv_convert(const XrPosef& input)
{
	XRPose output;
	output.position_ = tv_convert(input.position);
	output.rotation_ = tv_convert(input.orientation);
	return output;
}

XrPosef tv_convert_to_xr(const Pose& input)
{
	XrPosef output;
	output.position = tv_convert_to_xr(input.position_);
	output.orientation = tv_convert_to_xr(input.get_combined_rotation());
	return output;
}
#endif

constexpr bool is_xr_pose_valid(XrSpaceLocationFlags locationFlags)
{
	constexpr XrSpaceLocationFlags valid_mask = XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
	return (locationFlags & valid_mask) == valid_mask;
}


OpenXR::OpenXR() : xr_input_{ *this }
{
}

OpenXR::~OpenXR()
{
}

bool OpenXR::start_session()
{
	if(!is_initialized() || xr_session_ || is_session_running())
	{
		return false;
	}
	
	assert(xr_instance_ != XR_NULL_HANDLE);
	assert(xr_session_ == XR_NULL_HANDLE);

	{
		//Log::Write(Log::Level::Verbose, Fmt("Creating session..."));

		XrSessionCreateInfo createInfo{ XR_TYPE_SESSION_CREATE_INFO };
		createInfo.next = get_graphics_binding();
		createInfo.systemId = xr_system_id_;
		XrResult create_session_res = xrCreateSession(xr_instance_, &createInfo, &xr_session_);

		if (create_session_res != XR_SUCCESS)
		{
			assert(false);
			return false;
		}

#if ENABLE_OPENXR_FB_REFRESH_RATE
		GetMaxRefreshRate();
		SetRefreshRate(DEFAULT_HZ);
#endif
	}

	//LogReferenceSpaces();
	//CreateVisualizedSpaces();

	//XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = GetXrReferenceSpaceCreateInfo("View");
	//XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = GetXrReferenceSpaceCreateInfo("Stage");
	XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = GetXrReferenceSpaceCreateInfo("Local");
	XrResult create_appspace_res = xrCreateReferenceSpace(xr_session_, &referenceSpaceCreateInfo, &xr_app_space_);

	if(create_appspace_res != XR_SUCCESS)
	{
		assert(false);
		return false;
	}

	get_system_properties();

#if ENABLE_OPENXR_FB_EYE_TRACKING_SOCIAL
	CreateSocialEyeTracker();
#endif

#if ENABLE_OPENXR_FB_BODY_TRACKING
	CreateBodyTracker();
#endif

#if ENABLE_OPENXR_FB_SIMULTANEOUS_HANDS_AND_CONTROLLERS
	if(AreSimultaneousHandsAndControllersSupported())
	{
		SetSimultaneousHandsAndControllersEnabled(true);
	}
#endif

	create_swapchain();

	const bool actions_ok = xr_input_.init_actions();

	if(!actions_ok)
	{
		assert(false);
		shutdown();
		return false;
	}

	set_initialized(true);
	return true;
}

bool OpenXR::stop_session()
{
	if(!is_initialized() || !xr_session_)// !is_session_running())
	{
		return false;
	}

	XrResult stop_res = xrRequestExitSession(xr_session_);

	if(xr_session_ != XR_NULL_HANDLE)
	{
		xrDestroySession(xr_session_);
		xr_session_ = XR_NULL_HANDLE;
	}

	return (stop_res == XR_SUCCESS);
}

bool OpenXR::is_session_running() const
{
	return is_xr_session_running_;
}

bool OpenXR::shutdown()
{
	xr_input_.shutdown();

	stop_session();

#if ENABLE_OPENXR_FB_BODY_TRACKING
	DestroyBodyTracker();
#endif

#if ENABLE_OPENXR_FB_EYE_TRACKING_SOCIAL
	DestroySocialEyeTracker();
#endif

#if ENABLE_EXT_EYE_TRACKING
	DestroyEXTEeyeTracking();
#endif

	destroy_swapchain();

	//for(XrSpace visualizedSpace : m_visualizedSpaces)
	{
		//xrDestroySpace(visualizedSpace);
	}

	if(xr_app_space_ != XR_NULL_HANDLE)
	{
		//xrDestroySpace(xr_app_space_);
		xr_app_space_ = XR_NULL_HANDLE;
	}

	destroy_instance();

	for (uint xr_command_buffer_index = 0; xr_command_buffer_index < NUM_COMMAND_BUFFERS; xr_command_buffer_index++)
	{
		xr_command_buffers_[xr_command_buffer_index].Destroy();
	}

	set_initialized(false);

	return true;
}

XrTime OpenXR::get_predicted_display_time() const
{
	XrTime xr_time = xr_frame_state_.predictedDisplayTime;

	if (override_predicted_display_time_)
	{
		xr_time += (display_time_offset_us_ * 1000);
	}

	return xr_time;
}

bool OpenXR::update()
{
	if(!is_initialized())
	{
		return false;
	}

	bool request_restart = false;
	bool exit_render_loop = false;

	poll_events(&exit_render_loop, &request_restart);

	if(exit_render_loop)
	{
		shutdown();
		return false;
	}

	if(is_session_running())
	{
		if(!xr_input_.is_initialized())
		{
			const bool inputs_ok = xr_input_.init();

			if(!inputs_ok)
			{
				assert(false);
				shutdown();
				return false;
			}
		}

		assert(xr_input_.is_initialized());

		wait_frame();
		begin_frame();

		xr_input_.update();

		XrTime predicted_display_time = get_predicted_display_time();
		update_views(predicted_display_time);
		update_controller_poses(predicted_display_time);

#if ENABLE_OPENXR_FB_EYE_TRACKING_SOCIAL
		UpdateSocialEyeTracker(predicted_display_time);
#endif

#if ENABLE_OPENXR_FB_BODY_TRACKING
		UpdateFBBodyTracking(predicted_display_time);
#endif

#if USE_RECENTERING
		if(recenter_event_occurred_)
		{
			engine_.get_vr_manager().trigger_recentering();
			recenter_event_occurred_ = false;
		}
#endif
	}
	
	if(request_restart)
	{
		stop_session();
		start_session();
	}

	return true;
}

inline bool EqualsIgnoreCase(const std::string& s1, const std::string& s2, const std::locale& loc = std::locale())
{
	const std::ctype<char>& ctype = std::use_facet<std::ctype<char>>(loc);
	const auto compareCharLower = [&](char c1, char c2)
	{
		return ctype.tolower(c1) == ctype.tolower(c2);
	};
	return s1.size() == s2.size() && std::equal(s1.begin(), s1.end(), s2.begin(), compareCharLower);
}


XrReferenceSpaceCreateInfo OpenXR::GetXrReferenceSpaceCreateInfo(const std::string& referenceSpaceTypeStr) 
{
	XrReferenceSpaceCreateInfo referenceSpaceCreateInfo{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };

	XrPosef xr_reference_pose = {};
	xr_reference_pose.orientation.w = 1.0f;
	referenceSpaceCreateInfo.poseInReferenceSpace = xr_reference_pose;
	referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;

#if 0
	Pose reference_pose;
	XrPosef xr_reference_pose = tv_convert_to_xr(reference_pose);
	referenceSpaceCreateInfo.poseInReferenceSpace = xr_reference_pose;

	if(EqualsIgnoreCase(referenceSpaceTypeStr, "View")) 
	{
		referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
	}
	else if(EqualsIgnoreCase(referenceSpaceTypeStr, "ViewFront")) 
	{
		// Render head-locked 2m in front of device.
		reference_pose.position_ = float3(0.0f, 0.0f, -2.0f);
		xr_reference_pose = tv_convert_to_xr(reference_pose);
		referenceSpaceCreateInfo.poseInReferenceSpace = xr_reference_pose;
		referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
	}
	else if(EqualsIgnoreCase(referenceSpaceTypeStr, "Local")) 
	{
		referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
	}
	else if(EqualsIgnoreCase(referenceSpaceTypeStr, "Stage")) 
	{
		referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
	}
	else 
	{
		assert(false);
	}
#endif
	return referenceSpaceCreateInfo;
}

std::vector<XrSwapchainImageBaseHeader*> OpenXR::AllocateSwapchainImageStructs(uint32_t capacity, const XrSwapchainCreateInfo& swapchainCreateInfo)
{
	m_swapchainImageContexts.push_back(XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR);

	XRSwapchainImageContext& swapchainImageContext = m_swapchainImageContexts.back();

	std::vector<XrSwapchainImageBaseHeader*> bases = swapchainImageContext.Create(vk_logical_device_, &xr_memory_allocator_, capacity, swapchainCreateInfo);

	for(auto& base : bases) 
	{
		m_swapchainImageContextMap[base] = &swapchainImageContext;
	}

	return bases;
}


const XrEventDataBaseHeader* OpenXR::read_next_event()
{
	XrEventDataBaseHeader* baseHeader = reinterpret_cast<XrEventDataBaseHeader*>(&xr_event_data_buffer_);
	*baseHeader = { XR_TYPE_EVENT_DATA_BUFFER };

	const XrResult xr = xrPollEvent(xr_instance_, &xr_event_data_buffer_);

	if(xr == XR_SUCCESS)
	{
		if(baseHeader->type == XR_TYPE_EVENT_DATA_EVENTS_LOST)
		{
			const XrEventDataEventsLost* const eventsLost = reinterpret_cast<const XrEventDataEventsLost*>(baseHeader);
			//Log::Write(Log::Level::Warning, Fmt("%d events lost", eventsLost));
		}

		return baseHeader;
	}

	if(xr == XR_EVENT_UNAVAILABLE)
	{
		return nullptr;
	}
	//THROW_XR(xr, "xrPollEvent");

	return nullptr;
}

void OpenXR::handle_sesssion_state_changed_event(const XrEventDataSessionStateChanged& stateChangedEvent, bool* exitRenderLoop, bool* requestRestart)
{
	const XrSessionState oldState = xr_session_state_;
	xr_session_state_ = stateChangedEvent.state;

	//Log::Write(Log::Level::Info, Fmt("XrEventDataSessionStateChanged: state %s->%s session=%lld time=%lld", to_string(oldState),
	//to_string(xr_session_state_), stateChangedEvent.session, stateChangedEvent.time));

	if((stateChangedEvent.session != XR_NULL_HANDLE) && (stateChangedEvent.session != xr_session_))
	{
		//Log::Write(Log::Level::Error, "XrEventDataSessionStateChanged for unknown session");
		return;
	}

	switch(xr_session_state_) 
	{
	case XR_SESSION_STATE_READY: 
	{
		assert(xr_session_ != XR_NULL_HANDLE);
		XrSessionBeginInfo sessionBeginInfo{ XR_TYPE_SESSION_BEGIN_INFO };
		sessionBeginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

		XrResult begin_res = xrBeginSession(xr_session_, &sessionBeginInfo);

		if (begin_res == XR_SUCCESS)
		{
			is_xr_session_running_ = true;
		}
		else
		{
			is_xr_session_running_ = false;
		}
		
		break;
	}
	case XR_SESSION_STATE_STOPPING: 
	{
		assert(xr_session_ != XR_NULL_HANDLE);
		is_xr_session_running_ = false;

		xrEndSession(xr_session_);
		break;
	}
	case XR_SESSION_STATE_EXITING: 
	{
		*exitRenderLoop = true;
		// Do not attempt to restart because user closed this session.
		*requestRestart = false;
		break;
	}
	case XR_SESSION_STATE_LOSS_PENDING: 
	{
		*exitRenderLoop = true;
		// Poll for a new instance.
		*requestRestart = true;
		break;
	}
	default:
		break;
	}
}


void OpenXR::poll_events(bool* exitRenderLoop, bool* requestRestart)
{
	*exitRenderLoop = *requestRestart = false;

	// Process all pending messages.
	while(const XrEventDataBaseHeader* event = read_next_event())
	{
		switch(event->type) 
		{
		case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: 
		{
			const auto& instanceLossPending = *reinterpret_cast<const XrEventDataInstanceLossPending*>(event);
			//Log::Write(Log::Level::Warning, Fmt("XrEventDataInstanceLossPending by %lld", instanceLossPending.lossTime));
			*exitRenderLoop = true;
			*requestRestart = true;
			return;
		}
		case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: 
		{
			auto sessionStateChangedEvent = *reinterpret_cast<const XrEventDataSessionStateChanged*>(event);
			handle_sesssion_state_changed_event(sessionStateChangedEvent, exitRenderLoop, requestRestart);
			break;
		}
		case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
			break;
		case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
#if USE_RECENTERING
			recenter_event_occurred_ = true;
#endif
		default: 
		{
			//Log::Write(Log::Level::Verbose, Fmt("Ignoring event type %d", event->type));
			break;
		}
		}
	}
}

bool OpenXR::wait_frame()
{
	if(!is_initialized() || !is_session_running())
	{
		return false;
	}

	XrFrameWaitInfo xr_frame_wait_info{ XR_TYPE_FRAME_WAIT_INFO };
	XrResult res = xrWaitFrame(xr_session_, &xr_frame_wait_info, &xr_frame_state_);

	if (res != VK_SUCCESS)
	{
		return false;
	}

	return true;
}

bool OpenXR::begin_frame()
{
	if(!is_initialized() || !is_session_running())
	{
		return false;
	}

	XrFrameBeginInfo xr_frame_begin_info{ XR_TYPE_FRAME_BEGIN_INFO };
	XrResult res = xrBeginFrame(xr_session_, &xr_frame_begin_info);

	if(res != VK_SUCCESS)
	{
		return false;
	}

	return true;
}

bool OpenXR::end_frame(VkCommandBuffer* external_command_buffer)
{
	if(!is_initialized() || !is_session_running())
	{
		return false;
	}

	if(!xr_frame_state_.shouldRender)
	{
		return true;
	}

	std::vector<XrCompositionLayerBaseHeader*> composition_layers;
	XrCompositionLayerProjection composition_layer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };

	std::vector<XrCompositionLayerProjectionView> projection_layer_views;
	
	if(render_composition_layer(projection_layer_views, composition_layer, external_command_buffer))
	{
		XrCompositionLayerBaseHeader* header = reinterpret_cast<XrCompositionLayerBaseHeader*>(&composition_layer);
		composition_layers.push_back(header);
	}

	XrFrameEndInfo frame_end_info{ XR_TYPE_FRAME_END_INFO };
	frame_end_info.displayTime = get_predicted_display_time();
	frame_end_info.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
	frame_end_info.layerCount = (uint32_t)composition_layers.size();
	frame_end_info.layers = composition_layers.data();

	xrEndFrame(xr_session_, &frame_end_info);

	return true;
}

bool OpenXR::render_composition_layer(std::vector<XrCompositionLayerProjectionView>& projection_layer_views, XrCompositionLayerProjection& composition_layer, VkCommandBuffer* external_command_buffer)
{
	if(!is_initialized() || !is_session_running())
	{
		return false;
	}

	projection_layer_views.resize(NUM_EYES);

	for(uint32_t view_id = 0; view_id < NUM_EYES; view_id++)
	{
		const XRSwapchain viewSwapchain = xr_swapchains_[view_id];

		XrSwapchainImageAcquireInfo acquireInfo{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };

		uint32_t swapchainImageIndex = 0;
		xrAcquireSwapchainImage(viewSwapchain.handle, &acquireInfo, &swapchainImageIndex);

		XrSwapchainImageWaitInfo waitInfo{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
		waitInfo.timeout = XR_INFINITE_DURATION;
		xrWaitSwapchainImage(viewSwapchain.handle, &waitInfo);

		const XrSwapchainImageBaseHeader* const swapchainImage = xr_swapchain_images_[viewSwapchain.handle][swapchainImageIndex];

		projection_layer_views[view_id] = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
		projection_layer_views[view_id].pose = xr_views_[view_id].pose;
		projection_layer_views[view_id].fov = xr_views_[view_id].fov;
		projection_layer_views[view_id].subImage.swapchain = viewSwapchain.handle;
		projection_layer_views[view_id].subImage.imageRect.offset = { 0, 0 };
		projection_layer_views[view_id].subImage.imageRect.extent = { viewSwapchain.width, viewSwapchain.height };

		if(supports_depth_layer_)
		{
#if 0
			projection_layer_views[view_id].next = &depthInfos[view_id];

			depthInfos[view_id].type = XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR;
			depthInfos[view_id].subImage.swapchain = m_depthSwapchains[view_id].handle;
			depthInfos[view_id].subImage.imageRect.offset = { 0, 0 };
			depthInfos[view_id].subImage.imageRect.extent = { viewSwapchain.width, viewSwapchain.height };
			depthInfos[view_id].minDepth = 0;
			depthInfos[view_id].maxDepth = 1;
			depthInfos[view_id].nearZ = DEFAULT_NEAR_Z;
			depthInfos[view_id].farZ = DEFAULT_FAR_Z;
#endif
		}

		render_projection_layer_view(projection_layer_views[view_id], swapchainImage, xr_colour_swapchain_format_, view_id, external_command_buffer);

		XrSwapchainImageReleaseInfo releaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
		xrReleaseSwapchainImage(viewSwapchain.handle, &releaseInfo);

		//m_swapchainImages[viewSwapchain.handle]->ReleaseDepthSwapchainImage();
	}

	composition_layer.space = xr_app_space_;

	composition_layer.layerFlags = 0;/// (EnvironmentBlendMode == XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND) ? XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT : 0;
	composition_layer.viewCount = (uint32_t)projection_layer_views.size();
	composition_layer.views = projection_layer_views.data();

	return true;
}


extern "C" VkResult vkpt_simple_vr_blit(VkCommandBuffer cmd_buf, unsigned int image_index, bool filtered, bool warped, int view_id);

void OpenXR::render_projection_layer_view(const XrCompositionLayerProjectionView& projection_layer_view, const XrSwapchainImageBaseHeader* swapchain_image, int64_t swapchain_format, int view_id, VkCommandBuffer* external_command_buffer)
{
	if (!render_into_xr_swapchain_)
	{
		return;
	}

	assert(projection_layer_view.subImage.imageArrayIndex == 0);

	XRSwapchainImageContext* swapchain_context = m_swapchainImageContextMap[swapchain_image];
	uint32_t image_index = swapchain_context->ImageIndex(swapchain_image);

	VkCommandBuffer command_buffer = {};

	if(external_command_buffer)
	{
		command_buffer = *external_command_buffer;
	}
	else
	{
		CommandBuffer& xr_command_buffer = xr_command_buffers_[xr_command_buffer_index_];

		xr_command_buffer.Wait();
		xr_command_buffer.Reset();
		xr_command_buffer.Begin();

		command_buffer = xr_command_buffer.vk_command_buffer_;
	}

	swapchain_context->depthBuffer.TransitionLayout(command_buffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	static std::array<VkClearValue, 2> clearValues;
	clearValues[0].color.float32[0] = 0.0f;
	clearValues[0].color.float32[1] = 1.0f;
	clearValues[0].color.float32[2] = 0.0f;
	clearValues[0].color.float32[3] = 1.0f;
	clearValues[1].depthStencil.depth = 1.0f;
	clearValues[1].depthStencil.stencil = 0;
	
	VkExtent2D extent = VkExtent2D{ (uint32_t)projection_layer_view.subImage.imageRect.extent.width, (uint32_t)projection_layer_view.subImage.imageRect.extent.height };
	VkOffset2D offset{ 0, 0 };

	VkImageView swapchain_colour_view = swapchain_context->get_colour_view(image_index);
	assert(swapchain_colour_view);

	VkRenderingAttachmentInfoKHR swapchain_colour_attachment_info{};
	swapchain_colour_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	swapchain_colour_attachment_info.imageView = swapchain_colour_view;
	swapchain_colour_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	swapchain_colour_attachment_info.resolveMode = VK_RESOLVE_MODE_NONE;
	swapchain_colour_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	swapchain_colour_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	swapchain_colour_attachment_info.clearValue = clearValues[0];
	swapchain_colour_attachment_info.pNext = nullptr;

	VkRenderingInfoKHR rendering_info = {};
	rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	rendering_info.pNext = VK_NULL_HANDLE;
	rendering_info.layerCount = 0;
	rendering_info.viewMask = 0;
	rendering_info.flags = 0;
	rendering_info.layerCount = 1;
	rendering_info.viewMask = 0;
	rendering_info.colorAttachmentCount = 1;
	rendering_info.pColorAttachments = &swapchain_colour_attachment_info;

#if PASS_DEPTH_TO_OPENXR
	VkImageView swapchain_depth_view = swapchain_context->get_depth_view(image_index);
	assert(swapchain_depth_view);

	VkRenderingAttachmentInfoKHR depth_attachment_info{};
	depth_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	depth_attachment_info.imageView = swapchain_depth_view;
	depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	depth_attachment_info.resolveMode = VK_RESOLVE_MODE_NONE;
	depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment_info.clearValue = clearValues[1];
	depth_attachment_info.pNext = VK_NULL_HANDLE;

	rendering_info.pDepthAttachment = &depth_attachment_info;
	rendering_info.pStencilAttachment = &depth_attachment_info;
#endif

	assert(swapchain_context->size.width == extent.width);
	assert(swapchain_context->size.height == extent.height);

	rendering_info.renderArea.extent = extent;
	rendering_info.renderArea.offset = offset;

	vkCmdBeginRendering(command_buffer, &rendering_info);

	//VkViewport viewport{ 0, 0, (float)projection_layer_view.subImage.imageRect.extent.width, (float)projection_layer_view.subImage.imageRect.extent.height, 0, 10000 };
	//vkCmdSetViewport(command_buffer, 0, 1, &viewport);
	
	//const VkRect2D rect = { offset, extent };
	//vkCmdSetScissor(command_buffer, 0, 1, &rect);

	int image = 24; // VKPT_IMG_TAA_OUTPUT
	VkResult draw_res = vkpt_simple_vr_blit(command_buffer, image, false, false, view_id);

	vkCmdEndRendering(command_buffer);

	if(!external_command_buffer)
	{
		CommandBuffer& xr_command_buffer = xr_command_buffers_[xr_command_buffer_index_];
		xr_command_buffer.End();
		//xr_command_buffer.Exec(engine_.get_device().get_vk_graphics_queue());
		xr_command_buffer_index_ = (xr_command_buffer_index_ + 1) % NUM_COMMAND_BUFFERS;
	}
}

std::vector<const char*> ParseExtensionString(char* names) 
{
	std::vector<const char*> list;

	while(*names != 0) 
	{
		list.push_back(names);

		while(*(++names) != 0) 
		{
			if(*names == ' ') 
			{
				*names++ = '\0';
				break;
			}
		}
	}
	return list;
}


static void LogVulkanExtensions(const std::string title, const std::vector<const char*>& extensions, unsigned int start = 0) 
{
	const std::string indentStr(1, ' ');

	//Log::Write(Log::Level::Verbose, Fmt("%s: (%d)", title.c_str(), extensions.size() - start));

	for(auto ext : extensions) 
	{
		if(start) 
		{
			start--;
			continue;
		}
		//Log::Write(Log::Level::Verbose, Fmt("%s  Name=%s", indentStr.c_str(), ext));
	}
}

XrResult OpenXR::CreateVulkanInstanceKHR(XrInstance instance, const XrVulkanInstanceCreateInfoKHR* createInfo,	VkInstance* vulkanInstance, VkResult* vulkanResult) 
{
	PFN_xrGetVulkanInstanceExtensionsKHR pfnGetVulkanInstanceExtensionsKHR = nullptr;
	xrGetInstanceProcAddr(instance, "xrGetVulkanInstanceExtensionsKHR",	reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetVulkanInstanceExtensionsKHR));

	uint32_t extensionNamesSize = 0;
	pfnGetVulkanInstanceExtensionsKHR(instance, createInfo->systemId, 0, &extensionNamesSize, nullptr);

	std::vector<char> extensionNames(extensionNamesSize);
	pfnGetVulkanInstanceExtensionsKHR(instance, createInfo->systemId, extensionNamesSize, &extensionNamesSize,	&extensionNames[0]);
	{
		// Note: This cannot outlive the extensionNames above, since it's just a collection of views into that string!
		std::vector<const char*> extensions = ParseExtensionString(&extensionNames[0]);
		LogVulkanExtensions("Vulkan Instance Extensions, requested by runtime", extensions);

		// Merge the runtime's request with the applications requests
		for(uint32_t i = 0; i < createInfo->vulkanCreateInfo->enabledExtensionCount; ++i)
		{
			extensions.push_back(createInfo->vulkanCreateInfo->ppEnabledExtensionNames[i]);
		}
		LogVulkanExtensions("Vulkan Instance Extensions, requested by application", extensions,	(uint32_t)extensions.size() - createInfo->vulkanCreateInfo->enabledExtensionCount);

		VkInstanceCreateInfo instInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		memcpy(&instInfo, createInfo->vulkanCreateInfo, sizeof(instInfo));
		instInfo.enabledExtensionCount = (uint32_t)extensions.size();
		instInfo.ppEnabledExtensionNames = extensions.empty() ? nullptr : extensions.data();

		auto pfnCreateInstance = (PFN_vkCreateInstance)createInfo->pfnGetInstanceProcAddr(nullptr, "vkCreateInstance");
		*vulkanResult = pfnCreateInstance(&instInfo, createInfo->vulkanAllocator, vulkanInstance);
	}

	return XR_SUCCESS;
}

#if 1
XrResult OpenXR::CreateVulkanDeviceKHR(XrInstance instance, const XrVulkanDeviceCreateInfoKHR* createInfo, VkDevice* vulkanDevice, VkResult* vulkanResult)
{
	PFN_xrGetVulkanDeviceExtensionsKHR pfnGetVulkanDeviceExtensionsKHR = nullptr;
	xrGetInstanceProcAddr(instance, "xrGetVulkanDeviceExtensionsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetVulkanDeviceExtensionsKHR));

	uint32_t deviceExtensionNamesSize = 0;
	pfnGetVulkanDeviceExtensionsKHR(instance, createInfo->systemId, 0, &deviceExtensionNamesSize, nullptr);

	std::vector<char> deviceExtensionNames(deviceExtensionNamesSize);
	pfnGetVulkanDeviceExtensionsKHR(instance, createInfo->systemId, deviceExtensionNamesSize, &deviceExtensionNamesSize, &deviceExtensionNames[0]);

	{
		// Note: This cannot outlive the extensionNames above, since it's just a collection of views into that string!
		std::vector<const char*> extensions = ParseExtensionString(&deviceExtensionNames[0]);

		// Merge the runtime's request with the applications requests
		for(uint32_t i = 0; i < createInfo->vulkanCreateInfo->enabledExtensionCount; ++i)
		{
			extensions.push_back(createInfo->vulkanCreateInfo->ppEnabledExtensionNames[i]);
		}

		VkDeviceCreateInfo deviceInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		memcpy(&deviceInfo, createInfo->vulkanCreateInfo, sizeof(deviceInfo));

		VkPhysicalDeviceFeatures features{};

		features.shaderStorageImageMultisample = VK_TRUE;

		if(!createInfo->vulkanCreateInfo->pNext)
		{
			deviceInfo.pEnabledFeatures = &features;
		}

		deviceInfo.enabledExtensionCount = (uint32_t)extensions.size();
		deviceInfo.ppEnabledExtensionNames = extensions.empty() ? nullptr : extensions.data();

		assert(vk_instance_);
		assert(vk_physical_device_);

		auto pfnCreateDevice = (PFN_vkCreateDevice)createInfo->pfnGetInstanceProcAddr(vk_instance_, "vkCreateDevice");
		*vulkanResult = pfnCreateDevice(vk_physical_device_, &deviceInfo, createInfo->vulkanAllocator, vulkanDevice);
	}

	return XR_SUCCESS;
}
#else

XrResult OpenXR::CreateVulkanDeviceKHR(XrInstance instance, const XrVulkanDeviceCreateInfoKHR* createInfo, VkDevice* vulkanDevice, VkResult* vulkanResult)
{
	PFN_xrCreateVulkanDeviceKHR pfnCreateVulkanDeviceKHR = nullptr;
	xrGetInstanceProcAddr(instance, "xrCreateVulkanDeviceKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnCreateVulkanDeviceKHR));

	return pfnCreateVulkanDeviceKHR(instance, createInfo, vulkanDevice, vulkanResult);
}
#endif

XrResult OpenXR::GetVulkanGraphicsDevice2KHR(XrInstance instance, const XrVulkanGraphicsDeviceGetInfoKHR* getInfo,
	VkPhysicalDevice* vulkanPhysicalDevice) 
{
	PFN_xrGetVulkanGraphicsDeviceKHR pfnGetVulkanGraphicsDeviceKHR = nullptr;
	xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsDeviceKHR",	reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetVulkanGraphicsDeviceKHR));

	if(getInfo->next != nullptr) 
	{
		return XR_ERROR_FEATURE_UNSUPPORTED;
	}

	pfnGetVulkanGraphicsDeviceKHR(instance, getInfo->systemId, getInfo->vulkanInstance, vulkanPhysicalDevice);
	return XR_SUCCESS;
}

XrResult OpenXR::GetVulkanGraphicsRequirements2KHR(XrInstance instance, XrSystemId systemId,
	XrGraphicsRequirementsVulkan2KHR* graphicsRequirements) 
{
	PFN_xrGetVulkanGraphicsRequirementsKHR pfnGetVulkanGraphicsRequirementsKHR = nullptr;
	xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsRequirementsKHR",	reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetVulkanGraphicsRequirementsKHR));

	XrGraphicsRequirementsVulkanKHR legacyRequirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR };
	pfnGetVulkanGraphicsRequirementsKHR(instance, systemId, &legacyRequirements);

	graphicsRequirements->maxApiVersionSupported = legacyRequirements.maxApiVersionSupported;
	graphicsRequirements->minApiVersionSupported = legacyRequirements.minApiVersionSupported;

	return XR_SUCCESS;
}

bool OpenXR::create_instance(const VkInstanceCreateInfo& instance_create_info)
{
	assert(xr_instance_ == XR_NULL_HANDLE);

	std::vector<const char*> xr_instance_extensions;

#if 1

#if ENABLE_OPENXR_HAND_TRACKING
	if(supports_hand_tracking_)
	{
		//Log::Write(Log::Level::Info, "Hand Tracking is supported");
		xr_instance_extensions.push_back(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
	}
	else
	{
		//Log::Write(Log::Level::Info, "Hand Tracking is NOT supported");
	}
#endif

#if ENABLE_OPENXR_FB_EYE_TRACKING_SOCIAL
	if(supports_eye_tracking_social_)
	{
		//Log::Write(Log::Level::Info, "Eye Tracking is supported (Quest Pro)");
		xr_instance_extensions.push_back(XR_FB_EYE_TRACKING_SOCIAL_EXTENSION_NAME);
	}
	else
	{
		//Log::Write(Log::Level::Info, "Eye Tracking is NOT supported");
	}
#endif

#if ENABLE_EXT_EYE_TRACKING
	if(supports_ext_eye_tracking_)
	{
		//Log::Write(Log::Level::Info, "EXT Eye Tracking is supported");
		xr_instance_extensions.push_back(XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME);
	}
	else
	{
		//Log::Write(Log::Level::Info, "EXT Eye Tracking is NOT supported");
	}
#endif

#if ENABLE_OPENXR_FB_FACE_TRACKING
	if(supports_face_tracking_)
	{
		//Log::Write(Log::Level::Info, "Face Tracking is supported");
		xr_instance_extensions.push_back(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
	}
	else
	{
		//Log::Write(Log::Level::Info, "Face Tracking is NOT supported");
	}
#endif

#if ENABLE_OPENXR_FB_BODY_TRACKING
	if(supports_fb_body_tracking_)
	{
		//Log::Write(Log::Level::Info, "FB Meta Body Tracking is supported");
		xr_instance_extensions.push_back(XR_FB_BODY_TRACKING_EXTENSION_NAME);
	}
	else
	{
		//Log::Write(Log::Level::Info, "FB Meta Body Tracking is NOT supported");
	}
#endif

#if ENABLE_OPENXR_META_BODY_TRACKING_FIDELITY
	if(supports_meta_body_tracking_fidelity_)
	{
		//Log::Write(Log::Level::Info, "XR_META_body_tracking_fidelity is supported");
		xr_instance_extensions.push_back(XR_META_BODY_TRACKING_FIDELITY_EXTENSION_NAME);
	}
	else
	{
		//Log::Write(Log::Level::Info, "XR_META_body_tracking_fidelity is NOT supported");
	}
#endif

#if ENABLE_OPENXR_META_FULL_BODY_TRACKING
	if(supports_meta_full_body_tracking_)
	{
		//Log::Write(Log::Level::Info, "XR_META_body_tracking_full_body is supported");
		xr_instance_extensions.push_back(XR_META_BODY_TRACKING_FULL_BODY_EXTENSION_NAME);
	}
	else
	{
		//Log::Write(Log::Level::Info, "XR_META_body_tracking_full_body is NOT supported");
	}
#endif

#if ENABLE_OPENXR_FB_SIMULTANEOUS_HANDS_AND_CONTROLLERS
	if(supports_simultaneous_hands_and_controllers_)
	{
		//Log::Write(Log::Level::Info, "Simultaneous hands and controllers are supported");
		xr_instance_extensions.push_back(XR_META_SIMULTANEOUS_HANDS_AND_CONTROLLERS_EXTENSION_NAME);
	}
	else
	{
		//Log::Write(Log::Level::Info, "Simultaneous hands and controllers are NOT supported");
	}
#endif

#if ENABLE_VIVE_TRACKERS
	if(supports_HTCX_vive_tracker_interaction_)
	{
		//Log::Write(Log::Level::Info, "Vive trackers are supported");
		xr_instance_extensions.push_back(XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
	}
	else
	{
		//Log::Write(Log::Level::Info, "Vive trackers are NOT supported");
	}
#endif

#endif

	const std::vector<std::string> platformExtensions = GetPlatformInstanceExtensions();
	std::transform(platformExtensions.begin(), platformExtensions.end(), std::back_inserter(xr_instance_extensions),[](const std::string& ext) { return ext.c_str(); });

	const std::vector<std::string> graphicsExtensions = GetGraphicsInstanceExtensions();

	std::transform(graphicsExtensions.begin(), graphicsExtensions.end(), std::back_inserter(xr_instance_extensions), [](const std::string& ext) { return ext.c_str(); });

	uint32_t instanceExtensionCount = 0;
	xrEnumerateInstanceExtensionProperties(nullptr, 0, &instanceExtensionCount, nullptr);
	std::vector<XrExtensionProperties> extensionProperties = std::vector<XrExtensionProperties>(instanceExtensionCount, { XR_TYPE_EXTENSION_PROPERTIES });

	xrEnumerateInstanceExtensionProperties(nullptr, (uint32_t)extensionProperties.size(), &instanceExtensionCount,	extensionProperties.data());

	// enable depth extension if supported
	auto depthExtensionProperties =	std::find_if(extensionProperties.begin(), extensionProperties.end(), [](const XrExtensionProperties& item) 
	{
		return 0 == strcmp(item.extensionName, XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME);
	});

	if(depthExtensionProperties != extensionProperties.end()) 
	{
		//Log::Write(Log::Level::Info, Fmt("Depth submission supported (%s)", depthExtensionProperties->extensionName));
		//extensions.push_back(depthExtensionProperties->extensionName);
		//supports_depth_layer_ = true;
	}
	else 
	{
		//Log::Write(Log::Level::Info, Fmt("Depth submission NOT supported (%s)", depthExtensionProperties->extensionName));
	}

	XrInstanceCreateInfo xr_instance_create_info{ XR_TYPE_INSTANCE_CREATE_INFO };
	xr_instance_create_info.next = nullptr;// GetInstanceCreateExtension();
	xr_instance_create_info.enabledExtensionCount = (uint32_t)xr_instance_extensions.size();
	xr_instance_create_info.enabledExtensionNames = xr_instance_extensions.data();

	strcpy(xr_instance_create_info.applicationInfo.applicationName, "Q2RTX_VRMOD");

	xr_instance_create_info.applicationInfo.apiVersion = XR_API_VERSION_1_0;
	
//	xr_instance_create_info.applicationInfo.apiVersion = XR_MAKE_VERSION(XR_VERSION_MAJOR(XR_CURRENT_API_VERSION), 0, XR_VERSION_PATCH(XR_CURRENT_API_VERSION));

	XrResult create_instance_result = xrCreateInstance(&xr_instance_create_info, &xr_instance_);

	if(create_instance_result != XR_SUCCESS)
	{
		return false;
	}

	assert(xr_instance_);

	log_instance_info();

	const bool system_ok = init_system();

	if(!system_ok)
	{
		return false;
	}

	XrResult xr_graphics_result = GetVulkanGraphicsRequirements2KHR(xr_instance_, xr_system_id_, &xr_graphics_requirements_);

	if(xr_graphics_result != XR_SUCCESS)
	{
		assert(false);
		return false;
	}

	XrVulkanInstanceCreateInfoKHR xr_create_info{ XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR };
	xr_create_info.systemId = xr_system_id_;
	xr_create_info.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
	xr_create_info.vulkanCreateInfo = &instance_create_info;
	xr_create_info.vulkanAllocator = nullptr;

	VkResult err = VK_SUCCESS;
	XrResult create_vk_instance_result = CreateVulkanInstanceKHR(xr_instance_, &xr_create_info, &vk_instance_, &err);

	if(create_vk_instance_result != XR_SUCCESS)
	{
		assert(false);
		return false;
	}

	assert(vk_instance_);

	return true;
}

void OpenXR::destroy_instance()
{
	if(xr_instance_ != XR_NULL_HANDLE)
	{
		xrDestroyInstance(xr_instance_);
		xr_instance_ = XR_NULL_HANDLE;
	}
}

bool OpenXR::init_system()
{
	if(!xr_instance_)
	{
		assert(false);
		return false;
	}

	assert(xr_system_id_ == XR_NULL_SYSTEM_ID);

	XrSystemGetInfo systemInfo{ XR_TYPE_SYSTEM_GET_INFO };
	XrFormFactor FormFactor{XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY};

	systemInfo.formFactor = FormFactor;

	XrResult xr_system_result = xrGetSystem(xr_instance_, &systemInfo, &xr_system_id_);

	if(xr_system_result != XR_SUCCESS)
	{
		assert(false);
		return false;
	}

	//Log::Write(Log::Level::Verbose, Fmt("Using system %d for form factor %s", xr_system_id_, to_string(FormFactor)));
	assert(xr_system_id_ != XR_NULL_SYSTEM_ID);

	return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugMessageThunk(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,	VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	return true;// static_cast<OpenXR*>(pUserData)->debugMessage(messageSeverity, messageTypes, pCallbackData);
}


bool OpenXR::create_device(const VkDeviceCreateInfo& device_create_info)
{
	if(!xr_instance_)
	{
		assert(false);
		return false;
	}

	if(!vk_physical_device_)
	{
		XrVulkanGraphicsDeviceGetInfoKHR device_get_info{ XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR };
		device_get_info.systemId = xr_system_id_;
		device_get_info.vulkanInstance = vk_instance_;
		GetVulkanGraphicsDevice2KHR(xr_instance_, &device_get_info, &vk_physical_device_);
		assert(vk_physical_device_);
	}

	if(m_queueFamilyIndex == INVALID_INDEX)
	{
		VkDeviceQueueCreateInfo queue_info{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		float queuePriorities = 0;
		queue_info.queueCount = 1;
		queue_info.pQueuePriorities = &queuePriorities;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device_, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device_, &queueFamilyCount, &queueFamilyProps[0]);

		std::vector<VkDeviceQueueCreateInfo> queue_infos;

		for(uint32_t i = 0; i < queueFamilyCount; ++i)
		{
			if((queueFamilyProps[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT)) == (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT))
			{
				queue_info.queueFamilyIndex = i;
				m_queueFamilyIndex = queue_info.queueFamilyIndex;
				queue_infos.push_back(queue_info);
			}
			else if((queueFamilyProps[i].queueFlags & (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT)) == (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT))
			{
				queue_info.queueFamilyIndex = i;
				m_queueFamilyIndexCompute = queue_info.queueFamilyIndex;
				queue_infos.push_back(queue_info);
			}
			else if((queueFamilyProps[i].queueFlags & (VK_QUEUE_TRANSFER_BIT)) == (VK_QUEUE_TRANSFER_BIT))
			{
				queue_info.queueFamilyIndex = i;
				m_queueFamilyIndexTransfer = queue_info.queueFamilyIndex;
				queue_infos.push_back(queue_info);
			}
		}

		if(m_queueFamilyIndexTransfer == INVALID_INDEX)
		{
			m_queueFamilyIndexTransfer = m_queueFamilyIndex;
		}
		
		assert(m_queueFamilyIndexCompute != INVALID_INDEX);
		assert(m_queueFamilyIndexTransfer != INVALID_INDEX);
	}

	assert(m_queueFamilyIndex != INVALID_INDEX);

	XrVulkanDeviceCreateInfoKHR deviceCreateInfo{ XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR };
	deviceCreateInfo.systemId = xr_system_id_;
	deviceCreateInfo.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
	deviceCreateInfo.vulkanCreateInfo = &device_create_info;
	deviceCreateInfo.vulkanPhysicalDevice = vk_physical_device_;
	deviceCreateInfo.vulkanAllocator = nullptr;

	VkResult vk_create_device_err = VK_SUCCESS;
	XrResult create_vk_device_result = CreateVulkanDeviceKHR(xr_instance_, &deviceCreateInfo, &vk_logical_device_, &vk_create_device_err);

	if(create_vk_device_result != XR_SUCCESS)
	{
		assert(false);
		return false;
	}

	xr_memory_allocator_.Init(vk_physical_device_, vk_logical_device_);

	// VERY IMPORTANT to patch these values, XR session won't start without them
	xr_graphics_binding_.instance = vk_instance_;
	xr_graphics_binding_.physicalDevice = vk_physical_device_;
	xr_graphics_binding_.device = vk_logical_device_;
	xr_graphics_binding_.queueFamilyIndex = m_queueFamilyIndex;
	xr_graphics_binding_.queueIndex = 0;

	for (uint xr_command_buffer_index = 0; xr_command_buffer_index < NUM_COMMAND_BUFFERS; xr_command_buffer_index++)
	{
		xr_command_buffers_[xr_command_buffer_index].Init(vk_logical_device_, m_queueFamilyIndex);
	}

	set_initialized(true);

	return true;
}

void OpenXR::destroy_device()
{

}

int64_t OpenXR::select_swap_format(const std::vector<int64_t>& runtimeFormats) const
{
#if 1
	VkFormat hdr_colour_format = VK_FORMAT_R16G16B16A16_SFLOAT;
	return VK_FORMAT_R16G16B16A16_SFLOAT;
#else
	constexpr int64_t SupportedColorSwapchainFormats[] = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB,VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM };

	auto swapchainFormatIt = std::find_first_of(runtimeFormats.begin(), runtimeFormats.end(), std::begin(SupportedColorSwapchainFormats), std::end(SupportedColorSwapchainFormats));

	if(swapchainFormatIt == runtimeFormats.end())
	{
		assert(false);
		return INVALID_INDEX;
	}

	return *swapchainFormatIt;
#endif
}

bool OpenXR::create_swapchain()
{
	assert(xr_session_ != XR_NULL_HANDLE);
	assert(xr_swapchains_.empty());
	assert(xr_config_views_.empty());
	assert(xr_system_properties_initialized_);

	// Query and cache view configuration views.
	uint32_t viewCount = 0;
	xrEnumerateViewConfigurationViews(xr_instance_, xr_system_id_, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr);
	xr_config_views_.resize(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });

	xrEnumerateViewConfigurationViews(xr_instance_, xr_system_id_, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewCount, &viewCount, xr_config_views_.data());

	// Create and cache view buffer for xrLocateViews later.
	xr_views_.resize(viewCount, { XR_TYPE_VIEW });

	assert(viewCount == NUM_EYES);

	if(viewCount > 0)
	{
		uint32_t swapchainFormatCount = 0;
		xrEnumerateSwapchainFormats(xr_session_, 0, &swapchainFormatCount, nullptr);
		std::vector<int64_t> swapchainFormats(swapchainFormatCount);

		xrEnumerateSwapchainFormats(xr_session_, (uint32_t)swapchainFormats.size(), &swapchainFormatCount, swapchainFormats.data());

		assert(swapchainFormatCount == swapchainFormats.size());
		xr_colour_swapchain_format_ = select_swap_format(swapchainFormats);

		{
			std::string swapchainFormatsString;

			for(int64_t format : swapchainFormats)
			{
				const bool selected = format == xr_colour_swapchain_format_;
				swapchainFormatsString += " ";

				if(selected)
				{
					swapchainFormatsString += "[";
				}

				swapchainFormatsString += std::to_string(format);

				if(selected)
				{
					swapchainFormatsString += "]";
				}
			}
			//Log::Write(Log::Level::Verbose, Fmt("Swapchain Formats: %s", swapchainFormatsString.c_str()));
		}

		for(uint32_t i = 0; i < viewCount; i++)
		{
			const XrViewConfigurationView& vp = xr_config_views_[i];

			//Log::Write(Log::Level::Info, Fmt("Creating swapchain for view %d with dimensions Width=%d Height=%d SampleCount=%d", i,	vp.recommendedImageRectWidth, vp.recommendedImageRectHeight, vp.recommendedSwapchainSampleCount));

			// Create the swapchain.
			XrSwapchainCreateInfo swapchainCreateInfo{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
			swapchainCreateInfo.arraySize = 1;
			swapchainCreateInfo.format = xr_colour_swapchain_format_;
#if USE_HARDCODED_RES_FOR_OPENXR
			swapchainCreateInfo.width = UPSCALED_WIDTH / 2;
			swapchainCreateInfo.height = UPSCALED_HEIGHT;
#else
			swapchainCreateInfo.width = vp.recommendedImageRectWidth;
			swapchainCreateInfo.height = vp.recommendedImageRectHeight;
#endif
			swapchainCreateInfo.mipCount = 1;
			swapchainCreateInfo.faceCount = 1;
			swapchainCreateInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
			swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

			XRSwapchain swapchain = {};
			swapchain.width = swapchainCreateInfo.width;
			swapchain.height = swapchainCreateInfo.height;
			XrResult create_res = xrCreateSwapchain(xr_session_, &swapchainCreateInfo, &swapchain.handle);
			
			assert(create_res == XR_SUCCESS);
			assert(swapchain.handle);

			xr_swapchains_.push_back(swapchain);

			uint32_t imageCount = 0;
			xrEnumerateSwapchainImages(swapchain.handle, 0, &imageCount, nullptr);

			std::vector<XrSwapchainImageBaseHeader*> swapchainImages = AllocateSwapchainImageStructs(imageCount, swapchainCreateInfo);

			xrEnumerateSwapchainImages(swapchain.handle, imageCount, &imageCount, swapchainImages[0]);

			xr_swapchain_images_.insert(std::make_pair(swapchain.handle, std::move(swapchainImages)));
		}
	}
	return true;
}

#if ENABLE_OPENXR_FB_REFRESH_RATE
const std::vector<float>& OpenXR::GetSupportedRefreshRates() const
{
	if(!supported_refresh_rates_.empty())
	{
		return supported_refresh_rates_;
	}

	if(supports_refresh_rate_)
	{
		if(xrEnumerateDisplayRefreshRatesFB == nullptr)
		{
			XR_LOAD(xr_instance_, xrEnumerateDisplayRefreshRatesFB);
		}

		uint32_t num_refresh_rates = 0;
		XrResult result = xrEnumerateDisplayRefreshRatesFB(xr_session_, 0, &num_refresh_rates, nullptr);

		if((result == XR_SUCCESS) && (num_refresh_rates > 0))
		{
			supported_refresh_rates_.resize(num_refresh_rates);
			result = xrEnumerateDisplayRefreshRatesFB(xr_session_, num_refresh_rates, &num_refresh_rates, supported_refresh_rates_.data());

			if(result == XR_SUCCESS)
			{
				std::sort(supported_refresh_rates_.begin(), supported_refresh_rates_.end());
			}

			//Log::Write(Log::Level::Info, "OPENXR : GetSupportedRefreshRates:\n");

			for(float refresh_rate : supported_refresh_rates_)
			{
				//Log::Write(Log::Level::Info, Fmt("OPENXR : \t %.2f Hz", refresh_rate));
			}
		}
	}

	return supported_refresh_rates_;
}


float OpenXR::GetCurrentRefreshRate()
{
	if(current_refresh_rate_ > 0.0f)
	{
		return current_refresh_rate_;
	}

	if(supports_refresh_rate_)
	{
		if(xrGetDisplayRefreshRateFB == nullptr)
		{
			XR_LOAD(xr_instance_, xrGetDisplayRefreshRateFB);
		}

		XrResult result = xrGetDisplayRefreshRateFB(xr_session_, &current_refresh_rate_);

		if(result == XR_SUCCESS)
		{
			//Log::Write(Log::Level::Info, Fmt("OPENXR : GetCurrentRefreshRate => %.2f Hz", current_refresh_rate_));
		}
	}
	else
	{
		current_refresh_rate_ = DEFAULT_HZ;
	}

	return current_refresh_rate_;
}

float OpenXR::GetMaxRefreshRate()
{
	if(max_refresh_rate_ > 0.0f)
	{
		return max_refresh_rate_;
	}

	GetSupportedRefreshRates();

	if(supported_refresh_rates_.empty())
	{
		max_refresh_rate_ = DEFAULT_HZ;
	}
	else
	{
		// supported_refresh_rates_ is pre-sorted, so the last element is the highest supported refresh rate
		max_refresh_rate_ = supported_refresh_rates_.back();
		//Log::Write(Log::Level::Info, Fmt("OPENXR : GetMaxRefreshRate => %.2f Hz", max_refresh_rate_));
	}

	return max_refresh_rate_;
}

bool OpenXR::IsRefreshRateSupported(const float refresh_rate)
{
	GetSupportedRefreshRates();

	if(!supported_refresh_rates_.empty())
	{
		const bool found_it = (std::find(supported_refresh_rates_.begin(), supported_refresh_rates_.end(), refresh_rate) != supported_refresh_rates_.end());
		return found_it;
	}

	return (refresh_rate == DEFAULT_HZ);
}

// 0.0 = system default
void OpenXR::SetRefreshRate(const float refresh_rate)
{
	if(!supports_refresh_rate_ || !xr_session_)
	{
		return;
	}

	if(current_refresh_rate_ == 0.0f)
	{
		GetCurrentRefreshRate();
	}

	if(refresh_rate == current_refresh_rate_)
	{
		return;
	}

	if(!IsRefreshRateSupported(refresh_rate))
	{
		return;
	}

	if(xrRequestDisplayRefreshRateFB == nullptr)
	{
		XR_LOAD(xr_instance_, xrRequestDisplayRefreshRateFB);
	}

	XrResult result = xrRequestDisplayRefreshRateFB(xr_session_, refresh_rate);

	if(result == XR_SUCCESS)
	{
		//Log::Write(Log::Level::Info, Fmt("OPENXR : SetRefreshRate SUCCESSFULLY CHANGED from %.2f TO = %.2f Hz", current_refresh_rate_, refresh_rate));
		current_refresh_rate_ = refresh_rate;
	}
}
#endif


#if ENABLE_OPENXR_FB_EYE_TRACKING_SOCIAL
bool OpenXR::GetGazePoseSocial(const int eye, XrPosef& gaze_pose)
{
	if(social_eye_tracking_enabled_ && social_eye_gazes_.gaze[eye].isValid)
	{
		gaze_pose = social_eye_gazes_.gaze[eye].gazePose;
		return true;
	}

	return false;
}

void OpenXR::SetSocialEyeTrackerEnabled(const bool enabled)
{
	if(supports_eye_tracking_social_ && xr_instance_ && xr_session_)
	{
		social_eye_tracking_enabled_ = enabled;
	}
}

void OpenXR::CreateSocialEyeTracker()
{
	if(supports_eye_tracking_social_ && xr_instance_ && xr_session_)
	{
		if(xrCreateEyeTrackerFB == nullptr)
		{
			XR_LOAD(xr_instance_, xrCreateEyeTrackerFB);
		}

		if(xrCreateEyeTrackerFB == nullptr)
		{
			return;
		}

		if(xrDestroyEyeTrackerFB == nullptr)
		{
			XR_LOAD(xr_instance_, xrDestroyEyeTrackerFB);
		}

		if(xrDestroyEyeTrackerFB == nullptr)
		{
			return;
		}

		if(xrGetEyeGazesFB == nullptr)
		{
			XR_LOAD(xr_instance_, xrGetEyeGazesFB);
		}

		if(xrGetEyeGazesFB == nullptr)
		{
			return;
		}

		XrEyeTrackerCreateInfoFB create_info{ XR_TYPE_EYE_TRACKER_CREATE_INFO_FB };
		XrResult result = xrCreateEyeTrackerFB(xr_session_, &create_info, &social_eye_tracker_);

		if(result == XR_SUCCESS)
		{
			//Log::Write(Log::Level::Info, "OPENXR - Eye tracking enabled and running...");
			social_eye_tracking_enabled_ = true;
		}
	}
}

void OpenXR::DestroySocialEyeTracker()
{
	if(social_eye_tracker_ && xrDestroyEyeTrackerFB)
	{
		xrDestroyEyeTrackerFB(social_eye_tracker_);
		social_eye_tracker_ = nullptr;
		social_eye_tracking_enabled_ = false;

		//Log::Write(Log::Level::Info, "OPENXR - Social Eye tracker destroyed...");
	}
}

void OpenXR::UpdateSocialEyeTracker(const XrTime predicted_display_time)
{
	if(social_eye_tracking_enabled_)
	{
		UpdateSocialEyeTrackerGazes(predicted_display_time);

		for(int eye : { LEFT, RIGHT })
		{
			XrPosef gaze_pose;

			if(GetGazePoseSocial(eye, gaze_pose))
			{
				update_gaze_pose((uint)eye, gaze_pose);

				const XrPosef& eye_pose = xr_views_[eye].pose;

				const float laser_length = 10.0f;
				const float half_laser_length = laser_length * 0.5f;
				const float distance_to_eye = 0.1f;

				// Apply an offset so the lasers aren't overlapping the eye directly
				XrVector3f local_laser_offset = { 0.0f, 0.0f, (-half_laser_length - distance_to_eye) };

				XrMatrix4x4f gaze_rotation_matrix;
				XrMatrix4x4f_CreateFromQuaternion(&gaze_rotation_matrix, &gaze_pose.orientation);

				XrMatrix4x4f eye_rotation_matrix;
				XrMatrix4x4f_CreateFromQuaternion(&eye_rotation_matrix, &eye_pose.orientation);

				XrMatrix4x4f world_eye_gaze_matrix;
				XrMatrix4x4f_Multiply(&world_eye_gaze_matrix, &gaze_rotation_matrix, &eye_rotation_matrix);

				XrQuaternionf world_orientation;
				XrMatrix4x4f_GetRotation(&world_orientation, &world_eye_gaze_matrix);

				XrPosef final_pose;
				final_pose.position = eye_pose.position;
				//XrVector3f_Add(&final_pose.position, &gaze_pose.position, &eye_pose.position);
				final_pose.orientation = world_orientation;

				XrVector3f world_laser_offset;
				XrMatrix4x4f_TransformVector3f(&world_laser_offset, &world_eye_gaze_matrix, &local_laser_offset);

				XrVector3f_Add(&final_pose.position, &final_pose.position, &world_laser_offset);

				// Make a slender laser pointer-like box for gazes
				//XrVector3f gaze_cube_scale{ 0.001f, 0.001f, laser_length };
				//cubes.push_back(Cube{ final_pose, gaze_cube_scale });
			}
		}
	}
}

void OpenXR::UpdateSocialEyeTrackerGazes(const XrTime predicted_display_time)
{
	if(social_eye_tracker_ && social_eye_tracking_enabled_)
	{
		if(xrGetEyeGazesFB == nullptr)
		{
			XR_LOAD(xr_instance_, xrGetEyeGazesFB);
		}

		if(xrGetEyeGazesFB == nullptr)
		{
			return;
		}

		XrEyeGazesInfoFB gazes_info{ XR_TYPE_EYE_GAZES_INFO_FB };
		gazes_info.time = predicted_display_time;
		gazes_info.baseSpace = xr_app_space_;

#if LOG_EYE_TRACKING_DATA
		XrResult result = xrGetEyeGazesFB(social_eye_tracker_, &gazes_info, &social_eye_gazes_);

		if(result == XR_SUCCESS)
		{
			//Log::Write(Log::Level::Info, Fmt("OPENXR GAZES: Left Eye => %.2f, %.2f, Right Eye => %.2f, %.2f",	social_eye_gazes_.gaze[Side::LEFT].gazePose.orientation.x, social_eye_gazes_.gaze[Side::LEFT].gazePose.orientation.y,		social_eye_gazes_.gaze[Side::RIGHT].gazePose.orientation.x, social_eye_gazes_.gaze[Side::RIGHT].gazePose.orientation.y));
		}
#else
		xrGetEyeGazesFB(social_eye_tracker_, &gazes_info, &social_eye_gazes_);
#endif
	}
}
#endif


#if ENABLE_EXT_EYE_TRACKING
bool OpenXR::GetGazePoseEXT(XrPosef& gaze_pose)
{
	if(supports_ext_eye_tracking_ && ext_eye_tracking_enabled_ && ext_gaze_pose_valid_)
	{
		gaze_pose = ext_gaze_pose_;
		return true;
	}

	return false;
}

void OpenXR::CreateEXTEyeTracking()
{
	if(supports_ext_eye_tracking_ && xr_instance_ && xr_session_)
	{
		// https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_EXT_eye_gaze_interaction

		XrPosef pose_identity = {};
		pose_identity.position = { 0.0f, 0.0f, 0.0f };
		pose_identity.orientation = { 1.0f, 0.0f, 0.0f, 0.0f };

		// Create user intent action
		XrActionCreateInfo actionInfo{ XR_TYPE_ACTION_CREATE_INFO };
		strcpy(actionInfo.actionName, "gaze_action");
		actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
		strcpy(actionInfo.localizedActionName, "Gaze Action");

		xrCreateAction(xr_input_.xr_action_set_, &actionInfo, &xr_input_.gazeAction);

		XrActionSpaceCreateInfo createActionSpaceInfo{ XR_TYPE_ACTION_SPACE_CREATE_INFO };
		createActionSpaceInfo.action = xr_input_.gazeAction;
		createActionSpaceInfo.poseInActionSpace = pose_identity;

		xrCreateActionSpace(xr_session_, &createActionSpaceInfo, &xr_input_.gazeActionSpace);

		SetEXTEyeTrackerEnabled(true);
	}
}

void OpenXR::DestroyEXTEeyeTracking()
{
	if(supports_ext_eye_tracking_ && xr_instance_ && xr_session_)
	{
		SetEXTEyeTrackerEnabled(false);

#if 0
		if(xr_input_.localReferenceSpace)
		{
			xrDestroySpace(xr_input_.localReferenceSpace);
			xr_input_.localReferenceSpace = nullptr;
		}
#endif

		if(xr_input_.gazeActionSpace)
		{
			xrDestroySpace(xr_input_.gazeActionSpace);
			xr_input_.gazeActionSpace = nullptr;
		}
	}
}

void OpenXR::SetEXTEyeTrackerEnabled(const bool enabled)
{
	if(supports_ext_eye_tracking_ && xr_instance_ && xr_session_)
	{
		ext_eye_tracking_enabled_ = enabled;
	}
}

void OpenXR::UpdateEXTEyeTrackerGaze(XrTime predicted_display_time)
{
	if(!supports_ext_eye_tracking_ || !ext_eye_tracking_enabled_)
	{
		return;
	}

	XrSpaceLocation gaze_location{ XR_TYPE_SPACE_LOCATION };
	xrLocateSpace(xr_input_.gazeActionSpace, xr_app_space_, predicted_display_time, &gaze_location);

	ext_gaze_pose_valid_ = is_xr_pose_valid(gaze_location.locationFlags);

	if(ext_gaze_pose_valid_)
	{
		ext_gaze_pose_ = gaze_location.pose;

#if LOG_EYE_TRACKING_DATA
		//Log::Write(Log::Level::Info, Fmt("OPENXR EXT GAZE: X,Y,Z,W => %.2f, %.2f %.2f, %.2f",	ext_gaze_pose_.orientation.x, ext_gaze_pose_.orientation.y, ext_gaze_pose_.orientation.z, ext_gaze_pose_.orientation.w));
#endif
	}
}
#endif

#if ENABLE_OPENXR_FB_BODY_TRACKING

void OpenXR::CreateFBBodyTracker()
{
	if(!supports_fb_body_tracking_ || !xr_instance_ || !xr_session_ || body_tracker_)
	{
		return;
	}

	if(xrCreateBodyTrackerFB == nullptr)
	{
		XR_LOAD(xr_instance_, xrCreateBodyTrackerFB);
	}

	if(xrCreateBodyTrackerFB == nullptr)
	{
		return;
	}

	XrBodyTrackerCreateInfoFB create_info{ XR_TYPE_BODY_TRACKER_CREATE_INFO_FB };

#if ENABLE_OPENXR_META_FULL_BODY_TRACKING
	create_info.bodyJointSet = supports_meta_full_body_tracking_ ? XR_BODY_JOINT_SET_FULL_BODY_META : XR_BODY_JOINT_SET_DEFAULT_FB;
#else
	create_info.bodyJointSet = XR_BODY_JOINT_SET_DEFAULT_FB;
#endif

	XrResult result = xrCreateBodyTrackerFB(xr_session_, &create_info, &body_tracker_);

	if(result == XR_SUCCESS)
	{
		//Log::Write(Log::Level::Info, "OPENXR - Body tracking enabled and running...");
		fb_body_tracking_enabled_ = true;
	}
}

bool OpenXR::UpdateFBBodyTracking(const XrTime predicted_display_time)
{
	if(fb_body_tracking_enabled_)
	{
		UpdateFBBodyTrackerLocations(predicted_display_time);

		if(body_joint_locations_.isActive)
		{
#if ENABLE_WAIST_TRACKING
			const int waist_index = XR_BODY_JOINT_HIPS_FB;

			if(is_xr_pose_valid(body_joints_[waist_index].locationFlags))
			{
				const XrPosef& xr_waist_pose_LS = body_joints_[waist_index].pose;
				update_waist_pose_LS(xr_waist_pose_LS);
			}
#endif

#if 0
			for(int i = 0; i < XR_BODY_JOINT_COUNT_FB; ++i)
			{
				if(is_xr_pose_valid(body_joints_[i].locationFlags))
				{
					const XrPosef& body_joint_pose = body_joints_[i].pose;
				}
			}
#endif
		}
	}

}

void OpenXR::DestroyFBBodyTracker()
{
	if(!supports_fb_body_tracking_ || !body_tracker_)
	{
		return;
	}

	if(xrDestroyBodyTrackerFB == nullptr)
	{
		XR_LOAD(xr_instance_, xrDestroyBodyTrackerFB);
	}

	if(xrDestroyBodyTrackerFB == nullptr)
	{
		return;
	}

	xrDestroyBodyTrackerFB(body_tracker_);
	body_tracker_ = {};
	fb_body_tracking_enabled_ = false;

	//Log::Write(Log::Level::Info, "OPENXR - Body tracker destroyed...");
}


void OpenXR::UpdateFBBodyTrackerLocations(const XrTime& predicted_display_time)
{
	if(body_tracker_ && fb_body_tracking_enabled_)
	{
		if(xrLocateBodyJointsFB == nullptr)
		{
			XR_LOAD(xr_instance_, xrLocateBodyJointsFB);
		}

		if(xrLocateBodyJointsFB == nullptr)
		{
			return;
		}

#if 0
		if(xrGetSkeletonFB == nullptr)
		{
			XR_LOAD(xr_instance_, xrGetSkeletonFB);
		}

		if(xrGetSkeletonFB == nullptr)
		{
			return;
		}
#endif

		XrBodyJointsLocateInfoFB locate_info{ XR_TYPE_BODY_JOINTS_LOCATE_INFO_FB };
		locate_info.baseSpace = xr_app_space_;
		locate_info.time = predicted_display_time;

		body_joint_locations_.next = nullptr;

#if ENABLE_OPENXR_META_FULL_BODY_TRACKING
		body_joint_locations_.jointCount = supports_meta_full_body_tracking_ ? XR_FULL_BODY_JOINT_COUNT_META : XR_BODY_JOINT_COUNT_FB;
		body_joint_locations_.jointLocations = supports_meta_full_body_tracking_ ? full_body_joints_ : body_joints_;
#else
		body_joint_locations_.jointCount = XR_BODY_JOINT_COUNT_FB;
		body_joint_locations_.jointLocations = body_joints_;
#endif

#if LOG_BODY_TRACKING_DATA
		XrResult result = xrLocateBodyJointsFB(body_tracker_, &locate_info, &body_joint_locations_);

		if(result == XR_SUCCESS)
		{
			//Log::Write(Log::Level::Info, Fmt("OPENXR UPDATE BODY LOCATIONS SUCCEEDED"));
		}
		else
		{
			//Log::Write(Log::Level::Info, Fmt("OPENXR UPDATE BODY LOCATIONS FAILED"));
		}
#else
		xrLocateBodyJointsFB(body_tracker_, &locate_info, &body_joint_locations_);
#endif
	}
}

#endif // ENABLE_OPENXR_FB_BODY_TRACKING

#if ENABLE_OPENXR_META_BODY_TRACKING_FIDELITY
void OpenXR::RequestMetaFidelityBodyTracker(bool high_fidelity)
{
	XrBodyTrackingFidelityMETA new_fidelity = high_fidelity ? XR_BODY_TRACKING_FIDELITY_HIGH_META : XR_BODY_TRACKING_FIDELITY_LOW_META;

	if(!supports_meta_body_tracking_fidelity_ || !body_tracker_ || (current_fidelity_ == new_fidelity))
	{
		return;
	}

	if(xrRequestBodyTrackingFidelityMETA == nullptr)
	{
		XR_LOAD(xr_instance_, xrRequestBodyTrackingFidelityMETA);
	}

	if(xrRequestBodyTrackingFidelityMETA == nullptr)
	{
		return;
	}

	XrResult result = xrRequestBodyTrackingFidelityMETA(body_tracker_, new_fidelity);

	if(result == XR_SUCCESS)
	{
		//Log::Write(Log::Level::Info, Fmt("OPENXR - Meta Body tracking FIDELITY changed to %s", high_fidelity ? "XR_BODY_TRACKING_FIDELITY_HIGH_META" : "XR_BODY_TRACKING_FIDELITY_LOW_META"));
		current_fidelity_ = new_fidelity;
	}
}
#endif // ENABLE_OPENXR_META_BODY_TRACKING_FIDELITY

#if ENABLE_OPENXR_FB_SIMULTANEOUS_HANDS_AND_CONTROLLERS

bool OpenXR::AreSimultaneousHandsAndControllersSupported() const
{
	return supports_simultaneous_hands_and_controllers_;
}

bool OpenXR::AreSimultaneousHandsAndControllersEnabled() const
{
	return simultaneous_hands_and_controllers_enabled_;
}

void OpenXR::SetSimultaneousHandsAndControllersEnabled(const bool enabled)
{
	if(supports_simultaneous_hands_and_controllers_)
	{
		if(enabled)
		{
			if(xrResumeSimultaneousHandsAndControllersTrackingMETA == nullptr)
			{
				XR_LOAD(xr_instance_, xrResumeSimultaneousHandsAndControllersTrackingMETA);

				if(xrResumeSimultaneousHandsAndControllersTrackingMETA == nullptr)
				{
					return;
				}
			}

			XrSimultaneousHandsAndControllersTrackingResumeInfoMETA simultaneous_hands_controllers_resume_info = { XR_TYPE_SIMULTANEOUS_HANDS_AND_CONTROLLERS_TRACKING_RESUME_INFO_META };
			XrResult res = xrResumeSimultaneousHandsAndControllersTrackingMETA(xr_session_, &simultaneous_hands_controllers_resume_info);

			if(XR_UNQUALIFIED_SUCCESS(res))
			{
				simultaneous_hands_and_controllers_enabled_ = true;
				//Log::Write(Log::Level::Warning, Fmt("Simultaneous Hands and Controllers Successfully enabled"));
			}
		}
		else
		{
			if(xrPauseSimultaneousHandsAndControllersTrackingMETA == nullptr)
			{
				XR_LOAD(xr_instance_, xrPauseSimultaneousHandsAndControllersTrackingMETA);

				if(xrPauseSimultaneousHandsAndControllersTrackingMETA == nullptr)
				{
					return;
				}
			}

			XrSimultaneousHandsAndControllersTrackingPauseInfoMETA simultaneous_hands_controllers_pause_info = { XR_TYPE_SIMULTANEOUS_HANDS_AND_CONTROLLERS_TRACKING_PAUSE_INFO_META };
			XrResult res = xrPauseSimultaneousHandsAndControllersTrackingMETA(xr_session_, &simultaneous_hands_controllers_pause_info);

			if(XR_UNQUALIFIED_SUCCESS(res))
			{
				simultaneous_hands_and_controllers_enabled_ = false;
				//Log::Write(Log::Level::Warning, Fmt("Simultaneous Hands and Controllers Successfully disabled"));
			}
		}
	}
}

#endif // ENABLE_OPENXR_FB_SIMULTANEOUS_HANDS_AND_CONTROLLERS


void OpenXR::destroy_swapchain()
{
	for(XRSwapchain swapchain : xr_swapchains_)
	{
		if (swapchain.handle)
		{
			//xrDestroySwapchain(swapchain.handle);
			swapchain.handle = nullptr;
		}
	}

	m_swapchainImageContexts.clear();
	xr_swapchains_.clear();
}

bool OpenXR::get_system_properties()
{
	assert(xr_session_ != XR_NULL_HANDLE);

	if(xr_system_properties_initialized_)
	{
		return true;
	}

#if ENABLE_OPENXR_META_FULL_BODY_TRACKING
	XrSystemPropertiesBodyTrackingFullBodyMETA meta_full_body_tracking_properties{ XR_TYPE_SYSTEM_PROPERTIES_BODY_TRACKING_FULL_BODY_META };

	if(supports_meta_full_body_tracking_)
	{
		meta_full_body_tracking_properties.next = xr_system_properties_.next;
		xr_system_properties_.next = &meta_full_body_tracking_properties;
	}
#endif

#if ENABLE_OPENXR_FB_SIMULTANEOUS_HANDS_AND_CONTROLLERS
	XrSystemSimultaneousHandsAndControllersPropertiesMETA simultaneous_properties = { XR_TYPE_SYSTEM_SIMULTANEOUS_HANDS_AND_CONTROLLERS_PROPERTIES_META };

	if(supports_simultaneous_hands_and_controllers_)
	{
		simultaneous_properties.next = xr_system_properties_.next;
		xr_system_properties_.next = &simultaneous_properties;
	}
#endif

#if ENABLE_EXT_EYE_TRACKING
	if(supports_ext_eye_tracking_)
	{
		ext_gaze_interaction_properties_.next = xr_system_properties_.next;
		xr_system_properties_.next = &ext_gaze_interaction_properties_;
	}
#endif

#if ENABLE_OPENXR_FB_EYE_TRACKING_SOCIAL
	if(supports_eye_tracking_social_)
	{
		eye_tracking_social_properties_.next = xr_system_properties_.next;
		xr_system_properties_.next = &eye_tracking_social_properties_;
	}
#endif

	xrGetSystemProperties(xr_instance_, xr_system_id_, &xr_system_properties_);

	bool supports_EXT_ET = false;
	bool supports_Social_ET = false;

#if ENABLE_OPENXR_META_FULL_BODY_TRACKING
	supports_meta_full_body_tracking_ = meta_full_body_tracking_properties.supportsFullBodyTracking;
#endif

#if ENABLE_OPENXR_FB_SIMULTANEOUS_HANDS_AND_CONTROLLERS
	supports_simultaneous_hands_and_controllers_ = simultaneous_properties.supportsSimultaneousHandsAndControllers;
#endif

#if ENABLE_OPENXR_FB_EYE_TRACKING_SOCIAL
	supports_eye_tracking_social_ = eye_tracking_social_properties_.supportsEyeTracking;
	supports_Social_ET = supports_eye_tracking_social_;
#endif

#if ENABLE_EXT_EYE_TRACKING
	supports_ext_eye_tracking_ = ext_gaze_interaction_properties_.supportsEyeGazeInteraction;
	supports_EXT_ET = supports_ext_eye_tracking_;
#endif

#if ENABLE_OPENXR_FB_EYE_TRACKING_SOCIAL
	set_social_eye_tracking_supported(supports_Social_ET);
#endif
#if ENABLE_EXT_EYE_TRACKING
	set_EXT_eye_tracking_supported(supports_EXT_ET);
#endif

	// Log system properties.
	//Log::Write(Log::Level::Info, Fmt("System Properties: Name=%s VendorId=%d", xr_system_properties_.systemName, xr_system_properties_.vendorId));
	//Log::Write(Log::Level::Info, Fmt("System Graphics Properties: MaxWidth=%d MaxHeight=%d MaxLayers=%d",xr_system_properties_.graphicsProperties.maxSwapchainImageWidth, xr_system_properties_.graphicsProperties.maxSwapchainImageHeight,	xr_system_properties_.graphicsProperties.maxLayerCount));
	//Log::Write(Log::Level::Info, Fmt("System Tracking Properties: OrientationTracking=%s PositionTracking=%s",xr_system_properties_.trackingProperties.orientationTracking == XR_TRUE ? "True" : "False",	xr_system_properties_.trackingProperties.positionTracking == XR_TRUE ? "True" : "False"));
	//Log::Write(Log::Level::Info, Fmt("System Eye Tracking support: EXT = %d, Social Gazes = %d", (int)supports_EXT_ET, (int)supports_Social_ET));

	xr_system_properties_initialized_ = true;
	return true;
}

#if 0
HMDView& OpenXR::get_hmd_view()
{
	return hmd_view_;
}

const HMDView& OpenXR::get_hmd_view() const
{
	return hmd_view_;
}
#endif

bool OpenXR::update_views(XrTime predicted_display_time)
{
	if (!is_session_running())
	{
		return false;
	}
		
	XrViewState viewState{ XR_TYPE_VIEW_STATE };
	uint actual_num_views = 0;

	XrViewLocateInfo viewLocateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
	viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	viewLocateInfo.displayTime = predicted_display_time;
	viewLocateInfo.space = xr_app_space_;

	XrResult locate_views_res = xrLocateViews(xr_session_, &viewLocateInfo, &viewState, (uint)xr_views_.size(), &actual_num_views, xr_views_.data());

	if((locate_views_res != XR_SUCCESS) || !is_xr_pose_valid(viewState.viewStateFlags))
	{
		return false;
	}

	assert(actual_num_views == NUM_EYES);
	assert(actual_num_views == xr_config_views_.size());

	for (uint view_id = 0; view_id < actual_num_views; view_id++)
	{
		//XRView& xr_view = hmd_view_.eye_poses_[view_id];
		//xr_view.xr_pose_ = tv_convert(xr_views_[view_id].pose);
		//xr_view.fov_.set_from_xr(xr_views_[view_id].fov);
	}

	return true;
}

bool OpenXR::update_controller_poses(XrTime predicted_display_time)
{
	if(!is_session_running())
	{
		return false;
	}

	for(int hand = LEFT; hand < NUM_HANDS; hand++)
	{
#if SUPPORT_GRIP_POSE
		{
			XrSpaceLocation gripSpaceLocation{ XR_TYPE_SPACE_LOCATION };
			XrResult locate_res = xrLocateSpace(xr_input_.gripSpace[hand], xr_app_space_, predicted_display_time, &gripSpaceLocation);

			if(XR_UNQUALIFIED_SUCCESS(locate_res))
			{
				if(is_xr_pose_valid(gripSpaceLocation.locationFlags))
				{
					grip_pose_LS_[hand] = gripSpaceLocation.pose;
					grip_pose_valid_[hand] = true;
				}
				else
				{
					grip_pose_valid_[hand] = false;
				}
			}
			else
			{
				grip_pose_valid_[hand] = false;
			}
		}
#endif

#if SUPPORT_AIM_POSE
		{
			XrSpaceLocation aimSpaceLocation{ XR_TYPE_SPACE_LOCATION };
			XrResult locate_res = xrLocateSpace(xr_input_.aimSpace[hand], xr_app_space_, predicted_display_time, &aimSpaceLocation);

			if(XR_UNQUALIFIED_SUCCESS(locate_res))
			{
				if(is_xr_pose_valid(aimSpaceLocation.locationFlags))
				{
					aim_pose_LS_[hand] = aimSpaceLocation.pose;
					aim_pose_valid_[hand] = true;
				}
				else
				{
					aim_pose_valid_[hand] = false;
				}
			}
			else
			{
				aim_pose_valid_[hand] = false;
			}
		}
#endif
	}

	return true;
}

#if ENABLE_OPENXR_FB_EYE_TRACKING_SOCIAL
void OpenXR::set_social_eye_tracking_supported(const bool social_eye_tracking_supported)
{
	social_eye_tracking_supported_ = social_eye_tracking_supported;
}

bool OpenXR::is_social_eye_tracking_supported() const
{
	return social_eye_tracking_supported_;
}
#endif

#if ENABLE_EXT_EYE_TRACKING
void OpenXR::set_EXT_eye_tracking_supported(const bool EXT_eye_tracking_supported)
{
	EXT_eye_tracking_supported_ = EXT_eye_tracking_supported;
}

bool OpenXR::is_EXT_eye_tracking_supported() const
{
	return EXT_eye_tracking_supported_;
}
#endif

#if ENABLE_EYE_TRACKING

bool OpenXR::is_eye_tracking_supported() const
{
#if ENABLE_OPENXR_FB_EYE_TRACKING_SOCIAL
	if(social_eye_tracking_supported_)
	{
		return true;
	}
#endif

#if ENABLE_EXT_EYE_TRACKING
	if(EXT_eye_tracking_supported_)
	{
		return true;
	}
#endif

	return false;
}


void OpenXR::set_eye_tracking_enabled(const bool enabled)
{
#if ENABLE_OPENXR_FB_EYE_TRACKING_SOCIAL
	if(social_eye_tracking_supported_)
	{
		social_eye_tracking_enabled_ = enabled;
		return;
	}
#endif

#if ENABLE_EXT_EYE_TRACKING
	if(EXT_eye_tracking_supported_)
	{
		EXT_eye_tracking_enabled_ = enabled;
	}
#endif
}


bool OpenXR::is_eye_tracking_enabled() const
{
#if ENABLE_OPENXR_FB_EYE_TRACKING_SOCIAL
	if (social_eye_tracking_supported_)
	{
		return social_eye_tracking_enabled_;
	}
#endif

#if ENABLE_EXT_EYE_TRACKING
	if(EXT_eye_tracking_supported_)
	{
		return EXT_eye_tracking_enabled_;
	}
#endif

	return false;
}

void OpenXR::toggle_eye_tracking()
{
#if ENABLE_OPENXR_FB_EYE_TRACKING_SOCIAL
	if(social_eye_tracking_supported_)
	{
		social_eye_tracking_enabled_ = !social_eye_tracking_enabled_;
		return;
	}
#endif

#if ENABLE_EXT_EYE_TRACKING
	if(EXT_eye_tracking_supported_)
	{
		EXT_eye_tracking_enabled_ = !EXT_eye_tracking_enabled_;
	}
#endif
}

void OpenXR::update_gaze_pose(const uint eye, const XrPosef& gaze_pose)
{
	gaze_poses_[eye] = tv_convert_xr_pose_to_pose(gaze_pose);

	const float4x4& proj = hmd_view_.eye_poses_[eye].projection_;
	gaze_UVs_[eye] = tv_convert_gaze_pose_to_uv(gaze_poses_[eye], proj);
}


bool OpenXR::get_gaze_pose(const uint eye, Pose& gaze_pose)
{
	if(!is_eye_tracking_enabled())
	{
		return false;
	}

	gaze_pose = gaze_poses_[eye];
	return true;
}

bool OpenXR::get_gaze_UV(const uint eye, float2& gaze_uv)
{
	if(!is_eye_tracking_enabled())
	{
		return false;
	}

	gaze_uv = gaze_UVs_[eye];
	return true;
}

#endif

#if ENABLE_BODY_TRACKING
bool OpenXR::is_body_tracking_supported() const
{
#if ENABLE_OPENXR_FB_BODY_TRACKING
	return fb_body_tracking_supported_;
#else
	return false;
#endif
}

bool OpenXR::is_body_tracking_enabled() const
{
#if ENABLE_OPENXR_FB_BODY_TRACKING
	return fb_body_tracking_supported_ && fb_body_tracking_enabled_;
#else
	return false;
#endif
}

void OpenXR::set_body_tracking_enabled(const bool enabled)
{
#if ENABLE_OPENXR_FB_BODY_TRACKING
	if (fb_body_tracking_supported_)
	{
		fb_body_tracking_enabled_ = enabled;
	}
#else

#endif
}

void OpenXR::toggle_body_tracking()
{
#if ENABLE_OPENXR_FB_BODY_TRACKING
	if (fb_body_tracking_supported_)
	{
		fb_body_tracking_enabled_ = !fb_body_tracking_enabled_;
	}
#else
#endif
}
#endif // ENABLE_BODY_TRACKING

#if ENABLE_WAIST_TRACKING
bool OpenXR::get_waist_pose_LS(Pose& waist_pose_LS) const
{
	if (!is_body_tracking_enabled())
	{
		return false;
	}

	waist_pose_LS = waist_pose_LS_;
	return true;
}

void OpenXR::update_waist_pose_LS(const XrPosef& xr_waist_pose_LS)
{
	//fb_body_tracking_supported_ = true;
	waist_pose_LS_ = tv_convert_xr_pose_to_pose(xr_waist_pose_LS);

	const float3 euler_angles_radians(deg2rad(90.0f), deg2rad(-90.0f), deg2rad(0.0f));
	const Quat rotation = Quat(euler_angles_radians);
	waist_pose_LS_.rotation_ = waist_pose_LS_.rotation_ * rotation;
	waist_pose_LS_.rotation_.normalize();
}
#endif // ENABLE_WAIST_TRACKING


void OpenXR::update_thumbstick(const uint hand, const XrVector2f& stick_value)
{
	if(hand > RIGHT)
	{
		assert(false);
		return;
	}

	//thumbsticks_[hand].x = stick_value.x;
	//thumbsticks_[hand].y = stick_value.y;
}

void OpenXR::update_thumbstick_x(const uint hand, const float x_axis_value)
{
	if(hand > RIGHT)
	{
		assert(false);
		return;
	}

	//thumbsticks_[hand].x = x_axis_value;
}

void OpenXR::update_thumbstick_y(const uint hand, const float y_axis_value)
{
	if(hand > RIGHT)
	{
		assert(false);
		return;
	}

	//thumbsticks_[hand].y = y_axis_value;
}

float OpenXR::get_stick_x_value(const uint hand) const
{
	return 0.0f;
	//return thumbsticks_[hand].x;
}

float OpenXR::get_stick_y_value(const uint hand) const
{
	return 0.0f;
	//return thumbsticks_[hand].y;
}

void OpenXR::set_hand_gripping(const uint hand, const bool hand_gripping)
{
	assert(hand <= RIGHT);

	if(hand <= RIGHT)
	{
		if(hand_gripping_[hand] != hand_gripping)
		{
			hand_gripping_[hand] = hand_gripping;

			//DigitalButton& grip_click_button = engine_.get_vr_manager().get_grip_button(hand);
			//grip_click_button.set_state(hand_gripping);
		}
	}
}

bool OpenXR::is_hand_gripping(const uint hand) const
{
	if(hand > RIGHT)
	{
		assert(false);
		return false;
	}

	return hand_gripping_[hand];
}

void OpenXR::set_trigger_squeezed(const uint hand, const bool trigger_squeezed)
{
	assert(hand <= RIGHT);

	if(hand <= RIGHT)
	{
		if(trigger_squeezed_[hand] != trigger_squeezed)
		{
			trigger_squeezed_[hand] = trigger_squeezed;
		}
	}
}

bool OpenXR::is_trigger_squeezed(const uint hand) const
{
	if(hand > RIGHT)
	{
		assert(false);
		return false;
	}

	return trigger_squeezed_[hand];
}


void OpenXR::update_trigger(const uint hand, const float trigger_value)
{
	if(hand > RIGHT)
	{
		assert(false);
		return;
	}

	triggers_[hand] = trigger_value;
}


float OpenXR::get_trigger(const uint hand) const
{
	if (is_trigger_squeezed(hand))
	{
		return triggers_[hand];
	}

	return 0.0f;
}

void OpenXR::update_grip(const uint hand, const float grip_value)
{
	if(hand > RIGHT)
	{
		assert(false);
		return;
	}

	grips_[hand] = grip_value;
}

float OpenXR::get_grip(const uint hand) const
{
	if(is_hand_gripping(hand))
	{
		return grips_[hand];
	}

	return 0.0f;
}

std::vector<std::string> OpenXR::GetPlatformInstanceExtensions() const
{
	return {};
}

std::vector<std::string> OpenXR::GetGraphicsInstanceExtensions() const
{
	return {XR_KHR_VULKAN_ENABLE_EXTENSION_NAME};
}

void OpenXR::log_layers_and_extensions()
{
	const auto logExtensions = [](const char* layerName, OpenXR* openxr_ptr_, int indent = 0)
	{
		uint32_t instanceExtensionCount = 0;
		xrEnumerateInstanceExtensionProperties(layerName, 0, &instanceExtensionCount, nullptr);

		std::vector<XrExtensionProperties> extensions(instanceExtensionCount);

		for(XrExtensionProperties& extension : extensions)
		{
			extension.type = XR_TYPE_EXTENSION_PROPERTIES;
		}

		xrEnumerateInstanceExtensionProperties(layerName, (uint32_t)extensions.size(), &instanceExtensionCount, extensions.data());

		const std::string indentStr(indent, ' ');
		//Log::Write(Log::Level::Verbose, Fmt("%sAvailable Extensions: (%d)", indentStr.c_str(), instanceExtensionCount));

		for(const XrExtensionProperties& extension : extensions)
		{
			//Log::Write(Log::Level::Verbose, Fmt("%s  Name=%s SpecVersion=%d", indentStr.c_str(), extension.extensionName,extension.extensionVersion));

#if ENABLE_OPENXR_FB_REFRESH_RATE
			if(!strcmp(extension.extensionName, XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME))
			{
				//Log::Write(Log::Level::Info, "FB OPENXR XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME - DETECTED");
				openxr_ptr_->supports_refresh_rate_ = true;
			}
#endif

#if ENABLE_OPENXR_HAND_TRACKING
			if(!strcmp(extension.extensionName, XR_EXT_HAND_TRACKING_EXTENSION_NAME))
			{
				//Log::Write(Log::Level::Info, "FB OPENXR XR_EXT_HAND_TRACKING_EXTENSION_NAME - DETECTED");
				openxr_ptr_->supports_hand_tracking_ = true;
			}
#endif

#if ENABLE_OPENXR_FB_EYE_TRACKING_SOCIAL
			if(!strcmp(extension.extensionName, XR_FB_EYE_TRACKING_SOCIAL_EXTENSION_NAME))
			{
				//Log::Write(Log::Level::Info, "FB OPENXR XR_FB_EYE_TRACKING_SOCIAL_EXTENSION_NAME - DETECTED");
				openxr_ptr_->supports_eye_tracking_social_ = true;
			}
#endif

#if ENABLE_EXT_EYE_TRACKING
			if(!strcmp(extension.extensionName, XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME))
			{
				//Log::Write(Log::Level::Info, "FB OPENXR XR_EXT_eye_gaze_interaction - DETECTED");
				openxr_ptr_->supports_ext_eye_tracking_ = true;
			}
#endif

#if ENABLE_OPENXR_FB_FACE_TRACKING
			if(!strcmp(extension.extensionName, XR_FB_FACE_TRACKING_EXTENSION_NAME))
			{
				//Log::Write(Log::Level::Info, "FB OPENXR XR_FB_FACE_TRACKING_EXTENSION_NAME - DETECTED");
				openxr_ptr_->supports_face_tracking_ = true;
			}
#endif

#if ENABLE_OPENXR_FB_BODY_TRACKING
			if(!strcmp(extension.extensionName, XR_FB_BODY_TRACKING_EXTENSION_NAME))
			{
				//Log::Write(Log::Level::Info, "FB OPENXR XR_FB_BODY_TRACKING_EXTENSION_NAME - DETECTED");
				openxr_ptr_->supports_fb_body_tracking_ = true;
			}
#endif

#if ENABLE_OPENXR_META_BODY_TRACKING_FIDELITY
			if(!strcmp(extension.extensionName, XR_META_BODY_TRACKING_FIDELITY_EXTENSION_NAME))
			{
				//Log::Write(Log::Level::Info, "FB OPENXR XR_META_body_tracking_fidelity - DETECTED");
				openxr_ptr_->supports_meta_body_tracking_fidelity_ = true;
			}
#endif

#if ENABLE_OPENXR_META_FULL_BODY_TRACKING
			if(!strcmp(extension.extensionName, XR_META_BODY_TRACKING_FULL_BODY_EXTENSION_NAME))
			{
				//Log::Write(Log::Level::Info, "FB OPENXR XR_META_body_tracking_full_body - DETECTED");
				openxr_ptr_->supports_meta_full_body_tracking_ = true;
			}
#endif

#if ENABLE_OPENXR_FB_SIMULTANEOUS_HANDS_AND_CONTROLLERS
			if(!strcmp(extension.extensionName, XR_META_SIMULTANEOUS_HANDS_AND_CONTROLLERS_EXTENSION_NAME))
			{
				//Log::Write(Log::Level::Info, "FB OPENXR XR_META_SIMULTANEOUS_HANDS_AND_CONTROLLERS_EXTENSION_NAME - DETECTED");
				openxr_ptr_->supports_simultaneous_hands_and_controllers_ = true;
			}
#endif

#if ENABLE_VIVE_TRACKERS
			if(!strcmp(extension.extensionName, XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME))
			{
				//Log::Write(Log::Level::Info, "XR_HTCX_vive_tracker_interaction - DETECTED");
				openxr_ptr_->supports_HTCX_vive_tracker_interaction_ = true;
			}
#endif
		}

		return;
	};

	// Log non-layer extensions (layerName==nullptr).
	logExtensions(nullptr, this);

	// Log layers and any of their extensions.
	{
		uint32_t layerCount = 0;
		xrEnumerateApiLayerProperties(0, &layerCount, nullptr);

		std::vector<XrApiLayerProperties> layers(layerCount);

		for(XrApiLayerProperties& layer : layers)
		{
			layer.type = XR_TYPE_API_LAYER_PROPERTIES;
		}

		xrEnumerateApiLayerProperties((uint32_t)layers.size(), &layerCount, layers.data());

		//Log::Write(Log::Level::Info, Fmt("Available Layers: (%d)", layerCount));

		for(const XrApiLayerProperties& layer : layers)
		{
			//Log::Write(Log::Level::Verbose,
				//Fmt("  Name=%s SpecVersion=%s LayerVersion=%d Description=%s", layer.layerName,
					//GetXrVersionString(layer.specVersion).c_str(), layer.layerVersion, layer.description));

			logExtensions(layer.layerName, this, 4);
		}
	}
}


#if 0
inline std::string Fmt(const char* fmt, ...)
{
	va_list vl;
	va_start(vl, fmt);
	int size = std::vsnprintf(nullptr, 0, fmt, vl);
	va_end(vl);

	if(size != -1)
	{
		std::unique_ptr<char[]> buffer(new char[size + 1]);

		va_start(vl, fmt);
		size = std::vsnprintf(buffer.get(), size + 1, fmt, vl);
		va_end(vl);
		if(size != -1)
		{
			return std::string(buffer.get(), size);
		}
	}
}

inline std::string GetXrVersionString(XrVersion ver) 
{
	return Fmt("%d.%d.%d", XR_VERSION_MAJOR(ver), XR_VERSION_MINOR(ver), XR_VERSION_PATCH(ver));
}
#endif


void OpenXR::log_instance_info()
{
	assert(xr_instance_ != XR_NULL_HANDLE);

	XrInstanceProperties instanceProperties{ XR_TYPE_INSTANCE_PROPERTIES };
	xrGetInstanceProperties(xr_instance_, &instanceProperties);

	//Log::Write(Log::Level::Info, Fmt("Instance RuntimeName=%s RuntimeVersion=%s", instanceProperties.runtimeName, GetXrVersionString(instanceProperties.runtimeVersion).c_str()));
}

OpenXR openxr_;
bool started_session_ = false;

// C Interface wrapper for simplicity
extern "C"
{
	VkResult __cdecl CreateVulkanOpenXRInstance(const VkInstanceCreateInfo* instance_create_info, VkInstance* vk_instance)
	{
		const bool instance_ok = openxr_.create_instance(*instance_create_info);

		if(!instance_ok)
		{
			return VK_ERROR_UNKNOWN;
		}

		*vk_instance = openxr_.vk_instance_;
		 
		return VK_SUCCESS;
	}

	VkResult CreateVulkanOpenXRDevice(const VkDeviceCreateInfo* device_create_info, VkPhysicalDevice* vk_physical_device, VkDevice* vk_logical_device)
	{
		if(!openxr_.vk_instance_ || openxr_.vk_physical_device_ || openxr_.vk_logical_device_)
		{
			return VK_ERROR_UNKNOWN;
		}

		const bool device_ok = openxr_.create_device(*device_create_info);

		if(!device_ok)
		{
			return VK_ERROR_UNKNOWN;
		}

		*vk_physical_device = openxr_.vk_physical_device_;
		*vk_logical_device = openxr_.vk_logical_device_;

		return VK_SUCCESS;
	}

	void OpenXR_Update()
	{
		if(!started_session_)
		{
			openxr_.start_session();
			started_session_ = true;
		}

		openxr_.update();
	}

	void OpenXR_Shutdown()
	{
		openxr_.shutdown();
	}

	void OpenXR_Endframe(VkCommandBuffer* external_command_buffer)
	{
		openxr_.end_frame(external_command_buffer);
	}

	bool Is_OpenXR_Session_Running()
	{
		return openxr_.is_session_running();
	}

	bool GetEyePosition(const int view_id, float* eye_pos_vec3, float* tracking_to_world_matrix)
	{
		if(!openxr_.is_session_running())
		{
			return false;
		}

		XrView xr_view = {};

		if(!openxr_.get_view(view_id, xr_view))
		{
			return false;
		}

		if(tracking_to_world_matrix)
		{

		}
		else
		{
			memcpy(&eye_pos_vec3, &xr_view.pose.position, sizeof(float) * 3);
		}

		return true;
	}

	bool GetViewMatrix(const int view_id, const bool append, float* matrix_ptr)
	{
		if(!openxr_.is_session_running() || !matrix_ptr)
		{
			return false;
		}

		XrView xr_view = {};

		if(!openxr_.get_view(view_id, xr_view))
		{
			return false;
		}

		if(append)
		{
			XrMatrix4x4f local_matrix = {};
			XrMatrix4x4f_CreateFromRigidTransform(&local_matrix, &xr_view.pose);

			XrMatrix4x4f orig_view_matrix = {};
			memcpy(&orig_view_matrix, matrix_ptr, sizeof(float) * 16);
			XrMatrix4x4f_Multiply((XrMatrix4x4f*)matrix_ptr, &local_matrix, &orig_view_matrix);
		}
		else
		{
			XrPosef xr_pose_copy = xr_view.pose;

			XrQuaternionf rotate_X = {};
			XrQuaternionf rotate_Y = {};
			XrQuaternionf rotate_Z = {};

			static float angle_deg = 90.0f;
			const float angle_rad = angle_deg * (MATH_PI / 180.0f);

			XrVector3f axis_X = { 1.0f, 0.0f, 0.0f };
			XrVector3f axis_Y = { 0.0f, 1.0f, 0.0f };
			XrVector3f axis_Z = { 0.0f, 0.0f, 1.0f };

			XrQuaternionf_CreateFromAxisAngle(&rotate_X, &axis_X, angle_rad);
			XrQuaternionf_CreateFromAxisAngle(&rotate_Y, &axis_Y, angle_rad);
			XrQuaternionf_CreateFromAxisAngle(&rotate_Z, &axis_Z, angle_rad);

			static int do_index = 0;

			if(do_index == 1)
			{
				XrQuaternionf_Multiply(&xr_pose_copy.orientation, &xr_view.pose.orientation, &rotate_X);
			}
			else if(do_index == 2)
			{
				XrQuaternionf_Multiply(&xr_pose_copy.orientation, &xr_view.pose.orientation, &rotate_Y);
			}
			else if(do_index == 3)
			{
				XrQuaternionf_Multiply(&xr_pose_copy.orientation, &xr_view.pose.orientation, &rotate_Z);
			}

			static float offsetX = 0.0f;
			static float offsetY = 0.0f;
			static float offsetZ = 0.0f;

			xr_pose_copy.position.x += offsetX;
			xr_pose_copy.position.x += offsetY;
			xr_pose_copy.position.x += offsetZ;

			XrMatrix4x4f_CreateFromRigidTransform((XrMatrix4x4f*)matrix_ptr, &xr_pose_copy);
		}
		
		return true;
	}

	bool GetFov(const int view_id, XrFovf* fov_ptr)
	{
		if(!openxr_.is_session_running() || !fov_ptr)
		{
			return false;
		}

		XrView xr_view = {};

		if(!openxr_.get_view(view_id, xr_view))
		{
			return false;
		}

		XrFovf& fov = *fov_ptr;
		fov = xr_view.fov;

		return true;
	}

	bool GetHandPosition(const int hand_id, float* hand_pos_vec3, float* tracking_to_world_matrix)
	{
		if(!openxr_.is_session_running())
		{
			return false;
		}

		XrView xr_view = {};

		if(!openxr_.aim_pose_valid_[hand_id])
		{
			return false;
		}

		if(tracking_to_world_matrix)
		{

		}
		else
		{
			memcpy(&hand_pos_vec3, &xr_view.pose.position, sizeof(float) * 3);
		}

		return true;
	}

	bool GetHandMatrix(const int hand_id, const bool append, float* matrix_ptr)
	{
		if(!openxr_.is_session_running() || !matrix_ptr || !openxr_.aim_pose_valid_[hand_id])
		{
			return false;
		}

		if(append)
		{
			XrMatrix4x4f local_matrix = {};
			XrMatrix4x4f_CreateIdentity(&local_matrix);

			//local_matrix.m[12] = 50.0f; // + X is forward
			//local_matrix.m[13] = 50.0f; // +Y is to the right
			//local_matrix.m[14] = 50.0f; // +Z = UP, 10cm

			const float tracking_scale_forward = 10.0f;
			const float tracking_scale = 50.0f;

			local_matrix.m[12] = -openxr_.aim_pose_LS_[hand_id].position.z * tracking_scale_forward; // + X is forward
			local_matrix.m[13] = (hand_id == LEFT) ? openxr_.aim_pose_LS_[hand_id].position.x * tracking_scale : -openxr_.aim_pose_LS_[hand_id].position.x * tracking_scale; // +Y is to the right
			local_matrix.m[14] = openxr_.aim_pose_LS_[hand_id].position.y * tracking_scale; // +Z = UP, 10cm

			//openxr_.aim_pose_LS_[hand_id].orientation.x = 0.0f;
			//openxr_.aim_pose_LS_[hand_id].orientation.y = 0.0f;
			//openxr_.aim_pose_LS_[hand_id].orientation.z = 0.0f;
			//openxr_.aim_pose_LS_[hand_id].orientation.w = 0.0f;

			//openxr_.aim_pose_LS_[hand_id].position.x = 0.0f;
			//openxr_.aim_pose_LS_[hand_id].position.y = 0.0f;
			//openxr_.aim_pose_LS_[hand_id].position.z = 0.0f;

			//XrMatrix4x4f_CreateFromRigidTransform(&local_matrix, &openxr_.aim_pose_LS_[hand_id]);

			//XrMatrix4x4f local_matrixT = {};
			//XrMatrix4x4f_Transpose(&local_matrixT, &local_matrix);

			XrMatrix4x4f local_rotation_matrix = {};
			XrMatrix4x4f_CreateFromQuaternion(&local_rotation_matrix, &openxr_.aim_pose_LS_[hand_id].orientation);

			XrMatrix4x4f local_rotation_matrixT = {};
			XrMatrix4x4f_Transpose(&local_rotation_matrixT, &local_rotation_matrix);

			XrMatrix4x4f composed_local_matrix = {};
			XrMatrix4x4f_Multiply(&composed_local_matrix, &local_matrix, &local_rotation_matrixT);

			XrMatrix4x4f orig_hand_matrix = {};
			memcpy(&orig_hand_matrix, matrix_ptr, sizeof(float) * 16);
			XrMatrix4x4f_Multiply((XrMatrix4x4f*)matrix_ptr,  &orig_hand_matrix, &composed_local_matrix);
		}
		else
		{
			XrMatrix4x4f_CreateFromRigidTransform((XrMatrix4x4f*)matrix_ptr, &openxr_.aim_pose_LS_[hand_id]);
		}

		return true;
	}
}

#endif // SUPPORT_OPENXR

