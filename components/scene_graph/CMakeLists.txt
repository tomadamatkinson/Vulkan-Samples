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
    NAME scene_graph
    SRC
        # Scene Graph
        src/entt.cpp
        src/scene_graph.cpp

        # Loaders
        src/loaders/gltf.cpp
        src/loaders/astc.cpp
        src/loaders/ktx.cpp
        src/loaders/stb.cpp

    LINK_LIBS
        vkb__core
        EnTT
        glm
        vulkan
        stb
)


vkb__register_tests(
    COMPONENT scene_graph
    NAME scene_graph
    SRC
        tests/scene_graph.test.cpp
    LINK_LIBS
        vkb__scene_graph
)

vkb__register_tests(
    COMPONENT scene_graph
    NAME loaders
    SRC
        tests/stb.test.cpp
    LINK_LIBS
        vkb__scene_graph
)