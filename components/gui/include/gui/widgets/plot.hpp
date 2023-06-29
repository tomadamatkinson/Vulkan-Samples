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

#include <chrono>
#include <iostream>

#include <gui/widget.hpp>
#include <math.h>

namespace vkb
{
struct ScrollingBuffer
{
	double           MaxValue;
	int              MaxSize;
	int              Offset;
	ImVector<ImVec2> Data;
	ScrollingBuffer(int max_size = 750)
	{
		MaxSize = max_size;
		Offset  = 0;
		Data.reserve(MaxSize);
	}

	void AddPoint(float x, float y)
	{
		static std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

		if (Data.size() < MaxSize)
			Data.push_back(ImVec2(x, y));
		else
		{
			Data[Offset] = ImVec2(x, y);
			Offset       = (Offset + 1) % MaxSize;
		}

		if (std::chrono::steady_clock::now() - begin > std::chrono::seconds(1))
		{
			begin = std::chrono::steady_clock::now();
			calculate_max();
			std::cout << "calculate_max: " << MaxValue << std::endl;        // "calculate_max
		}
	}

	void Erase()
	{
		if (Data.size() > 0)
		{
			Data.shrink(0);
			Offset = 0;
		}
	}

  private:
	void calculate_max()
	{
		MaxValue = 0;
		for (int i = 0; i < Data.size(); i++)
		{
			if (Data[i].y > MaxValue)
			{
				MaxValue = Data[i].y;
			}
		}
	}
};

class Plot : public Widget
{
  public:
	struct Options
	{
		std::string label{"Plot"};
	};

  public:
	Plot(const std::string &id, const Options &options) :
	    Widget{id}, options{options}
	{
	}

	virtual ~Plot() = default;

	virtual void draw(float delta_time) override
	{
		values.push_back(delta_time);

		static ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels;
		static ScrollingBuffer sdata1, sdata2;
		ImVec2                 mouse = ImGui::GetMousePos();
		static float           t     = 0;
		t += ImGui::GetIO().DeltaTime;
		sdata1.AddPoint(t, mouse.x * 0.0005f);
		sdata2.AddPoint(t, mouse.y * 0.0005f);

		static float history = 10.0f;
		ImGui::SliderFloat("History", &history, 1, 30, "%.1f s");

		if (ImPlot::BeginPlot(id.c_str(), ImVec2(-1, 150)))
		{
			ImPlot::SetupAxes(nullptr, nullptr, flags, flags);
			ImPlot::SetupAxisLimits(ImAxis_X1, t - history, t, ImGuiCond_Always);
			ImPlot::SetupAxisLimits(ImAxis_Y1, 0, std::max(sdata1.MaxValue + 0.1f, sdata2.MaxValue + 0.1f), ImGuiCond_Always);
			ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
			ImPlot::PlotShaded("Mouse X", &sdata1.Data[0].x, &sdata1.Data[0].y, sdata1.Data.size(), -INFINITY, 0, sdata1.Offset, 2 * sizeof(float));
			ImPlot::PlotShaded("Mouse Y", &sdata2.Data[0].x, &sdata2.Data[0].y, sdata2.Data.size(), -INFINITY, 0, sdata2.Offset, 2 * sizeof(float));
			ImPlot::EndPlot();
		}
	}

  private:
	Options            options;
	std::vector<float> values;
};
}        // namespace vkb