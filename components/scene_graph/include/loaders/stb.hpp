#pragma once

#include <scene_graph/assets/image.hpp>

namespace vkb
{
class StbImageLoader final : public sg::ImageLoader
{
  public:
	StbImageLoader(sg::ContentType content_type = sg::ContentType::Unknown) :
	    content_type{content_type} {};

	sg::Image from_memory(const std::string &name, std::vector<uint8_t> &&data) override;

  private:
	sg::ContentType content_type;
};
}        // namespace vkb