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

#pragma once

#include <memory>
#include <set>
#include <unordered_map>

#include "vulkan/context.hpp"

namespace vkb
{
class ContextBuilder;

class InstanceBuilder
{
	friend class ContextBuilder;

	using Self = InstanceBuilder;

  public:
	InstanceBuilder(ContextBuilder &context_builder);

	/**
	 * @brief Set the application info for the instance
	 * @param application_info The application info to set
	 */
	Self &set_application_info(const vk::ApplicationInfo &application_info);

	/**
	 * @brief Request an extension to be enabled
	 * @param extension_name The name of the extension to enable
	 * @param mode Whether the extension is required or optional
	 */
	Self &request_extension(const char *extension_name, InclusionMode mode = InclusionMode::Required);

	/**
	 * @brief Request a layer to be enabled
	 * @param layer_name The name of the layer to enable
	 * @param mode Whether the layer is required or optional
	 */
	Self &request_layer(const char *layer_name, InclusionMode mode = InclusionMode::Required);

	/**
	 * @brief Request an extension to be enabled if a layer is enabled
	 * @param extension_name The name of the extension which owns the layer
	 * @param layer_name The name of the layer to enable
	 * @param mode Whether the extension is required or optional
	 */
	Self &request_layer(const char *extension_name, const char *layer_name, InclusionMode mode = InclusionMode::Required);

	/**
	 * @brief Add a logger callback to the instance
	 * @param logger_callbacks The logger callbacks to add
	 */
	Self &add_logger_callback(std::shared_ptr<LoggerCallbacks> &&logger_callbacks);

	/**
	 * @brief Finish configuring the instance
	 * @return The context builder
	 */
	ContextBuilder &done();

	static void enable_validation_layers(ContextBuilder &context_builder);
	static void enable_default_logger(ContextBuilder &context_builder);

  private:
	/**
	 * @brief Build the instance
	 * @param context The context to apply modifications to
	 */
	void build(Context &context);

	ContextBuilder &context_builder;

	vk::ApplicationInfo                           application_info;
	std::set<const char *>                        required_extensions;
	std::set<const char *>                        required_layers;
	std::set<const char *>                        optional_extensions;
	std::set<const char *>                        optional_layers;
	std::vector<std::shared_ptr<LoggerCallbacks>> logger_callbacks;
};

class PhysicalDeviceSelector
{
	friend class ContextBuilder;

  public:
	using Self = PhysicalDeviceSelector;

	// The score values are arbitrary, but should be consistent
	static const int RejectedScore  = -1;
	static const int DefaultScore   = 10;
	static const int PreferredScore = 100;

	// A score function takes a physical device and returns a score
	// The score should be higher for a better physical device
	// A score of -1 indicates that the physical device should be rejected
	using ScoreFunction = std::function<int(const vk::PhysicalDevice &physical_device)>;

  public:
	PhysicalDeviceSelector(ContextBuilder &context_builder);

	/**
	 * @brief Select a physical device based on a score function
	 * @note The score function should return a higher value for a better physical device
	 * @param score_function The score function to use
	 * @return The context builder
	 */
	Self &score(const ScoreFunction &score_function);

	/**
	 * @brief Finish configuring the physical device selector
	 * @return The context builder
	 */
	ContextBuilder &done();

	static ScoreFunction type_preference(const std::vector<vk::PhysicalDeviceType> &priority_order);
	static ScoreFunction default_type_preference();

	// static ScoreFunction supports_surface_presentation(const vk::SurfaceKHR &surface);

  private:
	/**
	 * @brief Select a physical device
	 * @param context The context to apply modifications to
	 */
	void select(Context &context);

	ContextBuilder &context_builder;

	std::vector<ScoreFunction> score_functions;
};

class DeviceBuilder
{
	friend class ContextBuilder;

  public:
	using Self = DeviceBuilder;

  private:
	struct QueueRequest
	{
		vk::QueueFlags              queue_type;
		uint32_t                    count;
		std::vector<vk::SurfaceKHR> supported_presentation_surfaces;
		vk::DeviceQueueCreateFlags  flags;
	};

  public:
	DeviceBuilder(ContextBuilder &context_builder);

	/**
	 * @brief Request a queue
	 * @param supported_types The required support queue types for the requested queue
	 * @param count The number of queues to request
	 * @param supported_presentation_surfaces The surfaces which the queue must support presenting to
	 * @param flags Flags to use when creating the queue
	 */
	Self &request_queue(const vk::QueueFlags &supported_types, uint32_t count, const std::vector<vk::SurfaceKHR> &supported_presentation_surfaces = {}, vk::DeviceQueueCreateFlags flags = {});

	/**
	 * @brief Request an extension to be enabled
	 * @param extension_name The name of the extension to enable
	 * @param mode Whether the extension is required or optional
	 */
	Self &request_extension(const char *extension_name, InclusionMode mode = InclusionMode::Required);

	/**
	 * @brief Request a layer to be enabled
	 * @param layer_name The name of the layer to enable
	 * @param mode Whether the layer is required or optional
	 */
	Self &request_layer(const char *layer_name, InclusionMode mode = InclusionMode::Required);

	/**
	 * @brief Request an extension to be enabled if a layer is enabled
	 * @param extension_name The name of the extension which owns the layer
	 * @param layer_name The name of the layer to enable
	 * @param mode Whether the extension is required or optional
	 */
	Self &request_layer(const char *extension_name, const char *layer_name, InclusionMode mode = InclusionMode::Required);

	/**
	 * @brief Toggle device features
	 * @param callback A callback which takes a reference to the device features
	 */
	Self &enable_features(std::function<void(vk::PhysicalDeviceFeatures &features)> &&callback);

	ContextBuilder &done();

  private:
	void build(Context &context);

	ContextBuilder &context_builder;

	std::vector<QueueRequest> queue_requests;

	std::set<const char *> required_extensions;
	std::set<const char *> required_layers;
	std::set<const char *> optional_extensions;
	std::set<const char *> optional_layers;

	vk::PhysicalDeviceFeatures enabled_features;
};

class ContextBuilder
{
  public:
	using Self = ContextBuilder;

	/**
	 * @brief Construct a context builder
	 * @param instance Optionally create a new context from an existing instance - multi gpu support
	 */
	ContextBuilder(const vk::Instance &instance = nullptr);

	/**
	 * @brief Configure the isntance
	 * @return The instance builder
	 */
	InstanceBuilder &configure_instance();

	/**
	 * @brief Select a physical device
	 * @return The physical device selector
	 */
	PhysicalDeviceSelector &select_physical_device();

	/**
	 * @brief Configure the device
	 * @return The device builder
	 */
	DeviceBuilder &configure_device();

	/**
	 * @brief Build the context using the current configuration
	 * @return The context
	 */
	ContextPtr build();

  private:
	InstanceBuilder        instance_builder;
	PhysicalDeviceSelector physical_device_selector;
	DeviceBuilder          device_builder;

	vk::Instance starting_instance;
	Context      context;
};
}        // namespace vkb