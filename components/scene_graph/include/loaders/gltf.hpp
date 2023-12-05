#pragma once

#include <scene_graph/scene_graph.hpp>

namespace vkb
{
class GltfLoader final
{
  public:
	struct Options
	{
		std::string              scene_name;
		std::vector<std::string> extensions;
		std::vector<std::string> excluded_node_names;
	};

	static Options default_options;

	GltfLoader(const Options &options = default_options) :
	    options{options} {};

	sg::NodePtr from_memory(std::vector<uint8_t> &&data) const;

  private:
	Options options;
};
}        // namespace vkb