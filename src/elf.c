#include "elf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool isELFormat(void* memory) {
  octet *target = memory;
  for (int i=ELFMAG0; i<=ELFMAG3; i++) {
    if (MAGIC[i] == target[i]) continue;
    return false;
  }
  return true;
}

int getABIClass(struct ELFHeader_Pre* hd) {
  if (!isELFormat(hd)) return -1;
  switch (hd->e_magic[ELFCLASS]) {
    case ELFCLASS_32:
        return 32;
    case ELFCLASS_64:
        return 64;
  }
  return 0;
}

char* const get_filetype_str(void* mem)
{
  static char* str[] = {
    "unknown",
    "relocatable",
    "executable",
    "shared",
    "coredump"
  };
  struct ELFHeader_Pre *hd = mem;
  return str[hd->e_type & 0x07];
}

char* const get_os_str(void* mem)
{
  static char* os[] = {
    "SYSV",
    "HPUX",
    "NETBSD",
    "LINUX",
  };
  struct ELFHeader_Pre *hd = mem;
  return os[hd->e_magic[ELFOSABI] & 0x03];
}

// create an easy manipulated section table from raw memory
struct sectbl build_sectbl(void* mem)
{
  struct sectbl table = {
    .class  = getABIClass(mem),
    .num    = 0,
    .ents   = NULL
  };
  union unihdr hdr = {mem};
  union unisechdr sechdr;
  sechdr.shdr_32 = NULL;
  size_t entry_offset = 0;
  size_t entry_size = 0;

  if (table.class == 32) {
    table.num = hdr.hdr_32->e_shnum;
    entry_offset = hdr.hdr_32->e_shoff;
    entry_size = hdr.hdr_32->e_shentsize;
  } else if (table.class == 64) {
    table.num = hdr.hdr_64->e_shnum;
    entry_offset = hdr.hdr_64->e_shoff;
    entry_size = hdr.hdr_64->e_shentsize;
  } else {
    table.num = -1;
    return table;
  }

  size_t size = sizeof(struct section) * table.num;
  struct section *sections = malloc(size);
  memset(sections, 0, size);
  table.ents = sections;

  qword sh_size = 0;
  qword sh_offset = 0;

  for (int i = 0; i < table.num; i++) {
    void* buffer = malloc(entry_size);
    memmove(buffer, mem+entry_offset+i*entry_size, entry_size);
    sections[i].hdr = buffer;
    // now prepare the section data;
    buffer = NULL;
    if (table.class == 32) {
      sechdr.shdr_32 = (Shdr_32*)sections[i].hdr;
      sh_size = sechdr.shdr_32->sh_size;
      sh_offset = sechdr.shdr_32->sh_offset;
    } else {
      sechdr.shdr_64 = (Shdr_64*)sections[i].hdr;
      sh_size = sechdr.shdr_64->sh_size;
      sh_offset = sechdr.shdr_64->sh_offset;
    }
    if (sh_size != 0) {
      buffer = malloc(sh_size);
      memmove(buffer, mem+sh_offset, sh_size);
    }
    sections[i].sec = buffer;
  }
  return table;
}

void free_sectbl(struct sectbl *tb)
{
  if (tb->num == -1) return;
  for (int i = 0; i < tb->num; i++) {
    free(tb->ents[i].hdr);
    free(tb->ents[i].sec);
  }
  free(tb->ents);
  tb->num = -1;
  tb->ents = NULL;
  return;
}
