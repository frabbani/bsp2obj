#include "clock.h"
#include <time.h>

gint64 now_us(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (gint64)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
}