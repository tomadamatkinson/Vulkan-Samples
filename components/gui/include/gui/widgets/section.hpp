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

#include <string>

#include <gui/widget.hpp>

namespace vkb
{
class Section : public Widget
{
  public:
	struct Options
	{
		std::string title;
		bool        border = false;
		uint32_t    width  = 0;
		uint32_t    height = 0;
	};

  public:
	Section(const std::string &id, const Options &options, std::vector<std::unique_ptr<Widget>> &&widgets = {}) :
	    Widget{id}, options{options}, widgets{std::move(widgets)}
	{}

	virtual ~Section() = default;

	virtual void draw(float delta_time) override
	{
		if (ImGui::CollapsingHeader(options.title.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::BeginChild(id.c_str(), ImVec2(options.width, options.height), options.border);
			for (auto &widget : widgets)
			{
				widget->draw(delta_time);
			}
			ImGui::EndChild();
		}
	}

  private:
	Options                              options;
	std::vector<std::unique_ptr<Widget>> widgets;
};

class SectionBuilder : public WidgetBuilder
{
  public:
	SectionBuilder(const std::string &id) :
	    WidgetBuilder{id}
	{}
	~SectionBuilder() override = default;

	SectionBuilder &title(const std::string &title)
	{
		options.title = title;
		return *this;
	}

	SectionBuilder &border(bool border)
	{
		options.border = border;
		return *this;
	}

	SectionBuilder &width(uint32_t width)
	{
		options.width = width;
		return *this;
	}

	SectionBuilder &height(uint32_t height)
	{
		options.height = height;
		return *this;
	}

	std::unique_ptr<Widget> done() override
	{
		return std::make_unique<Section>(id, options, std::move(widgets));
	}

  private:
	Section::Options options;
};

template <>
struct widget_builder<Section>
{
	typedef SectionBuilder type;
};

}        // namespace vkb