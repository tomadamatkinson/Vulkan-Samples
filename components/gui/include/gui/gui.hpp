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

#include <memory>

#include <imgui.h>
#include <implot.h>

#include "gui/style.hpp"
#include "gui/widget.hpp"

namespace vkb
{
// Represents external information which is shared with the GUI
class GuiRenderContext
{
  public:
	virtual ~GuiRenderContext() = default;
};

// A class with the capabilities to render the GUI
class GuiRenderer
{
  public:
	virtual ~GuiRenderer() = default;

	// Prepare resources to render the GUI
	virtual void prepare() = 0;

	// Render the GUI
	virtual void render(GuiRenderContext *context) = 0;

	// Destroy the GUI renderers resources
	virtual void destroy() = 0;
};

// A class to manage the GUI
class GUI final
{
  public:
	static void initialize();
	static void destroy();

  public:
	static GUI *instance();

	static void use_style(std::unique_ptr<Style> &&style);

	static void add(std::unique_ptr<Widget> &&widget);

	static void remove(const std::string &id);

	static void draw(float delta_time, const uint32_t width, const uint32_t height);

  private:
	Style	                           *last_style{nullptr};
	std::unique_ptr<Style>               style;
	std::vector<std::unique_ptr<Widget>> widgets;
};
}        // namespace vkb