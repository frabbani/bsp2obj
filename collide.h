#pragma once
#include "vec.h"
#include <glib.h>
#include <math.h>

// point a vector from p0 to p1 using vec3_sub()
#define point_vec(p0, p1) vec3_sub(p1, p0)
#ifndef SQ
#define SQ(x) ((x) * (x))
#endif

#ifndef SWAP
#define SWAP(a, b)                                                             \
  do {                                                                         \
    typeof(a) t = a;                                                           \
    a = b;                                                                     \
    b = t;                                                                     \
  } while (0)
#endif

struct plane_s {
  struct vec3_s n;
  gfloat dist;
};

struct sphere_s {
  struct vec3_s c;
  gfloat radius;
};

struct ray_s {
  struct vec3_s o, d;
};

struct rayseg_s {
  union {
    struct {
      struct vec3_s o;
      struct vec3_s d;
    };
    struct ray_s ray;
  };
  struct vec3_s e;
  gfloat len;
};

gboolean sphere_touches_plane(const struct sphere_s *sphere,
                              const struct plane_s *plane,
                              struct vec3_s *touch_p) {
  gfloat dist = vec3_dot(sphere->c, plane->n) - plane->dist;
  if (fabsf(dist) < sphere->radius) {
    if (touch_p) {
      *touch_p =
          vec3_add(sphere->c, vec3_mul(plane->n, sphere->radius - fabsf(dist)));
    }
    return TRUE;
  }
  return FALSE;
}

// not to be mistaken with ray_sphere_isect() which will return, 0, 1, or 2
// points
gboolean sphere_touches_ray(const struct sphere_s *sphere,
                            const struct ray_s *ray) {
  struct vec3_s v_par, v_perp;
  struct vec3_s v = point_vec(ray->o, sphere->c);
  vec3_ortho_dec(ray->d, v, &v_par, &v_perp);
  gfloat len_sq = vec3_lensq(v_perp);
  return vec3_lensq(v_perp) < SQ(sphere->radius);
}

struct ray_s create_ray(struct vec3_s p0, struct vec3_s p1) {
  struct ray_s ray;
  ray.o = p0;
  ray.d = vec3_norm(point_vec(p0, p1));
  return ray;
}

struct rayseg_s create_rayseg(struct vec3_s p0, struct vec3_s p1) {
  struct rayseg_s seg;
  seg.o = p0;
  seg.e = p1;
  seg.d = point_vec(p0, p1);
  seg.len = vec3_lensq(seg.d);
  if (fabsf(seg.len) < 1e-12f) {
    seg.len = 0.0f;
    seg.d = vec3_zero();
    return seg;
  }
  vec3_mul(seg.d, 1.0f / seg.len);
  return seg;
}

gfloat ray_plane_isect(const struct ray_s *ray, struct plane_s plane) {
  gfloat denom = vec3_dot(ray->d, plane.n);
  if (fabsf(denom) < 1e-6f) {
    return -FLT_MAX; // parallel
  }
  gfloat numer = plane.dist - vec3_dot(ray->o, plane.n);
  return numer / denom; // negative value means intersection is behind ray
}

guint ray_sphere_isect(const struct ray_s *ray, const struct sphere_s *sphere,
                       gfloat *ts) {
  // use the quadratic equation to solve 0, 1 or 2 hits
  // ray := o + td
  // sphere :+ (p - c)^2 = r^2
  // let m := o - c
  // (m + td)^2 = r^2
  // A := dot(d, d)
  // B = 2 * dot(m, d)
  // C = dot(m, m) - r^2
  // disc = B^2 - 4AC

  struct vec3_s m = vec3_sub(ray->o, sphere->c);
  gfloat A =
      vec3_dot(ray->d, ray->d); // A should be one since ray->d is normalized
  gfloat B = 2.0f * vec3_dot(m, ray->d);
  gfloat C = vec3_dot(m, m) - SQ(sphere->radius);
  gfloat disc = SQ(B) - 4.0f * A * C;
  if (disc < 0.0f) {
    return 0; // no intersection
  } else if (fabsf(disc) < 1e-6f) {
    // one intersection (tangent)
    ts[0] = -B / (2.0f * A);
    return 1;
  } else {
    // two intersections
    gfloat inv_2A = 1.0f / (2.0f * A);
    gfloat sqrt_disc = sqrtf(disc);
    ts[0] = (-B - sqrt_disc) * inv_2A;
    ts[1] = (-B + sqrt_disc) * inv_2A;
    if (ts[0] < ts[1]) {
      SWAP(ts[0], ts[1]);
    }
    return 2;
  }
}

gboolean rayseg_plane_isect(const struct rayseg_s *seg, struct plane_s plane,
                            gfloat *t) {
  gfloat t_ = ray_plane_isect(&seg->ray, plane);
  if (t) {
    *t = t_;
  }
  return t_ < 0.0f || t_ > seg->len;
}

struct collider_s {
  guint poly_id;
  struct plane_s plane;
  struct vec3_s centroid;
  gfloat radius;
  guint edge_count;
  struct plane_s *edge_planes;
  gfloat *edge_dists;
  struct vec3_s *ps;
};
