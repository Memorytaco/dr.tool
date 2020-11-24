#ifndef DTOOLPKG
#define DTOOLPKG

/* Search path is ":" seperated */
#define SEARCHTOK ":"
#define SEARCH "DTOOLPKGSEARCH"

// provided user interface
enum pkgui {
  PKGSERVE      = 0x01, // use uniform interface providing function
  PKGLIBRARY    = 0x02, // can be linked directly, or use dlfcn
  PKGSTANDALONE = 0x04, // one standalone executable
};

struct pkgmeta {
  const char * name;
  const char * author;
  const char * desc;  // short description
  enum pkgui ui;  // package user interface
  struct pkgver {
    unsigned int major, minor, patch;
  } version;  // package version
};

// signature of a runtime package.
typedef struct pkg* pkgsig;

struct pkgreg {
  enum pkgui ui;
  const char *depend; // package dependency, seperate name with ':'
  struct pkgregsym {
    const char *sym;
    void (*addr)(pkgsig);
  } exports[];
};

enum pkgscope {
  PKGGLOBAL,
  PKGPRIVATE,
  PKGDEFAULT  // use default if you dont know what to do
};

pkgsig pkgload(const char *name, enum pkgscope scop);
void   pkgunload(pkgsig sig);
void   pkgshowinfo(pkgsig sig);
void   pkginvoke(pkgsig sig, const char *sym);
void * pkgsym(pkgsig sig, const char *sym);
void   pkgsetvar(pkgsig sig, void *var);
void * pkggetvar(pkgsig sig);

#endif
