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
#define OCTT  0x8000  // octet
#define TVAL  0x5000  // value prefix

/*
 *
 * h 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 l
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  + gtype |ayt-oct|     id/len    +
 *  + (gty) |   -   |     /suffix   +
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * gtype : type header. distinguish header type.
 *         ARRY B 0001, array type header
 *         AGGR B 0010, aggregate type header
 *         AEND B 0100, aggregate type end marker
 *         OCTT B 1000, single octet like type
 *         TVAL B 0110, value suffix type
 *         TEND B 0000. end marker for type bits.
 * ayt: array element type only useful when @gtype@ is $ARRY$.
 *        Aoctet  B 00
 *        Aarray  B 01
 *        Aaggre  B 10
 *        Aresrv  B 11
 * oct: octet type. only useful when @ayt@ is B 00.
 *       octet  B 00
 *       word   B 01
 *       dword  B 10
 *       qword  B 11
 *       it's the exponent of unsigned 1 for its size.
 *       octet is 1 << 0, so 1 byte.
 *       word  is 1 << 1, so 2 bytes.
 *       dword is 1 << 2, so 4 bytes.
 *       qword is 1 << 2, so 8 bytes.
 * if @gtype@ is $ARRY$, len is array elements length.
 *    one optional $TVAL$ type value can follow.
 * if @gtype@ is $AGGR$, id is used to identify
 *    aggregate type header identity. so one $AEND$
 *    can match the type ending.
 * if @gtype@ is $AEND$, the id field must match with
 *    $AGGR$ header id. otherwise, it's an error.
 * if @gtype@ is $OCTT$, suffix means how many fields
 *    have this type.
 * if @gtype@ is $TVAL$, suffix means the value, the number
 *    which is between 0 to 255.
 *
*/

// FIELD access mask and its relevant shift value used in
// "<<" or ">>" operator.
enum {
  GTYFIELD = 0XF000,
  GTYSHIFT = 12,
  AYTFIELD = 0X0C00,
  AYTSHIFT = 10,
  OCTFIELD = 0X0300,
  OCTSHIFT = 8,
  VALFIELD = 0X00FF,
  VALSHIFT = 0
};

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
#define isaggr(v) (AGGR&(v)&&!issval(v))
#define isaend(v) (AEND&(v)&&!issval(v))
#define isoctt(v) (OCTT&(v))
#define issval(v) ((AGGR|AEND)==((v)&GTYFIELD)) // if it is a suffix count

//////////////////////////////
// uniform field access macro.
//////////////////////////////
#define field(v, mask) ((v)&(mask))

//////////////////////////////
// octet field value, 00 - 11.
//////////////////////////////
// get octet field value, 0 ~ 3.
#define getoctet(v) (field(v, OCTFIELD)>>OCTSHIFT)
// e is exponent, 0 ~ 3.
#define setoctet(v, e) (field(e<<OCTSHIFT, OCTFIELD)|(v))

/////////////
// array type
/////////////
#define Aoctet  0x00
#define Aarray  0x01
#define Aaggre  0x02
#define Aresrv  0x03
// array inner ele type
#define getarryt(v) (field(v, AYTFIELD)>>AYTSHIFT)
#define setarryt(v, t) (field((t)<<AYTSHIFT, AYTFIELD)|(v))
// array len manipulate
#define getarrylen(v) (field(v, VALFIELD)>>VALSHIFT)
#define setarrylen(v, l) (field(l, VALFIELD)|(v))

// aggregrate id, from 0 ~ 255, unsigned int8
#define getaggrid(v) (field(v, VALFIELD)>>VALSHIFT)
#define setaggrid(v, i) (field(i, VALFIELD)|(v))

#define getval(v) (field(v, VALFIELD)>>VALSHIFT)
#define setval(v, n) (field(n, VALFIELD)|(v)) // set suffix count

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
    if (i%8 == 0) printf("\n");
    printf("%04x ", store->dbit[i]);
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
  // actual bits memory
  VType *bits = malloc(clen*mut*(sizeof(VType)));
  // bits index @bi@, bi should always point to an
  // element, and should satisfy condition that "bi < cursor".
  // and @cursor@ should always point an empty place.
  size_t bi = 0;
  // number 8 is preallocated space, so don't need check every
  // time before you want move cursor @bi forward.
#define checkbits(indx, mem) mem=indx>=(clen*mut-8)?realloc(mem,clen*(sizeof(VType))*(++mut)):mem

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
    unsigned char oct = 0;  // oct field value
    switch(*di) {
      case 'o':
      case 'O': oct = 0; goto SINGLE;
      case 'w':
      case 'W': oct = 1; goto SINGLE;
      case 'd':
      case 'D': oct = 2; goto SINGLE;
      case 'q':
      case 'Q': oct = 3; goto SINGLE;
      case '@': oct = 3; goto SINGLE;
    }
    goto AGGRBEG;

SINGLE:
    di += 1;  // next char
    ele = setoctet(OCTT,oct);
    if (isdigit(*di)) {
      num = strtol(di, &di, 10);
      num = num?num:1;  // make sure num is not zero
      ele = setval(ele, num);
    } else if (*di == '[') {
      // TODO add array support
    }
    *(bits+(bi++)) = ele;
    checkbits(bi, bits);
    if (si == 0) {
      size += (1<<oct) * num;  // add size to returned size
    } else {
      nstack[si-1][0] += (1<<oct) * num;
    }
    num = 1;  // reset num to 1
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
        if (num) *(bits+(bi++)) = setval(TVAL, num);
        num = num?num:1;
        checkbits(bi, bits);
      }
      pop(stack, &si);  // finish one aggregate type
      nstack[si][0] *= num;
      if (si == 0) {
        size += nstack[si][0];
      } else {
        nstack[si-1][0] += nstack[si][0];
      }
      nstack[si][0] = 0;
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
, size_t capi         // stack limit
, VType const **posi  // new position of next unit
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
        case Aoctet:
          size *= 1 << getoctet(*bit); // get array size
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
    } else if (isoctt(*bit)) {
      size = getval(*bit);
      size = size?size:1;
      size *= 1 << getoctet(*bit);
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
  if (isoctt(*bit)) {
    num = getval(*bit);
    num = num?num:1;
    size = 1 << getoctet(*bit);
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
      }
      num = num?num:1;  // make usre num is not zero
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
   * Aoctet current selected unit is owdq
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
               isaggr(*bit)?Aaggre:Aoctet;
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
               isaggr(*bit)?Aaggre:Aoctet;
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
          size = 1 << getoctet(*bit);
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
                 isaggr(*bit)?Aaggre:Aoctet;
        env[1] = *bit;
      } else {
        env[0] = Aoctet; // env[1] unchanged, 0 is enough
        env[1] = TEND;
      }
    } else {
      // the Aoctet env, means something is wrong
      printf("Error, depth exceeds.\n");
      guard(1);
    }
  }
  va_end(ap);
  return addr;
}

// TODO: add security check
void  vara_free(vtra store)
{
  free(store->dbit);
  free(store->name);
  free(store->data);
  free(store);
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

