/* Copyright (c) 2018-2023, Arm Limited and Contributors
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

#include <unordered_map>

namespace vkb
{
template <typename K, typename T, typename Container = std::unordered_map<K, T>>
class Map : public Container
{
  public:
	virtual ~Map() = default;

	typename Container::iterator find_or_emplace(const K &key, const T &value = {})
	{
		auto it = Container::find(key);
		if (it == Container::end())
		{
			it = Container::emplace(key, value).first;
		}
		return it;
	}

	bool contains(const K &key)
	{
		return Container::find(key) != Container::end();
	}
};
}        // namespace vkb