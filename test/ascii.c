#include "log.h"

/*
 *
 * Print the ansi color table.
 * A test program for logChannelColor.
 *
 */

int main()
{
  for (int i = 1; i < 513; i++) {
    logChannelColor(Default, "{%d}%3d{0}%s", i, i, i%16?" ":"\n");
  }
  return 0;
}
