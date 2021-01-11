#ifndef DTOOLDYNAMIX
#define DTOOLDYNAMIX

#include "Attribute.h"

typedef unsigned long Size;
typedef Size Offset;

typedef struct string * String;

/*
 * Relative, Absolute, Backward
 * Buffer: indicate
 */
enum Position {
  Relative = 0x01,
  Absolute = 0x02,
  Backward = 0x03,
  Buffer   = 0x10
};

enum Mode {
  Text,
  Binary
};

String StringBuild(const char *, Size limit);
void StringFree(String);
const char* StringIndex(String, Offset, Size*);
char * StringDump(String, enum Mode, Offset, Size*);
long StringWrite(String, const char*, enum Position, Offset, Size);
long StringReplace(String, const char*, enum Position, Offset, Size);
long StringInsert(String, const char*, enum Position, Offset, Size);
Size StringLen(String);


////////////////////////////
// An Enumeratable Interface
////////////////////////////

// a wrapper for enumerating items
struct ITEM {
  void *source;
  Size index;
  void *data;
};
typedef struct ITEM * ITEM;

enum EnumerationType {
  EnumIndex,
  EnumIter,
};

// a handy iterator used in EnumNext function
typedef ITEM (*IEnumNext)(void* source, ITEM pre);

// Each structure which supports Enumeration
// nature should have something like
// "Enumerate$StructureName$" which return
// an enumeration structure below. its signature is
// not specified.
struct Enumeration {
  enum EnumerationType type;
  void * source;
  struct ITEM *item;  // The pointer type current
  Size count;
  IEnumNext next;
};

typedef struct Enumeration * Enumeration;

enum ITEMType {
  ITEMNONE,
  ITEMINDEX,
  ITEMITER
};

// for implementor using only
USERFUNC ITEM EBuildITEM(ITEM pre, void* data, enum ITEMType indicator);
USERFUNC void FreeITEM(ITEM);

// it accepts a none null enumeration thing, and return
// its next item. it will return NULL if nothing more is
// available.
USERFUNC ITEM EnumNext(Enumeration e);

Enumeration EnumerateString(String);

#endif
