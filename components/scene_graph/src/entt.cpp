#include <scene_graph/entt.hpp>

namespace vkb
{
namespace sg
{
RegistryPtr make_registry() noexcept
{
	return std::make_shared<entt::registry>();
}
}        // namespace sg
}        // namespace vkb