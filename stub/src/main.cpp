#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <string>

#include "stub.h"

enum class status_t : std::uint8_t {
  success = 0,
  find_binary,
  map_binary,
};

std::unordered_map<status_t, std::string> const errors = {
  { status_t::find_binary,  "unable to find binary" },
  { status_t::map_binary,   "failed to map binary"  }
};

status_t run(int argc, char const **argv) {
  if (!stb::get().find_binary())
    return status_t::find_binary;

  stb::map_status_t status = stb::get().map_binary();
  if (status != stb::map_status_t::succes)
    return status_t::map_binary;

  stb::get().run_binary(argc, argv);
  return status_t::success;
}

int main(int argc, char const **argv) {
  status_t status = run(argc, argv);
  if (status != status_t::success) {
    std::printf("[-] Failed, error: %s\n", errors.at(status).c_str());
    return static_cast<int>(status);
  }
  return 0;
}