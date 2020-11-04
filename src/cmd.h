#ifndef DTOOLCMD
#define DTOOLCMD

#define CSI_PREFIX "CSI"
#define DTOOLCMD_VERSION "0.0.2"

/*
 * 1. use DEPLENGTH to determine the max length of dependency
 *    of option.
 */

#include <stdio.h>
#include <stdbool.h>

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

/////////////////////////////////
// Command Line User Interface //
/////////////////////////////////

/*
 *  principle:
 *    1. scalar option is always picked as first match result
 *    2. full match prioritizes partial match, longest match,
 *       in other words.
 */
enum cmdopt {
  cmd_opt_null     = 0x0000,   // @ means it is an empty option
  cmd_opt_short    = 0x0001,   // @ short option "-opt"
  cmd_opt_long     = 0x0002,   // @ long option
                               // "--longopt-with-description"
  cmd_opt_toggle   = 0x0010,   // @ short or long bool toggle value
  cmd_opt_optval   = 0x0020,   // @ this option should be used with
                               // toggle, indicate the option may
                               // accept an argument. something
                               // like -f or -ftrue.
  cmd_opt_expand   = 0x0100,   // @ accept as many values
                               // as possible
  cmd_opt_withequ  = 0x0040,   // @ option with "key=value"
                               // form, the value may be a list
                               // like "val[,val]..", it needs to
                               // be parsed by library user
  cmd_opt_seperate = 0x1000,   // @ something like "--" to seperate
                               // special value with option
  cmd_opt_val      = 0x2000,   // @ value type cmdline, something
                               // without '-'
};

#define cmdopt_allopt (cmd_opt_long | cmd_opt_short)

// interface for Extra values returned by cmd_match
typedef struct cmdarglst* arglst;
const char* argvidx(arglst lst, size_t idx);
size_t argvctx(arglst lst);

// Opaque structure for arguments access in callback function
struct cmdargt;
void * cmdargt_access(struct cmdargt *arg, size_t ix, size_t *sz);
size_t cmdargt_length(struct cmdargt *arg);

// command line callback used to do something when the option
// appeared.
// @arg is used to identify its calling type provid
//     neccessory value
// @data is extra information which is used by user.
//     using $argvidx and $argc to access it.
// @store is something user defined global data center.
//     it needs to be allocated by user and known by user.
//     you can check something in here, user allocated memory.
//     eg. global settings or value.
typedef void (*cmdact)
  (struct cmdargt *arg, void* data, void* store);

// structure to describe the dependency between options
// and values.
// if *req == 0, it indicates an end.
#ifndef DEPLENGTH
#define DEPLENGTH 16
#endif

#define DEPEND 0

struct cmdept {
  // required dependency, end with DEPEND
  unsigned int req[DEPLENGTH];
  // optional dependency, end with DEPEND
  unsigned int opt[DEPLENGTH];
};

// the code is used to get description of the option
struct cmdregt {
  char const * key; // unique strine @key
  // command line register code to specific one option
  unsigned int code;
  enum cmdopt opt;  // required cmd option type
  // dependent option code for this option to run, end with 0.
  // don't get confused with group
  struct cmdept dep;
  cmdact call;      // action to take when facing the option
};

// main call for matching cmmand line arguments
struct cmdarglst* cmd_match(int argc, char** argv, struct cmdregt* regs, void* store);

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

// TODO: provide a handy interface for user to declare its desired
// option info
// e.g.
// user can provide something like this
//
// "4nice$1h@"  // remian to build
//  ^   ^^
//specif||ic the number of code name, here is "4"
//      ||
//      t|he code name is "nice"
//       |
//       "$", begin of option name. follow the name length, "1"
//       and the name is "h"

// TODO: add "getopt" interface, allow user to handle complicate
// situations.

#endif
