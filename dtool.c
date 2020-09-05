#include "dtool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

char* readfile(const char * const path, size_t *size)
{
  FILE *stream = fopen(path, "r");
  if (!stream) {
    perror("dtool.c loadfile");
    if (size) *size = -1;
    return NULL;
  }
  fseek(stream, 0, SEEK_END);
  long filesize = ftell(stream);
  char *buf = malloc(filesize);
  fseek(stream, 0, SEEK_SET);
  size_t len = fread(buf, 1, filesize, stream);
  if (len == filesize)
    *size = filesize;
  fclose(stream);
  return buf;
}
