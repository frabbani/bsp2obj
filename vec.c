#include "vec.h"

gfloat vec3_dot(const struct vec3_s a, const struct vec3_s b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

struct vec3_s vec3_max(const struct vec3_s a, const struct vec3_s b) {
  struct vec3_s r;
  r.x = MAX(a.x, b.x);
  r.y = MAX(a.y, b.y);
  r.z = MAX(a.z, b.z);
  return r;
}

struct vec3_s vec3_min(const struct vec3_s a, const struct vec3_s b) {
  struct vec3_s r;
  r.x = MIN(a.x, b.x);
  r.y = MIN(a.y, b.y);
  r.z = MIN(a.z, b.z);
  return r;
}

struct vec3_s vec3_set(gfloat x, gfloat y, gfloat z) {
  struct vec3_s v;
  v.x = x;
  v.y = y;
  v.z = z;
  return v;
}