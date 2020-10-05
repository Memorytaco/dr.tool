#ifndef DTOOLDISAS
#define DTOOLDISAS
#include <capstone/capstone.h>
#include <stdbool.h>

#include "cmd.h"

void derror(FILE* s, const char* const msg, cs_err code);
void dt_disas(char* name, unsigned char* filebuffer, size_t size, bool syntax);

#endif
