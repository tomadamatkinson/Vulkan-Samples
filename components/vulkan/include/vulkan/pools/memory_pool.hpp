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

#include <mutex>
#include <set>

#include <core/util/map.hpp>

#include "vulkan/context.hpp"

#include <vk_mem_alloc.h>

namespace vkb
{
// Define a set of memory usages that we want to support and their corresponding Vulkan memory property flags
enum class MemoryUsage : VkMemoryPropertyFlags
{
	GPU_ONLY,          // Prefers VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	CPU_ONLY,          // Guarantees VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	CPU_TO_GPU,        // Guarantees VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT Prefers VK_MEMORY_PROPERTY_HOST_CACHED_BIT
	LAZY_ALLOC,        // Prefers VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT - Lazily allocated by the implementation - useful for transient memory
	AUTO,              // Automatically chooses the best memory type for the usage
};

class MemoryPool;

class BufferAllocation final
{
	friend class MemoryPool;

  public:
	~BufferAllocation();

	vk::Buffer buffer{VK_NULL_HANDLE};

	vk::DeviceSize size() const
	{
		return byte_size;
	}

	bool is_valid() const
	{
		return buffer && allocation != VK_NULL_HANDLE;
	}

	void update(const void *data, size_t size);

	template <typename T>
	void update(const std::vector<T> &data);

	template <typename T>
	void update(const T &data);

  protected:
	BufferAllocation(MemoryPool &pool, vk::Buffer buffer, VmaAllocation allocation, vk::DeviceSize size);

	MemoryPool    &pool;
	VmaAllocation  allocation;
	vk::DeviceSize byte_size;
};

using BufferAllocationPtr = std::shared_ptr<BufferAllocation>;

class ImageAllocation final
{
	friend class MemoryPool;

  public:
	~ImageAllocation();

	vk::Image     image{VK_NULL_HANDLE};
	vk::ImageView view{VK_NULL_HANDLE};

	bool is_valid() const
	{
		return image && allocation != VK_NULL_HANDLE;
	}

  protected:
	ImageAllocation(MemoryPool &pool, vk::Image image, vk::ImageView view, VmaAllocation allocation);

	MemoryPool   &pool;
	VmaAllocation allocation{VK_NULL_HANDLE};
};

using ImageAllocationPtr = std::shared_ptr<ImageAllocation>;

/**
 * @brief A class that manages the allocation and aliasing of Vulkan memory
 */
class MemoryPool
{
	friend class BufferAllocation;
	friend class ImageAllocation;

  public:
	static std::shared_ptr<MemoryPool> make(ContextPtr &context);

	virtual ~MemoryPool() = default;

	ImageAllocationPtr allocate_image(const vk::ImageCreateInfo &image_create_info, MemoryUsage usage);

	BufferAllocationPtr allocate_buffer(const vk::BufferCreateInfo &buffer_create_info, MemoryUsage usage);

	static void poll_stats(ContextPtr &context, uint32_t interval_ms = 16);

  private:
	MemoryPool(ContextPtr context);

	ContextPtr context;

	static VmaAllocator *allocator(ContextPtr &context);

	void update_buffer(BufferAllocation &allocation, const void *data, size_t size);

	template <typename T>
	void update_buffer(BufferAllocation &allocation, const std::vector<T> &data)
	{
		update_buffer(allocation, data.data(), data.size() * sizeof(T));
	}

	template <typename T>
	void update_buffer(BufferAllocation &allocation, const T &data)
	{
		update_buffer(allocation, &data, sizeof(T));
	}

	void free_buffer(BufferAllocation &allocation);
	void free_image(ImageAllocation &allocation);
};

using MemoryPoolPtr = std::shared_ptr<MemoryPool>;

inline void BufferAllocation::update(const void *data, size_t size)
{
	pool.update_buffer(*this, data, size);
}

template <typename T>
inline void BufferAllocation::update(const std::vector<T> &data)
{
	pool.update_buffer(*this, data);
}

template <typename T>
inline void BufferAllocation::update(const T &data)
{
	pool.update_buffer(*this, &data);
}

}        // namespace vkb
