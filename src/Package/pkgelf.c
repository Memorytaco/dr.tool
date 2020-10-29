#include "pkg.h"
#include "cmd.h"
#include "elf.h"
#include "vara.h"

#include "dtool.h"

char * typesig = "elf:@d2";

void show_ascii(unsigned char* buf, size_t sz)
{
  for (size_t i = 0; i<sz; i++) {
    if (isgraph(buf[i]))
      printf("%c", buf[i]);
    else
      printf(".");
  }
}

void show_hex(unsigned char* buf, int num, int length) {
  int anchor = 0, i = 0;
  do {
    if (i%num == 0) printf("0x%08X ", i);
    if (i%4 == 3) {
      printf("%02X ", buf[i]);
    } else {
      printf("%02X", buf[i]);
    }
    if (i%num == num-1) {
      printf("   ");
      show_ascii(buf+anchor, num);
      printf("\n");
      anchor = i+1;
    }
  } while (++i<length);

  if (length%num != 0) {
    size_t len = length - anchor;
    len = num*2 + (num/4) + 3 - len * 2 - len/4;
    unsigned char *spaces = malloc(len);
    memset(spaces, ' ', len);
    printf("%s", spaces);
    show_ascii(buf+anchor, length - anchor);
    printf("\n");
    free(spaces);
  }
}

static void hex(pkgsig sig) {
  vtra para = pkggetvar(sig);
  show_hex(vptr(unsigned char*, para, 1, 0)
      , vptr(int, para, 1, 1)
      , vptr(int, para, 1, 2));
}

static void sym(pkgsig sig) {
  struct sectbl *tb = pkggetvar(sig);
  size_t symidx = sectbl_search(tb, ".symtab");
  if (symidx == 0) {
    printf ("Error when searching .symtab section.\n");
    exit(-1);
  }
  printf("SYM table Ndx: %4zu %s",
      symidx,
      sectbl_getname(tb, symidx)
      );
  struct section *sec = tb->ents + symidx;
  union unisechdr sechdr = {.all = sec->hdr};
  union unisymhdr symhdr = {.all = sec->sec};

  // color here
  char *clear = setcmd('m', 1, 0);
  char *namecolor = setcmd('m', 3, 38, 5, 153);
  char *hexcolor = setcmd('m', 3, 38, 5, 83);
  char *keycolor = setcmd('m', 3, 38, 5, 196);
  char *specolor = setcmd('m', 3, 38, 5, 11);
  char *spebcolor = setcmd('m', 3, 48, 5, 8);
  char *boldcmd = setcmd('m', 1, 1);

  size_t size = 0;
  struct section *strtb = NULL;
  size_t entsize = 0;
  if (tb->class == 32) {
    size = sechdr.shdr_32->sh_size;
    strtb = tb->ents+sechdr.shdr_32->sh_link;
    entsize = sechdr.shdr_32->sh_entsize;
    size_t count = size/entsize;
    printf(" Entsz %zu, Secsz %zu, %zu symbols.\n", entsize, size, count);
    for (size_t i = 0; i<count; i++) {
      printf("%sNum%s: %4zu= %sSize%s %8u, %sNdx %s%04X%s%s%s%2s%s, %sBind %s%02X%s %6s, %sType %s%02X%s %7s\nName %016u %04u: %s%s%s\n",
          keycolor,
          clear,
          i,
          keycolor,
          clear,
          (symhdr.sym_32+i)->st_size,
          keycolor,
          hexcolor,
          (symhdr.sym_32+i)->st_shndx,
          boldcmd,
          specolor,
          spebcolor,
          (symhdr.sym_32+i)->st_shndx == SHDR_INDEX_UNDEF?"U ":
          (symhdr.sym_32+i)->st_shndx == SHDR_INDEX_ABS?" A":
          (symhdr.sym_32+i)->st_shndx == SHDR_INDEX_COMMON?" C":
          "  ",
          clear, // Before Bind
          keycolor,
          hexcolor,
          SYM_BIND_FIELD((symhdr.sym_32+i)->st_info),
          clear,
          get_symbind((symhdr.sym_32+i)->st_info),
          keycolor,
          hexcolor,
          SYM_TYPE_FIELD((symhdr.sym_32+i)->st_info),
          clear,
          get_symtype((symhdr.sym_32+i)->st_info),
          (symhdr.sym_32+i)->st_value,
          (symhdr.sym_32+i)->st_name,
          namecolor,
          ((char*)strtb->sec)+(symhdr.sym_32+i)->st_name,
          clear
          );
    }
  } else if (tb->class == 64) {
    size = sechdr.shdr_64->sh_size;
    strtb = tb->ents+sechdr.shdr_64->sh_link;
    entsize = sechdr.shdr_64->sh_entsize;
    size_t count = size/entsize;
    printf(" Entsz %zu, Secsz %zu, %zu symbols, SNdx %u\n", entsize, size, count, sechdr.shdr_64->sh_link);
    for (size_t i = 0; i<count; i++) {
      printf("%sNum%s: %4zu= %sSize%s %8zu, %sNdx %s%04X%s%s%s%2s%s, %sBind %s%02X%s %6s, %sType %s%02X%s %7s\nName %016lu %04u: %s%s%s\n",
          keycolor,
          clear,
          i,
          keycolor,
          clear,
          (symhdr.sym_64+i)->st_size,
          keycolor,
          hexcolor,
          (symhdr.sym_64+i)->st_shndx,
          boldcmd,
          specolor,
          spebcolor,
          (symhdr.sym_64+i)->st_shndx == SHDR_INDEX_UNDEF?"U ":
          (symhdr.sym_64+i)->st_shndx == SHDR_INDEX_ABS?" A":
          (symhdr.sym_64+i)->st_shndx == SHDR_INDEX_COMMON?" C":
          "  ",
          clear, // Before Bind
          keycolor,
          hexcolor,
          SYM_BIND_FIELD((symhdr.sym_64+i)->st_info),
          clear,
          get_symbind((symhdr.sym_64+i)->st_info),
          keycolor,
          hexcolor,
          SYM_TYPE_FIELD((symhdr.sym_64+i)->st_info),
          clear,
          get_symtype((symhdr.sym_64+i)->st_info),
          (symhdr.sym_64+i)->st_value,
          (symhdr.sym_64+i)->st_name,
          namecolor,
          ((char*)strtb->sec)+(symhdr.sym_64+i)->st_name,
          clear
          );
    }
  } else {
    printf("Invalid class for section table info\n");
    exit(-1);
  }
}

struct pkgmeta metaelf = {
  .name = "elf",
  .author = "Memorytoco@gmail.com",
  .desc = "A handy package for displaying"
    " ELF file info.",
  .ui = PKGSERVE,
  { 0, 1, 0 }
};

struct pkgreg regelf = {
  .ui = PKGSERVE,
  .depend = NULL,
  .exports = {
    { "hex", hex },
    { "sym", sym },
    { NULL, NULL }
  }
};
