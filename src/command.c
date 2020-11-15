#include "command.h"
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#define PREFIX(p, v) p ## v

/////////////////////////////////
// 1. Command Line Escape Code //
/////////////////////////////////

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



////////////////////////////////////
// 2. Command Line User Interface //
////////////////////////////////////

/*
 *  principle:
 *    1. scalar option is always picked as first match result
 *    2. full match prioritizes partial match, longest match,
 *       in other words.
 */
// types of option: long, short, optional etc.
// TODO: remove these configuration
// The following options should be replaced by one single
// unsigned int and some special numbers. so library users
// can tweak these options and define their own attributes
// and its behaviour
enum genret {
  genre_null     = 0x0000,   // @ means it is an empty option
  genre_short    = 0x0001,   // @ short option "-opt"
  genre_long     = 0x0002,   // @ long option
                             // "--longopt-with-description"
  genre_toggle   = 0x0010,   // @ short or long bool toggle value
  genre_optval   = 0x0020,   // @ this option should be used with
                             // toggle, indicate the option may
                             // accept an argument. something
                             // like -f or -ftrue.
  genre_expand   = 0x0100,   // @ accept as many values
                             // as possible
  genre_withequ  = 0x0040,   // @ option with "key=value"
                             // form, the value may be a list
                             // like "val[,val]..", it needs to
                             // be parsed by library user
  genre_seperate = 0x1000,   // @ something like "--" to seperate
                             // special value with option
  genre_value    = 0x2000,   // @ value type cmdline, something
                             // without '-'
};

#define genre_all (genre_long | genre_short)

// This structure hold the parsed command line arguments.
typedef struct cmdunit {
  // name is full option text, like --option, -a.
  char * name;
  struct unitcell {
    // the cell content, which is a buffer.
    // it will always be a copy of something.
    // so it's safe to modify it and free it later.
    void *val;
    // length of the value buffer.
    size_t size;
    // pointer to next element
    struct unitcell *next;
  } * cells;
  // if valist is NULL, then empty value, which
  // may be judged by its user(the callback).
  struct cmdunit* next;
} Unit;

typedef struct unitcell Cell;

// structure for holding extra values
typedef struct ExtraVal {
  size_t argc;
  char **argv;
} ArgcArgv;

ArgcArgv* buildArgcArgv(Unit *link);

const char* argvidx(ArgcArgv *cv, size_t idx) {
  if (idx < cv->argc) return cv->argv[idx];
  return NULL;
}
size_t argvctx(ArgcArgv *cv) { return cv->argc; }


#ifndef __prefix
#define __prefix(v) PREFIX(Unit, v)

// free one single unit and return its next pointer.
static Unit* __prefix(free)
  (Unit *u);
// allocate one unit with its name, return the pointer.
// if name is null, set name to null. name would be
// a copy from parameter.
// if u is not NULL, append to this unit.
// if size is zero, name should be null terminated string.
static Unit* __prefix(alloc)
  (Unit *u, const char *name, size_t size);
// get the unit at position ix. if ix exceeds max
// position, return NULL.
static Unit* __prefix(ix)
  (const Unit *u, size_t ix);
// return number of units, return 0 if error occurred.
static size_t __prefix(cx)
  (const Unit *u);

/* Cell manipulation Calls */

// get cell at position of ix, return NULL if error.
static Cell* __prefix(Cellix)
  (const Unit *u, size_t ix);
// return number of cells in this unit.
static size_t __prefix(Cellcx)
  (const Unit *u);

// There are three pointers here, and 6 options here.
//    Unit*u     Cell*c    void*buf
// 1. NULL        *
// 2. *           NULL  
// 3. NULL        NULL
// 4. *           *
// 5. ?           ?         NULL
// 6. ?           ?         *
//
// 1. append cell to c
// 2. append the new created cell to Unit tail
// 3. do nothing
// 4. insert cell before Cell c in Unit u
// 5. buffer not provided, just allocate buffer and set to zero
// 6. copy the buffer
//
// if size not 0, allocate size memory. if provided buf, copy
// buf to memory with limit to size.
// if size is 0 and provided buf, take buf as 0 terminated
// string and copy to buf. otherwise, do nothing.
//
// return pointer to the new allocated cell or
// return NULL if error occurred. e.g. in case 4, the Cell c
// is not in the Unit u, so insert task can not be completed.
static Cell* __prefix(Cellalloc)
  (Unit *u, Cell *c, const void *buf, size_t size);

// return the next pointer from the cell.
// if u is not NULL, update the inner pointer.
// return NULL, if error occurred.
static Cell* __prefix(Cellfree)
  (Unit *u, Cell *c);

// append the string cell to one unit
#define UnitCellallocSM(unit, cell, str) \
    PREFIX(Unit, Cellalloc) (unit, cell, str, 0)

// User Interface Definiton
void * Argvix(Arg *u, size_t ix, size_t *sz) {
  Cell *c = __prefix(Cellix) (u, ix);
  if (c == NULL) return NULL;
  if (sz) *sz = c->size;
  return c->val;
}
size_t Argvcx(Arg *u) {
  return __prefix(Cellcx) (u);
}

#undef __prefix
#else
#error Do Not Touch __prefix
#endif

#ifndef __prefix
#define __prefix(v) PREFIX(Invoke, v)

typedef struct cmdinvoke {
  size_t capacity;
  size_t invcx;
  size_t fincx;
  void  *store;
  ArgcArgv *cv;
  struct invokeunit {
    const Spec * ctx;    // contain action and rely info
    const Alia * entry;  // index of alias
    Unit  *unit;         // arguments
  } *list, *finish;
} Invoke;

typedef struct invokeunit InvokeUnit;

// allocate and free whole memory
Invoke* __prefix(alloc)   (size_t capci);
void    __prefix(free)    (Invoke *);

/* Store manipulate */
void    __prefix(StoreSet)  (Invoke * inv, void* s);
void*   __prefix(StoreGet)  (Invoke * inv);

/* extra value */
void      __prefix(CVSet)   (Invoke * inv, ArgcArgv *cv);
ArgcArgv* __prefix(CVGet)   (Invoke * inv);

// add entry to invoke list
bool    __prefix(add)     (Invoke * inv, const Spec*, const Alia*, Unit*);
bool    __prefix(rm)      (Invoke * inv, size_t ix);
bool    __prefix(mv)      (Invoke * inv, size_t from, size_t to);

/* Invoke Function to execute the action */
// invoke function at position @ix, no rely check.
bool    __prefix(ix)      (Invoke * inv, size_t ix);
// same as ix, just remove it from list after invoking
bool    __prefix(ixrm)    (Invoke * inv, size_t ix);

// TODO: finish these Invoke function definition
// invoke function with name and with rely check
bool    __prefix(name)    (Invoke * inv, const char * name, size_t ix);
bool    __prefix(namerm)  (Invoke * inv, const char * name, size_t ix);

// invoke all action with rely check
bool    __prefix(all)     (Invoke * inv);

// search one optname in list, return its position(begin with 1).
// use offset to denote start position for searching which
// usually is 0 (start from first one).
// if error occurred, return 0 otherwise return (position + 1).
// YOU NEED TO SUBTRACT IT WITH 1 TO GET ITS POSITION OFFSET.
size_t  __prefix(search)  (Invoke * inv, const char* name, size_t offset);

// check rely status, return true if satisfied
// and false otherwise.
bool    __prefix(check)   (Invoke * inv, const char* name);
bool    __prefix(checkix) (Invoke * inv, size_t ix);
bool    __prefix(checkall)(Invoke * inv);

#else
#error Do Not Touch __prefix
#endif

// accept one command line string.
// pass @holder as NULL to generate a new option.
// pass @valbool as true to indicate the string is value. it will be set.
// pass @opt to accept cmd options.
// '-' will be treated as a value
// "--" will be treated a special key "--", and all the following
//     command line arguments will be treated as value and
//     appended to this argument value field.
// some value will be inserted to an argument with key as null.
static Unit * buildUnit
( const char * str
, Unit *holder
, enum genret *opt
, bool *valbool
) {
  static enum genret local = genre_null;
  static bool isval = false;

  opt = opt?opt:&local;
  *opt = genre_null;

  // if valbool not NULL, it will always update local flag.
  // and local flag can smooth the parsing progress.
  valbool = valbool?valbool:&isval;
  isval = *valbool;

  if (str == NULL) {
    printf("Str shouldn't be null, at file command.c line %d\n"
        , __LINE__);
    return NULL;
  }

  if (*valbool) {
    holder = holder?holder:Unitalloc(NULL, NULL, 0);
    UnitCellallocSM(holder, NULL, str);
    *opt |= genre_value;
    return holder;
  }

  // is "--" at the begining of str?
  if (strstr(str, "--") == str) {
    // Yes, it is a long option
    // and does it have a "=" inside?
    char * eq = strchr(str, '=');
    if (eq == NULL) {
      // no, "=" is not inside this string, so
      // generate a new long option
      if (strlen(str) == 2) {
        // is special "--"?
        *opt |= genre_seperate;
        *valbool = true;
      } else {
        // no, it's just normal long option
        *opt |= genre_long;
      }
      holder = Unitalloc(holder, str, 0);
    } else {
      // Yes, "=" is part of string
      size_t offset = eq - str;
      assert(offset >= 2);
      if (offset == 2) {
        // got a malform of "--=...", convert it to a
        // value option, with name NULL.
        holder = Unitalloc(holder, NULL, 0);
      } else {
        holder = Unitalloc(holder, str, offset);
        *opt |= genre_long;
      }
      size_t len = strlen(str);
      if (offset != len - 1) {
        assert(offset < len - 1);
        *opt |= genre_withequ;
        UnitCellallocSM(holder, NULL, str+offset+1);
      }
    }
  } else if (strstr(str, "-") == str) {
    if (strlen(str) == 1) {
      // is special value "-"?
      *opt |= genre_value;
      holder = holder?holder:Unitalloc(NULL, NULL, 0);
      UnitCellallocSM(holder, NULL, str);
    } else {
      // no, just normal short option
      *opt |= genre_short;
      holder = Unitalloc(holder, str, 0);
    }
  } else {
    // now handle the normal value
    *opt |= genre_value;
    holder = holder?holder:Unitalloc(NULL, NULL, 0);
    UnitCellallocSM(holder, NULL, str);
  }
  return holder;
}

typedef struct alia_desc {
  const Alia * alia;
  struct nametoken {
    char * name;
    enum genret genre;
  } ** tokens; // null terminated array
} AliaDesc;

typedef struct nametoken AliaToken;

// TODO: Add alias syntax check
bool checkAlia() {}

void AliaDescinfo(const AliaDesc *desc) {
  printf("%s\n", desc->alia->alia);
  for (AliaToken **tk = desc->tokens; *tk != NULL; tk++) {
    printf("  -> {%.2s} ", genre_toggle&((*tk)->genre)?" T":"  ");
    printf("%s\n", (*tk)->name);
  }
}

AliaDesc* buildAliaDesc(const Alia *alia)
{
  AliaDesc *desc = malloc(sizeof(AliaDesc));
  desc->alia = alia;

  // allocate the tokens from alia string, which can
  // be directly manipulated, we don't need allocate
  // memory for &struct nametoken name field.
  char * stream = malloc(strlen(alia->alia) + 1);
  memmove(stream, alia->alia, strlen(alia->alia) + 1);
  assert(!strcmp(stream, alia->alia));

  size_t count = 8;
  size_t ix = 0;
  desc->tokens = calloc(count, sizeof(void*));
  for ( char *entrysave = NULL
      , *entry = strtok_r(stream," ", &entrysave);
      entry != NULL;
      entry = strtok_r(NULL , " ", &entrysave))
  {
    desc->tokens[ix] = malloc(sizeof(struct nametoken));
    desc->tokens[ix]->name = entry;
    char *optsave = NULL;
    char *opt = strtok_r(entry, ":", &optsave);
    // TODO: Add special ':' handle, Here Just handle single ':'
    // something like "--address-base:start:?"
    opt = strtok_r(NULL , ":", &optsave);

    // we only want to make sure that name prefix with
    // "-" or "--"
    if (strstr(entry, "--") == entry) {
      desc->tokens[ix]->genre = genre_long;
    } else if (strstr(entry, "-") == entry) {
      desc->tokens[ix]->genre = genre_long;
    } else {
      // TODO Add an error handle here
      ;
    }

    if (opt != NULL) {
      // TODO: Add option parsing code Here
      // Now we just handle '?'
      if (*opt == '?') desc->tokens[ix]->genre |= genre_toggle;
    }

    ix ++;
    if (ix >= count) {
      count += count;
      desc->tokens = reallocarray(desc->tokens, count, sizeof(void*));
      assert(desc->tokens != NULL);
    }
  }

  desc->tokens = reallocarray(desc->tokens, ix+1, sizeof(void*));
  return desc;
}

// TODO expand this in future
typedef struct spec_desc {
  const Spec * spec;
  AliaDesc ** alias;
} SpecDesc;

SpecDesc * buildSpecDesc(Spec *spec)
{
  SpecDesc * desc = malloc(sizeof(SpecDesc));
  desc->spec = spec;

  size_t count = 8;
  size_t ix = 0;
  desc->alias = calloc(count, sizeof(void*));
  for (Alia *ptr = spec->alias; ptr->alia != NULL; ptr++) {
    desc->alias[ix] = buildAliaDesc(ptr);
    /* AliaDescinfo(desc->alias[ix]); */

    ix ++;
    if (ix >= count) {
      count += count;
      spec->alias = reallocarray(desc->alias, count, sizeof(void*));
      assert(spec->alias != NULL);
    }
  }
  spec->alias = reallocarray(desc->alias, ix + 1, sizeof(void*));
  return desc;
}


typedef struct spec_search_res {
  SpecDesc * specdesc;
  AliaDesc * aliadesc;
  AliaToken * aliatoken;
} SpecSearchRes;

// with longest match here
static SpecSearchRes cmdspec_search(char const * name, enum genret *opt, SpecDesc ** specs)
{
  SpecSearchRes res = { NULL, NULL, NULL };
  for (; *specs != NULL; specs++) {
    for (AliaDesc ** desc = (*specs)->alias; *desc != NULL; desc ++) {
      for (struct nametoken **tok = (*desc)->tokens; *tok != NULL; tok++) {
        if (!strncmp((*tok)->name, name, strlen((*tok)->name))) {
          if (opt) *opt = (*tok)->genre;
          res.specdesc = *specs;
          res.aliadesc = *desc;
          res.aliatoken = *tok;
        }
      }
    }
  }

  do {
    // TODO: Do the aggregate search here
    // options: -a -b -c and name with -acb
    // match it with -a -b -c
  } while (0);

  return res;
}

/*
 * small handy argc and argv iterator
 */
static int __argc;
static const char **__argv;
void cvinit(int argc, const char **argv) {
  __argc = argc;
  __argv = argv;
}
const char* cvget(void) {
  if (__argc > 0) {
    __argc --;
    return *(__argv++);
  } else return NULL;
}

// TODO: complete its logic
// TODO: Unfinished
/*
  do full match first logic:
    if i got two short option -A and -AAA.
    and input value is '-AAA', match it with -AAA instead of
    the -A with value 'AA'. because -A option with value AA
    can be achieved by "-A AA". You can't do that for -AAA.
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
ArgcArgv* cmdspec_match
( int argc, const char ** argv
, Spec * specs
, void* store
) {
  bool valbool = false; // indicator of "--"
  enum genret opt = genre_null;
  Unit *head = NULL, *ptr = NULL;
  Invoke *inv = Invokealloc(16);
  InvokeStoreSet(inv, store);

  size_t count = 8;
  size_t ix = 0;
  SpecDesc ** desc = calloc(count, sizeof(void*));
  for (Spec *ptr = specs; ptr->optname != NULL; ptr++) {
    desc[ix] = buildSpecDesc(ptr);

    ix ++;
    if (ix >= count) {
      count += count;
      desc = reallocarray(desc, count, sizeof(void*));
    }
  }

  cvinit(argc, argv); // init iterator

  char * warn = setcmd('m', 3, 38, 5, 226);
  char * error = setcmd('m', 3, 38, 5, 160);
  char * reset = setcmd('m', 1, 0);

  // start processing char** and build Unit from these strings
  for (const char *para = cvget(); para != NULL; para = cvget()) {
    if (head == NULL) {
      ptr = head = buildUnit(para, NULL, &opt, &valbool);
    } else {
      // we will always let new value be in a Null name or
      // "--" name, all required value for options should
      // be processed in below two blocks.
      if (ptr->name == NULL || valbool)
        ptr = buildUnit(para, ptr, &opt, &valbool);
      else
        ptr = ptr->next = buildUnit(para, NULL, &opt, &valbool);
    }

    // nothing to parse if para is not option
    // and "--" seprate is handled by valbool variable.
    if (opt & genre_value || valbool) {
      assert(ptr->name == NULL || !strcmp(ptr->name, "--"));
      continue;
    }

    /* now we take care of long or short options */
    // we have "-" or "--" inside the name, so
    // let's begin with searching specs registry.
    enum genret genreinfo = genre_null;
    SpecSearchRes search = cmdspec_search(ptr->name, &genreinfo, desc);
    if (search.specdesc == NULL) {
      printf("%sWARN!!%s Unrecognized Option %s, ignore...\n"
            , warn, reset, ptr->name);
      ptr = Unitalloc(ptr, NULL, 0);
      continue;
    }

    // handle long option first
    if (opt & genre_long) {
      if (genreinfo & genre_toggle) {
        Invokeadd(inv, search.specdesc->spec, search.aliadesc->alia, NULL);
        continue;
      }
      if (opt & genre_withequ) {
        Invokeadd(inv, search.specdesc->spec, search.aliadesc->alia, ptr);
        continue;
      }

      para = cvget();
      if (para == NULL) {
        printf(
            "%sError!!%s "
            "Require one more Arguments, but none.\n"
            , error, reset);
        exit(1);
      }
      Unit *tmp = buildUnit(para, ptr, &opt, &valbool);
      if (tmp != ptr) {
        // the new value is an option and not a value.
        printf(
            "%sError!!%s "
            "expected one value for %s, but got option %s\n"
            , error, reset, ptr->name, tmp->name);
        exit(1);
      }
      Invokeadd(inv, search.specdesc->spec, search.aliadesc->alia, ptr);
      continue;
    }

    // handle short option here
    if (opt & genre_short) {
      if (genreinfo & genre_toggle) {
        Invokeadd(inv, search.specdesc->spec, search.aliadesc->alia, NULL);
        continue;
      }
      if (strlen(ptr->name) > strlen(search.aliatoken->name)) {
        size_t len = strlen(search.aliatoken->name);
        UnitCellallocSM(ptr, NULL, ptr->name + len);
        ptr->name[len] = 0;
      }
      if (UnitCellcx(ptr) > 0) {
        Invokeadd(inv, search.specdesc->spec, search.aliadesc->alia, ptr);
        continue;
      }
      para = cvget();
      if (para == NULL) {
        printf(
            "%sError!!%s "
            "Require one more Arguments, but none.\n"
            , error, reset);
        exit(1);
      }
      Unit *tmp = buildUnit(para, ptr, &opt, &valbool);
      if (tmp != ptr) {
        printf("%sError!!%s "
            "Expected one value for %s, but got option %s\n"
            , error, reset, ptr->name, tmp->name);
        exit(1);
      }
      Invokeadd(inv, search.specdesc->spec, search.aliadesc->alia, ptr);
    }
  }

  free(error);
  free(reset);
  free(warn);

  ArgcArgv *extra = buildArgcArgv(head);
  InvokeCVSet(inv, extra);

  Invokeall(inv);

  /* printf("Got %zu values\n", extra->argc); */
  /* for (size_t p = 0; p < extra->argc; p++) */
  /*   printf("Arg %zu %s\n", p, extra->argv[p]); */

  return extra;
}

bool match_cmd(const char * cmds, const char * cmd) {
  char * i = cmds;
  size_t cmdlen = strlen(cmd);
  do {
    if (!strncmp(cmds, cmd, cmdlen))
      return true;
  } while (i = strchr(cmds, ' ') + 1 && i != 1);
  return false;
}

void cmdshortinfo(Command *cmd)
{
  size_t count = 8;
  size_t ix = 0;
  static char * reset = NULL;
  static char * category = NULL;
  static char * option = NULL;
  static char * info = NULL;
  reset = reset?reset:setcmd('m', 1, 0);
  category = category?category:setcmd('m', 3, 38, 5, 98);
  option = option?option:setcmd('m', 3, 38, 5, 155);
  info = info?info:setcmd('m', 3, 38, 5, 202);

  if (cmd == NULL || cmd->command == NULL) return;
  SpecDesc ** desc = calloc(count, sizeof(void*));
  for (Spec *ptr = cmd->spec; ptr->optname != NULL; ptr++) {
    desc[ix] = buildSpecDesc(ptr);
    ix ++;
    if (ix >= count) {
      count += count;
      desc = reallocarray(desc, count, sizeof(void*));
    }
  }
  printf("command alia:: %s\n", cmd->command);
  for (size_t i = 0; i < ix; i++) {
    // cateogry output
    printf("  %s::%-10.10s%s\n"
          , category
          , desc[i]->spec->optname
          , reset);
    for (AliaDesc ** a = desc[i]->alias; *a != NULL; a++) {
      printf("    :: (%s) ::\n", (*a)->alia->alia);
      printf("       ");
      for (AliaToken ** tk = (*a)->tokens; *tk != NULL; tk++) {
        printf("{%.2s} %s%s%s%s"
              , genre_toggle&((*tk)->genre)?" T":"  "
              , option
              , (*tk)->name
              , reset
              , *(tk+1)==NULL?" .\n":", ");
      }
      printf("    %s%s%s\n"
            , info
            , (*a)->alia->desc
            , reset);
    }
  }
  free(desc);
}

// TODO: complete cmd_match
int cmd_match(int argc, const char** argv, struct command *cmds, void *store)
{
  if (argc == 0) return -1;
  for (struct command *cmd = cmds; cmd->command != NULL; cmd++) {
    if (match_cmd(cmd->command, argv[0])) {
      ArgcArgv * cv =
        cmdspec_match(argc-1, argv+1, cmd->spec, store);
      if (cmd->action)
        cmd->action(cv->argc, cv->argv, store);
      return 0;
    }
  }
  return -1;
}

/////////////////////////////////
// Arg structure operation //
/////////////////////////////////

static Unit* Unitfree(Unit *u)
{
  if (u == NULL) {
    return NULL;
  }
  Unit *t = u->next;
  for (Cell *c = u->cells; c != NULL;) {
    c = UnitCellfree(u, c);
  }
  free(u->name);
  free(u);
  return t;
}

static Unit* Unitalloc(Unit *u, const char *name, size_t size)
{
  Unit *t = malloc(sizeof(Unit));
  memset(t, 0, sizeof(Unit));
  if (u) {
    t->next = u->next;
    u->next = t;
  }
  if (name) {
    if (size == 0) size = strlen(name);
    t->name = malloc(size+1);
    memmove(t->name, name, size);
    t->name[size] = 0;
  }
  return t;
}

static Unit* Unitix(const Unit *u, size_t ix)
{
  if (u == NULL) return NULL;
  for (; u != NULL && ix > 0; ix--) {
    u = u->next;
  }
  return u;
}

static size_t Unitcx(const Unit *u)
{
  if (u == NULL) return 0;
  size_t size = 0;
  for (; u != NULL; size++)
    u = u->next;
  return size;
}

static Cell* UnitCellix(const Unit *u, size_t ix)
{
  if (u == NULL) return NULL;
  Cell *c = NULL;
  for (c = u->cells; c != NULL && ix > 0; ix--)
    c = c->next;
  return c;
}

static size_t UnitCellcx(const Unit *u)
{
  if (u == NULL) return 0;
  size_t size = 0;
  for (Cell *c = u->cells; c != NULL; size++)
    c = c->next;
  return size;
}

static Cell* UnitCellalloc(Unit *u, Cell *c, const void *buf, size_t size)
{
  Cell *t = malloc(sizeof(Cell));
  memset(t, 0, sizeof(Cell));
  if (size != 0) {
    t->val = malloc(size);
    if (buf) {
      memmove(t->val, buf, size);
    } else {
      memset(t->val, 0, size);
    }
  } else if (buf != NULL) {
    size_t strsize = strlen(buf);
    t->val = malloc(strsize+1);
    memmove(t->val, buf, strsize+1);
  }

  if (c && u) {
    t->next = c;
    Cell *cursor = u->cells;
    for (; cursor != NULL && cursor != c && cursor->next != c;) {
      cursor = cursor->next;
    }
    if (cursor == NULL) {
      assert(UnitCellfree(NULL, t) == NULL);
      return NULL;
    }
    if (cursor == c) {
      u->cells = t;
    } else {
      cursor->next = t;
    }
  } else if (u) {
    t->next = c;
    Cell *cursor = u->cells;
    for (; cursor != NULL && cursor->next != NULL;) {
      cursor = cursor->next;
    }
    if (cursor == NULL)
      u->cells = t;
    else
      cursor->next = t;
  } else if (c) {
    t->next = c->next;
    c->next = t;
  }
  return t;
}

static Cell* UnitCellfree(Unit *u, Cell *c)
{
  if (c == NULL) return NULL;
  Cell *t = NULL;
  if (u == NULL) {
    t = c->next;
  } else {
    t = u->cells;
    for (; t != NULL && t != c && t->next != c; t = t->next)
      ;
    if (t == NULL) return NULL;
    if (t == c) {
      u->cells = c->next;
      t = u->cells;
    } else {
      t->next = c->next;
      t = t->next;
    }
  }
  if (c->val) free(c->val);
  free(c);
  return t;
}

ArgcArgv* buildArgcArgv(Unit * link)
{
  ArgcArgv * cv = malloc(sizeof(ArgcArgv));

  size_t count = 8;
  cv->argc = 0;
  cv->argv = calloc(count, sizeof(char*));

  for (size_t unitix = 0, unitcx = Unitcx(link); unitix < unitcx; unitix++) {
    Unit * u = Unitix(link, unitix);
    if (u->name == NULL || !strcmp(u->name, "--"))
      for (size_t cellix = 0, cellcx = UnitCellcx(u); cellix < cellcx; cellix++) {
        Cell *c = UnitCellix(u, cellix);
        cv->argv[cv->argc] = c->val;
        cv->argc += 1;
        if (cv->argc >= count) {
          count += count;
          cv->argv = reallocarray(cv->argv, count, sizeof(char*));
        }
      }
  }
  cv->argv = reallocarray(cv->argv, cv->argc, sizeof(char*));
  return cv;
}

/////////////////////////////////////
// Invoke List structure operation //
/////////////////////////////////////

Invoke* Invokealloc(size_t capci)
{
  Invoke *inv = malloc(sizeof(Invoke));
  memset(inv, 0, sizeof(Invoke));
  if (capci == 0) capci = 1;
  inv->capacity = capci;
  inv->list = calloc(inv->capacity, sizeof(struct invokeunit));
  inv->finish = calloc(inv->capacity, sizeof(struct invokeunit));
  return inv;
}

void Invokefree(Invoke * inv)
{
  if (inv == NULL) return;
  free(inv->list);
  free(inv->finish);
  free(inv);
}

void InvokeStoreSet(Invoke * inv, void* s)
{
  if (inv == NULL) return ;
  inv->store = s;
}

void* InvokeStoreGet(Invoke * inv)
{
  if (inv == NULL) return NULL;
  return inv->store;
}

void InvokeCVSet(Invoke * inv, ArgcArgv *cv)
{
  if (inv == NULL) return ;
  inv->cv = cv;
}

ArgcArgv* InvokeCVGet(Invoke * inv)
{
  if (inv == NULL) return NULL;
  return inv->cv;
}

bool Invokeadd(Invoke * inv, const Spec* s, const Alia* a, Unit* u)
{
  if (inv == NULL) return false;
  inv->list[inv->invcx].ctx = s;
  inv->list[inv->invcx].entry = a;
  inv->list[inv->invcx].unit = u;
  inv->invcx ++;
  if (inv->invcx >= inv->capacity) {
    inv->capacity += inv->capacity;
    inv->list = reallocarray(inv->list, inv->capacity, sizeof(InvokeUnit));
    inv->finish = reallocarray(inv->finish, inv->capacity, sizeof(InvokeUnit));
  }
  return true;
}

bool Invokerm(Invoke * inv, size_t ix)
{
  if (inv == NULL) return false;
  if (ix >= inv->invcx) return false;
  memmove(inv->list+ix, inv->list+ix+1, (inv->invcx - ix - 1) * sizeof(struct invokeunit));
  inv->invcx --;
  return true;
}

bool Invokemv(Invoke * inv, size_t from, size_t to)
{
  if (inv == NULL) return false;
  if (from >= inv->invcx) return false;
  if (to == from) return true;

  InvokeUnit iunit = inv->list[from];
  if (to < from) {
    memmove(inv->list+to+1, inv->list+to, (from - to) * sizeof(InvokeUnit));
    inv->list[to] = iunit;
  } else {
    /* to > from */
    if (to >= inv->invcx) to = inv->invcx - 1;
    memmove(inv->list+from, inv->list+from+1, (to - from) * sizeof(InvokeUnit));
    inv->list[to] = iunit;
  }
  return true;
}

bool Invokeix(Invoke * inv, size_t ix)
{
  if (inv == NULL) return false;
  if (ix >= inv->invcx) return false;
  InvokeUnit *iunit = inv->list + ix;
  iunit->ctx->call(iunit->unit, inv->cv, inv->store);
  return true;
}

bool Invokeixrm(Invoke * inv, size_t ix)
{
  if (Invokeix(inv, ix)) {
    Invokerm(inv, ix);
    return true;
  }
  return false;
}

size_t Invokesearch(Invoke * inv, const char* name, size_t offset)
{
  if (inv == NULL || name == NULL || offset >= inv->invcx)
    return false;
  size_t namelen = strlen(name);

  for (size_t i = offset; i < inv->invcx; i++) {
    if (!strncmp(name, inv->list[i].ctx->optname, namelen)) {
      return i + 1;
    }
  }
  return 0;
}

// TODO: Add rely check
bool Invokeall(Invoke * inv)
{
  if (inv == NULL) return false;
  for (size_t i = 0; i < inv->invcx; i++) {
    InvokeUnit * iunit = inv->list + i;
    iunit->ctx->call(iunit->unit, inv->cv, inv->store);
  }
  return true;
}

