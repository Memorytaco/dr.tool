#include "dtool.h"
#include "mod.h"
#include "cmd.h"
#include "elf.h"
#include "disas.h"
#include "util/file.h"

// include module header here, for communication between modules
#include "Module/modelf.h"

struct store {
  char* name;
  bool syntax;
  bool ascii;
  bool info;
};

void cmd_call_help(struct cmdargt* arg, void* data, void* store) {
  char *color = setcmd('m', 3, 38, 5, 153);
  char *endcmd = setcmd('m', 1, 0);
  printf("Author: Memorytaco, github: github.com/Memorytaco.\n");
  printf("Version " DTOOLVERSION ";\n");
  printf("dtool <command> [option [arguments]]...\n");
  printf("  command: elf, disas\n");
  printf("  %self%s   [-h,--help,-ascii,-sec,-sym] file\n", color, endcmd);
  printf("  %sdisas%s [-h,--help,-att,--name] file\n", color, endcmd);
  free(color);
  free(endcmd);
}

void cmd_call_name(struct cmdargt* arg, void* data, void* store) {
  struct store* ptr = store;
  size_t len = 0;
  ptr->name = cmdargt_fetch_val(arg, &len);
}

void cmd_call_syntax(struct cmdargt* arg, void* data, void* store) {
  struct store* ptr = store;
  ptr->syntax = true;
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
  size_t len = 0;
  int index = atoi(cmdargt_fetch_val(arg, &len));
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
    struct modelft elfpara = { table.ents[index].sec, .num = 16, size };
    printf("Opening the module\n");
    modcore handle = modprobe("elf", RTLD_NOW);
    if (handle == NULL) {
      printf("Error when loading module\n");
    } else {
      modshow(handle);
    }
    modsetpara(handle, &elfpara);
    modcall(handle, "hex");
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
  modcore handle = modprobe("elf", RTLD_NOW);
  modsetpara(handle, &tbl);
  modcall(handle, "sym");
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
    { .key = NULL, .code = 0, .call = NULL }
  };
  struct store* store = malloc(sizeof(struct store));
  memset(store, 0, sizeof(struct store));
  arglst extra = cmd_match(argc-2, argv+2, regs, store);

  if (strcmp("disas", argv[1])) {
    if (strcmp("elf", argv[1])) {
      printf("Avaliable commands: disas, elf.\n");
    }
    return 0;
  }

  char *file = NULL;
  if (argvctx(extra) <= 0) {
    printf("Please specific one file.\n");
  } else file = argvidx(extra, 0);
  char* name = store->name;
  if (!name) {
    name = file;
  }
  size_t size=0;
  unsigned char* buf = (unsigned char*)readfile(file, &size);
  dt_disas(name, buf, size, store->syntax);

  return 0;
}
