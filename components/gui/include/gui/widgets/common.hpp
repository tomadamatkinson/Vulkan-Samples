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
#include <type_traits>

#include <gui/widget.hpp>

namespace vkb
{
class Checkbox : public Widget
{
  public:
	struct Options
	{
		std::string label{"Checkbox"};
		bool       *handle = nullptr;

		std::vector<std::unique_ptr<Widget>> checked_widgets{};
	};

  public:
	Checkbox(const std::string &id, Options &&options) :
	    Widget{id}, options{std::move(options)}
	{
	}

	virtual ~Checkbox() = default;

	virtual void draw(float delta_time) override
	{
		if (options.handle == nullptr)
		{
			options.handle = &checked;
		}

		ImGui::TextUnformatted(options.label.c_str());
		ImGui::SameLine();

		if (ImGui::Checkbox(id.c_str(), options.handle))
		{
			for (auto &widget : options.checked_widgets)
			{
				widget->draw(delta_time);
			}
		}
	}

  private:
	Options options;
	bool    checked = false;
};

template <typename T>
class Input : public Widget
{
  public:
	struct Options
	{
		std::string label{"Input"};
		float      *handle = nullptr;
		float       step   = 0.1f;
		float       step_fast;
	};

  public:
	Input(const std::string &id, Options &&_options) :
	    Widget{id}, options{std::move(_options)}
	{
	}

	virtual ~Input() = default;

	virtual void draw(float delta_time) override
	{
		if (options.handle == nullptr)
		{
			options.handle = &value;
		}

		ImGui::TextUnformatted(options.label.c_str());
		ImGui::SameLine();

		if constexpr (std::is_same_v<T, float>)
		{
			ImGui::InputFloat(id.c_str(), options.handle, options.step, options.step_fast);
		}
		else if constexpr (std::is_same_v<T, int>)
		{
			ImGui::InputInt(id.c_str(), options.handle, options.step, options.step_fast);
		}
	}

  private:
	Options options;
	T       value = 0.0f;
};

template <typename T>
class Slider : public Widget
{
  public:
	struct Options
	{
		std::string label{"Slider"};
		T		  *handle = nullptr;
		T           min    = 0;
		T           max    = 1;
	};

  public:
	Slider(const std::string &id, Options &&_options) :
	    Widget{id}, options{std::move(_options)}
	{
	}

	virtual ~Slider() = default;

	virtual void draw(float delta_time) override
	{
		if (options.handle == nullptr)
		{
			options.handle = &value;
		}

		ImGui::TextUnformatted(options.label.c_str());
		ImGui::SameLine();

		if constexpr (std::is_same_v<T, int>)
		{
			ImGui::SliderInt(id.c_str(), options.handle, options.min, options.max);
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			ImGui::SliderFloat(id.c_str(), options.handle, options.min, options.max);
		}
	}

  private:
	Options options;
	T       value = 0;
};

class ComboBox : public Widget
{
  public:
	struct Options
	{
		std::string              label{"Combo"};
		int		             *handle = nullptr;
		std::vector<std::string> items{};
	};

  public:
	ComboBox(const std::string &id, Options &&_options) :
	    Widget{id}, options{std::move(_options)}

	{
	}

	virtual ~ComboBox() = default;

	virtual void draw(float delta_time) override
	{
		if (options.handle == nullptr)
		{
			options.handle = &value;
		}

		ImGui::TextUnformatted(options.label.c_str());
		ImGui::SameLine();

		if (ImGui::BeginCombo(id.c_str(), options.items[value].c_str()))
		{
			for (int i = 0; i < options.items.size(); i++)
			{
				bool is_selected = (value == i);
				if (ImGui::Selectable(options.items[i].c_str(), is_selected))
				{
					value = i;
				}

				if (is_selected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

  private:
	Options options;
	int     value = 0;
};

class Text : public Widget
{
  public:
	Text(const std::string &id, const std::string &text) :
	    Widget{id}, text{text}
	{
	}

	virtual ~Text() = default;

	virtual void draw(float delta_time) override
	{
		ImGui::TextUnformatted(text.c_str());
	}

  private:
	std::string text;
};
}        // namespace vkb