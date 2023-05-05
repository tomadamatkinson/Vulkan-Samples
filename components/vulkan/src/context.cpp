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

#include "vulkan/context.hpp"

namespace vkb
{
Context::~Context()
{
	// clean up requested resources in reverse order
	for (auto it = cleanup_callbacks.rbegin(); it != cleanup_callbacks.rend(); ++it)
	{
		if ((*it))
		{
			(*it)();
		}
	}

	if (device)
	{
		device.destroy();
	}

	if (debug_utils_messenger)
	{
		instance.destroyDebugUtilsMessengerEXT(debug_utils_messenger);
	}
	if (debug_report_callback)
	{
		instance.destroyDebugReportCallbackEXT(debug_report_callback);
	}

	if (instance)
	{
		instance.destroy();
	}
}

void Context::add_cleanup_callback(CleanupCallback &&callback)
{
	cleanup_callbacks.push_back(callback);
}

vk::Queue Context::get_queue(vk::QueueFlags supported_types) const
{
	for (auto &group : queue_groups)
	{
		if (group.supported_queues & supported_types && group.queues.size() > 0)
		{
			// TODO: make this more complex
			return group.queues[0];
		}
	}

	return vk::Queue{VK_NULL_HANDLE};
}

uint32_t Context::get_queue_family_index(vk::Queue queue) const
{
	for (auto &group : queue_groups)
	{
		for (auto q : group.queues)
		{
			if (q == queue)
			{
				return group.queue_family_index;
			}
		}
	}

	return VK_QUEUE_FAMILY_IGNORED;
}

}        // namespace vkb