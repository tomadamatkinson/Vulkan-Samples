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

#define PLATFORM_LINUX
#include <core/platform/entrypoint.hpp>
#include <core/util/logging.hpp>
#include <vulkan/shaders/shader_compiler.hpp>

std::string TRIANGLE_VERT = R"(#version 320 es
precision mediump float;

layout(location = 0) in vec2 pos;

layout(location = 0) out vec3 out_color;

void main()
{
    gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);

    out_color = vec3(1.0, 0.0, 0.0);
})";

std::string TRIANGLE_FRAG = R"(#version 320 es
precision mediump float;

layout(location = 0) in vec3 in_color;

layout(location = 0) out vec4 out_color;

void main()
{
	out_color = vec4(in_color, 1.0);
})";

CUSTOM_MAIN(platform_context)
{
	vkb::GlslShaderCompiler compiler;
	std::vector<uint32_t>   spirv = compiler.compile(vk::ShaderStageFlagBits::eVertex, TRIANGLE_VERT, "main", {});
	if (spirv.empty())
	{
		LOGE("Failed to compile vertex shader");
		return 1;
	}

	spirv = compiler.compile(vk::ShaderStageFlagBits::eFragment, TRIANGLE_FRAG, "main", {});
	if (spirv.empty())
	{
		LOGE("Failed to compile fragment shader");
		return 1;
	}

	return 0;
}