#include "disas.h"

void derror(FILE* s, const char* const msg, cs_err code)
{
  fprintf(s, "%s: %s\n", msg, cs_strerror(code));
}

// @syntax: true for ATT, false for Intel. Default is Intel.
void dt_disas(char* name, char* filebuffer, size_t size, bool syntax)
{
  // prepare commandline setting
  char *color = setcmd('m', 3, 38, 5, 153);
  char *endcmd = setcmd('m', 1, 0);

  // init capstone virtual machine
  csh handle = 0;
  cs_err err = CS_ERR_OK;

  err = cs_open(CS_ARCH_X86, CS_MODE_64, &handle);
  if (err) {
    derror(stderr, "CSOPENERROR", err);
    return;
  }

  err = cs_option(handle, CS_OPT_SYNTAX, syntax ? CS_OPT_SYNTAX_ATT : CS_OPT_SYNTAX_INTEL);
  if (err) {
    derror(stderr, "SETTINGSYNTAX", err);
    return;
  }
  printf("%s%s%s size: %zu:\n", color, name, endcmd, size);
  cs_insn *insn = NULL;
  size = cs_disasm(handle, (unsigned char*)filebuffer, size, 0, 0, &insn);

  if (size > 0) {
    printf("Got %zu instructions\n", size);
  } else {
    derror(stderr, "DISASM", cs_errno(handle));
    return;
  }

  size_t count = 0;
  while (count < size)
  {
    cs_insn *val = insn + count;
    printf("0x%08lx  %15s  %-20s => %d\n", val->address, val->mnemonic, val->op_str, val->size);
    count ++;
  }

  free(color);
  free(endcmd);
  cs_free(insn, size);
  cs_close(&handle);
}
