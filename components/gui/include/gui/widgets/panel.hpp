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
class Panel : public Widget
{
  public:
	Panel(const std::string &id, const std::string &title, std::vector<std::unique_ptr<Widget>> &&widgets = {}) :
	    Widget{id}, title{title}, widgets{std::move(widgets)}
	{}

	virtual ~Panel() = default;

	virtual void draw(float delta_time) override
	{
		ImGui::Begin(title.c_str());

		for (auto &widget : widgets)
		{
			widget->draw(delta_time);
		}

		ImGui::End();
	}

  private:
	std::string                          title;
	std::vector<std::unique_ptr<Widget>> widgets;
};

class PanelBuilder : public WidgetBuilder
{
  public:
	PanelBuilder(const std::string &id) :
	    WidgetBuilder{id}
	{}

	~PanelBuilder() override = default;

	PanelBuilder &title(const std::string &title)
	{
		_title = title;
		return *this;
	}

	std::unique_ptr<Widget> done() override
	{
		return std::make_unique<Panel>(id, _title, std::move(widgets));
	}

	std::string _title;
};

template <>
struct widget_builder<Panel>
{
	typedef PanelBuilder type;
};

}        // namespace vkb