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

#include "vulkan/context.hpp"

#include "vulkan/async/synchronization.hpp"

namespace vkb
{

class Fence final : public SynchronizationPoint
{
  public:
	Fence(ContextPtr &context, vk::FenceCreateFlags flags = {});
	~Fence();

	bool is_signaled() const override;

	bool wait_until(uint64_t timeout) const override;

	vk::Fence release_handle();

  private:
	ContextPtr context;
	vk::Fence  handle;

	// used to track the state of the fence
	mutable bool signaled{false};
};

using FencePtr = std::shared_ptr<Fence>;
}        // namespace vkb