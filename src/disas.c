#include "disas.h"
#include "log.h"

void dt_error(FILE* s, const char* const msg, cs_err code)
{
  fprintf(s, "%s: %s\n", msg, cs_strerror(code));
}

// @syntax: true for ATT, false for Intel. Default is Intel.
void dt_disas(char* name, unsigned char* filebuffer, size_t size, void *option)
{
  // init capstone virtual machine
  csh handle = 0;
  cs_err err = CS_ERR_OK;

  if (strcmp(vara_name(option), "Disas")) {
    logChannelColor(Alert, "[{197}Error{0}] Invalid Option Parameter\n");
    return;
  }

  err = cs_open( vptr(enum cs_arch, option, 1, 0)
               , vptr(enum cs_mode, option, 1, 1)
               , &handle);

  if (err) {
    dt_error(stderr, "CSOPENERROR", err);
    return;
  }

  err = cs_option(handle, CS_OPT_SYNTAX
                 , vptr(bool, option, 1, 2) ? CS_OPT_SYNTAX_ATT
                                            : CS_OPT_SYNTAX_INTEL);
  if (err) {
    dt_error(stderr, "SETTINGSYNTAX", err);
    return;
  }
  logInfoColor("{154}%s{0} size: %zu:\n", name, size);
  cs_insn *insn = NULL;
  size = cs_disasm(handle, filebuffer, size
                  , vptr(uint64_t, option, 1, 3), 0, &insn);

  if (size > 0) {
    logInfo("%zu insts:\n", size);
  } else {
    dt_error(stderr, "DISASM", cs_errno(handle));
    return;
  }

  size_t count = 0;
  while (count < size)
  {
    cs_insn *val = insn + count;
    logInfo("0x%08lX  %15s  %-20s\n", val->address, val->mnemonic, val->op_str);
    count ++;
  }

  cs_free(insn, size);
  cs_close(&handle);
}
