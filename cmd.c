#include "cmd.h"
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

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

// if find '=', return its position or return 0;
static inline size_t search_eql_opt(char* arg, size_t len) {
  for (size_t i = 0; i<len; i++) {
    if (arg[i] == '=') return i;
  }
  return 0;
}

// accept one command line string.
// pass @holder as NULL to generate a new option.
// pass @valbool as true to indicate the string is value. it will be set.
// pass @opt to accept cmd options.
// '-' will be treated as a value
// "--" will be treated a special key "--", and all the following
//     command line arguments will be treated as value and
//     appended to this argument value field.
// some value will be inserted to an argument with key as null.
static struct cmdargt* cmd_trans
( char * const str
, struct cmdargt *holder
, enum cmdopt *opt
, bool *valbool
) {
  struct cmdargt *ptr = holder;
  if (ptr == NULL) {
    // caller wants us to build brand new cmdarg.
    ptr = malloc(sizeof(struct cmdargt));
    memset(ptr, 0, sizeof(struct cmdargt));
  }
  // otherwise we append the value to its value field or
  // generate new argument and append it.

  *opt = cmd_opt_null;  // clear the option
  // if be the value option
  if (*valbool) {
    // if so, just append it as the value
    cmdargt_append_string(ptr, str);
    *opt |= cmd_opt_val;
    return ptr;
  }

  // check whether the first token is '-', distinguish it with value
  if (str[0] != '-') {
    cmdargt_append_string(ptr, str);
    *opt |= cmd_opt_val;
    return ptr;
  }

  size_t arglen = strlen(str);
  // if only one character '-', it's a special value
  // which denotes standard input
  if (arglen == 1) {
    cmdargt_append_string(ptr, str);
    *opt |= cmd_opt_val;
    return ptr;
  }

  // it is now an option, so take care of @ptr value.
  if (holder != NULL) {
    ptr->next = malloc(sizeof(struct cmdargt));
    ptr = ptr->next;
    memset(ptr, 0, sizeof(struct cmdargt));
  }

  // now the arglen >= 2
  if (str[1] == '-') {
    // if its length is only 2, then it is "--"
    // which should denote a special seperation
    // between value and option. return it as a special option;
    // with key == "--"
    if (arglen == 2) {
      *valbool = true;
      *opt |= cmd_opt_seperate;
      ptr->key = malloc(3);
      memmove(ptr->key, str, 3);
      return ptr;
    }

    // if not, check equal sign "=", it is long option now.
    size_t posi = search_eql_opt(str, arglen);
    *opt |= cmd_opt_long;

    if (posi) {
      // Got an index of '='
      *opt |= cmd_opt_withequ;

      assert(posi >= 2);
      // the key will never be empty string.
      if (posi == 2) {
        // contain key as "="
        ptr->key = malloc(posi);
        *(ptr->key+0) = '=';
        *(ptr->key+1) = 0;
      } else {
        // normal key
        // posi
        //  +1 ==> translate it to length
        //  -3 ==> remove two '-' and one '='
        //  +1 ==> add the '\0' space.
        //  final length is posi - 1;
        ptr->key = malloc(posi-1);
        // start at str+2, and length which doesn't include '\0'
        memmove(ptr->key, str+2, posi-2);
        *(ptr->key+posi-2) = 0; // set the last one to zero.
      }
      // something like "--=value" will get "=" as key and "value" as value
      // if got "--=", then key as "=" and value NULL.
      if (posi < arglen-1) cmdargt_append_string(ptr, str+posi+1);
    } else {
      // doesn't find '=', so it is an option
      ptr->key = malloc(arglen-1);
      memmove(ptr->key, str+2, arglen-1);
      *(ptr->key+arglen-2) = 0;
    }
    return ptr;
  }

  // now it could only be short option.
  // it doesn't process the argument for the short
  // option because it known nothing about available
  // option requirement.
  // Further Processing is required.
  *opt |= cmd_opt_short;
  ptr->key = malloc(arglen);
  memmove(ptr->key, str+1, arglen);
  *(ptr->key+arglen-1) = 0;
  return ptr;
}

// macth the longest option
struct cmdregt* cmd_search(char const * key, enum cmdopt opt, struct cmdregt* regs)
{
  struct cmdregt *toret = NULL;
  size_t srclen = strlen(key);
  for ( int i = 0; (regs+i)->call != NULL
      && (regs+i)->code != 0; i++)
  {
    char const * dststr = (regs+i)->key;
    size_t dstlen = strlen(dststr);
    if (dstlen > srclen) continue;
    if (!strncmp(dststr, key, dstlen)) {
      // filter the long and short option
      // so -A and -AAA will not compared to --AAA or --A
      if (!(opt & cmdopt_allopt & (regs+i)->opt)) continue;
      if (toret) {
        size_t rlen = strlen(toret->key);
        // here the < or <= matters.
        // if choose '<', then latter keys will not override
        //    previous one
        // otherwise choose '<=' and it will.
#ifdef OPTIONOVERRIDE
        if (rlen <= dstlen)
#else
        if (rlen < dstlen)
#endif
        {
          toret = regs+i;
        }
      } else toret = regs+i;
    }
  }
  return toret;
}

// TODO: complete its logic
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
// core logic 2.
struct cmdarglst* cmd_match
( int argc, char** argv
, struct cmdregt* regs
, void* store
) {
  if (argc <= 0) return NULL;
  bool valbool = false; // indicator of "--"
  enum cmdopt opt = cmd_opt_null;
  struct cmdargt *head = NULL, *ptr = NULL;

  // prepare the header
  struct cmdlst *list = NULL, *listhdr = NULL;
  // start processing char** and transfer it to cmdargt one by one
  for (int idx = 0; idx < argc; idx++) {
    // transform the arguments and assign the address to @ptr
#define iter(v) cmd_trans(argv[idx], v, &opt, &valbool)
#define idx_guard(v) if (idx + 1 >= argc) {\
  printf("required one value for \"%s\", but no more arguments.\n", (v)->key);\
  exit(-1);\
  }
    struct cmdargt *tmp = NULL;
    if (ptr == NULL) {
      tmp = iter(NULL);
    } else if (ptr->key == NULL || !strcmp(ptr->key, "--")) {
      tmp = iter(ptr);
    } else {
      tmp = iter(NULL);
      ptr->next = tmp;
      ptr = tmp;
    }
    // allocate header if idx is 0;
    if (idx == 0) head = ptr = tmp;
    // if valbool is true, then ptr->key should be "--"
    if (valbool || opt & cmd_opt_val) {
      if (valbool)
        assert(!strncmp("--", ptr->key, 2) && tmp == ptr);
      continue;
    }
    
    /* now it will be long or short option */
    // search registry first
    ptr = tmp;  // pointer should always point to the last one
    struct cmdregt *founded = cmd_search(ptr->key, opt, regs);
    if (founded == NULL) {
      printf("Error, Unrecognized Option %s\n", ptr->key);
      exit(1);
    }

    if (list == NULL) {
      listhdr = list = cmdlst_alloc();
    } else {
      listhdr->next = cmdlst_alloc();
      listhdr = listhdr->next;
    }

    listhdr->reg = founded;
    listhdr->arg = ptr;
    listhdr->next = NULL;

    // handle long option first
    if (opt & cmd_opt_long) {
      if (opt & cmd_opt_withequ) continue;
      if (founded->opt & cmd_opt_toggle) continue;
      idx_guard(founded);  // make sure idx not exceed argc
      idx++;
      tmp = iter(ptr);
      if (tmp != ptr) {
        // the new value is an option and not a value.
        printf("expected one value for %s, but got option %s\n", ptr->key, tmp->key);
        exit(1);
      }
      continue;
    }
    // handle short option here
    size_t valindx = strlen(founded->key);
    if (strlen(ptr->key) > valindx) {
      char *key = malloc(valindx+1);
      memmove(key, ptr->key, valindx+1);
      key[valindx] = 0;
      cmdargt_append_string(ptr, ptr->key + valindx);
      free(ptr->key);
      ptr->key = key;
    }
    // if toggle, don't need any more operation
    if (founded->opt & cmd_opt_toggle) continue;
    if (cmdargt_valength(ptr) > 0) continue;
    idx_guard(founded);  // make sure idx not exceed argc
    idx++;
    tmp = iter(ptr);
    if (tmp != ptr) {
      printf("expected one value for %s, but got option %s\n", ptr->key, tmp->key);
      exit(1);
    }
  }
#undef iter
#undef idx_guard

  /* printf("Total Got %zu commands\n", cmdlst_length(list)); */
  struct cmdarglst *lst = cmdargt_collect(head, argc);
  cmdlst_chk(&list, regs);
  cmdlst_call(list, lst, store);
  return lst;
}

/////////////////////////////////
// cmdargt structure operation //
/////////////////////////////////

// free all things.
void cmdargt_free(struct cmdargt *arg)
{
  for (struct cmdargt *ptr = arg; ptr != NULL;) {
    arg = arg->next;
    for (struct valink *lnk = ptr->valist; lnk != NULL;) {
      struct valink *next = lnk->next;
      free(lnk->val);
      free(lnk);
      lnk = next;
    }
    free(ptr);  // ptr is also an mllocated value
    ptr = arg;
  }
}

size_t cmdargt_valength(struct cmdargt const * arg)
{
  if (arg->valist == NULL) return 0;
  size_t length = 0;
  for (struct valink *ptr = arg->valist; ptr != NULL; ptr = ptr->next) 
    length ++;
  return length;
}

void cmdargt_append_buffer(struct cmdargt *arg, void const * buffer, size_t size)
{
  struct valink **ptr = &(arg->valist);

  // find the last pointer;
  while (*ptr != NULL) ptr = &((*ptr)->next);

  // allocate the structure memory
  *ptr = malloc(sizeof(struct valink));
  memset(*ptr, 0, sizeof(struct valink));

  // allocate buffer memory, copy it;
  (*ptr)->val = malloc(size);
  memset((*ptr)->val, 0, size);
  memmove((*ptr)->val, buffer, size);
  (*ptr)->size = size;
  (*ptr)->next = NULL;
}

void cmdargt_append_string(struct cmdargt *arg, char* str) {
  size_t len = strlen(str); // size without \0
  cmdargt_append_buffer(arg, str, len + 1); // here add the \0
}

// idx is both the index and the returned value size.
void* cmdargt_fetch_val(struct cmdargt const * arg, size_t *idx)
{
  struct valink *ptr = arg->valist;
  if (*idx >= cmdargt_valength(arg)) {
    *idx = 0;
    return NULL;
  }
  for (; *idx > 0; *idx -= 1) ptr = ptr->next;
  *idx = ptr->size;
  return ptr->val;
}

// get the indexed argument. simple as inline.
inline struct cmdargt* cmdargt_fetch_arg(struct cmdargt const * arg, size_t idx) {
  for (; arg != NULL && idx > 0; arg = arg->next, idx-=1)
    ;
  return (struct cmdargt*)arg;
}

// allocate memory and collect all unused argument value into
// a single structure.
struct cmdarglst* cmdargt_collect(struct cmdargt *arg, size_t cap)
{
  struct cmdarglst *toret = malloc(sizeof(struct cmdarglst));
  size_t num = 0;
  struct cmdargt *ptr = arg;
  toret->argv = malloc(cap+1);
  memset(toret->argv, 0, cap+1);
  for (; ptr != NULL; ptr = ptr->next) {
    if (ptr -> key == NULL || !strcmp(ptr->key, "--")) {
      size_t length = cmdargt_valength(ptr);
#ifdef CMDUBUG
      printf("Got value Length %zd\n", length);
#endif
      size_t i = 0;
      for (; i < length; i++) {
        size_t idx = i;
        toret->argv[num+i] = cmdargt_fetch_val(ptr, &idx);
#ifdef CMDUBUG
        printf("The value is \"%s\", length %zd.\n", toret->argv[num+i], idx);
#endif
      }
      num += i;
    }
  }
  toret->argc = num;
  return toret;
}

/////////////////////////////////
// callist structure operation //
/////////////////////////////////

struct cmdlst * cmdlst_alloc() {
  struct cmdlst *lst = malloc(sizeof(struct cmdlst));
  memset(lst, 0, sizeof(struct cmdlst));
  return lst;
}
void cmdlst_free(struct cmdlst *lst) {
  for (struct cmdlst *ptr = lst; ptr != NULL; ptr = lst) {
    lst = lst->next;
    free(ptr);
  }
}
size_t cmdlst_length(struct cmdlst const * lst) {
  size_t len = 0;
  for (; lst != NULL; lst = lst->next) len++;
  return len;
}
struct cmdlst * cmdlst_get(struct cmdlst *hdr, size_t idx)
{
  size_t len = cmdlst_length(hdr);
  if (idx >= len) return NULL;
  for (; idx > 0; idx--) {
    hdr = hdr->next;
  }
  return hdr;
}
size_t cmdlst_insert(struct cmdlst **hdr, size_t idx, struct cmdlst *ptr)
{
  size_t len = cmdlst_length(*hdr);
  struct cmdlst *anchor = NULL;
  if (idx >= len) {
    anchor = cmdlst_get(*hdr, len-1);
    anchor->next = ptr;
    return len;
  } else if (idx == 0) {
    ptr->next = *hdr;
    *hdr = ptr;
    return 0;
  } else {
    anchor = cmdlst_get(*hdr, idx-1);
    ptr->next = anchor->next;
    anchor->next = ptr;
    return idx;
  }
}

struct cmdlst * cmdlst_remove(struct cmdlst **hdr, size_t idx)
{
  size_t len = cmdlst_length(*hdr);
  if (idx >= len) return NULL;
  struct cmdlst *ptr = *hdr;
  if (idx == 0) {
    *hdr = ptr->next;
    ptr->next = NULL;
    return ptr;
  }
  ptr = cmdlst_get(*hdr, idx-1);
  struct cmdlst *lptr = ptr->next;
  ptr->next = lptr->next;
  lptr->next = NULL;
  return lptr;
}

int cmdlst_move(struct cmdlst **hdr, size_t idx1, size_t idx2)
{
  struct cmdlst* ptr = cmdlst_remove(hdr, idx1);
  if (ptr == NULL) return -1;
  return cmdlst_insert(hdr, idx2, ptr);
}

struct cmdlst * cmdlst_search(struct cmdlst *hdr, size_t code, size_t *index)
{
  size_t i = 0;
  for (; hdr!= NULL; hdr = hdr->next) {
    if (hdr->reg->code == code) {
      if (index) *index = i;
      return hdr;
    }
    i++;
  }
  if (index) *index = 0;
  return NULL;
}

size_t cmdlst_reverse_get(struct cmdlst *hdr, struct cmdlst *ptr)
{
  for (size_t i = 0; hdr!=NULL; hdr = hdr->next, i++) {
    if (hdr == ptr) return i;
  }
  return 0;
}

/* static bool cmdlst_dep_chk(struct cmdlst *hdr, struct cmdlst *ptr) */

// TODO
bool cmdlst_chk(struct cmdlst **hdr, struct cmdregt* regs)
{
  struct cmdlst *ptr = *hdr;
  size_t listlength = cmdlst_length(*hdr);
  size_t array[listlength];
  for (int i=0; i<listlength; i++, ptr=ptr->next)
    array[i] = ptr->reg->code;

  // check option field
  for (int i = 0; i < listlength; i++) {
    size_t ptridx = 0;
    ptr = cmdlst_search(*hdr, array[i], &ptridx);
    struct cmdept* dpt = &ptr->reg->dep;
    for (int j = 0; dpt->opt[j] != 0; j++) {
      size_t fwdidx = 0;
      struct cmdlst *fwd = NULL;
optional:
      fwd = cmdlst_search(ptr, dpt->opt[j], &fwdidx);
      fwdidx += cmdlst_reverse_get(*hdr, ptr);
      if (fwd == NULL) continue;
      cmdlst_move(hdr, fwdidx, ptridx>0?ptridx-1:0);
      goto optional;
    }
  }
  // check requird field
  for (int i = 0; i < listlength; i++) {
    size_t ptridx = 0;
    ptr = cmdlst_search(*hdr, array[i], &ptridx);
    struct cmdept* dpt = &ptr->reg->dep;
    for (int j = 0; dpt->req[j] != 0; j++) {
      size_t fwdidx = 0;
      struct cmdlst *fwd = NULL;
required:
      fwd = cmdlst_search(ptr, dpt->req[j], &fwdidx);
      fwdidx += cmdlst_reverse_get(*hdr, ptr);
      if (fwd == NULL) {
        if (cmdlst_search(*hdr, dpt->req[j], NULL)) continue;
        int i = 0;
        for (; (regs+i)->code != 0 ; i++) {
          if ((regs+i)->code != dpt->req[j])
            break;
        }
        printf("Error: Lack option %s for %s\n", (regs+i)->key, ptr->reg->key);
        exit(-1);
      }
      cmdlst_move(hdr, fwdidx, ptridx>0?ptridx-1:0);
      goto required;
    }
  }
  return true;
}

void cmdlst_call(struct cmdlst *hdr, struct cmdarglst *lst, void* store)
{
  for (; hdr != NULL; hdr = hdr->next) {
    hdr->reg->call(hdr->arg, lst, store);
  }
}

