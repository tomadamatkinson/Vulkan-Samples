#pragma once

#include <entt/entt.hpp>

namespace vkb
{
using namespace entt;

using RegistryPtr = std::shared_ptr<entt::registry>;

// Create a new registry
RegistryPtr make_registry() noexcept;
}        // namespace vkb