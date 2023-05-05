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

#include <unordered_map>

#include <vulkan/context.hpp>
#include <vulkan/pools/memory_pool.hpp>

#define TASK_RESOURCE_HANDLE(name)                            \
	namespace vkb                                             \
	{                                                         \
	struct name                                               \
	{                                                         \
		uint32_t id;                                          \
	};                                                        \
	inline bool operator==(const name &lhs, const name &rhs)  \
	{                                                         \
		return lhs.id == rhs.id;                              \
	};                                                        \
	}                                                         \
	namespace std                                             \
	{                                                         \
	template <>                                               \
	struct hash<vkb::name>                                    \
	{                                                         \
		std::size_t operator()(const vkb::name &handle) const \
		{                                                     \
			return std::hash<uint32_t>()(handle.id);          \
		}                                                     \
	};                                                        \
	}

TASK_RESOURCE_HANDLE(TransientImageHandle)
TASK_RESOURCE_HANDLE(TransientBufferHandle)
TASK_RESOURCE_HANDLE(AliasedImageHandle)
TASK_RESOURCE_HANDLE(AliasedBufferHandle)

namespace vkb
{

class TaskRegistry
{
  public:
	TaskRegistry(ContextPtr &context, MemoryPoolPtr &memory_pool) :
	    context{context}, memory_pool{memory_pool}
	{}

	virtual ~TaskRegistry() = default;

	TransientImageHandle request_image(vk::ImageUsageFlags usage, vk::Format format, vk::Extent3D extent)
	{
		TransientImageHandle handle{next_transient_image_id++};
		requested_images[handle] = {usage, format, extent};
		return handle;
	}

	TransientBufferHandle request_buffer(vk::BufferUsageFlags usage, vk::DeviceSize size)
	{
		TransientBufferHandle handle{next_transient_buffer_id++};
		requested_buffers[handle] = {usage, size};
		return handle;
	}

	AliasedImageHandle read(TransientImageHandle handle)
	{
		AliasedImageHandle alias{next_alias_image_id++};
		aliased_images[alias] = handle;
		return alias;
	}

	AliasedImageHandle write(TransientImageHandle handle)
	{
		AliasedImageHandle alias{next_alias_image_id++};
		aliased_images[alias] = handle;
		return alias;
	}

	AliasedBufferHandle read(TransientBufferHandle handle)
	{
		AliasedBufferHandle alias{next_alias_buffer_id++};
		unallocated_alias_buffers[alias] = handle;
		return alias;
	}

	AliasedBufferHandle write(TransientBufferHandle handle)
	{
		AliasedBufferHandle alias{next_alias_buffer_id++};
		unallocated_alias_buffers[alias] = handle;
		return alias;
	}

	const vk::Image *image(AliasedImageHandle handle)
	{
		if (auto image = find_or_create_image(handle))
		{
			return &image->image;
		}
		return nullptr;
	}

	const vk::ImageView *image_view(AliasedImageHandle handle)
	{
		if (auto image = find_or_create_image(handle))
		{
			return &image->view;
		}
		return nullptr;
	}

	const vk::Buffer *buffer(AliasedBufferHandle handle)
	{
		return nullptr;
	}

	const vk::BufferView *buffer_view(AliasedBufferHandle handle)
	{
		return nullptr;
	}

  private:
	ContextPtr    context;
	MemoryPoolPtr memory_pool;

	uint32_t next_transient_image_id{0};
	uint32_t next_transient_buffer_id{0};

	uint32_t next_alias_image_id{0};
	uint32_t next_alias_buffer_id{0};

	struct ImageRequest
	{
		vk::ImageUsageFlags usage;
		vk::Format          format;
		vk::Extent3D        extent;
	};

	std::unordered_map<TransientImageHandle, ImageRequest>            requested_images;
	std::unordered_map<AliasedImageHandle, TransientImageHandle>      aliased_images;
	std::unordered_map<TransientImageHandle, vkb::ImageAllocationPtr> allocated_images;

	vkb::ImageAllocationPtr find_or_create_image(AliasedImageHandle handle)
	{
		auto aliased_it = aliased_images.find(handle);
		if (aliased_it == aliased_images.end())
		{
			return nullptr;
		}

		auto allocated_it = allocated_images.find(aliased_it->second);
		if (allocated_it == allocated_images.end())
		{
			vk::ImageCreateInfo create_info{};
			create_info.imageType     = vk::ImageType::e2D;
			create_info.format        = requested_images[aliased_it->second].format;
			create_info.extent        = requested_images[aliased_it->second].extent;
			create_info.mipLevels     = 1;
			create_info.arrayLayers   = 1;
			create_info.samples       = vk::SampleCountFlagBits::e1;
			create_info.tiling        = vk::ImageTiling::eOptimal;
			create_info.usage         = requested_images[aliased_it->second].usage;
			create_info.sharingMode   = vk::SharingMode::eExclusive;
			create_info.initialLayout = vk::ImageLayout::eUndefined;

			auto image_allocation = memory_pool->allocate_image(create_info, vkb::MemoryUsage::GPU_ONLY);
			if (!image_allocation)
			{
				return nullptr;
			}

			allocated_it = allocated_images.emplace(aliased_it->second, std::move(image_allocation)).first;
		}

		return allocated_it->second;
	}

	struct BufferRequest
	{
		vk::BufferUsageFlags usage;
		vk::DeviceSize       size;
	};

	std::unordered_map<TransientBufferHandle, BufferRequest>            requested_buffers;
	std::unordered_map<AliasedBufferHandle, TransientBufferHandle>      unallocated_alias_buffers;
	std::unordered_map<TransientBufferHandle, vkb::BufferAllocationPtr> allocated_buffers;
};
}        // namespace vkb