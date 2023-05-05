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

#include <stdint.h>

#include <memory>
#include <vector>

namespace vkb
{
class SynchronizationPoint
{
  public:
	SynchronizationPoint()          = default;
	virtual ~SynchronizationPoint() = default;

	void wait() const
	{
		wait_until(UINT64_MAX);
	}

	virtual bool wait_until(uint64_t timeout) const = 0;

	virtual bool is_signaled() const = 0;
};

using SynchronizationPointPtr = std::shared_ptr<SynchronizationPoint>;
using SyncPtr                 = SynchronizationPointPtr;

class SynchronizationGroup final : public SynchronizationPoint
{
  public:
	SynchronizationGroup(std::vector<SyncPtr> &&fences);

	bool is_signaled() const override;

	bool wait_until(uint64_t timeout) const override;

  private:
	std::vector<SyncPtr> fences;
};

using SynchronizationGroupPtr = std::shared_ptr<SynchronizationGroup>;
using SyncGroupPtr            = SynchronizationGroupPtr;
}        // namespace vkb