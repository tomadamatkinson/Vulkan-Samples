#pragma once

#include "buffer.hpp"

#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace sg
{
/**
 * @brief Type of content held in image.
 * This helps to steer the image loaders when deciding what the format should be.
 * Some image containers don't know whether the data they contain is sRGB or not.
 * Since most applications save color images in sRGB, knowing that an image
 * contains color data helps us to better guess its format when unknown.
 */
enum ContentType
{
	Unknown,
	Color,
	Other
};

struct Image
{
	std::string name;

	vk::Format format;
	uint32_t   width;
	uint32_t   height;
	uint32_t   depth;
	uint32_t   layers;
	uint32_t   levels;

	DataView data;
};

// Loads images from files or memory
class ImageLoader
{
  public:
	virtual ~ImageLoader() = default;

	// Load an image from a file
	// Calls from_memory with the loaded data
	// TODO: When FS merges
	// Image from_file(const std::string &name, const std::string &path);

	// Load an image from memory
	virtual Image from_memory(const std::string &name, std::vector<uint8_t> &&data) = 0;
};
}        // namespace sg
}        // namespace vkb