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

// define command line user interface.
/*
  principle:
    1. scalar option is alwayse getting first match
    2. full match prioritizes partial match
*/
enum cmdopt {
  cmd_opt_null     = 0x0000,   // means it is an empty option
  cmd_opt_short    = 0x0001,   // short option "-opt"
  cmd_opt_long     = 0x0002,   // long option "--longopt-with-description"
  cmd_opt_toggle   = 0x0010,   // short or long bool toggle value
  cmd_opt_optval   = 0x0020,   // this option should be used with toggle, indicate the option may accept an argument. something like -f or -ftrue.
  cmd_opt_expand   = 0x0100,   // accept as many values as possible 
  cmd_opt_withequ  = 0x0040,   // option with "key=val[,val]" form
  cmd_opt_seperate = 0x1000,   // something like "--" to seperate special value with option
  cmd_opt_val      = 0x2000,   // value type cmdline, something without '-'
};

#define cmdopt_allopt (cmd_opt_long | cmd_opt_short)

// this structure is only for holding the option value.
struct cmdargt {
  char *key;        // the option name without prefix, terminted with \0.
  struct valink {
    void *val;      // the value content, its a buffer, it will alwayse get a copy, so it's safe to free.
    size_t size;    // length of the value buffer.
    struct valink *next;  // maybe a next element.
  } *valist;
  // if valist is NULL, then empty value, which may be judged by its user(the callback).
  struct cmdargt* next;
};

void cmdargt_free(struct cmdargt *arg);
void cmdargt_append_buffer(struct cmdargt *arg, void const * buffer, size_t size);
void cmdargt_append_string(struct cmdargt *arg, char* str);
// idx is both the index and the returned value size.
void* cmdargt_fetch_val(struct cmdargt const * arg, size_t *idx);
inline struct cmdargt* cmdargt_fetch_arg(struct cmdargt const * arg, size_t idx);
size_t cmdargt_valength(struct cmdargt const * arg);

struct cmdarglst {
  char **argv;
  size_t argc;
};

typedef struct cmdarglst* arglst;

#define argvidx(v, i) ((struct cmdarglst*)(v))->argv[i]
#define argvctx(v) ((struct cmdarglst*)(v))->argc

struct cmdarglst* cmdargt_collect(struct cmdargt *arg, size_t cap);

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
typedef void (*cmdcallt)(struct cmdargt *arg, void* data, void* store);

// structure to describe the dependency between options
// and values.
// if *req == 0, it indicates an end.
#ifndef DEPLENGTH
#define DEPLENGTH 16
struct cmdept {
  // required dependency, end with zero
  unsigned int req[DEPLENGTH];
  // optional dependency, end with zero
  unsigned int opt[DEPLENGTH];
};
#endif
// the code is used to get description of the option
struct cmdregt {
  char const * key; // unique strine @key
  // command line register code to specific one option
  unsigned int code;
  enum cmdopt opt;  // required cmd option type
  // dependent option code for this option to run, end with 0.
  // don't get confused with group
  struct cmdept dep;
  cmdcallt call;    // call back: do the job
};

inline size_t cmdregtlen (struct cmdregt* regs) {
  size_t len = 0;
  for (; regs->call != NULL; regs++) len++;
  return len;
}

// a calling table to check correctness of the passed option
// and call the sequence.
struct cmdlst {
  struct cmdregt *reg;
  struct cmdargt *arg;
  struct cmdlst *next;
};

struct cmdlst * cmdlst_alloc();
void cmdlst_free(struct cmdlst *lst);
size_t cmdlst_length(struct cmdlst const * lst);
// this two calls doesn't allocate or free memory, just pointer
// manipulation
struct cmdlst * cmdlst_get(struct cmdlst *hdr, size_t idx);
// insert ptr at position idx, return its position.
size_t cmdlst_insert(struct cmdlst **hdr, size_t idx, struct cmdlst *ptr);
struct cmdlst * cmdlst_remove(struct cmdlst **hdr, size_t idx);
// move idx1 to idx2. return the new index of idx1 if success.
// otherwise return -1.
int cmdlst_move(struct cmdlst **hdr, size_t idx1, size_t idx2);
// search the specific code in the call list, return its index from
// @*index. pass NULL, to discard the return value.
struct cmdlst * cmdlst_search(struct cmdlst *hdr, size_t code, size_t *index);
size_t cmdlst_reverse_get(struct cmdlst *hdr, struct cmdlst *ptr);
// check the command option dependency, if satifised, return true
// otherwise return false.
// it will search all the command and try its best to fill the
// requirment.
bool cmdlst_chk(struct cmdlst **hdr, struct cmdregt* regs);
// call the list.
void cmdlst_call(struct cmdlst *hdr, struct cmdarglst *lst, void* store);

/*
 * The Big Brother is Watching You.
 */
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

#endif
