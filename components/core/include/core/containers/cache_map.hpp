#pragma once

#include <functional>
#include <unordered_map>

#include "core/containers/container_wrapper.hpp"

namespace vkb
{
// Stores a map of keys to values, where the values are created on demand using a factory function.
template <typename Key, typename Value, typename Container = std::unordered_map<Key, Value>>
class CacheMap : public ContainerWrapper<Key, Value, Container>
{
  public:
	using iterator       = typename Container::iterator;
	using const_iterator = typename Container::const_iterator;

	CacheMap()                            = default;
	CacheMap(const CacheMap &)            = delete;
	CacheMap(CacheMap &&)                 = default;
	CacheMap &operator=(const CacheMap &) = delete;
	CacheMap &operator=(CacheMap &&)      = default;

	// Finds the value at key, or inserts a new value if it doesn't exist.
	virtual iterator find_or_insert(const Key &key, std::function<Value()> &&create)
	{
		auto it = container.find(key);
		if (it == container.end())
		{
			it = container.emplace(key, std::move(create())).first;
		}
		return it;
	}

	// Replaces the value at key with the given value.
	// The value must be move assignable or move constructible.
	virtual iterator replace_emplace(const Key &key, Value &&value)
	{
		auto it = container.find(key);
		if (it == container.end())
		{
			return container.emplace(key, std::move(value)).first;
		}

		if constexpr (std::is_move_assignable_v<Value>)
		{
			it->second = std::move(value);
			return it;
		}
		else if constexpr (std::is_move_constructible_v<Value>)
		{
			container.erase(it);
			return container.emplace(key, std::move(value)).first;
		}

		static_assert(std::is_move_assignable_v<Value> || std::is_move_constructible_v<Value>,
		              "Value must be move assignable or move constructible");
	}

	using Wrapper = ContainerWrapper<Key, Value, Container>;
	using Wrapper::begin;
	using Wrapper::clear;
	using Wrapper::empty;
	using Wrapper::end;

  protected:
	using ContainerWrapper<Key, Value, Container>::container;
};
}        // namespace vkb