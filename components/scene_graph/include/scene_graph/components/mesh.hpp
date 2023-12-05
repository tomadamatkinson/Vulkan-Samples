/* Copyright (c) 2018-2023, Arm Limited and Contributors
 * Copyright (c) 2023, Tom Atkinson
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

#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include "scene_graph/assets/image.hpp"

namespace vkb
{
namespace sg
{

// How the alpha value of the main factor and texture should be interpreted
enum class AlphaMode
{
	/// Alpha value is ignored
	Opaque,
	/// Either full opaque or fully transparent
	Mask,
	/// Output is combined with the background
	Blend
};

// A texture is an image with a sampler
// The image data is not owned by the texture
struct Texture
{
	struct Sampler
	{
		vk::Filter             mag_filter{vk::Filter::eLinear};
		vk::Filter             min_filter{vk::Filter::eLinear};
		vk::SamplerMipmapMode  mipmap_mode{vk::SamplerMipmapMode::eLinear};
		vk::SamplerAddressMode address_mode_u{vk::SamplerAddressMode::eRepeat};
		vk::SamplerAddressMode address_mode_v{vk::SamplerAddressMode::eRepeat};
		vk::SamplerAddressMode address_mode_w{vk::SamplerAddressMode::eRepeat};
		float                  mip_lod_bias{0.0f};
		bool                   anisotropy_enable{false};
		float                  max_anisotropy{0.0f};
		bool                   compare_enable{false};
		vk::CompareOp          compare_op{vk::CompareOp::eNever};
		float                  min_lod{0.0f};
		float                  max_lod{0.0f};
		vk::BorderColor        border_color{vk::BorderColor::eFloatOpaqueWhite};
		bool                   unnormalized_coordinates{false};
	} sampler;

	Image image;
};

// A mesh is a collection of vertices and indices
// It also contains a material
// A material is a collection of textures and parameters
struct Mesh
{
	struct Indices
	{
		vk::IndexType type{vk::IndexType::eUint16};
		uint32_t      count{0U};
		DataView      data;

	} indices;

	struct Vertex
	{
		DataView data;

		// A vertex attribute is a piece of data associated with a vertex
		// It describes the usecase of the data
		struct Attribute
		{
			vk::Format format = vk::Format::eUndefined;
			uint32_t   stride = 0;
			uint32_t   offset = 0;
		};

		std::unordered_map<std::string, Attribute> attributes;
	} vertex;

	struct Material
	{
		std::unordered_map<std::string, Texture> textures;

		/// Emissive color of the material
		glm::vec3 emissive{0.0f, 0.0f, 0.0f};

		/// Whether the material is double sided
		bool double_sided{false};

		/// Cutoff threshold when in Mask mode
		float alpha_cutoff{0.5f};

		/// Alpha rendering mode
		AlphaMode alpha_mode{AlphaMode::Opaque};

		glm::vec4 base_color_factor{0.0f, 0.0f, 0.0f, 0.0f};

		float metallic_factor{0.0f};

		float roughness_factor{0.0f};
	} material;
};
}        // namespace sg
}        // namespace vkb
