#include "cmd.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

static const char* cmd_prefix = "\033[";

char* setcmd(char cmd, int argc, ...)
{
#define CMD_LENGTH 16
  char* buf = malloc(CMD_LENGTH);
  size_t blklen = 1;
  size_t buflen = 0;

  buflen = strlen(cmd_prefix);
  memmove(buf, cmd_prefix, buflen);

  va_list ap;
  va_start(ap, argc);
  for (int i = 0; i < argc; i++) {
    int val = va_arg(ap, int);
    int len = 0;
redo:
    len = snprintf(buf+buflen, blklen * CMD_LENGTH - buflen
        ,"%d%c" ,val, i+1==argc ? cmd : ';');
    if (len + buflen >= blklen * CMD_LENGTH) {
      // snprintf success only if (len + buflent < blklen * CMD_LENGTH)
      // and it will also copy 0 endmarker to buffer but return
      // string length without 0 endmarker.
      // query c11 standard for information of snprintf
      blklen ++;
      buf = realloc(buf, blklen * CMD_LENGTH);
      goto redo;
    }
    buflen += len;
  }
  buf = realloc(buf, buflen+1); // include the 
  va_end(ap);
  return buf;
#undef CMD_LENGTH
}

char* setcolor(int type, int color, const char* const src, int offset, int len)
{
  char *cmd = setcmd('m', 3, type, 5, color);
  size_t cmdlen = strlen(cmd);
  char *buf = malloc(cmdlen + len + 1);
  memmove(buf, cmd, cmdlen);
  free(cmd);
  memmove(buf+cmdlen, src+offset, len);
  buf[cmdlen+len] = 0;
  return buf;
}

inline char* setforecolor
(int color, const char* const src, int offset, int len)
{ return setcolor(38, color, src, offset, len); }
inline char* setbackcolor
(int color, const char* const src, int offset, int len)
{ return setcolor(48, color, src, offset, len); }

char* strnjoin(int num, ...)
{
  va_list ap;
  va_start(ap, num);

  size_t total = 0;
  struct slice {
    void *buf;
    size_t len;
  } *sarry;
  sarry = malloc(sizeof(struct slice) * num);
  for (int i = 0; i < num; i++) {
    char *str = va_arg(ap, char*);
    size_t len = va_arg(ap, size_t);
    sarry[i].buf = str;
    sarry[i].len = len;
    total += len;
  }
  va_end(ap);

  char* buf = malloc(total+1);
  total = 0;
  for (int i = 0; i < num; i++) {
    memmove(buf + total, sarry[i].buf, sarry[i].len);
    total += sarry[i].len;
  }

  buf[total] = 0;
  free(sarry);
  return buf;
}
