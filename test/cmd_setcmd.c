#include "cmd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
  char *cmd = NULL;
  char *buf = NULL;
  if (argc < 3) {
    printf("%s string color\n", argv[0]);
    return 0;
  }
  cmd = setcmd('m', 3, 38, 5, strtol(argv[2], NULL, 10));
  buf = malloc(strlen(argv[1]) + strlen(cmd) + 1);
  memmove(buf, cmd, strlen(cmd));
  memmove(buf+strlen(cmd), argv[1], strlen(argv[1])+1);
  printf("%s\n", buf);
  free (buf);
  return 0;
}
