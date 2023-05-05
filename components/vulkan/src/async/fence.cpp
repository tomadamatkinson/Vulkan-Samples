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

#include "vulkan/async/fence.hpp"

#include "core/util/logging.hpp"

namespace vkb
{
Fence::Fence(ContextPtr &context, vk::FenceCreateFlags flags) :
    context{context}, signaled{flags & vk::FenceCreateFlagBits::eSignaled}
{
	vk::FenceCreateInfo create_info{flags};
	handle = context->device.createFence(create_info);
}

Fence::~Fence()
{
	if (context && handle)
	{
		context->device.destroyFence(handle);
	}
}

bool Fence::is_signaled() const
{
	if (context && handle)
	{
		signaled = context->device.getFenceStatus(handle) == vk::Result::eSuccess;
	}
	return signaled;
}

bool Fence::wait_until(uint64_t timeout) const
{
	if (!context || !handle)
	{
		LOGE("Fence has no context or handle");
		return true;
	}

	vk::Result return_value = context->device.waitForFences(handle, VK_TRUE, timeout);
	if (return_value == vk::Result::eSuccess)
	{
		signaled = true;
		return true;
	}
	if (return_value == vk::Result::eTimeout)
	{
		signaled = false;
		return false;
	}

	throw std::runtime_error("Failed to wait for fence");
}

vk::Fence Fence::release_handle()
{
	signaled = false;
	return handle;
}
}        // namespace vkb