#include "../mod.h"

#include "../util/file.h"

static void getfile(modcore handle) {
  char *name = modgetpara(handle);
  size_t size = 0;
  char *buffer = readfile(name, &size);
  modsetpara(handle, buffer);
}

static void test(modcore handle) {
  printf("hello World\n");
}

struct modreg modfile = {
  .modname = "file",
  .modep = NULL,
  .calls = {
    { "getfile", getfile },
    { "test", test },
    { NULL, NULL }
  }
};
