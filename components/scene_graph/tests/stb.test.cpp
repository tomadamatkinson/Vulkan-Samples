#include <catch2/catch_test_macros.hpp>

#include <loaders/stb.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// Creates a 256x256 red image
std::vector<uint8_t> create_png(size_t width, size_t height)
{
	std::vector<uint8_t> pixels(width * height * 4);
	for (size_t i = 0; i < width * height; ++i)
	{
		pixels[i * 4 + 0] = 255;
		pixels[i * 4 + 1] = 0;
		pixels[i * 4 + 2] = 0;
		pixels[i * 4 + 3] = 255;
	}

	int                  len;
	auto                *data = stbi_write_png_to_mem(pixels.data(), width * 4 * sizeof(pixels[0]), width, height, 4, &len);
	std::vector<uint8_t> image(data, data + len);
	STBIW_FREE(data);
	return image;
}

TEST_CASE("StbImageLoader", "[scene_graph]")
{
	SECTION("decode png")
	{
		auto image = create_png(256, 256);

		auto decoded_image = vkb::StbImageLoader{}.from_memory("image.png", std::move(image));

		REQUIRE(decoded_image.width == 256);
		REQUIRE(decoded_image.height == 256);
		REQUIRE(decoded_image.format == vk::Format::eR8G8B8A8Unorm);
		REQUIRE(decoded_image.data.size == 256 * 256 * 4);
		REQUIRE(decoded_image.data.offset == 0);
		REQUIRE(decoded_image.data.raw->size() == 256 * 256 * 4);
		REQUIRE(decoded_image.layers == 1);
		REQUIRE(decoded_image.levels == 1);
	}
}