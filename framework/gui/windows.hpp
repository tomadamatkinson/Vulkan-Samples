/* Copyright (c) 2018-2023, Arm Limited and Contributors
 * Copyright (c) 2019-2023, Sascha Willems
 * Copyright (c) 2023, Tom Atkinson
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

#include <gui/gui.hpp>
#include <gui/widgets/panel.hpp>

namespace vkb
{
const char *APP_INFO_WINDOW_ID = "app_info_window";

std::unique_ptr<Widget> create_app_info_window(const std::string &app_name)
{
	return ui<Panel>(APP_INFO_WINDOW_ID)
	    .title(app_name)
	    .done();
}

const char *STATS_WINDOW_ID = "stats_window";

std::unique_ptr<Widget> create_stats_window()
{
	return ui<Panel>(STATS_WINDOW_ID)
	    .title("Stats")
	    .done();
}

}        // namespace vkb