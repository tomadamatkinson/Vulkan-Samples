/* Copyright (c) 2019-2023, Arm Limited and Contributors
 * Copyright (c) 2023, Thomas Atkinson
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

#include <cstdint>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace vkb
{

class ShaderCompiler
{
  public:
	virtual ~ShaderCompiler() = default;

	virtual std::vector<uint32_t> compile(vk::ShaderStageFlagBits stage, const std::string &source, const std::string &entry_point, const std::vector<std::string> &definitions) = 0;
};

class GlslShaderCompiler final : public ShaderCompiler
{
  public:
	std::vector<uint32_t> compile(vk::ShaderStageFlagBits stage, const std::string &source, const std::string &entry_point, const std::vector<std::string> &definitions) override;
};

};        // namespace vkb