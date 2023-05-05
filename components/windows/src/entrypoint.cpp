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

#include <core/platform/entrypoint.hpp>

#include <Windows.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <core/util/logging.hpp>

#include "windows/context.hpp"

std::unique_ptr<vkb::PlatformContext> create_platform_context(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
	vkb::initialize_logger({std::make_shared<spdlog::sinks::stdout_color_sink_mt>()});

	return std::make_unique<vkb::WindowsPlatformContext>(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
