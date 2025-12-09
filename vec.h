#ifndef _VEC_
#define _VEC_

#include <glib.h>

struct vec2_s {
  gfloat x;
  gfloat y;
};

struct vec3_s {
  gfloat x;
  gfloat y;
  gfloat z;
};

gfloat vec3_dot(const struct vec3_s a, const struct vec3_s b);

#endif // _VEC_
