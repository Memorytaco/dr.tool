#include "cmd.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>

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

enum match_res {
  partial_match,
  full_match,
  null_match
};

// match "mat" with "src". partial or full match.
// used to check short or long option.
// is mat is longer than src then return null_match;
static enum match_res match_str(char* src, char *mat)
{
  size_t srclen = strlen(src);
  size_t matlen = strlen(mat);
  size_t smallsz = srclen > matlen ? matlen : srclen;
  enum match_res res = null_match;
  for (int i = 0; i < smallsz; i++) {
    if (src[i] != mat[i]) return res;
  }
  if (smallsz != srclen || smallsz != matlen)
    if (smallsz == srclen)
      res = null_match;
    else
      res = partial_match;
  else
    res = full_match;
  return res;
}

static inline void set_val_opt(struct cmdarg* cmdarg, void* val, size_t len) {
  cmdarg->opt |= cmd_opt_val;
  cmdarg->val = val;
  cmdarg->len = len;
}

// if find '=', return its position or return 0;
static inline size_t search_eql_opt(char* arg, size_t len) {
  for (size_t i = 0; i<len; i++) {
    if (arg[i] == '=') return i;
  }
  return 0;
}

// accept one command line string, and generate
// struct cmd_arg structure from it.
struct cmdarg* parse_arg(char* arg, bool* all_value)
{
  struct cmdarg* cmdarg = malloc(sizeof(struct cmdarg));
  size_t arglen = strlen(arg);
  memset(cmdarg, 0, sizeof(struct cmdarg));

  if (*all_value) {
    set_val_opt(cmdarg, arg, arglen);
    return cmdarg;
  }

  // check whether the first token is '-', distinguish it with value
  if (arg[0] != '-') {
    set_val_opt(cmdarg, arg, arglen);
    return cmdarg;
  }

  // if only one character '-', it's a special value
  // which denotes standard input
  if (arglen == 1) {
    set_val_opt(cmdarg, arg, 1);
    return cmdarg;
  }

  // now the arglen >= 2
  if (arg[1] == '-') {
    // if its length is only 2, then it is "--"
    // which should denote a special seperation
    // between value and option. return it;
    if (arglen == 2) {
      *all_value = true;
      cmdarg->opt |= cmd_opt_seperate;
      cmdarg->key = cmdarg->val = arg;
      cmdarg->len = 2;
      return cmdarg;
    }

    // if not, check equal sign "=", it is long option now
    size_t posi = search_eql_opt(arg, arglen);
    cmdarg->opt |= cmd_opt_long;

    if (posi) {
      // Got a index of '='
      cmdarg->opt |= cmd_opt_withequ;
      cmdarg->key = arg+2;  // the key may be an empty string ""
      // something like "--=value" => "--\0value"
      arg[posi] = 0;  // here, it change '=' to 0, make key unique string
      bool islast = posi == arglen - 1;
      cmdarg->val = islast ? "" : arg+posi+1;
      cmdarg->len = islast ? 0 : strlen(cmdarg->val);
    } else {
      // doesn't find '=', so it is an option
      cmdarg->key = arg+2;  // point to the value without "--"
    }
    return cmdarg;
  }

  // now it could only be short option
  cmdarg->opt |= cmd_opt_short;
  cmdarg->key = arg+1;
  return cmdarg;
}

// search available register using cmdarg as indicator.
// partial core logic here.
// core logic 1.
struct cmd_reg* cmd_search(struct cmdarg* cmdarg, struct cmd_reg* reg, size_t *posi)
{
  struct cmd_reg *ptr = reg;
  enum match_res res = null_match;
  struct cmd_reg *regres = NULL;
  for (ptr = reg; ptr->call != NULL; ptr++) {
    // check option type of cmdarg and ptr
    // only continue when they match.
    static enum cmdopt selector = (cmd_opt_short | cmd_opt_long);
    if (!(selector & cmdarg->opt & ptr->arg.opt)) continue;
    // after the type check, do the job
    switch (match_str(cmdarg->key, ptr->arg.key)) {
      case partial_match:
        // long option has no meaning when partial match
        // you can't distinguish pure long option --hellonice
        // and the option --hello with suffix value nice.
        // so, jump.
        if (cmdarg->opt & cmd_opt_long) continue;
        // if it's a short option. then something
        // -fhello.c is valid. and so is -WIall.
        // but if the short option is two characters, then we can't
        // distinguish between -hl and -h with an 'l' prefix value.
        *posi = strlen(ptr->arg.key);
        cmdarg->val = cmdarg->key + (*posi);
        cmdarg->len = *posi;
        return ptr;
      case full_match:
        *posi = 0;
        return ptr;
      default:
        ; // do nothing
    }
  }
  return NULL;
}

// TODO: complete its logic
/*
  do full match first logic:
    if i got two short option -A and -AAA.
    and input value is '-AAA', match it with -AAA instead of
    the -A with value 'AA'.
  do aggregate boolean short option parse:
    if i defined four short boolean option -a, -b, -c, -d.
    and user input "-abcd", parse it as four boolean options.
    only one character short option can be aggregated. and if we
    got -abcd option, match it first (not four boolean option).
  do a comprehensive invoke process:
    not just call the callback immediately, but generate a structure
    which can be queried with option values. or do both, call
    it immediately and return a queriable structure.
*/
// core logic 2.
void cmd_match(int argc, char** argv, struct cmd_reg* table, void* store)
{
  struct cmdarg* cmdargs[argc];
  bool allvalue = false;  // indicator of "--"
  for (int i=0; i<argc; i++) {
    cmdargs[i] = parse_arg(argv[i], &allvalue);
  }
  size_t posi = 0;
  struct cmd_reg* reg = NULL;
  for (int i=0; i<argc; i++) {
    // precheck if it is a value option
    if (cmdargs[i]->opt & cmd_opt_val) {
      printf("Got Value Cmd %s\n", cmdargs[i]->val);
      continue;
    }
    reg = cmd_search(cmdargs[i], table, &posi);
    // reg NULL guard
    if (!reg) {
      printf("UnRecognized Option %s\n", cmdargs[i]->key);
      continue;
    }
    // first handle long option
    if (reg->arg.opt & cmd_opt_long) {
      if ((reg->arg.opt & cmd_opt_toggle) || cmdargs[i]->val) {
        reg->call(cmdargs[i], NULL, store);
        continue;
      }
      if (i == argc - 1) {
        printf("Invalid long option: --%s, need one value\n", cmdargs[i]->key);
        continue;
      }
      if (cmdargs[i+1]->opt & cmd_opt_val) {
        i++;
        cmdargs[i-1]->val = cmdargs[i]->val;
        cmdargs[i-1]->len = cmdargs[i]->len;
        reg->call(cmdargs[i-1], NULL, store);
        continue;
      }
      printf("Invalid value, Expect value for '--%s', but got option %s\n", cmdargs[i]->key, cmdargs[i+1]->key);
    } else {
      // now we handle short option
      if ((reg->arg.opt & cmd_opt_toggle) || cmdargs[i]->val) {
        reg->call(cmdargs[i], NULL, store);
        continue;
      }
      // this option will need further value
      if (i == argc - 1) {
        printf("Invalid short option: -%s, need one value\n", cmdargs[i]->key);
        continue;
      }
      if (cmdargs[i+1]->opt & cmd_opt_val) {
        i++;
        cmdargs[i-1]->val = cmdargs[i]->val;
        cmdargs[i-1]->len = cmdargs[i]->len;
        reg->call(cmdargs[i-1], NULL, store);
        continue;
      }
      printf("Invalid value, Expect value for '-%s', but got option %s\n", cmdargs[i]->key, cmdargs[i+1]->key);
    }
  }
  return ;
}
