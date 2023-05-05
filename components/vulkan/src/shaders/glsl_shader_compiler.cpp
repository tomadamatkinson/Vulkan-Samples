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

#include "vulkan/shaders/shader_compiler.hpp"

#include <core/util/logging.hpp>

#include <SPIRV/GLSL.std.450.h>
#include <SPIRV/GlslangToSpv.h>

#include <StandAlone/DirStackFileIncluder.h>
#include <glslang/Include/ShHandle.h>
#include <glslang/OSDependent/osinclude.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>

namespace vkb
{
namespace
{
inline EShLanguage FindShaderLanguage(vk::ShaderStageFlagBits stage)
{
	switch (stage)
	{
		case vk::ShaderStageFlagBits::eVertex:
			return EShLangVertex;

		case vk::ShaderStageFlagBits::eTessellationControl:
			return EShLangTessControl;

		case vk::ShaderStageFlagBits::eTessellationEvaluation:
			return EShLangTessEvaluation;

		case vk::ShaderStageFlagBits::eGeometry:
			return EShLangGeometry;

		case vk::ShaderStageFlagBits::eFragment:
			return EShLangFragment;

		case vk::ShaderStageFlagBits::eCompute:
			return EShLangCompute;

		case vk::ShaderStageFlagBits::eRaygenKHR:
			return EShLangRayGen;

		case vk::ShaderStageFlagBits::eAnyHitKHR:
			return EShLangAnyHit;

		case vk::ShaderStageFlagBits::eClosestHitKHR:
			return EShLangClosestHit;

		case vk::ShaderStageFlagBits::eMissKHR:
			return EShLangMiss;

		case vk::ShaderStageFlagBits::eIntersectionKHR:
			return EShLangIntersect;

		case vk::ShaderStageFlagBits::eCallableKHR:
			return EShLangCallable;

		case vk::ShaderStageFlagBits::eMeshNV:
			return EShLangMesh;

		case vk::ShaderStageFlagBits::eTaskNV:
			return EShLangTask;

		default:
			return EShLangVertex;
	}
}
}        // namespace

std::vector<uint32_t> GlslShaderCompiler::compile(vk::ShaderStageFlagBits stage, const std::string &source, const std::string &entry_point, const std::vector<std::string> &definitions)
{
	glslang::InitializeProcess();

	EShMessages messages = static_cast<EShMessages>(EShMsgDefault | EShMsgVulkanRules | EShMsgSpvRules);

	EShLanguage language    = FindShaderLanguage(stage);
	std::string source_copy = std::string(source.begin(), source.end());

	const char *file_name_list[1] = {""};
	const char *shader_source     = reinterpret_cast<const char *>(source_copy.data());

	std::string              definitions_string;
	std::vector<std::string> processes;

	for (auto &definition : definitions)
	{
		definitions_string += "#define " + definition + "\n";
		processes.push_back("D" + definition);
	}

	glslang::TShader shader(language);
	shader.setStringsWithLengthsAndNames(&shader_source, nullptr, file_name_list, 1);
	shader.setEntryPoint(entry_point.c_str());
	shader.setSourceEntryPoint(entry_point.c_str());
	shader.setPreamble(definitions_string.c_str());
	shader.addProcesses(processes);
	shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);

	DirStackFileIncluder includeDir;
	includeDir.pushExternalLocalDirectory("shaders");

	if (!shader.parse(GetDefaultResources(), 100, false, messages, includeDir))
	{
		LOGE("Failed to parse shader:\n\tInfo: {}\n\tDebug: {}", shader.getInfoLog(), shader.getInfoDebugLog());
		return {};
	}

	// Add shader to new program object.
	glslang::TProgram program;
	program.addShader(&shader);

	// Link program.
	if (!program.link(messages))
	{
		LOGE("Failed to link shader:\n\tInfo: {}\n\tDebug: {}", shader.getInfoLog(), shader.getInfoDebugLog());
		return {};
	}

	glslang::TIntermediate *intermediate = program.getIntermediate(language);

	// Translate to SPIRV.
	if (!intermediate)
	{
		if (shader.getInfoLog())
		{
			LOGW("Shader info log: {}", shader.getInfoLog());
		}

		if (shader.getInfoDebugLog())
		{
			LOGW("Shader info debug log: {}", shader.getInfoDebugLog());
		}

		return {};
	}

	spv::SpvBuildLogger logger;

	std::vector<uint32_t> spirv;

	glslang::GlslangToSpv(*intermediate, spirv, &logger);

	if (logger.getAllMessages().size() > 0)
	{
		LOGW("Shader Glsl to Spirv: {}", logger.getAllMessages());
	}
	// Shutdown glslang library.
	glslang::FinalizeProcess();

	return spirv;
}

}        // namespace vkb