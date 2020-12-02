#include "pkg.h"
#include "command.h"
#include "elf.h"
#include "vara.h"
#include "log.h"

#include "dtool.h"

char * typesig = "elf:@d2";

void show_ascii(unsigned char* buf, size_t sz)
{
  for (size_t i = 0; i<sz; i++) {
    if (isgraph(buf[i]))
      logInfo("%c", buf[i]);
    else
      logInfo(".");
  }
}

void show_hex(unsigned char* buf, int num, int length) {
  int anchor = 0, i = 0;
  do {
    if (i%num == 0) logInfo("0x%08X ", i);
    if (i%4 == 3) {
      logInfo("%02X ", buf[i]);
    } else {
      logInfo("%02X", buf[i]);
    }
    if (i%num == num-1) {
      logInfo("   ");
      show_ascii(buf+anchor, num);
      logInfo("\n");
      anchor = i+1;
    }
  } while (++i<length);

  if (length%num != 0) {
    size_t len = length - anchor;
    len = num*2 + (num/4) + 3 - len * 2 - len/4;
    unsigned char *spaces = malloc(len);
    memset(spaces, ' ', len);
    logInfo("%s", spaces);
    show_ascii(buf+anchor, length - anchor);
    logInfo("\n");
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
    logChannelColor(Alert, "[{197}Error{0}] Can't find .symtab section\n");
    exit(-1);
  }
  logInfo("SYM table Ndx: %4zu %s",
      symidx,
      sectbl_getname(tb, symidx)
      );
  struct section *sec = tb->ents + symidx;
  union unisechdr sechdr = {.all = sec->hdr};
  union unisymhdr symhdr = {.all = sec->sec};

  size_t size = 0;
  struct section *strtb = NULL;
  size_t entsize = 0;
  const char *info =
    "{209}Num{0}: %4zu{118}@{0} {209}Size{0} %8zu, "
    "{209}Ndx {222}%04X{265}{233}%2s{0}, "
    "{209}Bind {222}%02X{0} %6s, "
    "{209}Type {222}%02X{0} %7s\n"
    "{196}Name{0} %016u %04u: {153}%s{0}\n"
    ;
  if (tb->class == 32) {
    size = sechdr.shdr_32->sh_size;
    strtb = tb->ents+sechdr.shdr_32->sh_link;
    entsize = sechdr.shdr_32->sh_entsize;
    size_t count = size/entsize;
    logInfo(" Entsz %zu, Secsz %zu, %zu symbols.\n", entsize, size, count);
    for (size_t i = 0; i<count; i++) {
      logInfoColor(info, i,
          (symhdr.sym_32+i)->st_size,
          (symhdr.sym_32+i)->st_shndx,

          (symhdr.sym_32+i)->st_shndx == SHDR_INDEX_UNDEF?"U ":
            (symhdr.sym_32+i)->st_shndx == SHDR_INDEX_ABS?" A":
            (symhdr.sym_32+i)->st_shndx == SHDR_INDEX_COMMON?" C":
            "  ",

          SYM_BIND_FIELD((symhdr.sym_32+i)->st_info),
          get_symbind((symhdr.sym_32+i)->st_info),

          SYM_TYPE_FIELD((symhdr.sym_32+i)->st_info),
          get_symtype((symhdr.sym_32+i)->st_info),

          (symhdr.sym_32+i)->st_value,
          (symhdr.sym_32+i)->st_name,
          ((char*)strtb->sec)+(symhdr.sym_32+i)->st_name
          );
    }
  } else if (tb->class == 64) {
    size = sechdr.shdr_64->sh_size;
    strtb = tb->ents+sechdr.shdr_64->sh_link;
    entsize = sechdr.shdr_64->sh_entsize;
    size_t count = size/entsize;
    logInfo(" Entsz %zu, Secsz %zu, %zu symbols, SNdx %u\n", entsize, size, count, sechdr.shdr_64->sh_link);
    for (size_t i = 0; i<count; i++) {
      logInfoColor( info,
          i,
          (symhdr.sym_64+i)->st_size,
          (symhdr.sym_64+i)->st_shndx,
          (symhdr.sym_64+i)->st_shndx == SHDR_INDEX_UNDEF?"U ":
            (symhdr.sym_64+i)->st_shndx == SHDR_INDEX_ABS?" A":
            (symhdr.sym_64+i)->st_shndx == SHDR_INDEX_COMMON?" C":
            "  ",
          SYM_BIND_FIELD((symhdr.sym_64+i)->st_info),
          get_symbind((symhdr.sym_64+i)->st_info),
          SYM_TYPE_FIELD((symhdr.sym_64+i)->st_info),
          get_symtype((symhdr.sym_64+i)->st_info),
          (symhdr.sym_64+i)->st_value,
          (symhdr.sym_64+i)->st_name,
          ((char*)strtb->sec)+(symhdr.sym_64+i)->st_name
          );
    }
  } else {
    logChannelColor(Alert, "[{197}Error{0}] Invalid class for section table info\n");
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
