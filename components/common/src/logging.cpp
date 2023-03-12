#include "components/common/logging.hpp"

#include <components/common/error.hpp>


VKBP_DISABLE_WARNINGS()
#include <spdlog/fmt/fmt.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
VKBP_ENABLE_WARNINGS()

namespace logging
{
void init_default_logger()
{
	// TODO: Add android support
	auto console = spdlog::stdout_color_mt("console");
	spdlog::set_default_logger(console);
	spdlog::set_level(spdlog::level::trace);
	spdlog::set_pattern("[%^%l%$] %v");
}
}        // namespace logging