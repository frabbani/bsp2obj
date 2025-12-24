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

struct vec3_s vec3_mul(const struct vec3_s a, gfloat s) {
  struct vec3_s r;
  r.x = a.x * s;
  r.y = a.y * s;
  r.z = a.z * s;
  return r;
}

struct vec3_s vec3_add(const struct vec3_s a, const struct vec3_s b) {
  struct vec3_s r;
  r.x = a.x + b.x;
  r.y = a.y + b.y;
  r.z = a.z + b.z;
  return r;
}

struct vec3_s vec3_sub(const struct vec3_s a, const struct vec3_s b) {
  struct vec3_s r;
  r.x = a.x - b.x;
  r.y = a.y - b.y;
  r.z = a.z - b.z;
  return r;
}

struct vec3_s vec3_cross(const struct vec3_s a, const struct vec3_s b) {
  struct vec3_s r;
  r.x = a.y * b.z - a.z * b.y;
  r.y = a.z * b.x - a.x * b.z;
  r.z = a.x * b.y - a.y * b.x;
  return r;
}