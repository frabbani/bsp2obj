#include "vec.h"

gfloat vec3_dot(const struct vec3_s a, const struct vec3_s b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}