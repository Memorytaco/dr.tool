#include "dtool.h"
#include "pkg.h"
#include "command.h"
#include "elf.h"
#include "vara.h"
#include "disas.h"
#include "util/file.h"
#include "log.h"

struct store {
  char* name;
  bool ascii;
  bool info;
  vtra disasopt;
};

void cmd_call_name(Arg* arg, void* data, void* store) {
  struct store* ptr = store;
  ptr->name = Argvix(arg, 0, NULL);
}

void cmd_call_syntax(Arg* arg, void* data, void* store) {
  struct store* ptr = store;
  vptr(unsigned char, ptr->disasopt, 1, 2) = 1;
}

void cmd_call_arch(Arg* arg, void* data, void* store)
{
  vtra option = ((struct store*)store)->disasopt;
  char *arch = Argvix(arg, 0, NULL);
  if (!strcmp(arch, "ARM")) {
    vptr(enum cs_arch, option, 1, 0) = CS_ARCH_ARM;
  } else if (!strcmp(arch, "ARM64")) {
    vptr(enum cs_arch, option, 1, 0) = CS_ARCH_ARM64;
  } else if (!strcmp(arch, "X86")) {
    vptr(enum cs_arch, option, 1, 0) = CS_ARCH_X86;
  } else if (!strcmp(arch, "help")) {
    logInfo("Available arch: \n");
    logInfo("ARM, ARM64, default X86.\n");
    exit(0);
  } else {
    logInfoColor("[{82}INFO{0}] Invalid arch, default to X86\n");
    vptr(enum cs_arch, option, 1, 0) = CS_ARCH_X86;
  }
}

void cmd_call_mode(Arg* arg, void* data, void* store)
{
  vtra option = ((struct store*)store)->disasopt;
  char *mode = Argvix(arg, 0, NULL);
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
    logInfo("Available modes: \n");
    logInfo("default 64, 32, 16, ARM, ARMM, ARMV8, ARMT.\n");
    exit(0);
  } else {
    logInfoColor("[{82}INFO{0}] Invalid mode, default to 64\n");
    vptr(enum cs_mode, option, 1, 1) = CS_MODE_64;
  }
}

void cmd_call_addr(Arg* arg, void* data, void* store)
{
  vtra option = ((struct store*)store)->disasopt;
  char *addr = Argvix(arg, 0, NULL);
  vptr(uint64_t, option, 1, 3) = strtol(addr, NULL, 16);
}

void cmd_call_setascii(Arg* arg, void* data, void* store) {
  struct store* ptr = store;
  ptr->ascii = true;
}

void cmd_call_setinfo(Arg* arg, void* data, void* store) {
  struct store* ptr = store;
  ptr->info = true;
}

void cmd_call_elfsec
( Arg* arg
, void* data
, void* store
) {
  struct store* ptr = store;
  char *file = NULL;
  if (argvctx(data) <= 0) {
    logChannelColor(Warn, "[{221}Warn{0}] Lack file name\n");
    return;
  }
  else file = argvidx(data, 0);
  char* buffer = readfile(file, NULL);
  if (buffer == NULL) {
    perror("elfsec @ readfile");
    exit(-1);
  }
  struct sectbl table = build_sectbl(buffer);
  free(buffer);
  int index = atoi(Argvix(arg, 0, NULL));
  if (table.num < index) {
    logInfo( "number of entries %d,"
        " index from %d ~ %d.\n"
        , table.num, 0, table.num-1);
    return;
  }
  union unisechdr hdr = {table.ents[index].hdr};
  size_t size = 0;
  if (table.class == 32) size = hdr.shdr_32->sh_size;
  else size = hdr.shdr_64->sh_size;
  if (size == 0) {
    logInfo("The number %d Section has size 0.\n", index);
    return;
  }
  if (ptr->ascii) {
    logInfo("%s: %zd | %08zX\n", sectbl_getname(&table, index), size, size);
    vtra para = vara_alloc("elf:@D2");
    vptr(void*, para, 1, 0) = table.ents[index].sec;
    vptr(int, para, 1, 1) = 16;
    vptr(int, para, 1, 2) = size;
    pkgsig sig = pkgload("elf", PKGGLOBAL);
    if (sig == NULL) {
      logChannelColor(Alert, "[{197}Error{0}] failed loading package\n");
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
( Arg* arg
, void* data
, void* global
) {
  struct store *store = global;
  size_t size;
  char* buffer = NULL;
  if (argvctx(data) == 0) {
    logChannelColor(Warn, "[{221}Warn{0}] Lack File name.\n");
    exit(1);
  }
  buffer = readfile(argvidx(data, 0), &size);
  if (buffer == NULL) {
    logChannelColor(Alert, "[{197}Error{0}] File %s doesn't exist!\n", argvidx(data, 0));
    exit(-1);
  }
  struct sectbl tbl = build_sectbl(buffer);
  pkgsig sig = pkgload("elf", PKGGLOBAL);
  pkgsetvar(sig, &tbl);
  pkginvoke(sig, "sym");
}

struct cmdspect specdisas[] = {
  { .optname = "Syntax"
  , .alias = ALIASARRAY {
      { .alia = "-att:?"
      , .desc = "Toggle AT&T Assembly Syntax"
      , .valset = NULL
      }, SPECALIAEND
    }
  , .rely = NULL
  , .call = cmd_call_syntax
  },
  { .optname = "Mode"
  , .alias = ALIASARRAY {
      { .alia = "--mode: -m:"
      , .desc = "Specific disassembly mode, default 64"
      , .valset = "[64 32 16 ARM ARMM ARMT ARMV8 help]"
      },
      { .alia = "-thumb:?"
      , .desc = "Specific mode to arm Thumb mode"
      , .valset = NULL
      }, SPECALIAEND
    }
  , .rely = NULL
  , .call = cmd_call_mode
  },
  { .optname = "Arch"
  , .alias = ALIASARRAY {
      { .alia = "--arch: -ar:"
      , .desc = "Specific disassembly Architecture, default X86"
      , .valset = "[ARM ARM64 X86 help]"
      }, SPECALIAEND
    }
  , .rely = NULL
  , .call = cmd_call_arch
  },
  { .optname = "Addr"
  , .alias = ALIASARRAY {
      { .alia = "-addr:"
      , .desc = "set initial address of disassembly code"
      , .valset = NULL
      }, SPECALIAEND
    }
  , .rely = NULL
  , .call = cmd_call_addr
  },
  CMDSPECEND
};

struct cmdspect specelf[] = {
  { .optname = "Name"
  , .alias = ALIASARRAY {
      { .alia = "-name:"
      , .desc = "designate the name field"
      , .valset = NULL
      }, SPECALIAEND
    }
  , .rely = NULL
  , .call = cmd_call_name
  },
  { .optname = "Section"
  , .alias = ALIASARRAY {
      { .alia = "-sec: --section:"
      , .desc = "choose which section to dump"
      , .valset = NULL
      }, SPECALIAEND
    }
  , .rely = RELYSARRAY {
      { "Ascii", RELYOPT }
    , SPECRELYEND
    }
  , .call = cmd_call_elfsec
  },
  { .optname = "Ascii"
  , .alias = ALIASARRAY {
      { .alia = "--ascii:?"
      , .desc = "Toggle ascii output mode"
      , .valset = NULL
      }, SPECALIAEND
    }
  , .rely = NULL
  , .call = cmd_call_setascii
  },
  { .optname = "Symbol"
  , .alias = ALIASARRAY {
      { .alia = "-sl:? --symbol-list:?"
      , .desc = "list the elf file's symbol table"
      , .valset = NULL
      }, SPECALIAEND
    }
  , .rely = NULL
  , .call = cmd_call_elfsym
  },
  CMDSPECEND
};

void disascmd(int argc, char **argv, void* store);

struct command builtin_cmds[] = {
  { .command = "disas d"
  , .package = "disas"
  , .info = "A simple disassembly tool."
  , .spec = specdisas
  , .action = disascmd
  },
  { .command = "elf e"
  , .package = "elf"
  , .info = "An ELF INFO extraction tool."
  , .spec = specelf
  }, COMMANDEND
};

void disascmd(int argc, char **argv, void* store) {
  if (argc == 0) {
    logChannelColor(Warn, "[{221}Warn{0}] Please provide one file.\n");
    return ;
  }
  char *file = argv[0];
  char *name = ((struct store*)store)->name;
  name = name?name:file;
  size_t size = 0;

  unsigned char* buf = (unsigned char*)readfile(file, &size);
  dt_disas(name, buf, size, ((struct store*)store)->disasopt);
}

int main(int argc, char** argv)
{
  struct store* store = malloc(sizeof(struct store));
  memset(store, 0, sizeof(struct store));
  store->disasopt = vara_alloc(dt_option);
  vptr(enum cs_arch, store->disasopt, 1, 0) = CS_ARCH_X86;
  vptr(enum cs_mode, store->disasopt, 1, 1) = CS_MODE_64;

  int status =
    cmd_match(argc-1, (const char**)(argv+1), builtin_cmds, store);

  if (status == -1) {
    logInfoColor("{230}Dr.tool{0} {255}(github.com/Memorytaco){0} "
                  DTOOLVERSION "\n");
    logInfoColor("Usage: {215}%s{0} "
                 "{command} [option].. [argument]..\n"
                , argv[0]);
    logInfo("Commands: ");
    for (Command *c = builtin_cmds; c->command != NULL; c++) {
      logInfo("%s%s", c->command
                    , (c+1)->command == NULL?" .\n":", ");
    }
    for (Command *c = builtin_cmds; c->command != NULL; c++) {
      logInfoColor("  {156}%s{0}:\n", c->command);
      logInfo("     %s\n", c->info);
    }
    logInfo("\n");
    for (Command *c = builtin_cmds; c->command != NULL; c++) {
      cmdshortinfo(c);
      if ((c+1)->command != NULL)
        logInfo("\n");
    }
  }
  return 0;
}

