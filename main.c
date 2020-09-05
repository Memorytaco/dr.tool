#include <capstone/capstone.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#include "cmd.h"
#include "dtool.h"

void derror(FILE* s, const char* const msg, cs_err code)
{
  fprintf(s, "%s: %s\n", msg, cs_strerror(code));
}

int main(int argc, char** argv)
{
  csh handle = 0;
  cs_err err = CS_ERR_OK;

  char *color = setcmd('m', 3, 38, 5, 153);
  char *endcmd = setcmd('m', 1, 0);
  if (argc < 2) {
    printf("%sPlease specific a raw binary file%s\n", color, endcmd);
    return 0;
  }
  
  err = cs_open(CS_ARCH_X86, CS_MODE_64, &handle);
  if (err) {
    derror(stderr, "CSOPENERROR", err);
    return 1;
  }

  size_t size = 0;
  char* filebuffer = readfile(argv[1], &size);
  if (size == -1) {
    free(color);
    color = setcmd('m', 3, 38, 5, 38);
    printf("%sError on file %s%s\n", color, endcmd, argv[1]);
    return 1;
  }

  err = cs_option(handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_ATT);
  if (err) {
    derror(stderr, "SETTINGSYNTAX", err);
    return 1;
  }
  printf("%s%s%s size: %zu:\n", color, argv[1], endcmd, size);
  cs_insn *insn = NULL;
  size = cs_disasm(handle, (unsigned char*)filebuffer, size, 0, 0, &insn);

  if (size > 0) {
    printf("Got %zu instructions\n", size);
  } else {
    derror(stderr, "DISASM", cs_errno(handle));
    return 1;
  }

  size_t count = 0;
  while (count < size)
  {
    cs_insn *val = insn + count;
    printf("0x%08lx  %15s  %-20s => %d\n", val->address, val->mnemonic, val->op_str, val->size);
    count ++;
  }

  cs_free(insn, size);
  cs_close(&handle);

  return 0;
}
