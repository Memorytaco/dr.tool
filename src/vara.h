#ifndef DTOOLVARA
#define DTOOLVARA

#include <stdarg.h>
#include <string.h>

typedef struct vara* vtra;

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

/*
 * Syntax:
 *  Type := Name ":" Qualifier+
 *  Name := [A~Za~z0-9_]+
 *  Qualifier := Primitive Suffix? Qualifier*
 *            | "(" Qualifier+ ")" Qualifier*
 *  Primitive := [OoWwDdQq]
 *  Suffix := ArrySuffix Digit? | Digit
 *  ArrySuffix := "[" Digit "]" ArrySuffix*
 *  Digit := [0-9]
 *
 * e.g.
 *  struct Type1 {
 *    int i;
 *    unsigned j;
 *  };  == "Type1:DD"
 *
 * struct Type2 {
 *    struct Type1 pre;
 *    struct Type1 *next;
 *  }; == "Type2:(DD)@"
 *
 * struct Type3 {
 *    int i[20];
 *    int j[20];
 *    int k[20];
 *  }; == "Type3:D[20]3"
 *     or "Type3:D[20]D[20]D[20]"
 *
 */

/*
 *
 * oO : octet/signed octet 8bit
 * wW : word
 * dD : dword
 * qQ : qword
 * @  : pointer
 *
 * lower case should be used to denote signed version.
 * and uppger case should be used to denote unsigned verison.
 *
 * But They are no difference currently, because they are
 * used only to denote the storage size. And you need to
 * specific the way how to inteprete these memory bits via
 * type cast in c using a void pointer or just the vptr
 * pointer.
 *
 * vara_ptr will return the storage size via @size parameter
 * if you want to directly use the call.
 *
 * use vara_free if you don't need it anymore.
 *
 * vara_info will output info of the type for debuging
 * purpose only.
 *
 * vara_name return the type, but this call may be deprecated
 * in future.
 *
 */

// allocate the memory access holder.
vtra  vara_alloc(const char* dsc);  // TODO: add align support
void* vara_ptr(vtra store, int depth, size_t* size, ...);
const char* vara_name(vtra store);
void  vara_free(vtra store);
void  vara_info(vtra store);  // TODO: finish vara_info


#endif
