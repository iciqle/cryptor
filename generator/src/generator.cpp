#include "generator.h"

#include <Windows.h>

#include <fstream>
#include <optional>
#include <tuple>
#include <string_view>
#include <algorithm>

#include "pe.h"

namespace fs = std::filesystem;

namespace gen {

bool is_valid_file(path_t const &path) {
  return fs::exists(path) && fs::is_regular_file(path);
}

std::optional<binary_t> read_binary(path_t const &path) {
  if (!is_valid_file(path))
    return std::nullopt;

  std::ifstream file{ path, std::ios::in | std::ios::binary };
  if (!file.good())
    return std::nullopt;

  binary_t result{ };
  result.resize(file.seekg(0, std::ios::end).tellg());
  file.seekg(std::ios::beg).read(reinterpret_cast<char *>(result.data()), result.size());

  return std::optional<binary_t>{ std::move(result) };
}

bool generator::load_stub(path_t const &path) {
  std::optional<binary_t> result = read_binary(path);
  if (!result.has_value())
    return false;

  stub_ = result.value();
  return true;
}

bool generator::load_payload(path_t const &path) {
  std::optional<binary_t> result = read_binary(path);
  if (!result.has_value())
    return false;

  payload_ = result.value();
  return true;
}

std::uint32_t align(std::uint32_t value, std::uint32_t boundary) {
  if (value % boundary)
    value = (value + boundary) - value % boundary;
  return value;
}

template <class T>
std::tuple<std::uint8_t *, std::uint8_t *> mem_bounds(T *value) {
  auto address = reinterpret_cast<std::uint8_t *>(value);
  return std::make_tuple(address, address + sizeof(T));
}

void insert_payload(binary_t &binary, binary_t const &payload) {
  IMAGE_NT_HEADERS *nt_hdr = pe::nt_header(binary.data());

  std::uint32_t const sect_align = nt_hdr->OptionalHeader.SectionAlignment;
  IMAGE_SECTION_HEADER const *last_sect_hdr = pe::sect_header(nt_hdr, pe::sect_cnt(nt_hdr) - 1);
  auto payload_size = static_cast<std::uint32_t>(payload.size());
  IMAGE_SECTION_HEADER sect_hdr = {
    .Name = ".bin",
    .Misc = {.VirtualSize = payload_size },
    .VirtualAddress = align(last_sect_hdr->VirtualAddress + last_sect_hdr->Misc.VirtualSize, sect_align),
    .SizeOfRawData = payload_size,
    .PointerToRawData = static_cast<std::uint32_t>(binary.size()),
    .PointerToRelocations = 0,
    .PointerToLinenumbers = 0,
    .NumberOfRelocations = 0,
    .NumberOfLinenumbers = 0,
    .Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_CNT_INITIALIZED_DATA | 0x1337
  };

  auto const &[sect_hdr_start, sect_hdr_end] = mem_bounds(&sect_hdr);
  std::uint64_t const insert_offset = reinterpret_cast<std::uint64_t>(last_sect_hdr + 1) 
                                    - reinterpret_cast<std::uint64_t>(binary.data());
  binary.insert(binary.begin() + insert_offset, sect_hdr_start, sect_hdr_end); // Pointers to .data() must be updated
  nt_hdr = pe::nt_header(binary.data());

  ++nt_hdr->FileHeader.NumberOfSections;
  nt_hdr->OptionalHeader.SizeOfImage += align(payload_size + sizeof(sect_hdr), sect_align);
  nt_hdr->OptionalHeader.SizeOfInitializedData += align(payload_size, sect_align);
  nt_hdr->OptionalHeader.SizeOfHeaders += sizeof(sect_hdr);

  //Update file offsets to account for inserted header
  for (int i = 0; i < pe::sect_cnt(nt_hdr); ++i) {
    IMAGE_SECTION_HEADER *hdr = pe::sect_header(nt_hdr, i);
    if (!hdr->SizeOfRawData)
      continue;
    hdr->PointerToRawData += sizeof(sect_hdr);
  }

  //Re-align section file offsets and determine required padding insertions
  std::uint32_t inserted_padding = 0;
  std::uint32_t const file_align = nt_hdr->OptionalHeader.FileAlignment;
  std::vector<std::tuple<std::uint32_t, std::uint32_t>> padding_targets{ };
  for (int i = 0; i < pe::sect_cnt(nt_hdr); ++i) {
    IMAGE_SECTION_HEADER *hdr = pe::sect_header(nt_hdr, i);
    if (!hdr->SizeOfRawData)
      continue;
    std::uint32_t const orig = hdr->PointerToRawData + inserted_padding;
    std::uint32_t aligned = align(orig, file_align);
    std::uint32_t const padding = aligned - orig;
    padding_targets.push_back(std::make_tuple(orig, padding));
    inserted_padding += padding;

    hdr->PointerToRawData = aligned;
  }

  for (auto const &[offset, amount] : padding_targets) {
    binary.insert(binary.begin() + offset, amount, 0);
  }

  binary.insert(binary.end(), payload.begin(), payload.end());
}

void encrypt_binary(binary_t &binary) {
  constexpr std::string_view key = "hello world";
  int cursor = 0;
  std::transform(binary.begin(), binary.end(), binary.begin(), [&] (std::uint8_t byte) {
    cursor = cursor % key.length();
    return byte ^ key.at(cursor++);
  });
}

binary_t generator::generate() const {
  binary_t result = stub_;
  binary_t payload = payload_;
  encrypt_binary(payload);
  insert_payload(result, payload);
  return result;
}

bool generator::save(binary_t const &binary, path_t const &path) const {
  std::ofstream file{ path, std::ios::out | std::ios::binary };
  if (!file.good())
    return false;

  file.write(reinterpret_cast<char const *>(binary.data()), binary.size());
  return true;
}

generator inst{ };
generator &get() { return inst; }

}