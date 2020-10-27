#ifndef DTOOLPARA
#define DTOOLPARA

#include <stdarg.h>
#include <string.h>

struct vara;
typedef struct vara* vtra;

// oO octet/signed octet 8bit
// wW : word
// dD : dword
// qQ : qword
// @  : pointer
// !@#$%^&*()[]{}?/.>,<;:"'

// a handy macro for using generic parameter as
// normal variable.
//
// warning: don't use it if you don't know the details
// of the structure. otherwise it will may generate
// memory segment fault.
#define vptr(type, store, depth, ...) \
  (*((type)*)vara_ptr(store, depth, NULL, __VA_ARGS__))

// "name:@dw(w[16]2)@"
vtra  vara_alloc(const char* dsc);
void* vara_ptr(vtra store, int depth, size_t* size, ...);
const char* vara_name(vtra store);
void  vara_free(vtra store);
void  vara_info(vtra store);


#endif
