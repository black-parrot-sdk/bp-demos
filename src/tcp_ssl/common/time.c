#include <stdint.h>
#include <time.h>

time_t time (time_t *t)
{
  time_t now; // Unit: second
  uint64_t mtime;
  mtime = *(volatile uint64_t *)(0x30bff8UL);
  now = 1740028653UL + (mtime * 4 / 5 / 1000000);
  if(t)
    *t = now;
  return now;
}
