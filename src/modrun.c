#include "pkg.h"
#include <stdio.h>

int main(int argc, char** argv) {
  pkgsig sig = pkgload("file", PKGGLOBAL);
  pkgsetvar(sig, argv[2]);
  pkginvoke(sig, argv[1]);
  printf("%s", pkggetvar(sig));
  return 0;
}
