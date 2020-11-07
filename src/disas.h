#ifndef DTOOLDISAS
#define DTOOLDISAS
#include <capstone/capstone.h>
#include <stdbool.h>

#include "cmd.h"
#include "vara.h"

// arch:mode:syntax:start addr
static const char *dt_option = "Disas:DDOQ";

void dt_error(FILE* s, const char* const msg, cs_err code);
void dt_disas(char* name, unsigned char* filebuffer, size_t size, void *option);

#endif
