#include "loaders/stb.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace vkb
{

sg::Image StbImageLoader::from_memory(const std::string &name, std::vector<uint8_t> &&data)
{
	int   width;
	int   height;
	int   comp;
	int   req_comp = 4;
	auto *raw_data = stbi_load_from_memory(data.data(), data.size(), &width, &height, &comp, req_comp);

	if (!raw_data)
	{
		throw std::runtime_error{"Failed to load " + name + ": " + stbi_failure_reason()};
	}

	sg::Image image{
	    .name   = name,
	    .width  = static_cast<uint32_t>(width),
	    .height = static_cast<uint32_t>(height),
	    .depth  = 1u,
	    .format = content_type == sg::ContentType::Color ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm,
	    .data   = sg::DataView::from_memory(std::vector<uint8_t>(raw_data, raw_data + width * height * req_comp)),

	    .layers = 1u,
	    .levels = 1u,
	};

	stbi_image_free(raw_data);

	return image;
}
}        // namespace vkb