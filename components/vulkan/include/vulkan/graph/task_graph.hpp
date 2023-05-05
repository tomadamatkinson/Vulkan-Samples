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

#include <functional>
#include <vector>

#include <vulkan/async/fence.hpp>
#include <vulkan/context.hpp>
#include <vulkan/graph/task_registry.hpp>

namespace vkb
{
class TaskExecutionContext : public SynchronizationPoint
{
  public:
	TaskExecutionContext(ContextPtr &context, const TaskRegistry &registry) :
	    _context{context}, registry{registry} {};

	ContextPtr context() const
	{
		return _context;
	}

	void defer_cleanup(std::function<void(ContextPtr &context)> &&cleanup)
	{
		deferred_cleanups.push_back(std::move(cleanup));
	}

	const vk::Image *image(AliasedImageHandle handle)
	{
		return registry.image(handle);
	}

	const vk::ImageView *image_view(AliasedImageHandle handle)
	{
		return registry.image_view(handle);
	}

	const vk::Buffer *buffer(AliasedBufferHandle handle)
	{
		return registry.buffer(handle);
	}

	const vk::BufferView *buffer_view(AliasedBufferHandle handle)
	{
		return registry.buffer_view(handle);
	}

	void append_fence(FencePtr &fence)
	{
		fences.push_back(fence);
	}

	bool wait_until(uint64_t timeout) const override
	{
		for (auto &fence : fences)
		{
			if (!fence->wait_until(timeout))
			{
				return false;
			}
		}

		return true;
	}

	bool is_signaled() const override
	{
		for (auto &fence : fences)
		{
			if (!fence->is_signaled())
			{
				return false;
			}
		}

		return true;
	}

	~TaskExecutionContext()
	{
		// cleanup in reverse order of creation
		for (size_t i = deferred_cleanups.size(); i > 0; --i)
		{
			deferred_cleanups[i - 1](_context);
		}
	}

  private:
	ContextPtr   _context;
	TaskRegistry registry;

	std::vector<std::function<void(ContextPtr &context)>> deferred_cleanups;

	std::vector<FencePtr> fences;
};

using ExecutionFunction  = std::function<void(TaskExecutionContext &exec, vk::CommandBuffer)>;
using DefinitionFunction = std::function<ExecutionFunction(TaskRegistry &)>;

class TaskGraphExecution
{
  public:
	TaskGraphExecution(ContextPtr &context, const TaskRegistry &registry, std::vector<ExecutionFunction> &&tasks) :
	    context{context}, registry{registry}, tasks{std::move(tasks)}
	{
	}

	std::shared_ptr<TaskExecutionContext> execute()
	{
		auto exec = std::make_shared<TaskExecutionContext>(context, registry);

		auto graphics_queue = context->get_queue(vk::QueueFlagBits::eGraphics);

		vk::CommandPoolCreateInfo pool_info{};
		pool_info.queueFamilyIndex = context->get_queue_family_index(graphics_queue);
		pool_info.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

		auto command_pool = context->device.createCommandPool(pool_info);

		// clean up the pool last
		exec->defer_cleanup([command_pool](ContextPtr &context) {
			context->device.destroyCommandPool(command_pool);
		});

		vk::CommandBufferAllocateInfo alloc_info{};
		alloc_info.commandPool        = command_pool;
		alloc_info.level              = vk::CommandBufferLevel::ePrimary;
		alloc_info.commandBufferCount = 1;

		auto command_buffer = context->device.allocateCommandBuffers(alloc_info)[0];

		vk::CommandBufferBeginInfo begin_info{};
		begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

		command_buffer.begin(begin_info);

		for (auto &task : tasks)
		{
			if (task)
			{
				task(*exec.get(), command_buffer);
			}
		}

		command_buffer.end();

		auto fence = std::make_shared<Fence>(context);

		vk::SubmitInfo submit_info{};

		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers    = &command_buffer;

		graphics_queue.submit(submit_info, fence->release_handle());

		exec->append_fence(fence);

		return exec;
	}

  private:
	ContextPtr                     context;
	TaskRegistry                   registry;
	std::vector<ExecutionFunction> tasks;
};

class TaskGraph
{
  public:
	TaskGraph(ContextPtr &context, MemoryPoolPtr &memory_pool) :
	    context{context}, memory_pool{memory_pool}, registry{context, memory_pool}
	{}

	TaskGraph(const TaskGraph &)            = delete;
	TaskGraph &operator=(const TaskGraph &) = delete;

	TaskGraph(TaskGraph &&)            = default;
	TaskGraph &operator=(TaskGraph &&) = default;

	~TaskGraph() = default;

	void add_task(DefinitionFunction &&definition)
	{
		if (definition)
		{
			tasks.push_back(std::move(definition(registry)));
		}
	}

	TaskGraphExecution build()
	{
		return {context, registry, std::move(tasks)};
	}

  private:
	ContextPtr    context;
	MemoryPoolPtr memory_pool;

	TaskRegistry registry;

	std::vector<ExecutionFunction> tasks;
};
}        // namespace vkb