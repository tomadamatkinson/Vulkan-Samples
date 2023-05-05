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
#include <core/util/profiling.hpp>

#include <vulkan/async/fence.hpp>
#include <vulkan/context_builder.hpp>
#include <vulkan/pools/memory_pool.hpp>

vkb::ContextPtr create_context()
{
	// Configure Instance, Physical Device and Device
	vkb::ContextBuilder builder;
	builder
	    .configure_instance()
	    .set_application_info(vk::ApplicationInfo{"vulkan-test", VK_MAKE_VERSION(1, 0, 0), "vulkan-test-engine", VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_2})
	    .done();

	// Attempt to enqueue the validation layers if they are available
	vkb::InstanceBuilder::enable_validation_layers(builder);
	vkb::InstanceBuilder::enable_default_logger(builder);

	builder.configure_instance().add_logger_callback(std::make_shared<vkb::LoggerCallbacks>(
	    vkb::LoggerCallbacks{[](vkb::LogLevel level, const char *msg) {
		    // Validation layer errors are not allowed in tests
		    throw std::runtime_error(msg);
	    }}));

	builder.select_physical_device()
	    .score(vkb::PhysicalDeviceSelector::default_type_preference())
	    .done();

	builder.configure_device()
	    .request_queue(vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer, 1)
	    .done();

	return builder.build();
}

CUSTOM_MAIN(platform_context)
{
	auto context = create_context();

	// Create staging buffer setup
	auto memory_pool = vkb::MemoryPool::make(context);

	const auto allocate_images = [&]() {
		std::vector<vkb::ImageAllocationPtr> image_allocations;

		for (int i = 0; i < 10; i++)
		{
			vk::ImageCreateInfo image_info;
			image_info.imageType     = vk::ImageType::e2D;
			image_info.format        = vk::Format::eR8G8B8A8Unorm;
			image_info.extent.width  = 1280;
			image_info.extent.height = 720;
			image_info.extent.depth  = 1;
			image_info.mipLevels     = 1;
			image_info.arrayLayers   = 1;
			image_info.samples       = vk::SampleCountFlagBits::e1;
			image_info.tiling        = vk::ImageTiling::eOptimal;
			image_info.usage         = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
			image_info.sharingMode   = vk::SharingMode::eExclusive;
			image_info.initialLayout = vk::ImageLayout::eUndefined;

			auto allocation = memory_pool->allocate_image(image_info, vkb::MemoryUsage::GPU_ONLY);

			image_allocations.push_back(std::move(allocation));
		}

		return image_allocations;
	};

	const auto allocate_buffers = [&]() {
		std::vector<vkb::BufferAllocationPtr> buffer_allocations;

		for (int i = 0; i < 10; i++)
		{
			vk::BufferCreateInfo buffer_info;
			buffer_info.size  = 1024;
			buffer_info.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferSrc;

			auto allocation = memory_pool->allocate_buffer(buffer_info, vkb::MemoryUsage::GPU_ONLY);
			buffer_allocations.push_back(std::move(allocation));
		}

		for (int i = 0; i < 10; i++)
		{
			vk::BufferCreateInfo buffer_info;
			buffer_info.size  = 1024;
			buffer_info.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferSrc;

			auto allocation = memory_pool->allocate_buffer(buffer_info, vkb::MemoryUsage::CPU_TO_GPU);
			buffer_allocations.push_back(std::move(allocation));
		}

		for (int i = 0; i < 10; i++)
		{
			vk::BufferCreateInfo buffer_info;
			buffer_info.size  = 1024;
			buffer_info.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferSrc;

			auto allocation = memory_pool->allocate_buffer(buffer_info, vkb::MemoryUsage::CPU_ONLY);
			buffer_allocations.push_back(std::move(allocation));
		}

		return buffer_allocations;
	};

	size_t frame_count = 0;

	while (frame_count < 100)
	{
		std::vector<vkb::ImageAllocationPtr>  image_allocations;
		std::vector<vkb::BufferAllocationPtr> buffer_allocations;

		for (size_t i = 0; i < frame_count % 10; i++)
		{
			auto images = allocate_images();
			image_allocations.insert(image_allocations.end(), images.begin(), images.end());
			auto buffers = allocate_buffers();
			buffer_allocations.insert(buffer_allocations.end(), buffers.begin(), buffers.end());
		}

		frame_count++;
	}

	return 0;
}