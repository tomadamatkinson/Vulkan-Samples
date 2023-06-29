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

#include <imgui.h>

// Styles
namespace vkb
{
class Style
{
  public:
	virtual void apply(ImGuiStyle &style) = 0;
};

// ImGui Default Classic Theme
class ClassicTheme : public Style
{
  public:
	void apply(ImGuiStyle &style) override;
};

// ImGui Default Light Theme
class LightTheme : public Style
{
  public:
	void apply(ImGuiStyle &style) override;
};

// ImGui Default Dark Theme
class DarkTheme : public Style
{
  public:
	void apply(ImGuiStyle &style) override;
};

// A clean engine style theme
// Adaptation of https://github.com/ocornut/imgui/issues/707#issuecomment-917151020
class EngineDarkTheme : public Style
{
  public:
	void apply(ImGuiStyle &style) override;
};
}        // namespace vkb