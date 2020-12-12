#ifndef DTOOLDYNAMIX
#define DTOOLDYNAMIX

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

typedef struct vector * Vec;

Vec VecBuild();

#endif
