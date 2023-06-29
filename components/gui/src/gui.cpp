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

#include "gui/gui.hpp"

#include "gui/style.hpp"

namespace vkb
{

void GUI::initialize()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void) io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;        // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;         // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;            // Enable Docking
	io.ConfigFlags |= ImGuiWindowFlags_AlwaysAutoResize;
}

void GUI::destroy()
{
	ImPlot::DestroyContext();
	ImGui::DestroyContext();
}

GUI *GUI::instance()
{
	static std::unique_ptr<GUI> instance = std::make_unique<GUI>();
	return instance.get();
}

void GUI::use_style(std::unique_ptr<Style> &&style)
{
	instance()->style = std::move(style);
}

void GUI::add(std::unique_ptr<Widget> &&widget)
{
	auto *gui = instance();

	for (auto it = gui->widgets.begin(); it != gui->widgets.end(); ++it)
	{
		if ((*it)->id == widget->id)
		{
			gui->widgets.erase(it);
			break;
		}
	}

	gui->widgets.push_back(std::move(widget));
}

void GUI::remove(const std::string &id)
{
	std::string id_with_hash = "##" + id;
	auto       &widgets      = instance()->widgets;
	for (auto it = widgets.begin(); it != widgets.end();)
	{
		if ((*it)->id == id_with_hash)
		{
			it = widgets.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void GUI::draw(float delta_time, const uint32_t width, const uint32_t height)
{
	auto *gui = instance();

	if (!gui->style)
	{
		gui->style = std::make_unique<EngineDarkTheme>();
	}

	if (gui->style && gui->style.get() != gui->last_style)
	{
		gui->style->apply(ImGui::GetStyle());
		gui->last_style = gui->style.get();
	}

	auto &io         = ImGui::GetIO();
	io.DisplaySize.x = static_cast<float>(width);
	io.DisplaySize.y = static_cast<float>(height);
	io.DeltaTime     = delta_time;

	ImGui::NewFrame();
	ImGui::DockSpaceOverViewport();

	for (auto &widget : instance()->widgets)
	{
		widget->draw(delta_time);
	}

	ImGui::EndFrame();

	ImGui::Render();
}
}        // namespace vkb