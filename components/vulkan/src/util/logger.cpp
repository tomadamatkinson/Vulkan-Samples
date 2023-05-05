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

#include "logger.hpp"

VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
                                                              VkDebugUtilsMessageTypeFlagsEXT             message_type,
                                                              const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                                                              void                                       *user_data)
{
	if (!user_data)
	{
		return VK_FALSE;
	}

	auto *logger_callbacks = static_cast<vkb::LoggerCallbacks *>(user_data);

	if (logger_callbacks->simple_callback)
	{
		vkb::LogLevel log_level = vkb::LogLevel::Verbose;
		switch (message_severity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				log_level = vkb::LogLevel::Verbose;
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				log_level = vkb::LogLevel::Info;
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				log_level = vkb::LogLevel::Warning;
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				log_level = vkb::LogLevel::Error;
				break;
			default:
				log_level = vkb::LogLevel::Verbose;
				break;
		}

		if (callback_data && callback_data->pMessage)
		{
			logger_callbacks->simple_callback(log_level, callback_data->pMessage);
		}
	}

	if (logger_callbacks->debug_utils_callback)
	{
		logger_callbacks->debug_utils_callback(
		    static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(message_severity),
		    static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(message_type),
		    callback_data);
	}

	return VK_FALSE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT      flags,
                                              VkDebugReportObjectTypeEXT type,
                                              uint64_t                   object,
                                              size_t                     location,
                                              int32_t                    message_code,
                                              const char                *layer_prefix,
                                              const char                *message,
                                              void                      *user_data)
{
	if (!user_data)
	{
		return VK_FALSE;
	}

	auto *logger_callbacks = static_cast<vkb::LoggerCallbacks *>(user_data);

	if (logger_callbacks->simple_callback)
	{
		vkb::LogLevel log_level = vkb::LogLevel::Verbose;
		switch (flags)
		{
			case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
				log_level = vkb::LogLevel::Info;
				break;
			case VK_DEBUG_REPORT_WARNING_BIT_EXT:
				log_level = vkb::LogLevel::Warning;
				break;
			case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
				log_level = vkb::LogLevel::Warning;
				break;
			case VK_DEBUG_REPORT_ERROR_BIT_EXT:
				log_level = vkb::LogLevel::Error;
				break;
			case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
				log_level = vkb::LogLevel::Debug;
				break;
			default:
				log_level = vkb::LogLevel::Verbose;
				break;
		}

		logger_callbacks->simple_callback(log_level, message);
	}

	if (logger_callbacks->debug_report_callback)
	{
		logger_callbacks->debug_report_callback(
		    static_cast<vk::DebugReportFlagsEXT>(flags),
		    static_cast<vk::DebugReportObjectTypeEXT>(type),
		    object,
		    location,
		    message_code,
		    layer_prefix,
		    message);
	}

	return VK_FALSE;
}