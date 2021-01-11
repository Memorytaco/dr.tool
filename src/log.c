#include "log.h"
#include "dynamix.h"

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
    String buffer;
  } out;
} Channel_Default
, Channel_Normal
, Channel_Warn
, Channel_Alert
, Channel_Debug
, Black_Hole;

static void __LogInit();

static long matchbrace(const char *cur,char **endptr, size_t *len);
char* sprintfCSI(const char *format, const struct CSI *csi, size_t limit);
char* CSIformat(struct CSI csi, size_t *len);
char* saprintf(const char *format, ...);
char* vsaprintf(const char *format, va_list);
size_t ChannelWrite(const char* buf, struct ChannelEntity *chl);
int vlogChannel(enum Channel chl, const char* format, va_list ap);
int vlogChannelColor(enum Channel chl, const char* format, va_list ap);

size_t CSIlength(const struct CSI *table);

struct CSI* ColorTable;

void logInit()
{
  __LogInit();
}

void __LogInit()
{
  static bool isinit = false;
  if (isinit) return;
  isinit = true;
  Black_Hole.ident = StdFile;
  Black_Hole.out.file = fopen("/dev/null", "w+");
  if (Black_Hole.out.file == NULL) {
    Black_Hole.out.file = tmpfile();
  }

  Channel_Normal.ident = StdFile;
  Channel_Normal.out.file = stdout;
  Channel_Warn = Channel_Default = Channel_Normal;

  const char *val = getenv(DEBUGLOG);
  if (val) {
    if (!strcmp(val, "true") || !strcmp(val, "1")) {
      Channel_Debug = Channel_Normal;
    } else {
      Channel_Debug = Black_Hole;
    }
  } else {
    Channel_Debug = Black_Hole;
  }

  Channel_Alert.ident = StdFile;
  Channel_Alert.out.file = stderr;

  ColorTable = calloc(514, sizeof(struct CSI));
  ColorTable[0].para = "0";
  ColorTable[0].end = 'm';

  // prepare the output table
  for (int i = 1; i <= 512; i++) {
    ColorTable[i].end = 'm';
    ColorTable[i].para =
      saprintf("%d;%d;%d"
            , i<=256?38:48
            , 5
            , i<=256?i-1:i-257);
  }
  ColorTable[513] = (struct CSI) CSIEND;
}

// TODO
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

int vlogChannelColor(enum Channel chl, const char* format, va_list ap)
{
  if (format == NULL) return 0;
  char *str = vsaprintf(format, ap);
  char *out =
    sprintfCSI(str, ColorTable, CSIlength(ColorTable));
  size_t ret = 0;
  switch (chl) {
    default:
    case Default:
      ret = ChannelWrite(out, &Channel_Default);
      break;
    case Normal:
      ret = ChannelWrite(out, &Channel_Normal);
      break;
    case Warn:
      ret = ChannelWrite(out, &Channel_Warn);
      break;
    case Alert:
      ret = ChannelWrite(out, &Channel_Alert);
      break;
    case Debug:
      ret = ChannelWrite(out, &Channel_Debug);
      break;
    case BlackHole:
      ret = ChannelWrite(out, &Black_Hole);
      break;
  }
  free(str);
  free(out);
  return ret;
}

int logChannelColor(enum Channel chl, const char* format, ...)
{
  __LogInit();
  va_list ap;
  va_start(ap, format);
  int ret = vlogChannelColor(chl, format, ap);
  va_end(ap);
  return ret;
}

int logChannelCSI(enum Channel chl, const struct CSI* table, const char* format, ...)
{
  __LogInit();
  if (format == NULL) return 0;
  va_list ap;
  va_start(ap, format);
  char * toformat = vsaprintf(format, ap);
  char * final = sprintfCSI(toformat, table, CSIlength(table));
  int ret = 0;
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

int vlogChannel(enum Channel chl, const char* format, va_list ap)
{
  if (format == NULL) return 0;
  char * str = vsaprintf(format, ap);
  int ret = 0;
  switch (chl) {
    default:
    case Default:
      ret = ChannelWrite(str, &Channel_Default);
      break;
    case Normal:
      ret = ChannelWrite(str, &Channel_Normal);
      break;
    case Warn:
      ret = ChannelWrite(str, &Channel_Warn);
      break;
    case Alert:
      ret = ChannelWrite(str, &Channel_Alert);
      break;
    case Debug:
      ret = ChannelWrite(str, &Channel_Debug);
      break;
    case BlackHole:
      ret = ChannelWrite(str, &Black_Hole);
      break;
  }
  free(str);
  return ret;
}

int logInfo(const char* format, ...)
{
  __LogInit();
  va_list ap;
  va_start(ap, format);
  int ret = vlogChannel(Normal, format, ap);
  va_end(ap);
  return ret;
}

int logInfoColor(const char* format, ...)
{
  __LogInit();
  va_list ap;
  va_start(ap, format);
  int ret = vlogChannelColor(Normal, format, ap);
  va_end(ap);
  return ret;
}

int logChannel(enum Channel chl, const char* format, ...)
{
  __LogInit();
  va_list ap;
  va_start(ap, format);
  int ret = vlogChannel(chl, format, ap);
  va_end(ap);
  return ret;
}

// length doesn't include \0
// return \0 terminated string, should be freed after using
char* CSIformat(struct CSI csi, size_t *len)
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

char *sprintfCSI(const char *format, const struct CSI *csi, size_t limit)
{
  static struct CSI reset = { "0", NULL, 'm' };
  if (format == NULL || strlen(format) == 0) return NULL;
  // FIXME
  if (csi == NULL || limit == 0) { csi = &reset; limit = 1; }
  size_t formatlen = strlen(format);
  const char *cur = format;
  size_t len = 0;

  String builder = StringBuild(NULL, 0);
  while (cur + len < format + formatlen) {
    char * sub = NULL;
    if (cur[len] == '{') {
      if (len > 0) {
        StringWrite(builder, cur, Relative, 0, len);
        cur += len;
        len = 0;
      }
      long ix = matchbrace(cur, &sub, &len);
      if (ix != -1) {
        cur = sub;
        sub = CSIformat(csi[((size_t)ix)>=limit?0:ix], &len);
        StringWrite(builder, sub, Relative, 0, len);
        free(sub);
        len = 0;
      }
    } else len ++;
  }
  if (len > 0)
    StringWrite(builder, cur, Relative, 0, len);
  char *buf = StringDump(builder, Text, 0, NULL);
  StringFree(builder);
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
  long remain = vsnprintf(ret, size, format, ap);
  // we cast size to int, because remain could be
  // negative.
  // TODO: Need a better logic here to handle type cast
  if (remain >= (long)size) {
    size = (size_t) remain + 1;
    ret = reallocarray(ret, size, sizeof(char));
    remain = vsnprintf(ret, size, format, aq);
    // here, remain is positive, it is
    // safe to cast it to size_t.
    assert((size_t)remain == size - 1);
    // same reason for condition below
  } else if (remain >= 0 && remain < (long)size) {
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
  long remain = vsnprintf(ret, size, format, ap);
  // TODO: Need a better logic here
  if (remain >= (long)size) {
    size = (size_t) remain + 1;
    ret = reallocarray(ret, size, sizeof(char));
    remain = vsnprintf(ret, size, format, aq);
    assert((size_t)remain == size - 1);
  } else if (remain >= 0 && (size_t)remain < size){
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

size_t CSIlength(const struct CSI *table)
{
  if (table == NULL) return 0;
  size_t len = 0;
  while (table[len].end != 0) {
    len ++;
  }
  return len;
}
