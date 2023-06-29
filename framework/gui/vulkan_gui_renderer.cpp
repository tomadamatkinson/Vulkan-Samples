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

#include "gui/gui_renderer.hpp"

//
#include "common/vk_initializers.h"
#include "platform/filesystem.h"

namespace vkb
{
VulkanGuiRenderer::VulkanGuiRenderer(RenderContext &render_context) :
    render_context{render_context}
{
}

void VulkanGuiRenderer::prepare()
{
	auto &device = render_context.get_device();

	upload_missing_fonts();

	// Create a sampler if we dont already have one
	if (!sampler)
	{
		VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
		sampler_info.maxAnisotropy = 1.0f;
		sampler_info.magFilter     = VK_FILTER_LINEAR;
		sampler_info.minFilter     = VK_FILTER_LINEAR;
		sampler_info.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		sampler_info.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_info.addressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_info.addressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_info.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		sampler = std::make_unique<core::Sampler>(device, sampler_info);
		sampler->set_debug_name("GUI sampler");
	}

	if (descriptor_pool == VK_NULL_HANDLE)
	{
		// Descriptor pool
		std::vector<VkDescriptorPoolSize> pool_sizes = {
		    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vkb::initializers::descriptor_pool_create_info(pool_sizes, 2);
		VK_CHECK(vkCreateDescriptorPool(render_context.get_device().get_handle(), &descriptorPoolInfo, nullptr, &descriptor_pool));
	}

	if (descriptor_set_layout == VK_NULL_HANDLE)
	{
		// Descriptor set layout
		std::vector<VkDescriptorSetLayoutBinding> layout_bindings = {
		    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
		};
		VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(layout_bindings);
		VK_CHECK(vkCreateDescriptorSetLayout(render_context.get_device().get_handle(), &descriptor_set_layout_create_info, nullptr, &descriptor_set_layout));

		// Descriptor set
		VkDescriptorSetAllocateInfo descriptor_allocation = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1);
		VK_CHECK(vkAllocateDescriptorSets(render_context.get_device().get_handle(), &descriptor_allocation, &descriptor_set));
		VkDescriptorImageInfo font_descriptor = vkb::initializers::descriptor_image_info(
		    sampler->get_handle(),
		    font_image_view->get_handle(),
		    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
		    vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &font_descriptor)};
		vkUpdateDescriptorSets(render_context.get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
	}

	// Create a pipeline layout if we don't already have one
	if (pipeline != VK_NULL_HANDLE)
	{
		return;
	}

	std::string vertex_shader = R"(#version 320 es

            precision mediump float;

            layout (location = 0) in vec2 inPos;
            layout (location = 1) in vec2 inUV;
            layout (location = 2) in vec4 inColor;

            layout (push_constant) uniform PushConstants {
                mat4 transform;
            } pushConstants;

            layout (location = 0) out vec2 outUV;
            layout (location = 1) out vec4 outColor;

            out gl_PerVertex
            {
                vec4 gl_Position;
            };

            void main()
            {
                outUV = inUV;
                outColor = inColor;
                gl_Position = pushConstants.transform * vec4(inPos.xy, 0.0, 1.0);
            }
        )";

	vkb::ShaderSource vert_shader{};
	vert_shader.set_source(vertex_shader);

	std::string fragment_shader = R"(#version 320 es

            precision mediump float;

            layout (binding = 0) uniform sampler2D fontSampler;

            layout (location = 0) in vec2 inUV;
            layout (location = 1) in vec4 inColor;

            layout (location = 0) out vec4 outColor;

            void main()
            {
                outColor = inColor * texture(fontSampler, inUV);
            }
        )";

	vkb::ShaderSource frag_shader{};
	frag_shader.set_source(fragment_shader);

	std::vector<vkb::ShaderModule *> shader_modules;
	shader_modules.push_back(&device.get_resource_cache().request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, vert_shader, {}));
	shader_modules.push_back(&device.get_resource_cache().request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader, {}));

	pipeline_layout = &device.get_resource_cache().request_pipeline_layout(shader_modules);

	if (render_pass == VK_NULL_HANDLE)
	{
		std::array<VkAttachmentDescription, 1> attachments = {};
		// Color attachment
		attachments[0].format         = render_context.get_format();
		attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_reference = {};
		color_reference.attachment            = 0;
		color_reference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass_description    = {};
		subpass_description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass_description.colorAttachmentCount    = 1;
		subpass_description.pColorAttachments       = &color_reference;
		subpass_description.pDepthStencilAttachment = nullptr;
		subpass_description.inputAttachmentCount    = 0;
		subpass_description.pInputAttachments       = nullptr;
		subpass_description.preserveAttachmentCount = 0;
		subpass_description.pPreserveAttachments    = nullptr;
		subpass_description.pResolveAttachments     = nullptr;

		// Subpass dependencies for layout transitions
		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass      = 0;
		dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass      = 0;
		dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo render_pass_create_info = {};
		render_pass_create_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_create_info.attachmentCount        = static_cast<uint32_t>(attachments.size());
		render_pass_create_info.pAttachments           = attachments.data();
		render_pass_create_info.subpassCount           = 1;
		render_pass_create_info.pSubpasses             = &subpass_description;
		render_pass_create_info.dependencyCount        = static_cast<uint32_t>(dependencies.size());
		render_pass_create_info.pDependencies          = dependencies.data();

		VK_CHECK(vkCreateRenderPass(device.get_handle(), &render_pass_create_info, nullptr, &render_pass));
	}

	std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
	for (auto &shader_module : shader_modules)
	{
		VkPipelineShaderStageCreateInfo stage;
		stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage.pNext  = nullptr;
		stage.flags  = 0;
		stage.module = shader_module->create_module(device.get_handle());
		stage.pName  = "main";
		stage.stage  = shader_module->get_stage();
		shader_stages.push_back(stage);
	}

	// Setup graphics pipeline for UI rendering
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

	// Enable blending
	VkPipelineColorBlendAttachmentState blend_attachment_state{};
	blend_attachment_state.blendEnable         = VK_TRUE;
	blend_attachment_state.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blend_attachment_state.alphaBlendOp        = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);

	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT);

	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(dynamic_state_enables);

	VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(pipeline_layout->get_handle(), render_pass);

	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;
	pipeline_create_info.stageCount          = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages             = shader_stages.data();
	pipeline_create_info.subpass             = 0;

	// Vertex bindings an attributes based on ImGui vertex definition
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)),         // Location 0: Position
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)),          // Location 1: UV
	    vkb::initializers::vertex_input_attribute_description(0, 2, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)),        // Location 0: Color
	};
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state_create_info.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state_create_info.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state_create_info.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state_create_info.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;

	VK_CHECK(vkCreateGraphicsPipelines(device.get_handle(), VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline));

	for (auto &stage : shader_stages)
	{
		vkDestroyShaderModule(device.get_handle(), stage.module, nullptr);
	}
}

void VulkanGuiRenderer::upload_missing_fonts()
{
	auto &io = ImGui::GetIO();

	// A list of requested fonts
	std::vector<std::string> fonts = {
	    "Roboto-Regular",
	    // "RobotoMono-Regular",
	};

	// Check if we need to upload any fonts
	// Add fonts to imgui if they are not already present
	for (auto &font : fonts)
	{
		bool already_uploaded = false;
		for (auto *imgui_font : io.Fonts->Fonts)
		{
			if (imgui_font->GetDebugName() == font)
			{
				LOGW("Font {} already uploaded", font);
				already_uploaded = true;
				continue;
			}
		}
		if (already_uploaded)
		{
			continue;
		}

		if (!vkb::fs::is_file(vkb::fs::path::get(vkb::fs::path::Assets) + "fonts/" + font + ".ttf"))
		{
			LOGE("Could not find font file: {}", font);
			continue;
		}

		ImFontConfig font_config{};
		font_config.FontDataOwnedByAtlas = false;

		auto font_data = vkb::fs::read_asset("fonts/" + font + ".ttf");
		io.Fonts->AddFontFromMemoryTTF(font_data.data(), static_cast<int>(font_data.size()), 16.0f, &font_config);
	}

	unsigned char *font_data;
	int            tex_width, tex_height;
	io.Fonts->GetTexDataAsRGBA32(&font_data, &tex_width, &tex_height);
	size_t upload_size = tex_width * tex_height * 4 * sizeof(char);

	auto &device = render_context.get_device();

	// Create target image for copy
	VkExtent3D font_extent{to_u32(tex_width), to_u32(tex_height), 1u};

	font_image = std::make_unique<core::Image>(device, font_extent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	font_image->set_debug_name("GUI font image");

	font_image_view = std::make_unique<core::ImageView>(*font_image, VK_IMAGE_VIEW_TYPE_2D);
	font_image_view->set_debug_name("View on GUI font image");

	VkFenceCreateInfo fence_info = vkb::initializers::fence_create_info();
	vkCreateFence(device.get_handle(), &fence_info, nullptr, &font_upload_fence);

	transfer_command_pool = std::make_unique<CommandPool>(render_context.get_device(), device.get_queue_by_flags(VK_QUEUE_TRANSFER_BIT, 0).get_family_index());

	// Upload font data into the vulkan image memory
	core::Buffer stage_buffer{device, upload_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, 0};
	stage_buffer.update({font_data, font_data + upload_size});

	auto &command_buffer = transfer_command_pool->request_command_buffer();

	// Begin recording
	command_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 0);

	{
		// Prepare for transfer
		ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_UNDEFINED;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		memory_barrier.src_access_mask = 0;
		memory_barrier.dst_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_HOST_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;

		command_buffer.image_memory_barrier(*font_image_view, memory_barrier);
	}

	// Copy
	VkBufferImageCopy buffer_copy_region{};
	buffer_copy_region.imageSubresource.layerCount = font_image_view->get_subresource_range().layerCount;
	buffer_copy_region.imageSubresource.aspectMask = font_image_view->get_subresource_range().aspectMask;
	buffer_copy_region.imageExtent                 = font_image->get_extent();

	command_buffer.copy_buffer_to_image(stage_buffer, *font_image, {buffer_copy_region});

	{
		// Prepare for fragmen shader
		ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		memory_barrier.src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
		memory_barrier.dst_access_mask = VK_ACCESS_SHADER_READ_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		command_buffer.image_memory_barrier(*font_image_view, memory_barrier);
	}

	// End recording
	command_buffer.end();

	auto &queue = device.get_queue_by_flags(VK_QUEUE_TRANSFER_BIT, 0);

	queue.submit(command_buffer, font_upload_fence);
}

bool VulkanGuiRenderer::update_buffers()
{
	ImDrawData *draw_data = ImGui::GetDrawData();
	bool        updated   = false;

	if (!draw_data)
	{
		return false;
	}

	size_t vertex_buffer_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
	size_t index_buffer_size  = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

	if ((vertex_buffer_size == 0) || (index_buffer_size == 0))
	{
		return false;
	}

	if ((vertex_buffer && vertex_buffer->get_handle() == VK_NULL_HANDLE) || (vertex_buffer_size != last_vertex_buffer_size))
	{
		last_vertex_buffer_size = vertex_buffer_size;
		updated                 = true;

		vertex_buffer.reset();
		vertex_buffer = std::make_unique<core::Buffer>(render_context.get_device(), vertex_buffer_size,
		                                               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		                                               VMA_MEMORY_USAGE_GPU_TO_CPU);
		vertex_buffer->set_debug_name("GUI vertex buffer");
	}

	if ((index_buffer && index_buffer->get_handle() == VK_NULL_HANDLE) || (index_buffer_size != last_index_buffer_size))
	{
		last_index_buffer_size = index_buffer_size;
		updated                = true;

		index_buffer.reset();
		index_buffer = std::make_unique<core::Buffer>(render_context.get_device(), index_buffer_size,
		                                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		                                              VMA_MEMORY_USAGE_GPU_TO_CPU);
		index_buffer->set_debug_name("GUI index buffer");
	}

	// Upload data
	ImDrawVert *vtx_dst = (ImDrawVert *) vertex_buffer->map();
	ImDrawIdx  *idx_dst = (ImDrawIdx *) index_buffer->map();

	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList *cmd_list = draw_data->CmdLists[n];
		memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtx_dst += cmd_list->VtxBuffer.Size;
		idx_dst += cmd_list->IdxBuffer.Size;
	}

	vertex_buffer->flush();
	index_buffer->flush();

	vertex_buffer->unmap();
	index_buffer->unmap();

	return updated;
}

void VulkanGuiRenderer::render(GuiRenderContext *context)
{
	auto *vulkan_context = dynamic_cast<VulkanGuiRenderContext *>(context);
	assert(vulkan_context != nullptr && "Context must be a VulkanGuiRenderContext");

	auto &device = render_context.get_device();

	update_buffers();

	if (font_upload_fence != VK_NULL_HANDLE)
	{
		// wait for fonts to be uploaded before rendering
		vkWaitForFences(device.get_handle(), 1, &font_upload_fence, VK_TRUE, UINT64_MAX);
		vkDestroyFence(device.get_handle(), font_upload_fence, nullptr);
		font_upload_fence = VK_NULL_HANDLE;
	}

	auto &command_buffer = vulkan_context->command_buffer;

	auto       &io            = ImGui::GetIO();
	ImDrawData *draw_data     = ImGui::GetDrawData();
	int32_t     vertex_offset = 0;
	int32_t     index_offset  = 0;

	if ((!draw_data) || (draw_data->CmdListsCount == 0))
	{
		return;
	}

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout->get_handle(), 0, 1, &descriptor_set, 0, NULL);

	// Push constants
	auto push_transform = glm::mat4(1.0f);

	// Pre-rotation
	if (render_context.has_swapchain())
	{
		auto transform = render_context.get_swapchain().get_transform();

		glm::vec3 rotation_axis = glm::vec3(0.0f, 0.0f, 1.0f);
		if (transform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR)
		{
			push_transform = glm::rotate(push_transform, glm::radians(90.0f), rotation_axis);
		}
		else if (transform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR)
		{
			push_transform = glm::rotate(push_transform, glm::radians(270.0f), rotation_axis);
		}
		else if (transform & VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR)
		{
			push_transform = glm::rotate(push_transform, glm::radians(180.0f), rotation_axis);
		}
	}

	push_transform = glm::translate(push_transform, glm::vec3(-1.0f, -1.0f, 0.0f));
	push_transform = glm::scale(push_transform, glm::vec3(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y, 0.0f));
	vkCmdPushConstants(command_buffer, pipeline_layout->get_handle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &push_transform);

	VkDeviceSize offsets[1] = {0};

	VkBuffer vertex_buffer_handle = vertex_buffer->get_handle();
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer_handle, offsets);

	VkBuffer index_buffer_handle = index_buffer->get_handle();
	vkCmdBindIndexBuffer(command_buffer, index_buffer_handle, 0, VK_INDEX_TYPE_UINT16);

	for (int32_t i = 0; i < draw_data->CmdListsCount; i++)
	{
		const ImDrawList *cmd_list = draw_data->CmdLists[i];
		for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
		{
			const ImDrawCmd *cmd = &cmd_list->CmdBuffer[j];
			VkRect2D         scissor_rect;
			scissor_rect.offset.x      = std::max(static_cast<int32_t>(cmd->ClipRect.x), 0);
			scissor_rect.offset.y      = std::max(static_cast<int32_t>(cmd->ClipRect.y), 0);
			scissor_rect.extent.width  = static_cast<uint32_t>(cmd->ClipRect.z - cmd->ClipRect.x);
			scissor_rect.extent.height = static_cast<uint32_t>(cmd->ClipRect.w - cmd->ClipRect.y);

			// Adjust for pre-rotation if necessary
			if (render_context.has_swapchain())
			{
				auto transform = render_context.get_swapchain().get_transform();
				if (transform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR)
				{
					scissor_rect.offset.x      = static_cast<uint32_t>(io.DisplaySize.y - cmd->ClipRect.w);
					scissor_rect.offset.y      = static_cast<uint32_t>(cmd->ClipRect.x);
					scissor_rect.extent.width  = static_cast<uint32_t>(cmd->ClipRect.w - cmd->ClipRect.y);
					scissor_rect.extent.height = static_cast<uint32_t>(cmd->ClipRect.z - cmd->ClipRect.x);
				}
				else if (transform & VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR)
				{
					scissor_rect.offset.x      = static_cast<uint32_t>(io.DisplaySize.x - cmd->ClipRect.z);
					scissor_rect.offset.y      = static_cast<uint32_t>(io.DisplaySize.y - cmd->ClipRect.w);
					scissor_rect.extent.width  = static_cast<uint32_t>(cmd->ClipRect.z - cmd->ClipRect.x);
					scissor_rect.extent.height = static_cast<uint32_t>(cmd->ClipRect.w - cmd->ClipRect.y);
				}
				else if (transform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR)
				{
					scissor_rect.offset.x      = static_cast<uint32_t>(cmd->ClipRect.y);
					scissor_rect.offset.y      = static_cast<uint32_t>(io.DisplaySize.x - cmd->ClipRect.z);
					scissor_rect.extent.width  = static_cast<uint32_t>(cmd->ClipRect.w - cmd->ClipRect.y);
					scissor_rect.extent.height = static_cast<uint32_t>(cmd->ClipRect.z - cmd->ClipRect.x);
				}
			}

			vkCmdSetScissor(command_buffer, 0, 1, &scissor_rect);
			vkCmdDrawIndexed(command_buffer, cmd->ElemCount, 1, index_offset, vertex_offset, 0);
			index_offset += cmd->ElemCount;
		}
		vertex_offset += cmd_list->VtxBuffer.Size;
	}
}

void VulkanGuiRenderer::destroy()
{
	vkDestroyDescriptorPool(render_context.get_device().get_handle(), descriptor_pool, nullptr);
	vkDestroyDescriptorSetLayout(render_context.get_device().get_handle(), descriptor_set_layout, nullptr);
	vkDestroyPipeline(render_context.get_device().get_handle(), pipeline, nullptr);
}
}        // namespace vkb