#include <capstone/capstone.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>

#include "cmd.h"

int main(int argc, char** argv)
{
  csh handle = 0;
  cs_err err = CS_ERR_OK;
  int fd;
  ssize_t size = 0;
  size_t len = 0;
  char *buf = NULL;
  char *ptr = NULL;
  cs_insn *insn = NULL;

  char *color = setcmd('m', 3, 38, 5, 153);
  char *endcmd = setcmd('m', 1, 0);
  if (argc < 2) {
    printf("%sPlease specific a raw binary file%s", color, endcmd);
    return 0;
  }

  fd = open(argv[1], O_RDWR);

  if (fd == -1) {
    printf("Error when opening file\n");
    perror("in open ");
    exit(1);
  }

  buf = malloc(1024);
  memset(buf, 0, 1024);
  ptr = buf;

  while (true) {
    size = read(fd, ptr, 128);
    if (size == 0) {
      close(fd);
      break;
    }
    if (size == -1) {
      perror("read error; ");
      errno = 0;
    }
      
    ptr += size;
    len += size;
  }
  ptr = buf;

  err = cs_open(CS_ARCH_X86, CS_MODE_64, &handle);
  
  if (err != CS_ERR_OK)
    printf("Can't open capstone!\n");

  cs_option(handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_INTEL);
  size = cs_disasm(handle, (uint8_t*)ptr, len, 0, 0, &insn);

  size_t count = 0;
  while (count < size)
  {
    printf("0x%016lx\t%s\t\t%s\n", insn[count].address, insn[count].mnemonic, insn[count].op_str);
    count ++;
  }

  cs_free(insn, size);
  cs_close(&handle);

  return 0;
}
