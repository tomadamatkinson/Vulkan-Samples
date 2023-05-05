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

#include "vulkan/pools/memory_pool.hpp"

#include <sstream>

#include <core/util/logging.hpp>
#include <core/util/profiling.hpp>

static const char *MEMORY_POOL_BUFFER_ALLOCATIONS_NAME = "Buffer Allocations";
static const char *MEMORY_POOL_IMAGE_ALLOCATIONS_NAME  = "Image Allocations";

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace vkb
{
BufferAllocation::BufferAllocation(MemoryPool &pool, vk::Buffer buffer, VmaAllocation allocation, vk::DeviceSize size) :
    pool{pool}, buffer{buffer}, allocation{allocation}, byte_size{size}
{
}

BufferAllocation::~BufferAllocation()
{
	if (is_valid())
	{
		pool.free_buffer(*this);
	}
}

ImageAllocation::~ImageAllocation()
{
	if (is_valid())
	{
		pool.free_image(*this);
	}
}

ImageAllocation::ImageAllocation(MemoryPool &pool, vk::Image image, vk::ImageView view, VmaAllocation allocation) :
    pool{pool},
    image{image},
    view{view},
    allocation{allocation}
{}

VmaAllocator *MemoryPool::allocator(ContextPtr &context)
{
	static VmaAllocator allocator = VK_NULL_HANDLE;

	if (allocator == VK_NULL_HANDLE)
	{
		VmaVulkanFunctions vulkan_functions    = {};
		vulkan_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		vulkan_functions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocator_info = {};
		allocator_info.physicalDevice         = context->physical_device;
		allocator_info.device                 = context->device;
		allocator_info.instance               = context->instance;
		allocator_info.vulkanApiVersion       = VK_API_VERSION_1_2;
		allocator_info.pVulkanFunctions       = &vulkan_functions;

		if (vmaCreateAllocator(&allocator_info, &allocator) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create allocator");
		}

		context->add_cleanup_callback([&]() { vmaDestroyAllocator(allocator); });
	}

	return &allocator;
}

void MemoryPool::poll_stats(ContextPtr &context, uint32_t interval_ms)
{
	// rate limit to 20 fps
	static std::chrono::time_point last_poll_time = std::chrono::high_resolution_clock::now();
	if (std::chrono::high_resolution_clock::now() - last_poll_time < std::chrono::milliseconds(interval_ms))
	{
		return;
	}
	last_poll_time = std::chrono::high_resolution_clock::now();

	auto *allocator = MemoryPool::allocator(context);

	auto properties = context->physical_device.getMemoryProperties();

	VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
	vmaGetHeapBudgets(*allocator, budgets);

	static std::vector<std::unique_ptr<std::string>> heap_usage_labels;
	if (heap_usage_labels.size() != properties.memoryHeapCount)
	{
		for (uint32_t i = 0; i < properties.memoryHeapCount; ++i)
		{
			vk::MemoryPropertyFlags flags;
			for (uint32_t j = 0; j < properties.memoryTypeCount; ++j)
			{
				if (properties.memoryTypes[j].heapIndex == i)
				{
					flags |= properties.memoryTypes[j].propertyFlags;
				}
			}

			std::stringstream ss;
			ss << "Heap (" << i << ") " << vk::to_string(flags);

			heap_usage_labels.push_back(std::make_unique<std::string>(ss.str()));
		}
	}

	for (uint32_t i = 0; i < properties.memoryHeapCount; ++i)
	{
		Plot<int64_t>::plot(heap_usage_labels[i]->c_str(), budgets[i].usage, tracy::PlotFormatType::Memory);
	}
}

std::shared_ptr<MemoryPool> MemoryPool::make(ContextPtr &context)
{
	return std::shared_ptr<MemoryPool>(new MemoryPool{context});
}

MemoryPool::MemoryPool(ContextPtr context) :
    context{context}
{
}

VmaMemoryUsage to_vma_memory_usage(MemoryUsage usage)
{
	switch (usage)
	{
		case MemoryUsage::CPU_TO_GPU:
			return VMA_MEMORY_USAGE_CPU_TO_GPU;
		case MemoryUsage::GPU_ONLY:
			return VMA_MEMORY_USAGE_GPU_ONLY;
		case MemoryUsage::CPU_ONLY:
			return VMA_MEMORY_USAGE_CPU_ONLY;
		case MemoryUsage::LAZY_ALLOC:
			return VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
		case MemoryUsage::AUTO:
			return VMA_MEMORY_USAGE_AUTO;
		default:
			throw std::runtime_error("Invalid memory usage");
	}
}

ImageAllocationPtr MemoryPool::allocate_image(const vk::ImageCreateInfo &image_create_info, MemoryUsage usage)
{
	auto *alloc = allocator(context);

	VkImageCreateInfo create_info = image_create_info;

	VmaAllocationCreateInfo alloc_info = {};
	alloc_info.usage                   = to_vma_memory_usage(usage);

	VkImage       image;
	VmaAllocation allocation;
	vk::Result    result = static_cast<vk::Result>(vmaCreateImage(*alloc, &create_info, &alloc_info, &image, &allocation, nullptr));
	if (result != vk::Result::eSuccess)
	{
		LOGE("Failed to allocate image: {}", vk::to_string(result));
		throw std::runtime_error("Failed to allocate image");
	}

	Plot<int64_t>::increment(MEMORY_POOL_IMAGE_ALLOCATIONS_NAME, 1);

	vk::ImageViewCreateInfo view_create_info;
	view_create_info.image                           = image;
	view_create_info.viewType                        = vk::ImageViewType::e2D;
	view_create_info.format                          = image_create_info.format;
	view_create_info.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
	view_create_info.subresourceRange.baseMipLevel   = 0;
	view_create_info.subresourceRange.levelCount     = image_create_info.mipLevels;
	view_create_info.subresourceRange.baseArrayLayer = 0;
	view_create_info.subresourceRange.layerCount     = image_create_info.arrayLayers;

	auto image_view = context->device.createImageView(view_create_info);

	return std::shared_ptr<ImageAllocation>(new ImageAllocation{*this, image, image_view, allocation});
}

BufferAllocationPtr MemoryPool::allocate_buffer(const vk::BufferCreateInfo &buffer_create_info, MemoryUsage usage)
{
	auto *alloc = allocator(context);

	VkBufferCreateInfo create_info = buffer_create_info;

	VmaAllocationCreateInfo alloc_info = {};
	alloc_info.usage                   = to_vma_memory_usage(usage);

	VkBuffer          buffer;
	VmaAllocation     allocation;
	VmaAllocationInfo allocation_info;
	vk::Result        result = static_cast<vk::Result>(vmaCreateBuffer(*alloc, &create_info, &alloc_info, &buffer, &allocation, &allocation_info));
	if (result != vk::Result::eSuccess)
	{
		throw std::runtime_error("Failed to allocate buffer");
	}

	Plot<int64_t>::increment(MEMORY_POOL_BUFFER_ALLOCATIONS_NAME, 1);

	return std::shared_ptr<BufferAllocation>(new BufferAllocation{*this, buffer, allocation, create_info.size});
}

void MemoryPool::free_buffer(BufferAllocation &allocation)
{
	auto *alloc = allocator(context);

	if (!allocation.is_valid())
	{
		return;
	}

	vmaDestroyBuffer(*alloc, allocation.buffer, allocation.allocation);

	// Reset the allocation
	allocation.buffer     = VK_NULL_HANDLE;
	allocation.allocation = VK_NULL_HANDLE;

	Plot<int64_t>::decrement(MEMORY_POOL_BUFFER_ALLOCATIONS_NAME, 1);
}

void MemoryPool::free_image(ImageAllocation &allocation)
{
	auto *alloc = allocator(context);

	if (!allocation.is_valid())
	{
		return;
	}

	// Destroy the image view
	if (allocation.view)
	{
		context->device.destroyImageView(allocation.view);
	}

	vmaDestroyImage(*alloc, allocation.image, allocation.allocation);

	// Reset the allocation
	allocation.image      = VK_NULL_HANDLE;
	allocation.allocation = VK_NULL_HANDLE;

	Plot<int64_t>::decrement(MEMORY_POOL_IMAGE_ALLOCATIONS_NAME, 1);
}

void MemoryPool::update_buffer(BufferAllocation &allocation, const void *data, size_t size)
{
	auto *alloc = allocator(context);

	if (!allocation.is_valid())
	{
		return;
	}

	void *mapped_data;
	vmaMapMemory(*alloc, allocation.allocation, &mapped_data);
	memcpy(mapped_data, data, size);
	vmaUnmapMemory(*alloc, allocation.allocation);
}

}        // namespace vkb
