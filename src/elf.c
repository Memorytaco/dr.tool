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
    .name   = 0,
    .ents   = NULL
  };
  union unihdr hdr = {mem};
  union unisechdr sechdr;
  sechdr.shdr_32 = NULL;
  size_t entry_offset = 0;
  size_t entry_size = 0;

  if (table.class == 32) {
    table.num = hdr.hdr_32->e_shnum;
    table.name = hdr.hdr_32->e_shstrndx;
    entry_offset = hdr.hdr_32->e_shoff;
    entry_size = hdr.hdr_32->e_shentsize;
  } else if (table.class == 64) {
    table.num = hdr.hdr_64->e_shnum;
    table.name = hdr.hdr_64->e_shstrndx;
    entry_offset = hdr.hdr_64->e_shoff;
    entry_size = hdr.hdr_64->e_shentsize;
  } else {
    table.num = 0;
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
    sechdr.all = sections[i].hdr;
    if (table.class == 32) {
      sh_size = sechdr.shdr_32->sh_size;
      sh_offset = sechdr.shdr_32->sh_offset;
    } else {
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
  tb->name = 0;
  tb->num = -1;
  tb->ents = NULL;
  return;
}

char const* sectbl_getname(struct sectbl const * tb, size_t idx)
{
  const char* toret = "null";
  union unisechdr hdr;
  hdr.all = tb->ents[idx].hdr;
  if (tb->name == 0) return toret;
  if (tb->class == 32)
    toret = tb->ents[tb->name].sec + hdr.shdr_32->sh_name;
  else
    toret = tb->ents[tb->name].sec + hdr.shdr_64->sh_name;
  return toret;
}

size_t sectbl_search(struct sectbl const * tb, char* name)
{
  for (size_t i = 1; i<tb->num; i++) {
    if (!strcmp(name, sectbl_getname(tb, i)))
      return i;
  }
  return 0;
}

///////////////////////////////////////////////
// Elf File Symbol info operation functions  //
///////////////////////////////////////////////

const char* get_symbind(octet info)
{
  static const char* binds[16] = {
    "LOCAL",
    "GLOBAL",
    "WEAK",
    [13] = "LOPROC",
    [15] = "HIPROC"
  };
  return binds[SYM_BIND_FIELD(info)];
}
const char* get_symtype(octet info)
{
  static const char* types[16] = {
    "NOTYPE",
    "OBJECT",
    "FUNC",
    "SECTION",
    "FILE",
    "COMMON",
    "TLS",
    "NUM",
    [10] = "LOOS",
    [12] = "HIOS",
    [13] = "LOPROC",
    [15] = "HIPROC"
  };
  return types[SYM_TYPE_FIELD(info)];
}

