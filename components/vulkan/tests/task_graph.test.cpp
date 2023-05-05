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
#include <vulkan/graph/task_graph.hpp>
#include <vulkan/shaders/shader_compiler.hpp>

std::string TRIANGLE_VERT = R"(#version 320 es
precision mediump float;

layout(location = 0) in vec2 pos;

layout(location = 0) out vec3 out_color;

void main()
{
    gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);

    out_color = vec3(1.0, 0.0, 0.0);
})";

std::string TRIANGLE_FRAG = R"(#version 320 es
precision mediump float;

layout(location = 0) in vec3 in_color;

layout(location = 0) out vec4 out_color;

void main()
{
	out_color = vec4(in_color, 1.0);
})";

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

struct RenderPass
{
	vk::RenderPass render_pass;
};

RenderPass create_render_pass(vkb::ContextPtr &context)
{
	vk::AttachmentReference color_attachment;
	color_attachment.attachment = 0;
	color_attachment.layout     = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint    = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments    = &color_attachment;

	vk::SubpassDependency dependency;
	dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass    = 0;
	dependency.srcStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.srcAccessMask = {};

	vk::AttachmentDescription attachment;
	attachment.format        = vk::Format::eR8G8B8A8Unorm;
	attachment.samples       = vk::SampleCountFlagBits::e1;
	attachment.loadOp        = vk::AttachmentLoadOp::eClear;
	attachment.storeOp       = vk::AttachmentStoreOp::eStore;
	attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachment.initialLayout = vk::ImageLayout::eUndefined;
	attachment.finalLayout   = vk::ImageLayout::eColorAttachmentOptimal;

	vk::RenderPassCreateInfo create_info;
	create_info.attachmentCount = 1;
	create_info.pAttachments    = &attachment;
	create_info.subpassCount    = 1;
	create_info.pSubpasses      = &subpass;
	create_info.dependencyCount = 1;
	create_info.pDependencies   = &dependency;

	return {
	    context->device.createRenderPass(create_info),
	};
}

struct GraphicsPipeline
{
	vk::Pipeline       pipeline;
	vk::PipelineLayout layout;
};

GraphicsPipeline create_graphics_pipeline(vkb::ContextPtr &context, vk::RenderPass render_pass)
{
	vk::PipelineLayoutCreateInfo layout_info;
	auto                         layout = context->device.createPipelineLayout(layout_info);

	vkb::GlslShaderCompiler compiler;
	std::vector<uint32_t>   vert_spirv = compiler.compile(vk::ShaderStageFlagBits::eVertex, TRIANGLE_VERT, "main", {});
	std::vector<uint32_t>   frag_spirv = compiler.compile(vk::ShaderStageFlagBits::eFragment, TRIANGLE_FRAG, "main", {});

	vk::ShaderModuleCreateInfo vert_info;
	vert_info.codeSize = vert_spirv.size() * sizeof(uint32_t);
	vert_info.pCode    = vert_spirv.data();
	auto vert_module   = context->device.createShaderModule(vert_info);

	vk::PipelineShaderStageCreateInfo vert_stage;
	vert_stage.stage  = vk::ShaderStageFlagBits::eVertex;
	vert_stage.module = vert_module;
	vert_stage.pName  = "main";

	vk::ShaderModuleCreateInfo frag_info;
	frag_info.codeSize = frag_spirv.size() * sizeof(uint32_t);
	frag_info.pCode    = frag_spirv.data();
	auto frag_module   = context->device.createShaderModule(frag_info);

	vk::PipelineShaderStageCreateInfo frag_stage;
	frag_stage.stage  = vk::ShaderStageFlagBits::eFragment;
	frag_stage.module = frag_module;
	frag_stage.pName  = "main";

	vk::PipelineShaderStageCreateInfo stages[] = {vert_stage, frag_stage};

	vk::VertexInputBindingDescription binding;
	binding.binding   = 0;
	binding.stride    = sizeof(float) * 3;
	binding.inputRate = vk::VertexInputRate::eVertex;

	vk::VertexInputAttributeDescription attribute;
	attribute.binding  = 0;
	attribute.location = 0;
	attribute.format   = vk::Format::eR32G32B32Sfloat;
	attribute.offset   = 0;

	vk::PipelineVertexInputStateCreateInfo vertex_input_info;
	vertex_input_info.vertexBindingDescriptionCount   = 1;
	vertex_input_info.pVertexBindingDescriptions      = &binding;
	vertex_input_info.vertexAttributeDescriptionCount = 1;
	vertex_input_info.pVertexAttributeDescriptions    = &attribute;

	vk::PipelineInputAssemblyStateCreateInfo input_assembly;
	input_assembly.topology = vk::PrimitiveTopology::eTriangleList;

	vk::Viewport viewport;
	viewport.x      = 0.0f;
	viewport.y      = 0.0f;
	viewport.width  = 800.0f;
	viewport.height = 600.0f;

	vk::Rect2D scissor;
	scissor.offset = vk::Offset2D{0, 0};
	scissor.extent = vk::Extent2D{800, 600};

	vk::PipelineViewportStateCreateInfo viewport_state;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports    = &viewport;
	viewport_state.scissorCount  = 1;
	viewport_state.pScissors     = &scissor;

	vk::PipelineRasterizationStateCreateInfo rasterizer;
	rasterizer.lineWidth   = 1.0f;
	rasterizer.polygonMode = vk::PolygonMode::eFill;
	rasterizer.cullMode    = vk::CullModeFlagBits::eNone;
	rasterizer.frontFace   = vk::FrontFace::eClockwise;

	vk::PipelineMultisampleStateCreateInfo multisampling;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

	vk::PipelineColorBlendAttachmentState color_blend_attachment;
	color_blend_attachment.colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	color_blend_attachment.blendEnable         = VK_TRUE;
	color_blend_attachment.alphaBlendOp        = vk::BlendOp::eAdd;
	color_blend_attachment.colorBlendOp        = vk::BlendOp::eAdd;
	color_blend_attachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	color_blend_attachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;

	vk::PipelineColorBlendStateCreateInfo color_blending;
	color_blending.logicOpEnable   = VK_FALSE;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments    = &color_blend_attachment;

	vk::PipelineDepthStencilStateCreateInfo depth_stencil;
	depth_stencil.depthTestEnable       = VK_TRUE;
	depth_stencil.depthWriteEnable      = VK_TRUE;
	depth_stencil.depthCompareOp        = vk::CompareOp::eLess;
	depth_stencil.depthBoundsTestEnable = VK_FALSE;
	depth_stencil.stencilTestEnable     = VK_FALSE;

	vk::GraphicsPipelineCreateInfo create_info;
	create_info.renderPass          = render_pass;
	create_info.stageCount          = 2;
	create_info.pStages             = stages;
	create_info.pVertexInputState   = &vertex_input_info;
	create_info.pInputAssemblyState = &input_assembly;
	create_info.pViewportState      = &viewport_state;
	create_info.pRasterizationState = &rasterizer;
	create_info.pMultisampleState   = &multisampling;
	create_info.pDepthStencilState  = &depth_stencil;
	create_info.pColorBlendState    = &color_blending;
	create_info.pDynamicState       = nullptr;
	create_info.layout              = layout;

	auto res = context->device.createGraphicsPipeline(VK_NULL_HANDLE, create_info);
	if (res.result != vk::Result::eSuccess)
	{
		throw std::runtime_error("Failed to create graphics pipeline");
	}

	context->device.destroyShaderModule(vert_module);
	context->device.destroyShaderModule(frag_module);

	return {
	    res.value,
	    layout,
	};
}

struct Vertex
{
	float x, y, z;
};

std::vector<Vertex> vertices = {
    {-0.5f, -0.5f, 0.0f},
    {0.5f, -0.5f, 0.0f},
    {0.0f, 0.5f, 0.0f},
};

void upload_staging_buffer(vkb::ContextPtr context, vkb::BufferAllocationPtr staging, vkb::BufferAllocationPtr gpu)
{
	auto queue = context->get_queue(vk::QueueFlagBits::eTransfer);
	auto index = context->get_queue_family_index(queue);

	vk::CommandPoolCreateInfo pool_info;
	pool_info.queueFamilyIndex = index;

	auto pool = context->device.createCommandPool(pool_info);

	vk::CommandBufferAllocateInfo alloc_info;
	alloc_info.commandPool        = pool;
	alloc_info.commandBufferCount = 1;

	auto cmd = context->device.allocateCommandBuffers(alloc_info).front();

	vk::CommandBufferBeginInfo begin_info;
	begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	cmd.begin(begin_info);

	vk::BufferCopy copy;
	copy.size      = staging->size();
	copy.srcOffset = 0;
	copy.dstOffset = 0;

	cmd.copyBuffer(staging->buffer, gpu->buffer, copy);

	cmd.end();

	vk::SubmitInfo submit_info;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &cmd;

	vkb::Fence fence{context};

	queue.submit(submit_info, fence.release_handle());

	fence.wait();

	context->device.freeCommandBuffers(pool, cmd);

	context->device.destroyCommandPool(pool);
}

vkb::BufferAllocationPtr upload_vertices(vkb::ContextPtr context, vkb::MemoryPoolPtr memory_pool)
{
	LOGI("Uploading vertices");

	vk::BufferCreateInfo buffer_info;
	buffer_info.size  = vertices.size() * sizeof(Vertex);
	buffer_info.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferSrc;

	auto staging = memory_pool->allocate_buffer(buffer_info, vkb::MemoryUsage::CPU_TO_GPU);
	staging->update(vertices);

	buffer_info.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;

	auto vertex_buffer = memory_pool->allocate_buffer(buffer_info, vkb::MemoryUsage::GPU_ONLY);

	LOGI("Uploading staging buffer");
	upload_staging_buffer(context, staging, vertex_buffer);

	return vertex_buffer;
}

CUSTOM_MAIN(platform_context)
{
	auto context = create_context();

	RenderPass       render_pass = create_render_pass(context);
	GraphicsPipeline pipeline    = create_graphics_pipeline(context, render_pass.render_pass);
	auto             pool        = vkb::MemoryPool::make(context);

	auto vertex_buffer = upload_vertices(context, pool);

	LOGI("Creating graph");

	vkb::TaskGraph graph{context, pool};

	vk::ClearValue clear_value{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};

	graph.add_task([context, pipeline, render_pass, vertex_buffer, &clear_value](vkb::TaskRegistry &registry) {
		vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eColorAttachment;

		auto request_handle = registry.request_image(usage, vk::Format::eR8G8B8A8Unorm, {800, 600, 1});
		auto write_handle   = registry.write(request_handle);

		return [=](vkb::TaskExecutionContext &exec, vk::CommandBuffer command_buffer) -> void {
			vk::FramebufferCreateInfo framebuffer_info;
			framebuffer_info.renderPass      = render_pass.render_pass;
			framebuffer_info.attachmentCount = 1;
			framebuffer_info.pAttachments    = exec.image_view(write_handle);
			framebuffer_info.width           = 800;
			framebuffer_info.height          = 600;
			framebuffer_info.layers          = 1;

			auto framebuffer = context->device.createFramebuffer(framebuffer_info);

			exec.defer_cleanup([framebuffer](vkb::ContextPtr context) {
				context->device.destroyFramebuffer(framebuffer);
			});

			vk::RenderPassBeginInfo render_pass_info;
			render_pass_info.renderPass               = render_pass.render_pass;
			render_pass_info.framebuffer              = framebuffer;
			render_pass_info.renderArea.offset.x      = 0;
			render_pass_info.renderArea.offset.y      = 0;
			render_pass_info.renderArea.extent.width  = 800;
			render_pass_info.renderArea.extent.height = 600;
			render_pass_info.clearValueCount          = 1;
			render_pass_info.pClearValues             = &clear_value;

			command_buffer.beginRenderPass(&render_pass_info, vk::SubpassContents::eInline);

			command_buffer.bindVertexBuffers(0, vertex_buffer->buffer, {0});

			command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.pipeline);

			command_buffer.draw(3, 1, 0, 0);

			command_buffer.endRenderPass();
		};
	});

	LOGI("Building graph");

	auto exec = graph.build();

	LOGI("Executing graph");

	auto exec_context = exec.execute();

	exec_context->wait();

	context->device.destroyPipeline(pipeline.pipeline);
	context->device.destroyPipelineLayout(pipeline.layout);
	context->device.destroyRenderPass(render_pass.render_pass);

	return 0;
}