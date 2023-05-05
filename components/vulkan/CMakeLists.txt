# Copyright (c) 2023, Thomas Atkinson
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 the "License";
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

vkb__register_component(
    NAME vulkan
    HEADERS
        # core
        include/vulkan/context.hpp
        include/vulkan/context_builder.hpp
        
        # pools
        include/vulkan/pools/memory_pool.hpp
    
    SRC
        src/context.cpp
        src/context_builder.cpp

        src/async/synchronization.cpp
        src/async/fence.cpp

        src/pools/memory_pool.cpp

        src/shaders/glsl_shader_compiler.cpp

        # internal
        src/util/logger.cpp
    LINK_LIBS
        vkb__core
        volk
        VulkanMemoryAllocator
        glslang
        glslang-default-resource-limits
        SPIRV
)

target_compile_definitions(vkb__vulkan PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

vkb__register_tests(
    GPU_ONLY
    NO_CATCH2
    NAME vulkan_fence
    SRC
        tests/fence.test.cpp
    LINK_LIBS
        vkb__vulkan
)

vkb__register_tests(
    GPU_ONLY
    NO_CATCH2
    NAME vulkan_memory_pool
    SRC
        tests/memory_pool.test.cpp
    LINK_LIBS
        vkb__vulkan
)

vkb__register_tests(
    GPU_ONLY
    NO_CATCH2
    NAME vulkan_glsl_shader_compiler
    SRC
        tests/glsl_shader_compiler.test.cpp
    LINK_LIBS
        vkb__vulkan
)

vkb__register_tests(
    GPU_ONLY
    NO_CATCH2
    NAME vulkan_task_graph
    SRC
        tests/task_graph.test.cpp
    LINK_LIBS
        vkb__vulkan
)