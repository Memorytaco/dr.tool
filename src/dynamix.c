#include "dynamix.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

/*
 * ***************************
 *       Dynamix String
 * ***************************
 */

struct string {
  char * strbuf;
  Size cursor;    // point to current empty cell
  Size capacity;  // current buffer size
};

// initial capacity for string
#define STRINGINITSIZE 64
// scale factor for buffer
#define BUFSCALE 2

// TODO: handle memory failure
String StringBuild(const char * raw, Size limit)
{
  String builder = malloc(sizeof(struct string));
  memset(builder, 0, sizeof(struct string));

  if (raw == NULL) {
    if (limit == 0) limit = STRINGINITSIZE;
    builder->strbuf = calloc(sizeof(char), limit);
    builder->capacity = limit;
    return builder;
  }

  // raw is not NULL.
  if (limit == 0) limit = strlen(raw);
  builder->strbuf = calloc(sizeof(char), limit * 2);
  builder->capacity = limit * 2;
  builder->cursor = limit;
  memmove(builder->strbuf, raw, limit);

  return builder;
}

void StringFree(String builder)
{
  if (builder == NULL) return ;

  free(builder->strbuf);
  free(builder);
}

const char* StringIndex(String builder, Offset off, Size* szptr)
{
  if (builder == NULL) return NULL;
  if (off >= builder->cursor) return NULL;

  if (szptr) *szptr = builder->cursor - off;
  return builder->strbuf + off;
}

char * StringDump(String builder, enum Mode mode, Offset off, Size* szptr)
{
  Size length = 0;
  const char * consbuf = StringIndex(builder, off, &length);

  if (consbuf == NULL) return NULL;

  char * ret = NULL;

  switch (mode) {
    default:
    case Text:
      ret = calloc(sizeof(char), length + 1);
      break;
    case Binary:
      ret = calloc(sizeof(char), length);
      break;
  }

  memmove(ret, consbuf, length);
  if (szptr) *szptr = length;
  return ret;
}

long StringReplace(String builder, const char* raw, enum Position pos, Offset off, Size sz)
{
  if (builder == NULL) return -1;
  Size cur = builder->cursor;
  long status = StringWrite(builder, raw, pos, off, sz);
  if (builder->cursor < cur) builder->cursor = cur;
  return status;
}

long StringWrite(String builder, const char* raw, enum Position pos, Offset off, Size sz)
{
  if (builder == NULL || raw == NULL) return -1;
  if (sz == 0 && Buffer & ~pos) sz = strlen(raw);

  switch (pos) {
    case Relative:
      off += builder->cursor;
    case Absolute:
      break;
    case Backward:
      off = off >= builder->cursor ? 0 : builder->cursor - off;
      break;
    default:
      return -2;
  } // @off now is the absolute position indicator

  Size limit = builder->capacity;
  if (off + sz >= limit) {
    limit = off + sz;
    builder->strbuf =
      reallocarray(builder->strbuf, BUFSCALE, limit);
    limit *= BUFSCALE;
  }

  char *base = builder->strbuf;
  memmove(base + off, raw, sz);

  builder->cursor = off + sz;
  builder->capacity = limit;
  assert(builder->capacity * BUFSCALE == builder->cursor
      || builder->capacity > builder->cursor);

  return 0;
}

long StringInsert(String builder, const char* raw, enum Position pos, Offset off, Size sz)
{
  if (builder == NULL || raw == NULL) return -1;
  if (sz == 0 && Buffer & ~pos) sz = strlen(raw);

  switch (pos) {
    case Relative:
      off += builder->cursor;
    case Absolute:
      break;
    case Backward:
      off = off >= builder->cursor ? 0 : builder->cursor - off;
      break;
    default:
      return -2;
  }

  Size moveoff = off > builder->cursor ? 0 : builder->cursor - off;
  Size limit = builder->capacity;
  if (off + sz + moveoff >= limit) {
    limit = off + sz + moveoff;
    builder->strbuf =
      reallocarray(builder->strbuf, BUFSCALE, limit);
    limit *= BUFSCALE;
  }

  char *base = builder->strbuf;
  memmove(base + off + sz, base + off, moveoff);
  memmove(base + off, raw, sz);

  builder->cursor = off + sz + moveoff;
  builder->capacity = limit;
  assert(builder->capacity * BUFSCALE == builder->cursor
      || builder->capacity > builder->cursor);

  return 0;
}

Size StringLen(String builder)
{
  if (builder == NULL) return 0;
  return builder->cursor;
}

/*
 * **************************
 *        Enumeration
 * **************************
 */

USERFUNC void FreeITEM(ITEM it)
{
  if (it == NULL) return;
  if (it != NULL) free(it);
  return;
}


USERFUNC ITEM EBuildITEM(ITEM pre, void* data, enum ITEMType indicator)
{
  if (indicator == ITEMNONE) {
    return NULL;
  }

  if (pre == NULL) {
    pre = calloc(sizeof(struct ITEM), 1);
  }
  pre->data = data;
  pre->index ++;
  return pre;
}

USERFUNC ITEM EnumNext(Enumeration e)
{
  ITEM i = e->next(e->source, e->item);
  if (i != NULL) e->item = i;
  return i;
}

ITEM StringNext(void *source, ITEM pre)
{
  Size idx = 0;
  if (pre != NULL) {
    idx = pre->index;
  }

  if (idx >= StringLen(source)) {
    return EBuildITEM(pre, NULL, ITEMNONE);
  }

  char* c = (char*)StringIndex(source, idx, NULL);
  return EBuildITEM(pre, c, ITEMINDEX);
}

Enumeration EnumerateString(String s)
{
  if (s == NULL) {
    return NULL;
  }

  Enumeration e = calloc(sizeof(struct Enumeration), 1);
  e->source = s;
  e->type = EnumIndex;
  e->next = StringNext;
  e->count = StringLen(s);
  EnumNext(e);
  return e;
}
