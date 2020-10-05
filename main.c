#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <ctype.h>  // for isalpha some functions

#include "cmd.h"
#include "dtool.h"
#include "elf.h"
#include "disas.h"
#include "util/file.h"

struct store {
  char* name;
  bool syntax;
  bool ascii;
};

void cmd_call_help(struct cmdargt* arg, void* data, void* store) {
  char *color = setcmd('m', 3, 38, 5, 153);
  char *endcmd = setcmd('m', 1, 0);
  printf("Author: Memorytaco, github: github.com/Memorytaco.\n");
  printf("Version " DTOOLVERSION ".\n");
  printf("dtool <command> [option [arguments]]...\n");
  printf("  command: elf, disas\n");
  printf("  %self%s   [-h,--help,-ascii,-sec] file\n", color, endcmd);
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

void show_ascii(unsigned char* buf, size_t offset, size_t sz)
{
  for (size_t i = 0; i<sz; i++) {
    if (isgraph(buf[i+offset]))
      printf("%c", buf[i+offset]);
    else
      printf(".");
  }
}

void show_hex(unsigned char* buf, int num, int length) {
  int anchor = 0;
  printf("0x%08X ", 0);
  for (int i = 0; i < length; i++) {
    if (i%num == 0 && i != 0) {
      printf("   ");
      show_ascii(buf, anchor, num);
      printf("\n");
      printf("0x%08X ", i);
      anchor = i;
    }
    if (i%4 == 3)
      printf("%02X ", buf[i]);
    else
      printf("%02X", buf[i]);
  }
  if (length%num != 0) {
    size_t len = length - anchor;
    len = num*2 + (num/4) + 3 - len * 2 - len/4;
    unsigned char *buf = malloc(len);
    memset(buf, ' ', len);
    printf("%s", buf);
    show_ascii(buf, anchor, length - anchor);
    printf("\n");
    free(buf);
  }
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
    show_hex(table.ents[index].sec, 16, size);
  } else {
    fwrite(table.ents[index].sec, 1, size, stdout);
  }
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
        .opt = {6, 0}
      }
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
  char* buf = readfile(file, &size);
  dt_disas(name, buf, size, store->syntax);

  return 0;
}
