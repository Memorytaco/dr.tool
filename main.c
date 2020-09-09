
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#include "cmd.h"
#include "dtool.h"
#include "elf.h"
#include "disas.h"
#include "util/file.h"

struct store {
  char* file;
  char* name;
  bool syntax;
};

int cmd_call_help(struct cmdarg* arg, void* data, void* store) {
  printf("Hello World, this is %s help option.\n", arg->opt & cmd_opt_long ? "long":"short");
  return 0;
}

int cmd_call_file(struct cmdarg* arg, void* data, void* store) {
  struct store* ptr = store;
  ptr->file = arg->val;
  printf("Specific file is %s\n", arg->val);
  return 0;
}

int cmd_call_name(struct cmdarg* arg, void* data, void* store) {
  struct store* ptr = store;
  ptr->name = arg->val;
  printf("Got a parser name %s\n", ptr->name);
  return 0;
}

int cmd_call_syntax(struct cmdarg* arg, void* data, void* store) {
  struct store* ptr = store;
  ptr->syntax = true;
  return 0;
}

int main(int argc, char** argv)
{
  char *color = setcmd('m', 3, 38, 5, 153);
  char *endcmd = setcmd('m', 1, 0);
  if (argc < 2) {
    printf("%s%s {command} [option]%s\n", argv[0], color, endcmd);
    return 0;
  }

  if (strcmp("disas", argv[1])) {
    printf("disas is the only command.\n");
    return 0;
  }

  struct cmd_reg regs[] = {
    { .code = 0, .arg = { .opt = cmd_opt_short | cmd_opt_toggle, .key = "h" }, .call = cmd_call_help },
    { .code = 0, .arg = { .opt = cmd_opt_long | cmd_opt_toggle, .key = "help" }, .call = cmd_call_help },
    { .code = 2, .arg = { .opt = cmd_opt_long, .key = "file" }, .call = cmd_call_file },
    { .code = 3, .arg = { .opt = cmd_opt_long, .key = "name" }, .call = cmd_call_name },
    { .code = 4, .arg = { .opt = cmd_opt_short | cmd_opt_toggle, .key = "att" }, .call = cmd_call_syntax},
    { .code = -1, .call = NULL}
  };
  struct store* store = malloc(sizeof(struct store));
  memset(store, 0, sizeof(struct store));
  cmd_match(argc-2, argv+2, regs, store);

  if (store->file) {
    char* name = store->name;
    if (!name) {
      name = store->file;
    }
    size_t size=0;
    char* buf = readfile(store->file, &size);
    dt_disas(name, buf, size, store->syntax);
  } else printf("Please specific file option\n");

  return 0;
}
