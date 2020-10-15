#include "mod.h"
#include <stdio.h>

int main(int argc, char** argv) {
  modcore handle = modprobe("file", RTLD_NOW);
  modsetpara(handle, argv[2]);
  modcall(handle, argv[1]);
  printf("%s", modgetpara(handle));
  return 0;
}
