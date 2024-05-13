#pragma once
#include <Windows.h>

#include <cstdint>

namespace pe {

inline IMAGE_DOS_HEADER const *dos_header(std::uint8_t const *base) {
  return reinterpret_cast<IMAGE_DOS_HEADER const *>(base);
}

inline IMAGE_DOS_HEADER *dos_header(std::uint8_t *base) {
  return reinterpret_cast<IMAGE_DOS_HEADER *>(base);
}

inline IMAGE_NT_HEADERS const *nt_header(std::uint8_t const *base) {
  return reinterpret_cast<IMAGE_NT_HEADERS const *>(base + dos_header(base)->e_lfanew);
}

inline IMAGE_NT_HEADERS *nt_header(std::uint8_t *base) {
  return reinterpret_cast<IMAGE_NT_HEADERS *>(base + dos_header(base)->e_lfanew);
}

inline int sect_cnt(IMAGE_NT_HEADERS const *nt_header) {
  return nt_header->FileHeader.NumberOfSections;
}

inline IMAGE_SECTION_HEADER const *sect_header(IMAGE_NT_HEADERS const *nt_header, int index) {
  IMAGE_SECTION_HEADER const *sections = IMAGE_FIRST_SECTION(nt_header);
  return &sections[index];
}

inline IMAGE_SECTION_HEADER *sect_header(IMAGE_NT_HEADERS *nt_header, int index) {
  IMAGE_SECTION_HEADER *sections = IMAGE_FIRST_SECTION(nt_header);
  return &sections[index];
}

}