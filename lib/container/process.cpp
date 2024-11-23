#include <bit>
#include <cstdint>

template<typename T>
T read(const void* buf) {
  return std::byteswap(*static_cast<const T*>(buf));
}

const char* buf;
uint64_t id = read<uint64_t>(buf + 8);

