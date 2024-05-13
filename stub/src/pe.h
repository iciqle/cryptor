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

inline IMAGE_BASE_RELOCATION *reloc_hdr(std::uint8_t *block) {
  return reinterpret_cast<IMAGE_BASE_RELOCATION *>(block);
}

struct reloc_entry {
  std::uint16_t offset  : 12;
  std::uint16_t type    : 4;
};

inline int reloc_cnt(std::uint8_t *block) {
  IMAGE_BASE_RELOCATION *hdr = reloc_hdr(block);
  return (hdr->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(reloc_entry);
}

inline std::uint32_t sect_protect(std::uint32_t characteristics) {
  std::uint32_t constexpr ro  = IMAGE_SCN_MEM_READ;
  std::uint32_t constexpr re  = ro | IMAGE_SCN_MEM_EXECUTE;
  std::uint32_t constexpr rw  = ro | IMAGE_SCN_MEM_WRITE;
  std::uint32_t constexpr rwe = rw | re;

  switch (characteristics & rwe) {
    case ro:  return PAGE_READONLY;
    case re:  return PAGE_EXECUTE_READ;
    case rw:  return PAGE_READWRITE;
    case rwe: return PAGE_EXECUTE_READWRITE;
    default: return PAGE_READONLY;
  }
}

}