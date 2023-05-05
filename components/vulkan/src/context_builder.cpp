/* Copyright (c) 2023, Thomas Atkinson
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "vulkan/context_builder.hpp"

#define VOLK_IMPLEMENTATION
#include <volk.h>

#include <vulkan/vulkan.hpp>

#include <core/util/logging.hpp>

#include "util/logger.hpp"

// Initialize the loader with the default dispatch loader dynamic
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace vkb
{

InstanceBuilder::InstanceBuilder(ContextBuilder &context_builder) :
    context_builder{context_builder}
{}

InstanceBuilder &InstanceBuilder::set_application_info(const vk::ApplicationInfo &_application_info)
{
	application_info = _application_info;
	return *this;
}

InstanceBuilder &InstanceBuilder::request_extension(const char *extension_name, InclusionMode mode)
{
	if (mode == InclusionMode::Optional)
	{
		if (required_extensions.find(extension_name) == required_extensions.end())
		{
			// only append if not already required
			optional_extensions.emplace(extension_name);
		}
	}
	else
	{
		// remove from optional if already there
		optional_extensions.erase(extension_name);
		required_extensions.emplace(extension_name);
	}
	return *this;
}

InstanceBuilder &InstanceBuilder::request_layer(const char *layer_name, InclusionMode mode)
{
	if (mode == InclusionMode::Optional)
	{
		if (required_layers.find(layer_name) == required_layers.end())
		{
			// only append if not already required
			optional_layers.emplace(layer_name);
		}
	}
	else
	{
		// remove from optional if already there
		optional_layers.erase(layer_name);
		required_layers.emplace(layer_name);
	}
	return *this;
}

InstanceBuilder &InstanceBuilder::request_layer(const char *extension_name, const char *layer_name, InclusionMode mode)
{
	request_extension(extension_name, mode);
	request_layer(layer_name, mode);
	return *this;
}

InstanceBuilder &InstanceBuilder::add_logger_callback(std::shared_ptr<LoggerCallbacks> &&_logger_callbacks)
{
	logger_callbacks.emplace_back(std::move(_logger_callbacks));
	return *this;
}

ContextBuilder &InstanceBuilder::done()
{
	return context_builder;
}

void InstanceBuilder::enable_validation_layers(ContextBuilder &context_builder)
{
	static std::vector<std::vector<const char *>> validation_layer_priority_list =
	    {
	        // The preferred validation layer is "VK_LAYER_KHRONOS_validation"
	        {"VK_LAYER_KHRONOS_validation"},

	        // Otherwise we fallback to using the LunarG meta layer
	        {"VK_LAYER_LUNARG_standard_validation"},

	        // Otherwise we attempt to enable the individual layers that compose the LunarG meta layer since it doesn't exist
	        {
	            "VK_LAYER_GOOGLE_threading",
	            "VK_LAYER_LUNARG_parameter_validation",
	            "VK_LAYER_LUNARG_object_tracker",
	            "VK_LAYER_LUNARG_core_validation",
	            "VK_LAYER_GOOGLE_unique_objects",
	        },

	        // Otherwise as a last resort we fallback to attempting to enable the LunarG core layer
	        {"VK_LAYER_LUNARG_core_validation"}};

	auto available_layers = vk::enumerateInstanceLayerProperties();

	const auto all_layers_are_available = [&](const std::vector<const char *> &requested) -> bool {
		for (auto &layer : requested)
		{
			bool found = false;
			for (auto &available_layer : available_layers)
			{
				if (strcmp(layer, available_layer.layerName) == 0)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				LOGW("Validation layer {} is not available", layer)
				return false;
			}
		}
		return true;
	};

	// Attempt to enable requested validation layers in order of preference
	for (auto &validation_layers : validation_layer_priority_list)
	{
		if (all_layers_are_available(validation_layers))
		{
			auto &instance_builder = context_builder.configure_instance();
			for (auto &layer : validation_layers)
			{
				instance_builder.request_layer(layer, vkb::InclusionMode::Required);
			}
			break;
		}

		LOGW("Couldn't enable all validation layers (see log) - falling back");
	}
}

void InstanceBuilder::enable_default_logger(ContextBuilder &context_builder)
{
	// Add default log callback
	context_builder
	    .configure_instance()
	    .add_logger_callback(std::make_shared<vkb::LoggerCallbacks>(
	        vkb::LoggerCallbacks{[](vkb::LogLevel level, const char *msg) {
		        switch (level)
		        {
			        case vkb::LogLevel::Debug:
				        LOGD(msg);
				        break;
			        case vkb::LogLevel::Info:
				        LOGI(msg);
				        break;
			        case vkb::LogLevel::Warning:
				        LOGW(msg);
				        break;
			        case vkb::LogLevel::Error:
				        LOGE(msg);
				        break;
			        default:
				        LOGI(msg);
				        break;
		        }
	        }}));
}

void InstanceBuilder::build(Context &context)
{
	std::set<const char *> extensions;
	std::set<const char *> layers;

	auto available_extensions = vk::enumerateInstanceExtensionProperties();

	bool has_debug_utils  = false;
	bool has_debug_report = false;

	if (!logger_callbacks.empty())
	{
		for (auto &extension : available_extensions)
		{
			if (strcmp(extension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
			{
				has_debug_utils = true;
				break;        // early return as we prefer utils over report
			}
			else if (strcmp(extension.extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0)
			{
				has_debug_report = true;
			}
		}

		if (has_debug_utils)
		{
			request_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, InclusionMode::Optional);
		}
		else if (has_debug_report)
		{
			request_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, InclusionMode::Optional);
		}
		else
		{
			LOGI("No debug extension available - loggers will not be used");
		}
	}

	// Check if all required extensions are available
	for (auto &required_extension : required_extensions)
	{
		if (std::find_if(available_extensions.begin(), available_extensions.end(), [&required_extension](const vk::ExtensionProperties &extension) {
			    return strcmp(extension.extensionName, required_extension) == 0;
		    }) == available_extensions.end())
		{
			throw std::runtime_error("Required extension is not available: " + std::string(required_extension));
		}
	}

	// Add all required extensions
	for (auto &required_extension : required_extensions)
	{
		extensions.emplace(required_extension);
	}

	// Add available optional extensions
	for (auto &optional_extension : optional_extensions)
	{
		if (std::find_if(available_extensions.begin(), available_extensions.end(), [&optional_extension](const vk::ExtensionProperties &extension) {
			    return strcmp(extension.extensionName, optional_extension) == 0;
		    }) != available_extensions.end())
		{
			extensions.emplace(optional_extension);
		}
	}

	auto available_layers = vk::enumerateInstanceLayerProperties();

	// Check if all required layers are available
	for (auto &required_layer : required_layers)
	{
		if (std::find_if(available_layers.begin(), available_layers.end(), [&required_layer](const vk::LayerProperties &layer) {
			    return strcmp(layer.layerName, required_layer) == 0;
		    }) == available_layers.end())
		{
			throw std::runtime_error("Required layer is not available: " + std::string(required_layer));
		}
	}

	// Add all required layers
	for (auto &required_layer : required_layers)
	{
		layers.emplace(required_layer);
	}

	// Add available optional layers
	for (auto &optional_layer : optional_layers)
	{
		if (std::find_if(available_layers.begin(), available_layers.end(), [&optional_layer](const vk::LayerProperties &layer) {
			    return strcmp(layer.layerName, optional_layer) == 0;
		    }) != available_layers.end())
		{
			layers.emplace(optional_layer);
		}
	}

	vk::ApplicationInfo       app_info{application_info};
	std::vector<const char *> requested_extensions{extensions.begin(), extensions.end()};
	std::vector<const char *> requested_layers{layers.begin(), layers.end()};

	vk::InstanceCreateInfo create_info{
	    {},
	    &app_info,
	    static_cast<uint32_t>(requested_layers.size()),
	    requested_layers.data(),
	    static_cast<uint32_t>(requested_extensions.size()),
	    requested_extensions.data(),
	};

	std::shared_ptr<LoggerCallbacks> logger_ptr = std::make_shared<LoggerCallbacks>();

	logger_ptr->simple_callback = [callbacks = logger_callbacks](LogLevel level, const char *message) -> void {
		for (auto &callback : callbacks)
		{
			if (callback && callback->simple_callback)
			{
				callback->simple_callback(level, message);
			}
		}
	};

	logger_ptr->debug_utils_callback = [callbacks = logger_callbacks](vk::DebugUtilsMessageSeverityFlagBitsEXT    message_severity,
	                                                                  vk::DebugUtilsMessageTypeFlagsEXT           message_type,
	                                                                  const VkDebugUtilsMessengerCallbackDataEXT *callback_data) -> void {
		for (auto &callback : callbacks)
		{
			if (callback && callback->debug_utils_callback)
			{
				callback->debug_utils_callback(message_severity, message_type, callback_data);
			}
		}
	};

	logger_ptr->debug_report_callback = [callbacks = logger_callbacks](vk::DebugReportFlagsEXT      flags,
	                                                                   vk::DebugReportObjectTypeEXT type,
	                                                                   uint64_t                     object,
	                                                                   size_t                       location,
	                                                                   int32_t                      message_code,
	                                                                   const char                  *layer_prefix,
	                                                                   const char                  *message) -> void {
		for (auto &callback : callbacks)
		{
			if (callback && callback->debug_report_callback)
			{
				callback->debug_report_callback(flags, type, object, location, message_code, layer_prefix, message);
			}
		}
	};

	vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create_info;
	vk::DebugReportCallbackCreateInfoEXT debug_report_create_info;
	if (has_debug_utils)
	{
		debug_utils_create_info =
		    vk::DebugUtilsMessengerCreateInfoEXT(
		        {},
		        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
		        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
		        debug_utils_messenger_callback,
		        static_cast<void *>(logger_ptr.get()));

		create_info.pNext = &debug_utils_create_info;
	}
	else if (has_debug_report)
	{
		debug_report_create_info = vk::DebugReportCallbackCreateInfoEXT(
		    vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::ePerformanceWarning,
		    debug_callback,
		    static_cast<void *>(logger_ptr.get()));

		create_info.pNext = &debug_report_create_info;
	}

	context.instance = vk::createInstance(create_info);

	VULKAN_HPP_DEFAULT_DISPATCHER.init(context.instance);

	volkLoadInstance(context.instance);

	if (has_debug_utils)
	{
		context.debug_utils_messenger = context.instance.createDebugUtilsMessengerEXT(debug_utils_create_info);
	}
	else if (has_debug_report)
	{
		context.debug_report_callback = context.instance.createDebugReportCallbackEXT(debug_report_create_info);
	}

	context.logging_callbacks = logger_ptr;
}

PhysicalDeviceSelector::PhysicalDeviceSelector(ContextBuilder &context_builder) :
    context_builder{context_builder}
{
}

PhysicalDeviceSelector &PhysicalDeviceSelector::score(const ScoreFunction &score_function)
{
	score_functions.push_back(score_function);
	return *this;
}

ContextBuilder &PhysicalDeviceSelector::done()
{
	return context_builder;
}

void PhysicalDeviceSelector::select(Context &context)
{
	if (!context.instance)
	{
		return;
	}

	auto physical_devices = context.instance.enumeratePhysicalDevices();

	if (physical_devices.empty())
	{
		throw std::runtime_error("No physical devices available");
	}

	std::vector<std::pair<vk::PhysicalDevice, uint32_t>> scored_devices;

	for (auto &physical_device : physical_devices)
	{
		uint32_t score = 0;

		for (auto &score_function : score_functions)
		{
			score += score_function(physical_device);
		}

		scored_devices.emplace_back(physical_device, score);
	}

	std::sort(scored_devices.begin(), scored_devices.end(), [](const std::pair<vk::PhysicalDevice, uint32_t> &a, const std::pair<vk::PhysicalDevice, uint32_t> &b) {
		return a.second > b.second;
	});

	context.physical_device = scored_devices[0].first;
}

PhysicalDeviceSelector::ScoreFunction PhysicalDeviceSelector::type_preference(const std::vector<vk::PhysicalDeviceType> &priority_order)
{
	if (priority_order.empty())
	{
		return default_type_preference();
	}

	return [=](const vk::PhysicalDevice &gpu) -> int {
		auto properties = gpu.getProperties();
		auto features   = gpu.getFeatures();

		// A higher score is awarded for devices that are closer to the start of the priority order
		for (size_t i = 0; i < priority_order.size(); i++)
		{
			if (properties.deviceType == priority_order[i])
			{
				return PreferredScore - i * DefaultScore;
			}
		}

		// If the device type is not in the priority order, it is awarded a score of 0
		return RejectedScore;
	};
}

PhysicalDeviceSelector::ScoreFunction PhysicalDeviceSelector::default_type_preference()
{
	return type_preference({
	    vk::PhysicalDeviceType::eDiscreteGpu,
	    vk::PhysicalDeviceType::eIntegratedGpu,
	    vk::PhysicalDeviceType::eVirtualGpu,
	    vk::PhysicalDeviceType::eCpu,
	    vk::PhysicalDeviceType::eOther,
	});
}

DeviceBuilder::DeviceBuilder(ContextBuilder &context_builder) :
    context_builder{context_builder}
{
}

DeviceBuilder &DeviceBuilder::request_queue(const vk::QueueFlags &supported_types, uint32_t count, const std::vector<vk::SurfaceKHR> &supported_presentation_surfaces, vk::DeviceQueueCreateFlags flags)
{
	queue_requests.emplace_back(QueueRequest{supported_types, count, supported_presentation_surfaces, flags});
	return *this;
}

DeviceBuilder &DeviceBuilder::request_extension(const char *extension_name, InclusionMode mode)
{
	if (mode == InclusionMode::Required)
	{
		required_extensions.emplace(extension_name);
	}
	else
	{
		optional_extensions.emplace(extension_name);
	}

	return *this;
}

DeviceBuilder &DeviceBuilder::request_layer(const char *layer_name, InclusionMode mode)
{
	if (mode == InclusionMode::Required)
	{
		required_layers.emplace(layer_name);
	}
	else
	{
		optional_layers.emplace(layer_name);
	}
	return *this;
}

DeviceBuilder &DeviceBuilder::request_layer(const char *extension_name, const char *layer_name, InclusionMode mode)
{
	request_extension(extension_name, mode);
	request_layer(layer_name, mode);
	return *this;
}

DeviceBuilder &DeviceBuilder::enable_features(std::function<void(vk::PhysicalDeviceFeatures &features)> &&callback)
{
	callback(enabled_features);
	return *this;
}

ContextBuilder &DeviceBuilder::done()
{
	return context_builder;
}

void DeviceBuilder::build(Context &context)
{
	if (!context.instance || !context.physical_device)
	{
		return;
	}

	if (queue_requests.empty())
	{
		throw std::runtime_error("No queues requested - must request at least one queue");
	}

	auto available_extensions = context.physical_device.enumerateDeviceExtensionProperties();

	std::set<const char *> enabled_extensions;

	// Must enable VK_KHR_portability_subset if it is available
	if (std::find_if(available_extensions.begin(), available_extensions.end(), [](const vk::ExtensionProperties &extension) {
		    return strcmp(extension.extensionName, "VK_KHR_portability_subset") == 0;
	    }) != available_extensions.end())
	{
		required_extensions.emplace("VK_KHR_portability_subset");
	}

	// Check that all required extensions are available
	for (auto &extension_name : required_extensions)
	{
		if (std::find_if(available_extensions.begin(), available_extensions.end(), [&extension_name](const vk::ExtensionProperties &extension) {
			    return strcmp(extension.extensionName, extension_name) == 0;
		    }) == available_extensions.end())
		{
			throw std::runtime_error("Required layer is not available: " + std::string(extension_name));
		}
	}

	for (auto &extension_name : required_extensions)
	{
		enabled_extensions.emplace(extension_name);
	}

	for (auto &extension_name : optional_extensions)
	{
		if (std::find_if(available_extensions.begin(), available_extensions.end(), [&extension_name](const vk::ExtensionProperties &extension) {
			    return strcmp(extension.extensionName, extension_name) == 0;
		    }) != available_extensions.end())
		{
			enabled_extensions.emplace(extension_name);
		}
	}

	auto available_layers = context.physical_device.enumerateDeviceLayerProperties();

	std::vector<const char *> enabled_layers;

	// Check that all required layers are available
	for (auto &layer_name : required_layers)
	{
		if (std::find_if(available_layers.begin(), available_layers.end(), [&layer_name](const vk::LayerProperties &layer) {
			    return strcmp(layer.layerName, layer_name) == 0;
		    }) == available_layers.end())
		{
			throw std::runtime_error("Required layer is not available: " + std::string(layer_name));
		}
	}

	for (auto &layer_name : required_layers)
	{
		enabled_layers.emplace_back(layer_name);
	}

	for (auto &layer_name : optional_layers)
	{
		if (std::find_if(available_layers.begin(), available_layers.end(), [&layer_name](const vk::LayerProperties &layer) {
			    return strcmp(layer.layerName, layer_name) == 0;
		    }) != available_layers.end())
		{
			enabled_layers.emplace_back(layer_name);
		}
	}

	auto queue_family_properties = context.physical_device.getQueueFamilyProperties();

	std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;

	std::vector<Context::QueueGroup> requested_queues;

	float queue_priority = 1.0f;

	for (auto &queue_request : queue_requests)
	{
		auto queue_family_index = std::find_if(queue_family_properties.begin(), queue_family_properties.end(), [&queue_request](const vk::QueueFamilyProperties &queue_family) {
			                          return queue_family.queueFlags & queue_request.queue_type;
		                          }) -
		                          queue_family_properties.begin();

		if (queue_family_index == queue_family_properties.size())
		{
			throw std::runtime_error("No queue family supports the requested queue type");
		}

		bool valid_surfaces = true;
		for (auto &surface : queue_request.supported_presentation_surfaces)
		{
			if (!context.physical_device.getSurfaceSupportKHR(queue_family_index, surface))
			{
				valid_surfaces = false;
				break;
			}
		}

		if (!valid_surfaces)
		{
			throw std::runtime_error("No queue family supports the requested presentation surface");
		}

		queue_create_infos.push_back(vk::DeviceQueueCreateInfo()
		                                 .setQueueFamilyIndex(queue_family_index)
		                                 .setQueueCount(queue_request.count)
		                                 .setPQueuePriorities(&queue_priority));

		Context::QueueGroup queue_group;
		queue_group.queue_family_index              = queue_family_index;
		queue_group.supported_queues                = queue_request.queue_type;
		queue_group.queues                          = std::vector<vk::Queue>(queue_request.count, VK_NULL_HANDLE);
		queue_group.supported_presentation_surfaces = queue_request.supported_presentation_surfaces;
		requested_queues.push_back(queue_group);
	}

	// validate that the device supports the requested features
	auto supported_features = context.physical_device.getFeatures();

#define FEATURE_CHECK(feature)                                   \
	if (enabled_features.feature && !supported_features.feature) \
	{                                                            \
		throw std::runtime_error(#feature " is not supported");  \
	}

	FEATURE_CHECK(robustBufferAccess);
	FEATURE_CHECK(fullDrawIndexUint32);
	FEATURE_CHECK(imageCubeArray);
	FEATURE_CHECK(independentBlend);
	FEATURE_CHECK(geometryShader);
	FEATURE_CHECK(tessellationShader);
	FEATURE_CHECK(sampleRateShading);
	FEATURE_CHECK(dualSrcBlend);
	FEATURE_CHECK(logicOp);
	FEATURE_CHECK(multiDrawIndirect);
	FEATURE_CHECK(drawIndirectFirstInstance);
	FEATURE_CHECK(depthClamp);
	FEATURE_CHECK(depthBiasClamp);
	FEATURE_CHECK(fillModeNonSolid);
	FEATURE_CHECK(depthBounds);
	FEATURE_CHECK(wideLines);
	FEATURE_CHECK(largePoints);
	FEATURE_CHECK(alphaToOne);
	FEATURE_CHECK(multiViewport);
	FEATURE_CHECK(samplerAnisotropy);
	FEATURE_CHECK(textureCompressionETC2);
	FEATURE_CHECK(textureCompressionASTC_LDR);
	FEATURE_CHECK(textureCompressionBC);
	FEATURE_CHECK(occlusionQueryPrecise);
	FEATURE_CHECK(pipelineStatisticsQuery);
	FEATURE_CHECK(vertexPipelineStoresAndAtomics);
	FEATURE_CHECK(fragmentStoresAndAtomics);
	FEATURE_CHECK(shaderTessellationAndGeometryPointSize);
	FEATURE_CHECK(shaderImageGatherExtended);
	FEATURE_CHECK(shaderStorageImageExtendedFormats);
	FEATURE_CHECK(shaderStorageImageMultisample);
	FEATURE_CHECK(shaderStorageImageReadWithoutFormat);
	FEATURE_CHECK(shaderStorageImageWriteWithoutFormat);
	FEATURE_CHECK(shaderUniformBufferArrayDynamicIndexing);
	FEATURE_CHECK(shaderSampledImageArrayDynamicIndexing);
	FEATURE_CHECK(shaderStorageBufferArrayDynamicIndexing);
	FEATURE_CHECK(shaderStorageImageArrayDynamicIndexing);
	FEATURE_CHECK(shaderClipDistance);
	FEATURE_CHECK(shaderCullDistance);
	FEATURE_CHECK(shaderFloat64);
	FEATURE_CHECK(shaderInt64);
	FEATURE_CHECK(shaderInt16);
	FEATURE_CHECK(shaderResourceResidency);
	FEATURE_CHECK(shaderResourceMinLod);
	FEATURE_CHECK(sparseBinding);
	FEATURE_CHECK(sparseResidencyBuffer);
	FEATURE_CHECK(sparseResidencyImage2D);
	FEATURE_CHECK(sparseResidencyImage3D);
	FEATURE_CHECK(sparseResidency2Samples);
	FEATURE_CHECK(sparseResidency4Samples);
	FEATURE_CHECK(sparseResidency8Samples);
	FEATURE_CHECK(sparseResidency16Samples);
	FEATURE_CHECK(sparseResidencyAliased);
	FEATURE_CHECK(variableMultisampleRate);
	FEATURE_CHECK(inheritedQueries);

	std::vector<const char *> enabled_extension_names{enabled_extensions.begin(), enabled_extensions.end()};
	std::vector<const char *> enabled_layer_names{enabled_layers.begin(), enabled_layers.end()};

	vk::DeviceCreateInfo device_create_info{
	    {},
	    static_cast<uint32_t>(queue_create_infos.size()),
	    queue_create_infos.data(),
	    static_cast<uint32_t>(enabled_layer_names.size()),
	    enabled_layer_names.data(),
	    static_cast<uint32_t>(enabled_extension_names.size()),
	    enabled_extension_names.data(),
	    &enabled_features,
	};

	context.device = context.physical_device.createDevice(device_create_info);

	volkLoadDevice(context.device);

	// get queue handles
	for (auto &queue : requested_queues)
	{
		for (uint32_t i = 0; i < queue.queues.size(); i++)
		{
			queue.queues[i] = context.device.getQueue(queue.queue_family_index, i);
		}
	}

	context.queue_groups = std::move(requested_queues);
}

ContextBuilder::ContextBuilder(const vk::Instance &instance) :
    starting_instance{instance}, instance_builder{*this}, physical_device_selector{*this}, device_builder{*this}
{
	static vk::DynamicLoader dl;

	static bool initialized = false;
	if (!initialized)
	{
		VULKAN_HPP_DEFAULT_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

		if (volkInitialize() != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to initialize volk.");
		}

		initialized = true;
	}
}

InstanceBuilder &ContextBuilder::configure_instance()
{
	return instance_builder;
}

PhysicalDeviceSelector &ContextBuilder::select_physical_device()
{
	return physical_device_selector;
}

DeviceBuilder &ContextBuilder::configure_device()
{
	return device_builder;
}

ContextPtr ContextBuilder::build()
{
	ContextPtr context = std::unique_ptr<Context>(new Context());

	if (starting_instance)
	{
		context->instance = starting_instance;
	}
	else
	{
		instance_builder.build(*context);
	}

	physical_device_selector.select(*context);

	device_builder.build(*context);

	return context;
}

}        // namespace vkb