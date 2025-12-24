#include "vec.h"
#include <math.h>

gfloat vec3_dot(const struct vec3_s a, const struct vec3_s b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

gfloat vec3_len(const struct vec3_s a) {
  return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
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

struct vec3_s vec3_norm(const struct vec3_s a) {
  gfloat len = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
  if (len > 1e-8f) {
    return vec3_mul(a, 1.0f / len);
  } else {
    return vec3_set(0.0f, 0.0f, 0.0f);
  }
}

struct mat3_s mat3_ident() {
  struct mat3_s m;
  m.es[0][0] = 1.0f;
  m.es[0][1] = 0.0f;
  m.es[0][2] = 0.0f;
  m.es[1][0] = 0.0f;
  m.es[1][1] = 1.0f;
  m.es[1][2] = 0.0f;
  m.es[2][0] = 0.0f;
  m.es[2][1] = 0.0f;
  m.es[2][2] = 1.0f;
  return m;
}

struct mat3_s mat3_rot(struct vec3_s angles_rad) {
  struct mat3_s m;
  gfloat cx = cosf(angles_rad.x);
  gfloat sx = sinf(angles_rad.x);
  gfloat cy = cosf(angles_rad.y);
  gfloat sy = sinf(angles_rad.y);
  gfloat cz = cosf(angles_rad.z);
  gfloat sz = sinf(angles_rad.z);

  m.es[0][0] = cy * cz;
  m.es[0][1] = -cy * sz;
  m.es[0][2] = sy;
  m.es[1][0] = sx * sy * cz + cx * sz;
  m.es[1][1] = -sx * sy * sz + cx * cz;
  m.es[1][2] = -sx * cy;
  m.es[2][0] = -cx * sy * cz + sx * sz;
  m.es[2][1] = cx * sy * sz + sx * cz;
  m.es[2][2] = cx * cy;

  return m;
}

struct vec3_s vec3_transf(const struct mat3_s m, const struct vec3_s v) {
  struct vec3_s r;
  r.x = m.es[0][0] * v.x + m.es[0][1] * v.y + m.es[0][2] * v.z;
  r.y = m.es[1][0] * v.x + m.es[1][1] * v.y + m.es[1][2] * v.z;
  r.z = m.es[2][0] * v.x + m.es[2][1] * v.y + m.es[2][2] * v.z;
  return r;
}
