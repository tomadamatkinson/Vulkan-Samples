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

#include <stdint.h>

#include <tracy/Tracy.hpp>

// malloc and free are used by Tracy to provide memory profiling
void *operator new(size_t count);
void  operator delete(void *ptr) noexcept;

// Tracy a scope
#define PROFILE_SCOPE(name) ZoneScopedN(name)

// Trace a function
#define PROFILE_FUNCTION() ZoneScoped

// Create plots
template <typename T>
class Plot
{
  public:
	static void plot(const char *name, T value, tracy::PlotFormatType format = tracy::PlotFormatType::Number)
	{
		auto *p        = get_instance();
		p->plots[name] = value;
		update_tracy_plot(name, value, format);
	}

	static void increment(const char *name, T amount, tracy::PlotFormatType format = tracy::PlotFormatType::Number)
	{
		auto *p = get_instance();
		p->plots[name] += amount;
		update_tracy_plot(name, p->plots[name], format);
	}

	static void decrement(const char *name, T amount, tracy::PlotFormatType format = tracy::PlotFormatType::Number)
	{
		auto *p = get_instance();
		p->plots[name] -= amount;
		update_tracy_plot(name, p->plots[name], format);
	}

	static void reset(const char *name, tracy::PlotFormatType format = tracy::PlotFormatType::Number)
	{
		auto *p        = get_instance();
		p->plots[name] = T{};
		update_tracy_plot(name, p->plots[name], format);
	}

  private:
	static void update_tracy_plot(const char *name, T value, tracy::PlotFormatType format = tracy::PlotFormatType::Number)
	{
		TracyPlot(name, value);
		TracyPlotConfig(name, format, true, true, 0);
	}

	static Plot *get_instance()
	{
		static_assert((std::is_same<T, int64_t>::value || std::is_same<T, double>::value || std::is_same<T, float>::value), "PlotStore only supports int64_t, double and float");
		static Plot instance;
		return &instance;
	}

	std::unordered_map<const char *, T> plots;
};
