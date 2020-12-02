#ifndef DTOOLCMD
#define DTOOLCMD

#define CSI_PREFIX "CSI"
#define DTOOLCMD_VERSION "0.0.2"

/*
 * 1. use DEPLENGTH to determine the max length of dependency
 *    of option.
 */

#include <stdio.h>
#include <stdbool.h>

/////////////////////////////////
// Command Line User Interface //
/////////////////////////////////

// interface for Extra values returned by cmd_match
typedef struct ExtraVal ArgcArgv;
const char* argvidx(ArgcArgv *cv, size_t idx);
size_t argvctx(ArgcArgv *cv);

// Opaque structure for arguments access in callback function
typedef struct cmdunit Arg;
void * Argvix(Arg *arg, size_t ix, size_t *sz);
size_t Argvcx(Arg *arg);

// command line callback used to do something when the option
// appeared.
// @arg is used to identify its calling type provid
//     neccessory value
// @data is extra information which is used by user.
//     using $argvidx and $argc to access it.
// @store is something user defined global data center.
//     it needs to be allocated by user and known by user.
//     you can check something in here, user allocated memory.
//     eg. global settings or value.
typedef void (*optact)
  (Arg *arg, void* extra, void* store);

// rely enumeration used in specrely field
enum relyt {
  RELYEND,  // end marker for rely relation list
  RELYOPT,  // optional rely on
  RELYREQ,  // required rely on, error if not satisfied
};

/*
 * TODO: Finish the alia syntax definition
 * alia syntax:
 *
 *
 * e.g. --option-name:special_name_part:options
 * ? optional
 *
 * TODO: Finish the valset syntax definition
 */

typedef struct cmdspect {
  // option name, global unique
  const char * optname;
  struct specalia {
    // TODO: add syntax for alia
    const char * alia;
    const char * desc;  // short description for this alia
    // if not null, specific the acceptable
    // value for this option
    // TODO: Define value set syntax
    const char * valset;
  } * alias;
  // TODO: Change specrely syntax
  struct specrelyt {
    const char * optname; // option name relied on
    enum relyt type;
  } * rely;
  optact call;
} Spec;

typedef struct specalia Alia;
typedef struct specrelyt Rely;

#define SPECALIAEND { NULL, NULL, NULL }
#define SPECRELYEND { NULL, RELYEND }

#define CMDSPECSARRAY (struct cmdspect  [])
#define ALIASARRAY    (struct specalia  [])
#define RELYSARRAY    (struct specrelyt [])

// main call for matching command line arguments
ArgcArgv* cmdspec_match(int argc, const char** argv, struct cmdspect* specs, void* store);

typedef void (*cmdact) (int argc, char ** argv, void *store);

// subcommand registry structure
typedef struct command {
  // command names, seperate with one space
  // "cmdname alias1 alias2 ..."
  const char * command;
  const char * package;  // its package name
  const char * info;     // help info for this command
  // info for program, can be NULL.
  // which if not null, indicates it is static command,
  // otherwise, it should be fetched from the related
  // package.
  struct cmdspect * spec;
  // call to handle the command
  cmdact action;
} Command;

#define CMDSPECEND { NULL, NULL, NULL, NULL }
#define COMMANDEND { NULL, NULL, NULL, NULL, NULL }

#define CMDSARRAY (struct command [])

// call to match available commands
int cmd_match(int argc, const char** argv, Command *cmds, void *store);
void cmdshortinfo(Command *cmd);
void cmdlonginfo(Command *cmd);

// TODO: provide some helper function, helping parse some option values
// which look like a,b,c,d or something i=3,b=4 or curl like "url@a".

// void fun();
// void fun();
// void fun();
// void fun();

// TODO: provide error notification mechanism for situation that
// some required option value doesn't appear or some required
// option group doesn't be combined.

/*
  mechanism here.
*/

// TODO: provide a handy interface for user to declare its desired
// option info
// e.g.
// user can provide something like this
//
// "4nice$1h@"  // remian to build
//  ^   ^^
//specif||ic the number of code name, here is "4"
//      ||
//      t|he code name is "nice"
//       |
//       "$", begin of option name. follow the name length, "1"
//       and the name is "h"

// TODO: add "getopt" interface, allow user to handle complicate
// situations.

#endif
