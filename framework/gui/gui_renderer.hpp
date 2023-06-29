/* Copyright (c) 2018-2023, Arm Limited and Contributors
 * Copyright (c) 2019-2023, Sascha Willems
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

#include <gui/gui.hpp>

#include "core/buffer.h"
#include "core/command_pool.h"
#include "rendering/render_context.h"

namespace vkb
{
class VulkanGuiRenderContext : public GuiRenderContext
{
  public:
	VulkanGuiRenderContext(VkCommandBuffer command_buffer) :
	    command_buffer{command_buffer}
	{}

	virtual ~VulkanGuiRenderContext() = default;

	VkCommandBuffer command_buffer;
};

class VulkanGuiRenderer : public GuiRenderer
{
  public:
	VulkanGuiRenderer(RenderContext &render_context);

	virtual ~VulkanGuiRenderer() = default;

	virtual void prepare() override;

	virtual void render(GuiRenderContext *context) override;

	virtual void destroy() override;

  private:
	void upload_missing_fonts();
	bool update_buffers();

	RenderContext &render_context;
	VkRenderPass   render_pass{VK_NULL_HANDLE};

	PipelineLayout *pipeline_layout{nullptr};

	std::unique_ptr<core::Sampler> sampler{nullptr};

	std::unique_ptr<core::Buffer> vertex_buffer;
	std::unique_ptr<core::Buffer> index_buffer;
	size_t                        last_vertex_buffer_size;
	size_t                        last_index_buffer_size;

	VkDescriptorPool      descriptor_pool{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};
	VkDescriptorSet       descriptor_set{VK_NULL_HANDLE};
	VkPipeline            pipeline{VK_NULL_HANDLE};

	std::unique_ptr<core::Image>     font_image;
	std::unique_ptr<core::ImageView> font_image_view;
	VkFence                          font_upload_fence{VK_NULL_HANDLE};

	std::unique_ptr<CommandPool> transfer_command_pool;
};

std::unique_ptr<GuiRenderer> create_gui_renderer();

class VulkanHppGuiRenderer : public GuiRenderer
{
  public:
	VulkanHppGuiRenderer();

	virtual ~VulkanHppGuiRenderer() = default;

	virtual void prepare() override;

	virtual void render(GuiRenderContext *context) override;

	virtual void destroy() override;
};

std::unique_ptr<GuiRenderer> create_hpp_gui_renderer();
}        // namespace vkb