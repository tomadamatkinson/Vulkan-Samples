#pragma once

#include <memory>
#include <vector>

namespace vkb
{
namespace sg
{

// A buffer of raw data
using RawPtr = std::shared_ptr<std::vector<uint8_t>>;

// A view into a buffer of raw data
// The data is owned by the raw buffer
// The buffer is not responsible for freeing the data
struct DataView
{
	const RawPtr raw;
	size_t       offset;
	size_t       size;

	static DataView from_memory(std::vector<uint8_t> &&data)
	{
		size_t size = data.size();
		return {
		    std::make_shared<std::vector<uint8_t>>(std::move(data)),
		    0,
		    size,
		};
	}
};

}        // namespace sg
}        // namespace vkb