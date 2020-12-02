#include "log.h"

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>

enum ChannelOutput {
  StdFile,
  StdBuf
};

static struct ChannelEntity {
  enum ChannelOutput ident;
  union ChannelBuf {
    FILE * file;
    struct StdBuf {
      char * buf;
      size_t size;
    } stdbuf;
  } out;
} Channel_Default
, Channel_Normal
, Channel_Warn
, Channel_Alert
, Channel_Debug
, Black_Hole;

static long matchbrace(const char *cur,char **endptr, size_t *len);
char* sprintfCSI(char *format, struct LogCSI *csi, size_t limit);
char* CSIformat(struct LogCSI csi, size_t *len);
char* saprintf(const char *format, ...);
char* vsaprintf(const char *format, va_list);
size_t ChannelWrite(const char* buf, struct ChannelEntity *chl);

static struct LogCSI* ColorTable;

static void __LogInit()
{
  static bool isinit = false;
  if (isinit) return;
  isinit = true;
  Black_Hole.ident = StdFile;
  Black_Hole.out.file = fopen("/dev/null", "w+");
  if (Black_Hole.out.file == NULL) {
    Black_Hole.out.file = tmpfile();
  }
  Channel_Debug = Black_Hole;

  Channel_Normal.ident = StdFile;
  Channel_Normal.out.file = stdout;
  Channel_Warn = Channel_Default = Channel_Normal;

  Channel_Alert.ident = StdFile;
  Channel_Alert.out.file = stderr;

  ColorTable = calloc(513, sizeof(struct LogCSI));
  ColorTable[0].para = "0";
  ColorTable[0].end = 'm';

  // prepare the output table
  for (int i = 1; i <= 512; i++) {
    ColorTable[i].end = 'm';
    ColorTable[i].para = saprintf("%d;%d;%d", i>256?48:38, 5, i<=256?i:i-256);
    /* printf("%s, len %zu\n", ColorTable[i].para, strlen(ColorTable[i].para)); */
  }
}

void setChannel(enum Channel chl, FILE* f)
{
  __LogInit();
  if (f == NULL) return;
  switch (chl) {
    default:
    case Default:
      break;
    case Normal:
      break;
    case Warn:
      break;
    case Alert:
      break;
    case Debug:
      break;
    case BlackHole:
      break;
  }
}

int logChannelColor(enum Channel chl, const char* format, ...)
{
  __LogInit();
  va_list ap;
  va_start(ap, format);
  char * toformat = vsaprintf(format, ap);
  char * final = sprintfCSI(toformat, ColorTable, 513);
  size_t ret = 0;
  switch (chl) {
    default:
    case Default:
      ret = ChannelWrite(final, &Channel_Default);
      break;
    case Normal:
      ret = ChannelWrite(final, &Channel_Normal);
      break;
    case Warn:
      ret = ChannelWrite(final, &Channel_Warn);
      break;
    case Alert:
      ret = ChannelWrite(final, &Channel_Alert);
      break;
    case Debug:
      ret = ChannelWrite(final, &Channel_Debug);
      break;
    case BlackHole:
      ret = ChannelWrite(final, &Black_Hole);
      break;
  }
  free(toformat);
  free(final);
  va_end(ap);
  return ret;
}

// length doesn't include \0
// return \0 terminated string, should be freed after using
char* CSIformat(struct LogCSI csi, size_t *len)
{
  size_t paralen = csi.para?strlen(csi.para):0;
  size_t intelen = csi.inte?strlen(csi.inte):0;
  size_t total = paralen + intelen;
  total += 1; // endmark
  total += 2; // ESC [
  char *ret = calloc(total+1, 1);
  ret[0] = '\033';
  ret[1] = '[';
  if (csi.para)
    memmove(ret+2, csi.para, paralen);
  if (csi.inte)
    memmove(ret+2+paralen, csi.inte, intelen);
  ret[total-1] = csi.end;
  if (len) *len = total;
  return ret;
}

// search {} and return its inner number, return -1 if failed.
// **endptr will act same as which in function strtol's endptr.
// *len will be stored the invalid string length or valid string
//    length.
// if cur pointer is NULL or cur doesn't point to a '{' char, thus
// endptr and len is undetermined.
long matchbrace(const char *cur, char **endptr, size_t *len)
{
  if (cur == NULL) return -1;
  if (*cur != '{') return -1;
  char *end = NULL;
  long ret = strtol(cur+1, &end, 0);
  if (*end != '}') {
    if (endptr) *endptr = end;
    if (len) *len = end - cur;
    return -1;
  }
  if (endptr) *endptr = end + 1;
  if (len) *len = end - cur + 1;
  return ret;
}

char *sprintfCSI(char *format, struct LogCSI *csi, size_t limit)
{
  static struct LogCSI reset = { "0", NULL, 'm' };
  if (format == NULL || strlen(format) == 0) return NULL;
  // FIXME
  if (csi == NULL || limit == 0) { csi = &reset; limit = 1; }
  size_t formatlen = strlen(format);
  size_t size = formatlen;
  char * buf = calloc(size, sizeof(char));
  size_t bufsize = 0;
  char *cur = format;
  size_t len = 0;
  while (cur + len < format + formatlen) {
    char * sub = NULL;
    if (cur[len] == '{') {
      if (len > 0) {
        if (len + bufsize >= size) {
          size += size;
          buf = reallocarray(buf, size, sizeof(char));
        }
        memmove(buf+bufsize, cur, len);
        bufsize += len;
        cur += len;
        len = 0;
      }
      size_t ix = matchbrace(cur, &sub, &len);
      if (ix != -1) {
        cur = sub;
        sub = CSIformat(csi[ix>=limit?0:ix], &len);
        if (len + bufsize >= size) {
          size += size;
          buf = reallocarray(buf, size, sizeof(char));
        }
        memmove(buf+bufsize, sub, len);
        bufsize += len;
        free(sub);
        len = 0;
      }
    } else len ++;
  }
  if (len > 0) {
    if (len + bufsize >= size) {
        size += size;
        buf = reallocarray(buf, size, sizeof(char));
    }
    memmove(buf+bufsize, cur, len);
    bufsize += len;
  }
  buf = reallocarray(buf, bufsize+1, sizeof(char));
  return buf;
}

char* saprintf(const char *format, ...)
{
  if (format == NULL) return NULL;
  va_list ap, aq;
  va_start(ap, format);
  va_copy(aq, ap);

  size_t size = 32;
  char * ret = calloc(size, sizeof(char));
  int remain = vsnprintf(ret, size, format, ap);
  if (remain >= size) {
    size = (size_t) remain + 1;
    ret = reallocarray(ret, size, sizeof(char));
    remain = vsnprintf(ret, size, format, aq);
    assert(remain == size - 1);
  } else if (remain >= 0 && remain < size) {
    ret = reallocarray(ret, remain + 1, sizeof(char));
  } else {
    free(ret);
    ret = NULL;
  }

  va_end(aq);
  va_end(ap);
  return ret;
}

char* vsaprintf(const char *format, va_list ap)
{
  if (format == NULL) return NULL;
  va_list aq;
  va_copy(aq, ap);

  size_t size = 32;
  char * ret = calloc(size, sizeof(char));
  int remain = vsnprintf(ret, size, format, ap);
  if (remain >= size) {
    size = (size_t) remain + 1;
    ret = reallocarray(ret, size, sizeof(char));
    remain = vsnprintf(ret, size, format, aq);
    assert(remain == size - 1);
  } else if (remain >= 0 && remain < size){
    ret = reallocarray(ret, remain + 1, sizeof(char));
  } else {
    free(ret);
    ret = NULL;
  }

  va_end(aq);
  return ret;
}

size_t ChannelWrite(const char* buf, struct ChannelEntity *chl)
{
  switch (chl->ident) {
    case StdFile:
      return fwrite(buf, sizeof(char), strlen(buf), chl->out.file);
    case StdBuf:
      // TODO
      break;
    default:
      break;
  }
  return 0;
}
