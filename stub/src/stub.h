#pragma once
#include <cstdint>

namespace stb {

enum class map_status_t : std::uint8_t {
  succes = 0,
  allocate_image,
  relocations,
  imports,
  sect_protect
};

class stub {
public:
  bool          find_binary();
  void          decrypt_binary();
  map_status_t  map_binary();
  int           run_binary(int argc, char const **argv);

private:
  using entry_t = int(*)(int argc, char const **argv);

  std::uint8_t *binary_         = nullptr;
  std::uint32_t raw_size_       = 0;
  std::uint8_t *mapped_base_    = nullptr;
  std::uint32_t mapped_size_    = 0;
  entry_t entry_                = nullptr;
};

stub &get();

}