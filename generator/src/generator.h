#pragma once
#include <cstdint>
#include <vector>
#include <filesystem>

namespace gen {

using binary_t  = std::vector<std::uint8_t>;
using path_t    = std::filesystem::path;

class generator {
public:
  bool      load_stub(path_t const &path);
  bool      load_payload(path_t const &path);
  binary_t  generate() const;
  bool      save(binary_t const &binary, path_t const &path) const;

private:
  binary_t stub_;
  binary_t payload_;
};

generator &get();

}