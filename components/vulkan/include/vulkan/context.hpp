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

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include <volk.h>
#include <vulkan/vulkan.hpp>

namespace vkb
{
enum class LogLevel
{
	Verbose,
	Debug,
	Info,
	Warning,
	Error,
};

using SimpleLogCallback      = std::function<void(vkb::LogLevel, const char *)>;
using DebugUtilsLogCallback  = std::function<void(vk::DebugUtilsMessageSeverityFlagBitsEXT, vk::DebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT *)>;
using DebugReportLogCallback = std::function<void(vk::DebugReportFlagsEXT, vk::DebugReportObjectTypeEXT, uint64_t, size_t, int32_t, const char *, const char *)>;

/**
 * @brief A set of callbacks for logging Vulkan messages
 * @note It is not guaranteed that all callbacks will be called for every message
 *       as extension specific callbacks may or may not be available.
 */
struct LoggerCallbacks
{
	SimpleLogCallback      simple_callback{nullptr};
	DebugUtilsLogCallback  debug_utils_callback{nullptr};
	DebugReportLogCallback debug_report_callback{nullptr};
};

class ContextBuilder;

// How layers and extensions are handled
enum class InclusionMode
{
	Optional,        // Can be enabled, but not required
	Required,        // Must be enabled
};

/**
 * @brief A context contains the core Vulkan objects required for most operations
 */
class Context final
{
	friend class ContextBuilder;
	friend class InstanceBuilder;
	friend class DeviceBuilder;

  public:
	using CleanupCallback = std::function<void()>;

	~Context();

	vk::Instance       instance;
	vk::PhysicalDevice physical_device;
	vk::Device         device;

	void add_cleanup_callback(CleanupCallback &&callback);

	vk::Queue get_queue(vk::QueueFlags supported_types) const;
	uint32_t  get_queue_family_index(vk::Queue queue) const;

  private:
	// Only allow context to be created by the context builder
	// Prevents misuse of the Context class
	Context() = default;

	std::vector<CleanupCallback> cleanup_callbacks;

	struct QueueGroup
	{
		uint32_t                    queue_family_index;
		vk::QueueFlags              supported_queues;
		std::vector<vk::Queue>      queues;
		std::vector<vk::SurfaceKHR> supported_presentation_surfaces;
	};

	std::vector<QueueGroup> queue_groups;

	// keep the loggers alive for the lifetime of the context
	// void* user_data == LoggerCallbacks*
	std::shared_ptr<LoggerCallbacks> logging_callbacks;
	vk::DebugUtilsMessengerEXT       debug_utils_messenger;
	vk::DebugReportCallbackEXT       debug_report_callback;
};

using ContextPtr     = std::shared_ptr<Context>;
using WeakContextPtr = std::weak_ptr<Context>;
}        // namespace vkb