#pragma once

#include <scene_graph/assets/image.hpp>

namespace vkb
{
class AstcImageLoader : public sg::ImageLoader
{
  public:
	sg::Image from_memory(const std::string& name, std::vector<uint8_t> &&data) override;
};
}        // namespace vkb