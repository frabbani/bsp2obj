#ifndef _VEC_
#define _VEC_

#include <glib.h>

#define PI 3.14159265358979323846f
#define DEG2RAD(x) ((x) * PI / 180.0f)
#define RAD2DEG(x) ((x) * 180.0f / PI)

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

struct mat3_s {
  gfloat es[3][3];
};

gfloat vec3_dot(const struct vec3_s a, const struct vec3_s b);
gfloat vec3_len(const struct vec3_s a);
struct vec3_s vec3_set(gfloat x, gfloat y, gfloat z);
struct vec3_s vec3_max(const struct vec3_s a, const struct vec3_s b);
struct vec3_s vec3_min(const struct vec3_s a, const struct vec3_s b);
struct vec3_s vec3_mul(const struct vec3_s a, gfloat s);
struct vec3_s vec3_add(const struct vec3_s a, const struct vec3_s b);
struct vec3_s vec3_sub(const struct vec3_s a, const struct vec3_s b);
struct vec3_s vec3_cross(const struct vec3_s a, const struct vec3_s b);
struct vec3_s vec3_norm(const struct vec3_s a);

struct mat3_s mat3_ident();
struct mat3_s mat3_rot(struct vec3_s angles_rad);
struct vec3_s vec3_transf(const struct mat3_s m, const struct vec3_s v);

#endif // _VEC_
