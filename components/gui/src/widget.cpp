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

#include "gui/widget.hpp"

namespace vkb
{

// WidgetGroup is a widget that contains other widgets
class WidgetGroup final : public Widget
{
  public:
	WidgetGroup(const std::string &id, std::vector<std::unique_ptr<Widget>> &&widgets) :
	    Widget{id}, widgets{std::move(widgets)}
	{
	}

	~WidgetGroup() override = default;

	void draw(float delta_time)
	{
		for (auto &widget : widgets)
		{
			widget->draw(delta_time);
		}
	}

  private:
	std::vector<std::unique_ptr<Widget>> widgets;
};

WidgetBuilder &WidgetBuilder::with(std::unique_ptr<Widget> &&widget)
{
	widgets.push_back(std::move(widget));
	return *this;
}

std::unique_ptr<Widget> WidgetBuilder::done()
{
	return std::make_unique<WidgetGroup>(id, std::move(widgets));
}

}        // namespace vkb