#include <iostream>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "generator.h"

enum class status_t : std::uint8_t {
  success = 0,
  argument_cnt,
  load_stub,
  load_payload,
  save_result
};

std::unordered_map<status_t, std::string> const errors = {
  { status_t::argument_cnt, "incorrect number of arguments" },
  { status_t::argument_cnt, "failed to load stub" },
  { status_t::argument_cnt, "failed to load payload" },
  { status_t::argument_cnt, "failed to save generated binary" },
};

status_t run(int argc, char const **argv) {
  if (argc < 3)
    return status_t::argument_cnt;
  
  if (!gen::get().load_stub(argv[1]))
    return status_t::load_stub;

  if (!gen::get().load_payload(argv[2]))
    return status_t::load_payload;

  gen::binary_t result = gen::get().generate();
  if (!gen::get().save(result, "out.exe"))
    return status_t::save_result;

  return status_t::success;
}

int main(int argc, char const **argv) {
  status_t const status = run(argc, argv);
  if (status != status_t::success) {
    std::printf("[-] Generator failed, error: %s\n", errors.at(status).c_str());
    return static_cast<int>(status);
  }
  std::printf("[-] Generator finished\n");
  return 0;
}