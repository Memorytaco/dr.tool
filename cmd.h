#ifndef DTOOLCMD
#define DTOOLCMD

#define CSI_PREFIX "CSI"

// @cmd is the suffix of the CSI code
// @argc is variable argument count
// variable argument should be int
char* setcmd(char cmd, int argc, ...);
// len is not include 0 end marker.
char* setforecolor(int color, const char* const src, int offset, int len);
char* setbackcolor(int color, const char* const src, int offset, int len);

// variable argument is format as (const char* string, int string length)
// num is the number of the pairs; return a brand new string;
char* strnjoin(int num, ...);

#endif
