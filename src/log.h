#ifndef DTOOLOG
#define DTOOLOG

#include <stdio.h>
#include <stdarg.h>

enum Channel {
  Default =   0x00,
  Normal  =   0x01,
  Warn    =   0x02,
  Alert   =   0x03,
  Debug   =   0x04,
  BlackHole = 0xFF00
};

struct LogCSI {
  const char* para; 
  const char* inte;
  char end;
};

#define LOGCSIDEC   (struct LogCSI *)
#define LOGCSIEND   { NULL, NULL, 0 }

int   logInfo(const char* format, ...);
int   logInfoCSI(const char* format, ...);

int   logChannel(enum Channel chl, const char* format, ...);
int   logChannelCSI(enum Channel chl, struct LogCSI*, const char* format, ...);
int   logChannelColor(enum Channel chl, const char* format, ...);
void  redirectChannel(enum Channel from, enum Channel to);
void  setChannel(enum Channel chl, FILE* f);
// Not Ready
void  setChannelToBuf(enum Channel chl);
char* getChannelBuf(enum Channel chl);

#endif
