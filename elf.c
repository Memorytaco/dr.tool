#include "elf.h"


bool isValidELFormat(void* memory) {
  octet *target = memory;
  for (int i=ELFMAG0; i<=ELFMAG3; i++) {
    if (MAGIC[i] == target[i]) continue;
    return false;
  }
  return true;
}

int getAbiClass(struct ELFHeader_Pre* hd) {
  if (!isValidELFormat(hd)) return -1;
  switch (hd->e_magic[ELFCLASS]) {
    case ELFCLASS_32:
        return 32;
    case ELFCLASS_64:
        return 64;
  }
  return 0;
}
