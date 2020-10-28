#ifndef DTOOLPARA
#define DTOOLPARA

#include <stdarg.h>
#include <string.h>

typedef struct vara* vtra;

// oO octet/signed octet 8bit
// wW : word
// dD : dword
// qQ : qword
// @  : pointer
// !@#$%^&*()[]{}?/.>,<;:"'

// a handy macro for using generic parameter as
// normal variable.
// e.g.
//  #1: vptr(unsigned int, st, 1, 0) = 3;
//  #2: *vptr(unsigned int*, st, 1, 0) = 3;
//
//  #1 is different with #2. #1 interpret the memory as
//  unsigned int, but #2 take it as a pointer which points to
//  unsigned int. this is quite different, so be careful.
//
// warning: only use this macro when you are aware of action
//          you are taking.
// use it with !!!PACKED!!! data structure, or it will surprise
// you. Anyway be careful with using data structure along with
// this macro.
#define vptr(type, store, depth, ...) \
  (*(type *)vara_ptr(store, depth, NULL, __VA_ARGS__))

// short hand for vara_ptr.
#define vaddr(store, depth, ...) \
  vara_ptr(store, depth, NULL, __VA_ARGS__)

// "name:@dw(w[16]2)@"
// allocate the memory access holder.
vtra  vara_alloc(const char* dsc);  // TODO: add align support
void* vara_ptr(vtra store, int depth, size_t* size, ...);
const char* vara_name(vtra store);
void  vara_free(vtra store);  // TODO: add vara_free
void  vara_info(vtra store);  // TODO: finish vara_info


#endif
