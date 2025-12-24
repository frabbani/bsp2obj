#ifndef _VEC_
#define _VEC_

#include <glib.h>

struct ivec2_s {
  gint x;
  gint y;
};

struct vec2_s {
  union {
    struct {
      gfloat x;
      gfloat y;
    };
    gfloat xy[2];
  };
};

struct vec3_s {
  union {
    struct {
      gfloat x;
      gfloat y;
      gfloat z;
    };
    gfloat xyz[3];
  };
};

gfloat vec3_dot(const struct vec3_s a, const struct vec3_s b);
struct vec3_s vec3_set(gfloat x, gfloat y, gfloat z);
struct vec3_s vec3_max(const struct vec3_s a, const struct vec3_s b);
struct vec3_s vec3_min(const struct vec3_s a, const struct vec3_s b);
struct vec3_s vec3_mul(const struct vec3_s a, gfloat s);
struct vec3_s vec3_add(const struct vec3_s a, const struct vec3_s b);
struct vec3_s vec3_sub(const struct vec3_s a, const struct vec3_s b);
struct vec3_s vec3_cross(const struct vec3_s a, const struct vec3_s b);

#endif // _VEC_
