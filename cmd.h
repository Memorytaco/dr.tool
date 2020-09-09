#ifndef DTOOLCMD
#define DTOOLCMD

#define CSI_PREFIX "CSI"
#define DTOOLCMD_VERSION "0.0.1"

#include <stdio.h>

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

// define command line user interface.
/*
  principle:
    1. scalar option is alwayse getting first match
    2. full match prioritizes partial match
*/
enum cmdopt {
  cmd_opt_short    = 0x0001,   // short option "-opt"
  cmd_opt_long     = 0x0002,   // long option "--longopt-with-description"
  cmd_opt_toggle   = 0x0010,   // short or long bool toggle value
  cmd_opt_optval   = 0x0020,   // this option should be used with toggle, indicate the option may accept an argument. something like -f or -ftrue.
  cmd_opt_withequ  = 0x0040,   // option with "key=val[,val]" form
  cmd_opt_seperate = 0x1000,   // something like "--" to seperate special value with option
  cmd_opt_val      = 0x2000    // value type cmdline, something without '-'

};

// this structure is used both for parsing and registering
// command line parsing
struct cmdarg {
  enum cmdopt opt;  // set its option type
  char *key;        // the option name without prefix
  void *val;        // parsed option value or defined value
  size_t len;       // it will have different meaning
                    // depending on the "key"
};

// command line callback used to do something when the option
// appeared.
// @arg is used to identify its calling type.
// @data is extra information which is used by user.
// @store is something user defined global data center. you can check something in here, user allocated memory.
typedef int (*cmd_callback)(struct cmdarg *arg, void* data, void* store);

// the code is used to get description of the option
struct cmd_reg {
  int code;           // command line register code to specific one option
  struct cmdarg arg;  // ready value
  cmd_callback call;  // call back: do the job
};

void cmd_match(int argc, char** argv, struct cmd_reg* table, void* store);

// TODO: introduce subcommand system here.

// TODO: provide some helper function, helping parse some option values
// which look like a,b,c,d or something i=3,b=4 or curl like "url@a".

// void fun();
// void fun();
// void fun();
// void fun();

// TODO: provide option description and showcase help function.
// this may be built by user himself, better to provide some mechanism.

/*
  some structure here
*/

// TODO: provide error notification mechanism for situation that
// some required option value doesn't appear or some required
// option group doesn't be combined.

/*
  mechanism here.
*/

#endif
