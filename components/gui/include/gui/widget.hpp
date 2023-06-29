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
#include <string>
#include <vector>

#include <imgui.h>
#include <implot.h>

namespace vkb
{
// Widget is a base class for all widgets
// A widget represents a UI element
class Widget
{
	friend class GUI;

  public:
	Widget(const std::string &_id) :
	    id{_id}
	{
		if (id.substr(0, 2) != "##")
		{
			id = "##" + id;
		}
	}

	virtual ~Widget() = default;

	// start drawing the widget elements
	virtual void draw(float delta_time) = 0;

  protected:
	std::string id;
};

class WidgetBuilderInterface
{
  public:
	WidgetBuilderInterface(const std::string &_id) :
	    id{_id}
	{
		if (id.substr(0, 2) != "##")
		{
			id = "##" + id;
		}
	}

	virtual ~WidgetBuilderInterface() = default;

	virtual std::unique_ptr<Widget> done() = 0;

  protected:
	std::string id;
};

// A type trait to retrieve a widget builder for a given type
template <typename T>
struct widget_builder
{
	typedef T type;
};

// WidgetBuilder is a base class for all widget builders
class WidgetBuilder : public WidgetBuilderInterface
{
  public:
	WidgetBuilder(const std::string &id) :
	    WidgetBuilderInterface{id} {};
	virtual ~WidgetBuilder() = default;

	virtual WidgetBuilder &with(std::unique_ptr<Widget> &&widget);

	template <typename T, typename... Args>
	WidgetBuilder &with(const std::string &id, Args &&...args)
	{
		static_assert(std::is_base_of<Widget, T>::value, "T must be a widget");
		return with(std::make_unique<T>(id, std::forward<Args>(args)...));
	}

	virtual std::unique_ptr<Widget> done();

  protected:
	std::vector<std::unique_ptr<Widget>> widgets;
};

template <typename T>
typename widget_builder<T>::type ui(const std::string &id)
{
	return typename widget_builder<T>::type(id);
}
}        // namespace vkb