#include "dtool.h"
#include "pkg.h"
#include "cmd.h"
#include "elf.h"
#include "vara.h"
#include "disas.h"
#include "util/file.h"

struct store {
  char* name;
  bool ascii;
  bool info;
  vtra disasopt;
};

void cmd_call_help(struct cmdargt* arg, void* data, void* store) {
  char *color = setcmd('m', 3, 38, 5, 153);
  char *endcmd = setcmd('m', 1, 0);
  printf("Dtool (By github.com/Memorytaco) " DTOOLVERSION "\n");
  printf("dtool <command> [option [arguments]]...\n");
  printf("  command: elf, disas\n");
  printf("  %self%s   [-h,--help,-ascii,-sec,-sym] file\n", color, endcmd);
  printf("  %sdisas%s [-h,--help,-att,--name,--arch,--mode,--addr] file\n", color, endcmd);
  free(color);
  free(endcmd);
}

void cmd_call_name(struct cmdargt* arg, void* data, void* store) {
  struct store* ptr = store;
  ptr->name = cmdargt_access(arg, 0, NULL);
}

void cmd_call_syntax(struct cmdargt* arg, void* data, void* store) {
  struct store* ptr = store;
  vptr(unsigned char, ptr->disasopt, 1, 2) = 1;
}

void cmd_call_arch(struct cmdargt* arg, void* data, void* store)
{
  vtra option = ((struct store*)store)->disasopt;
  char *arch = cmdargt_access(arg, 0, NULL);
  if (!strcmp(arch, "ARM")) {
    vptr(enum cs_arch, option, 1, 0) = CS_ARCH_ARM;
  } else if (!strcmp(arch, "ARM64")) {
    vptr(enum cs_arch, option, 1, 0) = CS_ARCH_ARM64;
  } else if (!strcmp(arch, "X86")) {
    vptr(enum cs_arch, option, 1, 0) = CS_ARCH_X86;
  } else if (!strcmp(arch, "help")) {
    printf("Available arch: \n");
    printf("ARM, ARM64, default X86.\n");
    exit(0);
  } else {
    printf("INFO: Invalid arch, default to X86\n");
    vptr(enum cs_arch, option, 1, 0) = CS_ARCH_X86;
  }
}

void cmd_call_mode(struct cmdargt* arg, void* data, void* store)
{
  vtra option = ((struct store*)store)->disasopt;
  char *mode = cmdargt_access(arg, 0, NULL);
  if (!strcmp(mode, "64")) {
    vptr(enum cs_mode, option, 1, 1) = CS_MODE_64;
  } else if (!strcmp(mode, "32")) {
    vptr(enum cs_mode, option, 1, 1) = CS_MODE_32;
  } else if (!strcmp(mode, "16")) {
    vptr(enum cs_mode, option, 1, 1) = CS_MODE_16;
  } else if (!strcmp(mode, "ARM")) {
    vptr(enum cs_mode, option, 1, 1) = CS_MODE_ARM;
  } else if (!strcmp(mode, "ARMM")) {
    vptr(enum cs_mode, option, 1, 1) = CS_MODE_MCLASS;
  } else if (!strcmp(mode, "ARMT")) {
    vptr(enum cs_mode, option, 1, 1) = CS_MODE_THUMB;
  } else if (!strcmp(mode, "ARMV8")) {
    vptr(enum cs_mode, option, 1, 1) = CS_MODE_V8;
  } else if (!strcmp(mode, "help")) {
    printf("Available modes: \n");
    printf("default 64, 32, 16, ARM, ARMM, ARMV8, ARMT.\n");
    exit(0);
  } else {
    printf("INFO: Invalid mode, default to 64\n");
    vptr(enum cs_mode, option, 1, 1) = CS_MODE_64;
  }
}

void cmd_call_addr(struct cmdargt* arg, void* data, void* store)
{
  vtra option = ((struct store*)store)->disasopt;
  char *addr = cmdargt_access(arg, 0, NULL);
  vptr(uint64_t, option, 1, 3) = strtol(addr, NULL, 16);
}

void cmd_call_setascii(struct cmdargt* arg, void* data, void* store) {
  struct store* ptr = store;
  ptr->ascii = true;
}

void cmd_call_setinfo(struct cmdargt* arg, void* data, void* store) {
  struct store* ptr = store;
  ptr->info = true;
}

void cmd_call_elfsec
( struct cmdargt* arg
, void* data
, void* store
) {
  struct store* ptr = store;
  char *file = NULL;
  if (argvctx(data) <= 0) return;
  else file = argvidx(data, 0);
  char* buffer = readfile(file, NULL);
  if (buffer == NULL) {
    perror("elfsec @ readfile");
    exit(-1);
  }
  struct sectbl table = build_sectbl(buffer);
  free(buffer);
  int index = atoi(cmdargt_access(arg, 0, NULL));
  if (table.num < index) {
    printf( "number of entries %d,"
        " index from %d ~ %d.\n"
        , table.num, 0, table.num-1);
    return;
  }
  union unisechdr hdr = {table.ents[index].hdr};
  size_t size = 0;
  if (table.class == 32) size = hdr.shdr_32->sh_size;
  else size = hdr.shdr_64->sh_size;
  if (size == 0) {
    printf("The number %d Section has size 0.\n", index);
    return;
  }
  if (ptr->ascii) {
    printf("%s: %zd | %08zX\n", sectbl_getname(&table, index), size, size);
    vtra para = vara_alloc("elf:@D2");
    vptr(void*, para, 1, 0) = table.ents[index].sec;
    vptr(int, para, 1, 1) = 16;
    vptr(int, para, 1, 2) = size;
    pkgsig sig = pkgload("elf", PKGGLOBAL);
    if (sig == NULL) {
      printf("Error when loading package\n");
      exit(-1);
    }
    pkgsetvar(sig, para);
    pkginvoke(sig, "hex");
  } else {
    fwrite(table.ents[index].sec, 1, size, stdout);
  }
}

// TODO: Add symbol query function
void cmd_call_elfsym
( struct cmdargt* arg
, void* data
, void* global
) {
  struct store *store = global;
  size_t size;
  char* buffer = NULL;
  if (argvctx(data) == 0) {
    printf("Lack File name.\n");
    exit(1);
  }
  buffer = readfile(argvidx(data, 0), &size);
  if (buffer == NULL) {
    printf("File %s doesn't exist!\n", argvidx(data, 0));
    exit(-1);
  }
  struct sectbl tbl = build_sectbl(buffer);
  pkgsig sig = pkgload("elf", PKGGLOBAL);
  pkgsetvar(sig, &tbl);
  pkginvoke(sig, "sym");
}

int main(int argc, char** argv)
{
  if (argc < 2) {
    cmd_call_help(NULL, NULL, NULL);
    return 0;
  }

  struct cmdregt regs[] = {
    { .key = "h", .code = 1, .opt = cmd_opt_short | cmd_opt_toggle
    , .call = cmd_call_help
    },
    { .key = "help", .code = 2
    , .opt = cmd_opt_long | cmd_opt_toggle, .call = cmd_call_help
    },
    { .key = "name", .code = 4, .opt = cmd_opt_long
    , .call = cmd_call_name
    },
    { .key = "att", .code = 5
    , .opt = cmd_opt_short | cmd_opt_toggle
    , .call = cmd_call_syntax
    },
    { .key = "ascii", .code = 6
    , .opt = cmd_opt_short | cmd_opt_toggle
    , .call = cmd_call_setascii
    },
    { .key = "sec", .code = 7
    , .opt = cmd_opt_short
    , .call = cmd_call_elfsec
    , .dep = {
        .opt = {6, 8, 9, 0}
      }
    },
    { .key = "i", .code = 8
    , .opt = cmd_opt_short | cmd_opt_toggle
    , .call = cmd_call_setinfo
    },
    { .key = "info", .code = 9
    , .opt = cmd_opt_long | cmd_opt_toggle
    , .call = cmd_call_setinfo
    },
    { .key = "sym", .code = 10
    , .opt = cmd_opt_short | cmd_opt_toggle
    , .call = cmd_call_elfsym
    },
    { .key = "arch", .code = 11
    , .opt = cmd_opt_long
    , .call = cmd_call_arch
    },
    { .key = "mode", .code = 12
    , .opt = cmd_opt_long
    , .call = cmd_call_mode
    },
    { .key = "addr", .code = 13
    , .opt = cmd_opt_long
    , .call = cmd_call_addr
    },
    { .key = NULL, .code = 0, .call = NULL }
  };
  struct store* store = malloc(sizeof(struct store));
  memset(store, 0, sizeof(struct store));
  store->disasopt = vara_alloc(dt_option);
  vptr(enum cs_arch, store->disasopt, 1, 0) = CS_ARCH_X86;
  vptr(enum cs_mode, store->disasopt, 1, 1) = CS_MODE_64;

  arglst extra = cmd_match(argc-2, argv+2, regs, store);

  if (strcmp("disas", argv[1])) {
    if (strcmp("elf", argv[1])) {
      printf("Avaliable commands: disas, elf.\n");
    }
    return 0;
  }

  char *file = NULL;
  if (argvctx(extra) < 1) {
    printf("Please provide one file.\n");
    return -1;
  } else file = argvidx(extra, 0);
  char* name = store->name;
  if (!name) {
    name = file;
  }
  size_t size=0;
  unsigned char* buf = (unsigned char*)readfile(file, &size);
  dt_disas(name, buf, size, store->disasopt);

  return 0;
}
