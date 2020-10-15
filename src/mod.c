#include "mod.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct mod {
  void *modpara;  // value setted by caller, may be used by
                  // the module
  void *handle;   // dynamic module handle
  struct modreg *reg;
};

// global module cache
struct modcache {
  size_t count;
  struct mod** list;
} glob = { 0, NULL };

// TODO: add dependency check
struct mod* modprobe(const char *name, int flags)
{
  for (size_t i=0; i<glob.count; i++) {
    if (strcmp(name, glob.list[i]->reg->modname))
      continue;
    return glob.list[i];
  }
  glob.count++;
  glob.list = 
    realloc(glob.list, glob.count * sizeof(struct mod*));
  // allocate memory for handle
  struct mod *handle = glob.list[glob.count-1]
                     = malloc(sizeof(struct mod));
  memset(handle, 0, sizeof(struct mod));

  size_t namelen = strlen(name);

  char *libname = malloc(namelen + 7);  // "mod" + name + ".so"
  snprintf(libname, namelen + 7, "%s%s%s", "mod", name, ".so");
  handle->handle = dlopen(libname, flags);
  if (handle->handle == NULL) {
    printf("Error when loading file: %s", dlerror());
    return NULL;
  }
  char *modname = malloc(namelen+4);    // "mod" + name
  snprintf(modname, namelen+4, "%s%s", "mod", name);
  handle->reg = dlsym(handle->handle, modname);
  free(modname);
  free(libname);
  return handle;
}

void modsetpara(struct mod* modhandle, void *data)
{
  modhandle->modpara = data;
}

void* modgetpara(struct mod* modhandle)
{
  return modhandle->modpara;
}

void modshow(struct mod* modhandle)
{
  printf("mod name: %s @ %p.\n", modhandle->reg->modname, modhandle->reg);
  for ( struct modcall *call = modhandle->reg->calls;
        call->addr!=NULL; call++ ) {
    printf ("available function @ %p is %s\n", call->addr, call->name);
  }
  printf("====\n");
}

// TODO: interface is not stable
void modcall(struct mod* modhandle, const char *symname)
{
  for ( struct modcall *call = modhandle->reg->calls;
        call->addr != NULL; call++) {
    if (!strcmp(call->name, symname)) {
      call->addr(modhandle);
      return ;
    }
  }
}

// do nothing... just for now.
// TODO: complete the logic
void * modsym(struct mod* modhandle, const char *symname)
{
  return dlsym(modhandle->handle, symname);
}

