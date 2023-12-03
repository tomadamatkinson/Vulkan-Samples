#include <scene_graph/entt.hpp>

namespace vkb
{
RegistryPtr make_registry() noexcept
{
	return std::make_shared<entt::registry>();
}
}        // namespace vkb