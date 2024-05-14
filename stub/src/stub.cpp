#include "stub.h"

#include <Windows.h>

#include <algorithm>
#include <unordered_set>
#include <string_view>

#include "pe.h"

namespace stb {

bool is_binary_section(std::uint32_t characteristics) {
  return (characteristics & 0x1337) == 0x1337;
}

bool stub::find_binary() {
  auto base = reinterpret_cast<std::uint8_t *>(GetModuleHandleA(nullptr));
  if (!base)
    return false;
  
  IMAGE_NT_HEADERS const *nt_hdr = pe::nt_header(base);
  for (int i = 0; i < pe::sect_cnt(nt_hdr); ++i) {
    IMAGE_SECTION_HEADER const *sect_hdr = pe::sect_header(nt_hdr, i);
    if (is_binary_section(sect_hdr->Characteristics)) {
      binary_ = base + sect_hdr->VirtualAddress;
      raw_size_ = sect_hdr->SizeOfRawData;
      return true;
    }
  }

  return false;
}

bool perform_relocations(IMAGE_NT_HEADERS const *nt_hdr, std::uint8_t *base) {
  auto block = base + nt_hdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
  std::uint64_t const delta = reinterpret_cast<std::uint64_t>(base) - nt_hdr->OptionalHeader.ImageBase;
  for (IMAGE_BASE_RELOCATION *block_hdr = pe::reloc_hdr(block); block_hdr->SizeOfBlock; block_hdr = pe::reloc_hdr(block)) {
    auto relocs = reinterpret_cast<pe::reloc_entry *>(block + sizeof(IMAGE_BASE_RELOCATION));
    for (int i = 0; i < pe::reloc_cnt(block); ++i) {
      pe::reloc_entry const &reloc = relocs[i];
      switch (reloc.type) {
        case IMAGE_REL_BASED_ABSOLUTE: continue;
        case IMAGE_REL_BASED_DIR64:
          *reinterpret_cast<std::uint64_t *>(base + block_hdr->VirtualAddress + reloc.offset) += delta;
          break;
        default: return false;
      }
    }
    block += block_hdr->SizeOfBlock;
  }

  return true;
}

bool resolve_imports(IMAGE_NT_HEADERS const *nt_hdr, std::uint8_t *base) {
  auto desc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(base + nt_hdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
  while (desc->Characteristics) {
    auto lib_name = reinterpret_cast<char const *>(base + desc->Name);
    HMODULE lib = LoadLibraryA(lib_name);
    if (!lib)
      return false;

    auto ilt = reinterpret_cast<std::uint64_t const *>(base + desc->OriginalFirstThunk);
    auto iat = reinterpret_cast<std::uint64_t *>(base + desc->FirstThunk);

    for (int i = 0; ilt[i]; ++i) {
      char const *name = nullptr;

      if (IMAGE_SNAP_BY_ORDINAL(ilt[i])) {
        name = reinterpret_cast<char const *>(IMAGE_ORDINAL(ilt[i]));
      }
      else {
        auto by_name = reinterpret_cast<IMAGE_IMPORT_BY_NAME *>(base + ilt[i]);
        name = by_name->Name;
      }

      iat[i] = reinterpret_cast<std::uint64_t>(GetProcAddress(lib, name));
      if (!iat[i])
        return false;
    }
    ++desc;
  }
  return true;
}

bool protect_sections(IMAGE_NT_HEADERS const *nt_hdr, std::uint8_t *base) {
  for (int i = 0; i < pe::sect_cnt(nt_hdr); ++i) {
    IMAGE_SECTION_HEADER const *sect_hdr = pe::sect_header(nt_hdr, i);
    unsigned long protect = pe::sect_protect(sect_hdr->Characteristics);
    if (!VirtualProtect(base + sect_hdr->VirtualAddress, sect_hdr->Misc.VirtualSize, protect, &protect))
      return false;
  }
  return true;
}

void stub::decrypt_binary() {
  IMAGE_NT_HEADERS const *nt_hdr = pe::nt_header(binary_);
  constexpr std::string_view key = "hello world";
  int cursor = 0;
  std::transform(binary_, binary_ + raw_size_, binary_, [&] (std::uint8_t byte) {
    cursor = cursor % key.length();
    return byte ^ key.at(cursor++);
  });
}

map_status_t stub::map_binary() {
  IMAGE_NT_HEADERS const *nt_hdr = pe::nt_header(binary_);
  mapped_base_ = static_cast<std::uint8_t *>(VirtualAlloc(nullptr, nt_hdr->OptionalHeader.SizeOfImage, 
    MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
  if (!mapped_base_)
    return map_status_t::allocate_image;
  mapped_size_ = nt_hdr->OptionalHeader.SizeOfImage;

  for (int i = 0; i < pe::sect_cnt(nt_hdr); ++i) {
    IMAGE_SECTION_HEADER const *sect_hdr = pe::sect_header(nt_hdr, i);
    auto raw_base = reinterpret_cast<std::uint8_t const *>(binary_ + sect_hdr->PointerToRawData);
    std::copy(raw_base, raw_base + sect_hdr->SizeOfRawData, mapped_base_ + sect_hdr->VirtualAddress);
  }

  if (!perform_relocations(nt_hdr, mapped_base_))
    return map_status_t::relocations;

  if (!resolve_imports(nt_hdr, mapped_base_))
    return map_status_t::imports;

  if (!protect_sections(nt_hdr, mapped_base_))
    return map_status_t::sect_protect;

  entry_ = reinterpret_cast<entry_t>(mapped_base_ + nt_hdr->OptionalHeader.AddressOfEntryPoint);
  return map_status_t::succes;
}

int stub::run_binary(int argc, char const **argv) {
  return entry_(argc, argv);
}

stub inst{ };
stub &get() { return inst; }

}