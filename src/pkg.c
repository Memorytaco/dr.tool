#include "pkg.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: Add Guard here
#include <unistd.h>

// dynamic so file name:
//   pkgprefix + "name" + pkgsuffix
char * pkgsuffix = ".pkg";
char * pkgprefix = "";
char * metaprefix = "meta"; // pkg metainfo var prefix
char * regprefix = "reg";   // pkg reg table var prefix

struct pkg {
  struct pkgmeta const *meta;
  void * var;
  void * dl;
  struct pkgreg *reg;
  struct pkg *next;
};

// global register package cache
struct {
  char    * path;     // search path
  pkgsig    list;     // package list
} __CACHE = { NULL , NULL };


///////////////////////////////////////
// implementation for the interface. //
///////////////////////////////////////
char* pkgsearch(const char *name, const char *prefix)
{
  const char* envsearch = getenv(SEARCH);

  if (prefix != NULL) envsearch = prefix;
  if (envsearch == NULL || name == NULL) return NULL;

  size_t size = strlen(envsearch) + 1;
  size_t namelen = strlen(name);
  char * search = malloc(size);
  memmove(search, envsearch, size);

  char *ptr = strtok(search, SEARCHTOK);
  do {
    size_t len = strlen(ptr);
    if (len == 0) {
      ptr = strtok(NULL, SEARCHTOK);
      continue;
    }
    char *buf = malloc(len + namelen + 2);
    memset(buf, 0, len + namelen + 2);
    memmove(buf, ptr, len);
    if (buf[len-1] != '/') {
      buf[len] = '/';
      len ++;
    }
    memmove(buf+len, name, namelen);
    int status = access(buf, F_OK);
    if (status == -1) {
      // handle failure here
      printf("File %s doesn't exist.\n", buf);
      free (buf);
      ptr = strtok(NULL, SEARCHTOK);
      continue;
    }
    status = access(buf, R_OK | X_OK);
    if (status == -1) {
      printf("File %s couldn't be readed or executed.\n", buf);
      free (buf);
      ptr = strtok(NULL, SEARCHTOK);
    }
    // handle the right case
    free(search);
    return buf;
  } while (ptr != NULL);

  free(search);
  return NULL;
}

// TODO: add dependency check
pkgsig pkgload(const char *name, enum pkgscope scop)
{
  // check whether package has been loaded
  for (pkgsig iter = __CACHE.list; iter!=NULL; iter = iter->next) {
    if (!strcmp(iter->meta->name, name)) return iter;
  }

  pkgsig sig = malloc(sizeof(struct pkg));
  memset(sig, 0, sizeof(struct pkg));

  size_t namelen = strlen(name);
  size_t suffixlen = strlen(pkgsuffix);
  size_t prefixlen = strlen(pkgprefix);
  size_t totallen = namelen + suffixlen + prefixlen + 1;

  char *libname = malloc(totallen);
  snprintf(libname, totallen, "%s%s%s", pkgprefix, name, pkgsuffix);

  char *fullpath = pkgsearch(libname, NULL);
  switch (scop) {
    case PKGGLOBAL:
      sig->dl = dlopen(fullpath ? fullpath: libname
          , RTLD_LAZY | RTLD_GLOBAL | RTLD_NODELETE);
      break;
    default:
    case PKGPRIVATE:
      sig->dl = dlopen(fullpath ? fullpath: libname
          , RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
      break;
  }

  if (sig->dl == NULL) {
    printf("Error when loading file: %s", dlerror());
    free(sig);
    return NULL;
  }

  size_t metalen = strlen(metaprefix);
  char *metaname = malloc(namelen + metalen + 1);
  snprintf(metaname, namelen + metalen + 1, "%s%s", metaprefix, name);
  sig->meta = dlsym(sig->dl, metaname);
  if (sig->meta == NULL) {
    printf("Not Found %s in pkg %s.\n", metaname, libname);
  }

  size_t reglen = strlen(regprefix);
  char *regname = malloc(namelen + reglen + 1);
  snprintf(regname, namelen + reglen + 1, "%s%s", regprefix, name);
  sig->reg = dlsym(sig->dl, regname);
  if (sig->reg == NULL) {
    printf("Not Found %s in pkg %s.\n", regname, libname);
  }

  free(libname);
  free(metaname);
  free(regname);
  free(fullpath);

  sig->next = __CACHE.list;
  __CACHE.list = sig;
  return sig;
}

void   pkgunload(pkgsig sig)
{
  pkgsig pre = __CACHE.list;
  while (pre != NULL && pre != sig && pre->next != sig)
    pre = pre->next;
  if (pre == NULL) {
    printf("Error, signature doesn't exsist.\n");
    return ;
  }
  if (pre == sig) {
    __CACHE.list = sig->next;
  } else {
    pre->next = sig->next;
  }
  dlclose(sig->dl);
  free(sig);
}

void * pkgsym(pkgsig sig, const char *sym) {
  return dlsym(sig->dl, sym);
}

void   pkginvoke(pkgsig sig, const char *sym)
{
  struct pkgregsym *regsym = sig->reg->exports;
  for (; regsym->sym != NULL && regsym->addr != NULL; regsym++) {
    if (!strcmp(sym, regsym->sym))
      return regsym->addr(sig);
  }
  return ;
}

void pkgshowinfo(pkgsig sig)
{
  struct pkgver ver = sig->meta->version;
  printf("%s: %s\n", sig->meta->name, sig->meta->author);
  printf("Signature %02X ", sig->meta->ui);
  printf("Version %d.%d.%d\n", ver.major, ver.minor, ver.patch);
  printf("%s\n", sig->meta->desc);
}

void pkgsetvar(pkgsig sig, void *var) { sig->var = var; return ; }
void *pkggetvar(pkgsig sig) { return sig->var; }
