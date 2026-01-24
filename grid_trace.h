/* Auto-generated public header for grid_trace.
 * Inlines core public headers. Do not modify!
 */

#ifndef GRID_TRACE_H
#define GRID_TRACE_H

// clang-format off

/* ---------------- defs.h ---------------- */
#include <float.h>
#include <stdbool.h>
#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef unsigned int uint;
typedef void (*GridTr_dtor_func)(void *);
typedef void (*GridTr_move_func)(void *, void *);

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef CLAMP
#define CLAMP(x, a, b) (MAX(a, MIN(x, b)))
#endif

#ifndef SQ
#define SQ(x) ((x) * (x))
#endif

#ifndef SWAP
#define SWAP(a, b)                                                             \
  do {                                                                         \
    __typeof__(a) t = a;                                                       \
    a = b;                                                                     \
    b = t;                                                                     \
  } while (0)
#endif

#define PI 3.14159265358979323846f
#define DEG2RAD(x) ((x) * PI / 180.0f)
#define RAD2DEG(x) ((x) * 180.0f / PI)

#define TOL 1e-6f
#define TOL_SQ 1e-12f


#define PTR_SZ (sizeof(void *))
#define GridTr_new(size)  GridTr_allocmem(size, __FILE__, __LINE__)
#define GridTr_free(ptr) do{ if(ptr){GridTr_freemem(ptr); ptr = NULL; } } while(0)
#define GridTr_oftype(t) #t

// clang-format on

// thread safe!!!
extern void *GridTr_allocmem(size_t size, const char *file, int line);
extern void GridTr_freemem(void *ptr);
extern void GridTr_prmemstats(void);

/* ---------------- vecdefs.h ---------------- */
struct ivec2_s {
  int x;
  int y;
};
struct ivec3_s {
  union {
    struct {
      int x;
      int y;
      int z;
    };
    int xyz[3];
  };
};
struct vec2_s {
  union {
    struct {
      float x;
      float y;
    };
    float xy[2];
  };
};
struct vec3_s {
  union {
    struct {
      float x;
      float y;
      float z;
    };
    float xyz[3];
  };
};
struct mat3_s {
  float es[3][3];
};

/* ---------------- vec.h prototypes ---------------- */
struct ivec3_s ivec3_set(int x, int y, int z);
uint64 ivec3_fnv1a(struct ivec3_s v);
struct ivec3_s ivec3_min(struct ivec3_s a, struct ivec3_s b);
struct ivec3_s ivec3_max(struct ivec3_s a, struct ivec3_s b);
struct ivec3_s ivec3_add(struct ivec3_s a, struct ivec3_s b);
struct ivec3_s ivec3_sub(struct ivec3_s a, struct ivec3_s b);

struct vec2_s vec2_set(float x, float y);

float vec3_dot(const struct vec3_s a, const struct vec3_s b);
float vec3_len(const struct vec3_s a);
float vec3_lensq(const struct vec3_s a);

struct vec3_s vec3_set(float x, float y, float z);
struct vec3_s vec3_zero();
struct vec3_s vec3_max(const struct vec3_s a, const struct vec3_s b);
struct vec3_s vec3_min(const struct vec3_s a, const struct vec3_s b);
struct vec3_s vec3_mul(const struct vec3_s a, float s);
struct vec3_s vec3_add(const struct vec3_s a, const struct vec3_s b);
struct vec3_s vec3_sub(const struct vec3_s a, const struct vec3_s b);
struct vec3_s vec3_cross(const struct vec3_s a, const struct vec3_s b);
struct vec3_s vec3_norm(const struct vec3_s a);
struct vec3_s vec3_lerp(const struct vec3_s a, const struct vec3_s b, float t);

struct mat3_s mat3_ident();
struct mat3_s mat3_transp(const struct mat3_s m);
struct mat3_s mat3_rot(struct vec3_s angles_rad);
struct vec3_s vec3_transf(const struct mat3_s m, const struct vec3_s v);

extern void vec3_ortho_dec(struct vec3_s d, struct vec3_s v,
                           struct vec3_s *v_par, struct vec3_s *v_perp);

/* ---------------- geom.h ---------------- */
#define point_vec(p0, p1) vec3_sub(p1, p0)
struct GridTr_plane_s {
  struct vec3_s n;
  float dist;
};
struct GridTr_plane_s GridTr_create_plane(struct vec3_s n, struct vec3_s p);
#define eval_plane(pl, p) (vec3_dot(p, pl.n) - pl.dist)
struct GridTr_sphere_s {
  struct vec3_s c;
  float radius;
};
struct GridTr_ray_s {
  struct vec3_s o, d;
};
struct GridTr_rayseg_s {
  union {
    struct {
      struct vec3_s o;
      struct vec3_s d;
    };
    struct GridTr_ray_s ray;
  };
  struct vec3_s e;
  float len;
};
struct GridTr_aabb_s {
  struct vec3_s min;
  struct vec3_s max;
  struct vec3_s halfsize;
  struct vec3_s o;
  float radius;
};
struct GridTr_ray_s GridTr_create_ray(struct vec3_s p0, struct vec3_s p1);
struct GridTr_rayseg_s GridTr_create_rayseg(struct vec3_s p0, struct vec3_s p1);
bool GridTr_sphere_touches_plane(const struct GridTr_sphere_s *sphere,
                                 const struct GridTr_plane_s *plane,
                                 struct vec3_s *touch_p);
bool GridTr_sphere_touches_ray(const struct GridTr_ray_s *ray,
                               const struct GridTr_sphere_s *sphere);
float GridTr_ray_isect_plane(const struct GridTr_ray_s *ray,
                             struct GridTr_plane_s plane);
uint GridTr_ray_isect_sphere(const struct GridTr_ray_s *ray,
                             const struct GridTr_sphere_s *sphere, float *ts);
bool GridTr_rayseg_isect_plane(const struct GridTr_rayseg_s *seg,
                               struct GridTr_plane_s plane, float *t);
void GridTr_find_exts(const struct vec3_s *ps, uint nps, struct vec3_s *min,
                      struct vec3_s *max);
void GridTr_aabb_init(struct GridTr_aabb_s *aabb, struct vec3_s min,
                      struct vec3_s max);
void GridTr_aabb_from_ps(struct GridTr_aabb_s *aabb, const struct vec3_s *ps,
                         uint num_ps);
bool GridTr_aabb_clip_ray(const struct GridTr_aabb_s *aabb,
                          const struct GridTr_ray_s *ray, float *ts);
bool GridTr_aabb_touches_aabb(const struct GridTr_aabb_s *aabb0,
                              const struct GridTr_aabb_s *aabb1);

/* ---------------- array.h ---------------- */
struct GridTr_array_s {
  void *data;
  uint32 num_elems;
  uint32 elem_size;
  uint32 max_elems;
  uint32 grow;
  const char *file;
  int line;
  const char *oftype;
};
struct GridTr_array_s *GridTr_create_array_(uint32 elem_size,
                                            uint32 max_prelim_elems,
                                            uint32 grow, const char *file,
                                            int line);
void GridTr_clear_array(struct GridTr_array_s *array);
void GridTr_destroy_array(struct GridTr_array_s **array);
void GridTr_array_add(struct GridTr_array_s *array, const void *elem);
void *GridTr_array_get(struct GridTr_array_s *array, uint32 index);
void GridTr_destroy_array_dtor(struct GridTr_array_s **array,
                               GridTr_dtor_func dtor);
const void *GridTr_array_get_ro(const struct GridTr_array_s *array,
                                uint32 index);
void GridTr_array_swap_free(struct GridTr_array_s *array, uint32 index);
void GridTr_array_swap_free_dtor(struct GridTr_array_s *array, uint32 index,
                                 GridTr_dtor_func dtor);
#define GridTr_create_array(t, max, grow)                                      \
  GridTr_create_array_(t, max, grow, __FILE__, __LINE__)

/* ---------------- hash.h ---------------- */
#define GridTr_HASH_TABLE_LOAD_FACTOR 0.7
uint64 GridTr_hash_str_fnv1a(const char *s);
struct GridTr_hash_table_entry_s {
  union {
    void *data;
    uint8 *bytes;
  };
  uint64 hash;
};
struct GridTr_hash_table_s {
  struct GridTr_array_s **entries;
  uint size;
  uint total_elems;
  GridTr_dtor_func data_dtor;
};
struct GridTr_hash_table_s *
GridTr_create_hash_table(uint initial_size, GridTr_dtor_func data_dtor);
void GridTr_destroy_hash_table(struct GridTr_hash_table_s **tabler);
void GridTr_rehash_hash_table(struct GridTr_hash_table_s *table);
void **GridTr_hash_table_add_or_get(struct GridTr_hash_table_s *table,
                                    uint64 hash);
bool GridTr_hash_table_find(const struct GridTr_hash_table_s *table,
                            uint64 hash);
void **GridTr_hash_table_maybe_get(struct GridTr_hash_table_s *table,
                                   uint64 hash);
const void *
GridTr_hash_table_maybe_get_ro(const struct GridTr_hash_table_s *table,
                               uint64 hash);
const void **
GridTr_hash_table_get_all_ro(const struct GridTr_hash_table_s *table,
                             uint32 *num_elems);
bool GridTr_hash_table_free(struct GridTr_hash_table_s *table, uint64 hash);

/* ---------------- collide.h ---------------- */
struct GridTr_sat_s {
  struct vec3_s d;
  struct vec2_s min_maxs[2];
};
bool GridTr_sat_olap(const struct GridTr_sat_s *sat);
void GridTr_sat_setps(struct GridTr_sat_s *sat, const struct vec3_s *ps,
                      uint num_ps, bool first);
void GridTr_sat_setr(struct GridTr_sat_s *sat, struct vec3_s c, float radius,
                     bool first);
void GridTr_sat_setas(struct GridTr_sat_s *sat, struct vec3_s o,
                      const struct vec3_s *axes, struct vec3_s half_size,
                      bool first);
struct GridTr_collider_s {
  uint32 poly_id;
  struct GridTr_plane_s plane;
  struct vec3_s o;
  float radius;
  uint32 edge_count;
  struct GridTr_plane_s *edge_planes;
  float *edge_lens;
  struct vec3_s *ps;
  struct vec3_s *es;
};
void GridTr_create_collider(struct GridTr_collider_s *collider, uint32 id,
                            const struct vec3_s *ps, uint32 nps,
                            struct GridTr_plane_s plane);
void GridTr_destroy_collider(struct GridTr_collider_s *collider);
bool GridTr_collider_touches_aabb(const struct GridTr_collider_s *collider,
                                  const struct GridTr_aabb_s *aabb);
void GridTr_copy_collider(struct GridTr_collider_s *to,
                          const struct GridTr_collider_s *from);
void GridTr_collider_dtor(void *ptr);
bool GridTr_load_colliders_from_obj(struct GridTr_collider_s **colliders,
                                    uint32 *num_colliders,
                                    const char *filename);

/* ---------------- grid.h ---------------- */
struct GridTr_grid_cell_s {
  struct ivec3_s crl;
  uint64 hash;
  uint32 num_colliders;
  uint32 *colliders;
  uint32 _max_colliders_;
  struct GridTr_aabb_s aabb;
};
struct GridTr_grid_s {
  struct GridTr_hash_table_s *cell_table;
  struct GridTr_array_s *colliders;
  uint32 cell_size;
  struct GridTr_aabb_s aabb;
};
void GridTr_grid_cell_dtor(void *ptr);
struct ivec3_s GridTr_get_grid_cell_for_p(struct vec3_s p, float cell_size);
void GridTr_get_exts_for_grid_cell(struct ivec3_s crl, float cell_size,
                                   struct vec3_s *min, struct vec3_s *max);
void GridTr_get_aabb_for_grid_cell(struct ivec3_s crl, float cell_size,
                                   struct GridTr_aabb_s *aabb);
void GridTr_get_collider_grid_cell_exts(
    const struct GridTr_collider_s *collider, float cell_size,
    struct ivec3_s *crl_min, struct ivec3_s *crl_max, bool bloat);
void GridTr_create_grid(struct GridTr_grid_s *grid, float cell_size);
struct GridTr_grid_cell_s *GridTr_grid_get_grid_cell(struct GridTr_grid_s *grid,
                                                     struct ivec3_s crl);
void GridTr_destroy_grid(struct GridTr_grid_s *grid);
const struct GridTr_grid_cell_s *
GridTr_grid_get_grid_cell_ro(const struct GridTr_grid_s *grid,
                             struct ivec3_s crl);
void GridTr_add_collider_to_grid(struct GridTr_grid_s *grid,
                                 const struct GridTr_collider_s *collider);
void GridTr_get_colliders_for_cell(const struct GridTr_grid_s *grid,
                                   struct ivec3_s crl, const uint32 *indices,
                                   uint *num_indices,
                                   const struct GridTr_collider_s *colliders);
const void **GridTr_grid_get_all_grid_cells(const struct GridTr_grid_s *grid,
                                            uint32 *num_cells);
typedef bool (*GridTr_trace_cb)(const struct GridTr_grid_cell_s *cell,
                                struct ivec3_s crl,
                                const struct GridTr_rayseg_s *rayseg,
                                const struct GridTr_collider_s *colliders,
                                void *user_data);
bool GridTr_trace_ray_through_grid(const struct GridTr_grid_s *grid,
                                   const struct GridTr_rayseg_s *rayseg,
                                   GridTr_trace_cb cb, void *user_data);

/* ---------------- export.h ---------------- */
struct GridTr_shape_s {
  struct vec3_s *ps;
  struct ivec3_s *faces;
  uint32 num_ps, num_faces;
};
void GridTr_load_shape_from_obj(struct GridTr_shape_s *shape,
                                const char *filename);
char *GridTr_export_shape_to_obj_str(const struct GridTr_shape_s *shape,
                                     struct vec3_s o, float scale,
                                     uint32 starting_vertex);
void GridTr_free_shape(struct GridTr_shape_s *shape);
void GridTr_export_grid_boxes_to_obj(const struct GridTr_grid_s *grid,
                                     const char *filename);
bool GridTr_load_colliders_from_obj(struct GridTr_collider_s **colliders,
                                    uint32 *num_colliders,
                                    const char *filename);

#endif /* GRID_TRACE_H */
