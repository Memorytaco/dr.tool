#include "vara.h"

#include <ctype.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

//////////////////////////////////////////////////////
//                 high 2bits for aggregate type,
//                 .  b00 for single type,
//                 .  b01 for subarray type, follow array type
//                 .  b10 for aggregate type, follow by AGGR
//                 .  b11 preserved, mean nothing for now
//                 .
//               b00 00 low 2bits for octet type,
//               .    b00 octet,
//               .    b01 word,
//               .    b10 dword,
//               .    b11 qword.
//               .
#define ARRY  0x1000
//                ..
//                ..-> 0x00 for array length, max 255.
//////////////////////////////////////////////////////

//////////////////////////////////////////////////////
//                ..-> 0x00 for aggr id, used in AEND.
#define AGGR  0x2000
//               .
//               .
//               no use for now
//////////////////////////////////////////////////////

//////////////////////////////////////////////////////
//                ..-> 0x00 aggr id, to enclose aggr definition.
//                ..   match with AGGR id. if AGGR id dismatch
//                ..   with AEND id, and AEND doesn't close one
//                ..   AGGR, it indicates an error.
#define AEND  0x4000
//               .
//               .
//               no use for now
//////////////////////////////////////////////////////

#define TEND  0x0000  // end marker for type definition
#define OCTT  0x0000  // octet
#define TVAL  0x5000  // value prefix

/*
 * h 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 l
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  +       |       |       |       +
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

typedef unsigned short VType;

// trying using bit field here
#if 0
typedef struct VType {
  unsigned char :4;
  unsigned char :4;
  union {
    struct {
    };
    struct {
    };
  };
} VType;
#endif

// return true(1) if it holds related type.
#define isarry(v) (ARRY&(v))
#define isaggr(v) (AGGR&(v))
#define isaend(v) (AEND&(v))
#define issing(v) (!isarry(v) && !isaggr(v) && !isaend(v))
#define issval(v) (0x5==(v>>12)) // if it is a suffix count

// octet type
#define getoctet(v) (1<<(((v)&0x0300)>>8))  // 1,2,4,8
#define setoctet(v, e) ((((e)&0x03)<<8)|(v)) // e is 0,1,2,3

// array type
#define Asingle 0x00
#define Aarray  0x01
#define Aaggre  0x02
#define Aresrv  0x03
#define getarryt(v) ((((v)>>8)&0x0c)>>2) // array inner ele type
#define setarryt(v, t) (((t)<<10)|(v))

// array len manipulate
#define getarrylen(v) (0x00ff&(v))    // array len
#define setarrylen(v, l) (0xff00&(v)|(l))

// aggregrate id, from 0 ~ 255, unsigned int8
#define getaggrid(v) (0x00ff&(v))
#define setaggrid(v, i) (((i)&0x00ff)|(v))

#define setval(v, n) (0xff00&(v)|(n)) // set suffix count
#define getval(v) (0x00ff&(v))

// The structure
struct vara {
  VType * dbit;
  char * name;
  void * data;
};

// VType stack manipulation utility.
static void push(VType v, VType* m, int *i, int l);
static VType pop(VType* m, int *i);
static VType peek(VType* m, int i);


// unit manipulation utility, core logic here.
static size_t getunitsize(VType const *bit, size_t capi, VType const **posi);
static size_t getelesize(VType const *bit, size_t capi, VType const **posi, size_t *count);

// alloc prebuilt bits info.
static VType* buildbits(char const *d, size_t *len);


const char* vara_name(vtra store)
{
  return store->name;
}

void  vara_info(vtra store)
{
  printf("%s: ", store->name);
  for (int i = 0; *(store->dbit+i) != TEND; i++) {
    if (i%5 == 0) printf("\n");
    printf("%04x ", *(store->dbit+i));
  }
  printf("\n");
}

vtra vara_alloc(const char* dsc)
{
  vtra store = malloc(sizeof(struct vara));
  size_t clen = strlen(dsc);
  size_t size = 0;
  char const *str = dsc;
  char *name = NULL;
  while (*str != ':' && str - dsc < clen) str++;
  if (str - dsc >= clen) {
    printf("no ':' name marker.\n");
    return NULL;
  } else {
    name = malloc(str-dsc+1);
    memmove(name, dsc, str-dsc+1);
    name[str-dsc] = '\0';
    store->name = name;
  }
  str++; // move str from ':' to the type description.
  store->dbit = buildbits(str, &size);
  if (store->dbit == NULL) {
    printf("Error when building dbit.\n");
    return NULL;
  }
  store->data = malloc(size);
  return store;
}

static bool __checksyntax(char const *d) {
  bool flag = true;
  size_t len = strlen(d);
  char stack[len/2];
  size_t si = 0;
  for (size_t i = 0; i<len; i++) {
    if (d[i] == '[') {
      stack[si++] = '[';
      if (i == 0) {
        printf("@%zu couldn't be '['.\n'", i);
        flag = false;
      } else if (isdigit(d[i-1])) {
        printf("@%zu couldn't be digit before '['.\n", i);
        flag = false;
      }
    } else if (d[i] == ']') {
      if (si == 0 || stack[--si] != '[') {
        printf("mismatched '()' @%zu got '%c'.\n", i, d[i]);
        return false;
      }
      if (i == 0) {
        printf("invalid syntax @%zu.\n", i);
        flag = false;
      } else if (!isdigit(d[i-1])) {
        printf("at least one digit between '[' and ']' @%zu.\n", i);
        flag = false;
      }
    } else if (d[i] == '(') {
      stack[si++] = '(';
    } else if (d[i] == ')') {
      if (si == 0 || stack[--si] != '(') {
        printf("mismatched '[]' @%zu got '%c'.\n", i, d[i]);
        return false;
      }
    }
  }
  return flag;
}

// return the actual allocated memory size via *len pointer.
static VType* buildbits(char const *d, size_t *len)
{
  size_t clen = strlen(d);
  if (!__checksyntax(d)) {
    return NULL; // syntax error, return NULL.
  }
  if (clen == 0) return NULL;
  size_t mut = 1;   // factor for controling bits mem size
  VType *bits = malloc(clen*mut); // actual bits memory
  // bits index @bi@, bi should always point to an
  // element, and should satisfy condition that "bi < cursor".
  // and @cursor@ should always point an empty place.
  size_t bi = 0;
#define checkbits(i, mem) if (i>=clen*mut || i == clen*mut-1) {\
  mut++;\
  mem = realloc(mem, clen*mut*2);\
}

  size_t SLim = 64; // stack limit
  VType stack[SLim];  // environment stack
  // size stack syncing with environment
  // [0] is the size, [1] is the start location.
  size_t nstack[SLim][2];
  int si = 0;  // stack index
  size_t size = 0;  // data memory size
  // indicate what element has been prcessed previously
  // if encountor ')', fill in aggr
  // if encountor ']', fill in arry
  // it's illegal putting number before "[]".
  // e.g. o3[32] is illegal
  //      (od3w2w[3])20[3] is illegal
  // this syntax will be uptaded and be legal in future but
  // not now. write something like this:
  // e.g. o2o[32] : two octets, one 32 octets array
  //      (od3w2w[3])19(od3w2w[3])[3]
  VType ele = 0;
  unsigned char id = 0;   // aggr id accumulator

  unsigned char num = 0;  // for the number building
  char *di = d; // @d string index pointer
  while (di < d + clen) {
    unsigned char octsize = 0;
    switch(*di) {
      case 'o':
      case 'O': octsize = 1; goto SINGLE;
      case 'w':
      case 'W': octsize = 2; goto SINGLE;
      case 'd':
      case 'D': octsize = 4; goto SINGLE;
      case 'q':
      case 'Q': octsize = 8; goto SINGLE;
      case '@': octsize = 8; goto SINGLE;
    }
    goto AGGRBEG;

SINGLE:
    di++;
    if (isdigit(*di)) {
      num = strtol(di, &di, 10);
      switch (octsize) {
        case 1: ele = setoctet(OCTT,0); break;
        case 2: ele = setoctet(OCTT,1); break;
        case 4: ele = setoctet(OCTT,2); break;
        case 8: ele = setoctet(OCTT,3); break;
      }
      ele = setval(ele, num);
      *(bits+(bi++)) = ele;
      checkbits(bi, bits);
      if (si == 0) {
        size += octsize * num;  // add size to returned size
      } else {
        nstack[si-1][0] += octsize * num;
      }
    } else if (*di == '[') {
      // TODO: support mutiple array nesting
      di++;
      num = strtol(di, &di, 10);
      if (*di == ']') di++;
      ele = setoctet(setarryt(ARRY, Asingle), 0);
      ele = setarrylen(ele, num);
      *(bits+(bi++)) = ele;
      checkbits(bi, bits);
      if (isdigit(*di)) {
        num *= strtol(di, &di, 10);
        ele = setval(TVAL, num);
        *(bits+(bi++)) = ele;
        checkbits(bi, bits);
      }
      if (si == 0) {
        size += octsize * num;
      } else {
        nstack[si-1][0] += octsize * num;
      }
    }
    continue;

AGGRBEG:
    if (*di == '[' || *di == ']') {
      printf("Error, got '[]'. line %d.\n", __LINE__);
      return NULL;
    }
    if (*di == '(') {
      ele = setaggrid(AGGR, ++id);
      *(bits+(bi++)) = ele;
      checkbits(bi, bits);
      push(ele, stack, &si, 64);
      nstack[si-1][0] = 0;
      nstack[si-1][1] = bi-1;
      di++;
    } else if (*(di++) == ')') {
      if (si == 0) {
        printf("Unmatched ')' line %d.\n", __LINE__);
        return NULL;
      }
      ele = setaggrid(AEND, getaggrid(peek(stack, si)));
      *(bits+(bi++)) = ele;
      checkbits(bi, bits);
      if (isdigit(*di)) {
        num = strtol(di, &di, 10);
        *(bits+(bi++)) = setval(TVAL, num);
        checkbits(bi, bits);
      }
      pop(stack, &si);  // finish one aggregate type
      nstack[si][0] *= num;
      if (si == 0) {
        size += nstack[si][0];
      } else {
        nstack[si-1][0] += num;
      }
      // TODO: add array support for aggregate type
    }
  }
  *(bits+(bi++)) = TEND;
  if (len) *len = size;
  return bits;
}

// bit should point to one element. the header or single type.
// header should be aggr or arry.
// if point to array, get array whole size with suffix val.
// if point to aggre, get the aggregate type size with suffix val.
// if point to single, get the octet type size with suffix val.
// it count val suffix, you should calculate the element
//    size yourself. @*posi will always
//    point to the untouched or the next new unit.
// @capi is the max depth size capacity.
static size_t getunitsize
( VType const *bit
, size_t capi
, VType const **posi
) {
  // environment stack, store array and aggregate type.
  // used to move element cursor.
  VType Tstack[capi];
  // sync with stack, size of the type
  VType Tscount[capi];
  size_t total = 0, size = 0;
  int si = 0; // stack indice
  do {
    if (*bit == TEND) {
      return 0;
    }
    if (isarry(*bit)) {
      size = getarrylen(*bit);  // get its len first
      switch (getarryt(*bit)) {
        case Asingle:
          size *= getoctet(*bit); // get array size
          if (issval(*(++bit))) {
            // check suffix val
            size *= getval(*bit)?getval(*bit):1;
            bit++;  // point to next unite
          }
          while (isarry(peek(Tstack, si))) {
            // after the following statement, si now
            // point to previous array length.
            VType arr = pop(Tstack, &si);
            size *= Tscount[si];  // get whole size;
            Tscount[si] = 0;  // reset its val to 0
          }
          // aggre ctx env, just add the size.
          if (isaggr(peek(Tstack, si))) {
            if (issval(*bit)) size *= getval(*(bit++));
            Tscount[si-1] += size;
          } else {
            total += size;
          }
          break;
        case Aarray:
        case Aaggre:
          push(*bit, Tstack, &si, capi);
          if (issval(*(++bit))) {
            size *= getval(*bit)?getval(*bit):1;
            bit++;
          }
          Tscount[si-1] = size; // the size here is arry len
          break;
        default:
          break;
      }
    } else if (isaend(*bit)) {
      if (!isaggr(peek(Tstack, si)) ||
          getaggrid(peek(Tstack,si)) != getaggrid(*bit)) {
        printf("Mismatch type bit, expect aggr with id %d\n", getaggrid(*bit));
        return 0;
      }
      VType aggr = pop(Tstack, &si);
      size = Tscount[si];
      while (isarry(peek(Tstack, si))) {
        pop(Tstack, &si);
        size *= Tscount[si];
        Tscount[si] = 0;
      }
      // if got suffix val.
      if (issval(*(++bit))) size *= getval(*(bit++));
      if (isaggr(peek(Tstack, si))) {
        // it is an element of the aggregate type.
        // so add to its size.
        Tscount[si-1] += size;
      } else {
        total += size; // stack is balanced, add to total.
      }
    } else if (isaggr(*bit)) {
      push(*bit, Tstack, &si, capi);
      bit++;  // next unit
    } else if (issing(*bit)) {
      size = getval(*bit);
      size = size?size*getoctet(*bit):getoctet(*bit);
      bit++;  // next unit
      if (isarry(peek(Tstack, si))) {
        printf("Expected aggregrate type, but get array.\n");
        return 0;
      } else if (isaggr(peek(Tstack, si))) {
        Tscount[si-1] += size;
      }
      total += size;
    }
  } while (si > 0);
  if (posi) *posi = bit;
  return total;
}

// return the element size and maybe the number.
static size_t getelesize
( VType const *bit
, size_t capi
, VType const **posi
, size_t *count
) {
  VType const *next = bit;
  size_t size = 0;
  size_t num = 0;
  if (issing(*bit)) {
    num = getval(*bit);
    num = num?num:1;
    size = getoctet(*bit);
    next = bit+1;
  } else if (isarry(*bit)) {
    if (issval(*(bit+1))) {
      num = getval(*(bit+1));
      num = num?num:1;  // make sure num is large than 0
    }
    size = num ? getunitsize(bit, capi, NULL)/num
               : getunitsize(bit, capi, NULL);
    next = num?bit+2:bit+1; // if got val, point to next two unit.
    num = num?num:1;
  } else if (isaggr(*bit)) {
    size = getunitsize(bit, capi, &next);
    if (size) {
      if (issval(*(next-1))) {
        num = getval(*(next-1));
        num = num?num:1;
      }
      size /= num;
      next = bit + 1;
    }
  }
  // if size is valid, then posi and count should be valid
  if (size) {
    if (posi) *posi = next;
    if (count) *count = num;
  }
  return size;
}

// if any error, return NULL. so a program segmentfault will
// alert programmer. otherwise check the return value.
// if returned value is not NULL, it is a valid address otherwise
// is a LIBRARY ERROR and please inform me.
void* vara_ptr(vtra store, int depth, size_t* retsize, ...)
{
  // data address
  void *addr = store->data;
  // dbit pointer
  VType const *bit = store->dbit;
  va_list ap;
  va_start(ap, retsize);

  // current context, are we in an array or in
  // an aggregrate structure? index will have different
  // meaning here.
  // [0] is env, and [1] is the type bits
  VType env[2] = {Aresrv, TEND}; // Aresrv indicate toplevel
  /*
   * Asingle current selected unit is owdq
   * Aarray  array environment, may query its element size
   * Aaggre  aggregate environment, like toplevel
   */

  size_t size = 0;
  size_t ret = 0;

  // bit will always point to next unit type.
  // if you are processing bit+i, after
  // processing, bit will be bit+i+1 and always be.
  for (int d=0; d<depth; d++) {
    // invalid field, let the program crash. The only way
    // to handle when no error handler is available.
    size_t ix = va_arg(ap, size_t);
    if (*env == Aresrv) {
#define guard(cond) if (cond) { va_end(ap); return NULL; }
      for (size_t i = 0; i < ix;) {
        VType const *posi = bit;
        size_t count = 0;
        size = getelesize(bit, 64, &posi, &count);
        guard(size == 0);
        ret = size; // achieve the return type size
        if (count <= ix-i) {
          i += count;
          addr += getunitsize(bit, 64, &bit);
        } else {
          // bit stop at here. point to the structure.
          // o3 or o[23]5 the array.
          // or (od2)20 the aggr.
          addr += size * (ix-i);
          i = ix;
        }
      }
      // above for loop just put bit pointer to the right
      // position and if it exit successfully, id doesn't
      // check bit pointer status. so here we do the check.
      guard(*bit == TEND);
      // and then read the env, it won't be TEND now.
      env[0] = isarry(*bit)?Aarray:
               isaggr(*bit)?Aaggre:Asingle;
      // bit should point to the unit to be processed
      // and it is the environment.
      // it may point arry or aggr or normal octet.
      env[1] = *bit;
    } else if (*env == Aaggre) {
      unsigned short id = getaggrid(env[1]);
      getelesize(bit, 64, &bit, NULL); // move cursor to inner obj
      for (size_t i = 0; i<ix;) {
        // ix  exceeds aggregate type elements count
        guard(isaend(*bit) && getaggrid(*bit) == id);
        VType const *posi = bit;
        size_t count = 0;
        size = getelesize(bit, 64, &posi, &count);
        guard(size == 0);
        ret = size; // achieve the size
        if (count <= ix-i) {
          i += count;
          addr += getunitsize(bit, 64, &bit);
        } else {
          addr += size * (ix-i);
          i = ix;
        }
      }
      guard(*bit == TEND); // there may be no need, but do it.
      env[0] = isarry(*bit)?Aarray:
               isaggr(*bit)?Aaggre:Asingle;
      env[1] = *bit;
    } else if (*env == Aarray) {
      // bit should point to array type now, which
      // is the env[1].
      assert(env[1] == *bit);
      VType const *posi = bit;
      getelesize(bit, 64, &posi, NULL); // jump to its inner ele.
      // get inner type element size and move bit pointer.
      // and set env.
      switch(getarryt(*bit)) {
        case Aarray:
        case Aaggre:
          size = getelesize(posi, 64, NULL, NULL);
          bit = posi;
          break;
        default:
          size = getoctet(*bit);
          break;
      }
      guard(size == 0);
      ret = size; // set the return size
      size_t len = getarrylen(env[1]);
      if (ix >= len) {
        printf("index %zu exceeds max array size %zu\n", ix, len);
        guard(ix >= len);
      }
      addr += ix * size;
      if (bit == posi) {
        env[0] = isarry(*bit)?Aarray:
                 isaggr(*bit)?Aaggre:Asingle;
        env[1] = *bit;
      } else {
        env[0] = Asingle; // env[1] unchanged, 0 is enough
        env[1] = TEND;
      }
    } else {
      // the Asingle env, means something is wrong
      printf("Error, depth exceeds.\n");
      guard(1);
    }
  }
  va_end(ap);
  return addr;
}

// the index is always point to an empty location
static void push(VType v, VType* m, int *i, int l) {
  if (*i < l) {
    m[(*i)++] = v;
  } else {
    printf(__FILE__ " line %d, with i %d, limit %d\n", __LINE__, *i, l);
  }
}

static VType pop(VType* m, int *i) {
  if (*i >= 1) {
    return m[--(*i)];
  } else {
    printf(__FILE__ " line %d, with i %d\n", __LINE__, *i);
    return 0;
  }
}

static VType peek(VType* m, int i) {
  if (i >= 1) {
    return m[i-1];
  } else return 0;
}

