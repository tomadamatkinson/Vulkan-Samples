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

// This test checks that both Fence and FenceGroup work as expected

// Create a context with a single queue
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

// Some random data to upload to the GPU
struct Data
{
	uint32_t x;
	uint32_t y;
	uint32_t z;
};

// Create some random data - 32 * 32 * 32 * 1000 bytes = 99 KB (i think :D)
std::vector<Data> create_data(uint32_t count = 1000)
{
	std::vector<Data> data;
	data.reserve(count);
	for (uint32_t i = 0; i < count; i++)
	{
		data.push_back(Data{i, i, i});
	}
	return data;
}

// Hold important test objects
// If these die before the fence is signalled, the test will fail (deallocating memory which is in use)
// The fence is passed to a FenceGroup which will wait for it to be signalled
struct FenceTest
{
	vkb::BufferAllocationPtr gpu_buffer;
	vk::CommandBuffer        command_buffer;
	vkb::FencePtr            fence;
};

FenceTest upload_data(vkb::ContextPtr context, vk::CommandPool command_pool, vk::Queue queue, vkb::MemoryPool *memory_pool, vkb::BufferAllocationPtr staging)
{
	vk::BufferCreateInfo buffer_info;
	buffer_info.size  = staging->size();
	buffer_info.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;

	auto gpu = memory_pool->allocate_buffer(buffer_info, vkb::MemoryUsage::GPU_ONLY);

	vk::CommandBufferAllocateInfo allocate_info;
	allocate_info.commandPool        = command_pool;
	allocate_info.commandBufferCount = 1;
	allocate_info.level              = vk::CommandBufferLevel::ePrimary;

	vk::CommandBuffer command_buffer = context->device.allocateCommandBuffers(allocate_info)[0];

	vk::CommandBufferBeginInfo begin_info;
	begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	command_buffer.begin(begin_info);

	vk::BufferCopy copy_region;
	copy_region.size = sizeof(Data);

	command_buffer.copyBuffer(staging->buffer, gpu->buffer, copy_region);

	command_buffer.end();

	vkb::FencePtr fence = std::make_shared<vkb::Fence>(context);

	vk::SubmitInfo submit_info;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &command_buffer;

	queue.submit(submit_info, fence->release_handle());

	return {
	    std::move(gpu),
	    command_buffer,
	    std::move(fence),
	};
}

CUSTOM_MAIN(platform_context)
{
	auto context = create_context();
	auto queue   = context->get_queue(vk::QueueFlagBits::eTransfer);

	// Create staging buffer setup
	auto memory_pool = vkb::MemoryPool::make(context);

	vk::CommandPoolCreateInfo command_pool_info;
	command_pool_info.queueFamilyIndex = context->get_queue_family_index(queue);

	vk::CommandPool command_pool;
	if (context->device.createCommandPool(&command_pool_info, nullptr, &command_pool) != vk::Result::eSuccess)
	{
		throw std::runtime_error("Failed to create command pool");
	}

	auto data = create_data();

	vk::BufferCreateInfo buffer_info;
	buffer_info.size  = data.size() * sizeof(data[0]);
	buffer_info.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferSrc;

	auto staging = memory_pool->allocate_buffer(buffer_info, vkb::MemoryUsage::CPU_TO_GPU);
	staging->update(data);

	buffer_info.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;

	std::vector<FenceTest>    tests;
	std::vector<vkb::SyncPtr> sync_points;

	for (uint32_t i = 0; i < 100; i++)
	{
		auto test = upload_data(context, command_pool, queue, memory_pool.get(), staging);

		sync_points.push_back(test.fence);

		tests.push_back(std::move(test));
	}

	vkb::SynchronizationGroup sync_group{std::move(sync_points)};

	// wait for all fences to be signalled
	sync_group.wait();

	for (auto &test : tests)
	{
		context->device.freeCommandBuffers(command_pool, test.command_buffer);
	}

	context->device.destroyCommandPool(command_pool);

	return 0;
}