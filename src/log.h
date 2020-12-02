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

struct CSI {
  const char* para; 
  const char* inte;
  char end;
};

// Global ColorTable
// 256 foreground color and 256 background color
//
// Warn: Use it after logInit();
extern struct CSI* ColorTable;

#define CSIDEC   (struct CSI *)
#define CSIEND   { NULL, NULL, 0 }

int   logInfo(const char* format, ...);
int   logInfoColor(const char* format, ...);

int   logChannel(enum Channel chl, const char* format, ...);
int   logChannelCSI(enum Channel chl, const struct CSI*, const char* format, ...);
int   logChannelColor(enum Channel chl, const char* format, ...);
void  redirectChannel(enum Channel from, enum Channel to); // TODO
void  setChannel(enum Channel chl, FILE* f); // TODO
void  setChannelToBuf(enum Channel chl); // TODO
char* getChannelBuf(enum Channel chl); // TODO

void logInit();
#endif
