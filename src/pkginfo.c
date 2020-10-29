#include "pkg.h"
#include <stdio.h>

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("%s [package name]+\n", argv[0]);
    return 0;
  }
  for (int i = 1; i < argc; i++) {
    pkgsig sig = pkgload(argv[i], PKGGLOBAL);
    if (sig) {
      pkgshowinfo(sig);
      pkgunload(sig);
    } else {
      printf("Error when loading package %s.\n", argv[i]);
    }
  }
  return 0;
}
