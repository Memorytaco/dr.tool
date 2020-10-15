#ifndef DTOOLMOD
#define DTOOLMOD

#include <dlfcn.h>

struct mod;

typedef struct mod* modcore;
struct modreg {
  const char *modname;  // the module name, should be unique
  const char** modep;   // a list of module names, ended with 0
  struct modcall {
    const char *name;
    void (*addr)(modcore);
  } calls[];             // ended with NULL
};

// TODO: add generic parameter passing method
// struct ....

struct mod* modprobe(const char *name, int flags);
void modfree(struct mod* modhandle);
void modshow(struct mod* modhandle);
void * modsym(struct mod* modhandle, const char *symname);
void modcall(struct mod* modhandle, const char *symname);
void modsetpara(struct mod* modhandle, void *data);
void* modgetpara(struct mod* modhandle);

#endif
