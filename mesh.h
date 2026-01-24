#ifndef _MESH_
#define _MESH_

#include "grid_trace.h"
#include "img.h"
#include <glib.h>


#define LIST(type, name)                                                       \
  struct name##list_s {                                                        \
    type *data;                                                                \
    guint len;                                                                 \
    guint capacity;                                                            \
  };

#define LISTOF(name) struct name##list_s

#define LIST_INIT(list, initial_capacity)                                      \
  {                                                                            \
    list->data = g_new(typeof(*list->data), initial_capacity);                 \
    list->len = 0;                                                             \
    list->capacity = initial_capacity;                                         \
  }

#define LIST_APPEND(list, value)                                               \
  do {                                                                         \
    if (list->len >= list->capacity) {                                         \
      list->capacity += list->capacity;                                        \
      list->data =                                                             \
          g_realloc(list->data, sizeof(*(list)->data) * list->capacity);       \
    }                                                                          \
    list->data[list->len++] = value;                                           \
  } while (0);

#define LIST_FREE(list)                                                        \
  do {                                                                         \
    g_free(list->data);                                                        \
    list->data = NULL;                                                         \
    list->len = 0;                                                             \
    list->capacity = 0;                                                        \
  } while (0);

LIST(guint, index);

struct vertex_s {
  struct vec3_s position;
  struct vec2_s uvs[2];
};

gboolean vertex_eq_fn(gconstpointer a, gconstpointer b);
guint vertex_hash_fn(gconstpointer key);

struct tri_s {
  guint v0;
  guint v1;
  guint v2;
};

LIST(struct tri_s, tri);

struct poly_s {
  gint face_id;
  struct vec3_s plane_normal;
  gfloat plane_dist;
  guint num_vertices;
  guint *vertices; // array of guint (indices into mesh->vertices)
  guint num_tris;
  struct tri_s *tris;
};

struct poly_region_s {
  gint x, y, w, h;
  struct vec3_s o, s_axis, t_axis;
  struct vec2_s scale, bias;
};

struct atlas_s {
  guint width;
  guint height;
  struct mat3_s basis;
  guint *id_data;
  struct rgba_s *diffuse_data;
  struct vec3_s *normal_data;
  struct vec3_s *position_data;
  guint num_polys;
  struct poly_region_s *poly_regions;
};

struct vec3_s poly_region_coord_to_3d(const struct poly_region_s *region,
                                      struct ivec2_s co);

struct ivec2_s poly_region_coord_from_3d(const struct poly_region_s *region,
                                         struct vec3_s p);

extern void poly_add_vertex(struct poly_s *poly, guint vertex_index);
extern void triangulate_poly(struct poly_s *poly);

struct texinfo_s {
  gchar name[64];
  guint width;
  guint height;
  struct rgba_s *data;
};

struct mat_s {
  gchar name[64];
  LISTOF(index) * polys;
  LISTOF(tri) * tris;
  guint width, height;
  struct rgba_s *texture_data;
  struct rgba_s avg_color;
};

struct mesh_s {
  GHashTable
      *vertex_map; // key: struct vertex_s*, value: guint (index into vertices)
  GHashTable *material_map;      // key: struct mat_s*, value: guint (index into
                                 // mats)
  GArray *vertices;              // array of struct vertex_s
  GPtrArray *mats;               // array of struct mat_s
  GArray *polys;                 // array of struct poly_s
  struct atlas_s *texture_atlas; // texture atlas for lightmaps
};

extern void init_mesh(struct mesh_s *mesh);
extern struct poly_s *mesh_add_poly(struct mesh_s *mesh,
                                    const gchar *material_name);
extern guint mesh_add_get_vertex(struct mesh_s *mesh, struct vec3_s position,
                                 struct vec2_s uv, struct vec2_s uv2);
extern void build_mesh(struct mesh_s *mesh, const struct texinfo_s *texinfos,
                       guint num_texinfos, guint atlas_width,
                       guint atlas_height);

extern void free_mesh(struct mesh_s **mesh);

extern void export_mesh_with_mats_to_obj(struct mesh_s *mesh, gfloat scale);

extern void create_mesh_g_buffer(struct mesh_s *mesh);

struct ray_s {
  struct vec3_s o;
  struct vec3_s d;
};

extern void ray_ortho_decomp(const struct ray_s *ray, struct vec3_s p,
                             struct vec3_s *v_par, struct vec3_s *v_perp);

gboolean ray_sphere_touch(const struct ray_s *ray, const struct vec3_s origin,
                          gfloat radius);

struct collider_s {
  guint poly_id;
  struct vec3_s face_normal;
  gfloat face_dist;
  struct vec3_s centroid;
  gfloat radius;
  guint edge_count;
  struct vec3_s *edge_normals;
  gfloat *edge_dists;
  struct vec3_s *ps;
};

struct collision_partition_s {
  struct vec3_s min;
  struct vec3_s max;
  struct vec3_s origin;
  gfloat radius;
  LISTOF(index) * colliders;
};

struct collision_partitions_8x8x8_s {
  struct vec3_s min;
  struct vec3_s max;
  struct vec3_s origin;
  struct collision_partition_s cells[8][8][8];
};

struct collision_partitions_8x8x8_sweeper_s {
  const struct collider_s *colliders;
  guint num_colliders;
  guint test_val;
  guint *test_masks;
  LISTOF(index) * test_indices;
};

extern void init_collider(struct collider_s *collider,
                          const struct poly_s *poly, const GArray *vertices);
extern gboolean sweep_collision(const struct collider_s *collider,
                                struct vec3_s p0, struct vec3_s p1,
                                struct vec3_s *hit_p);
extern void free_collider(struct collider_s *collider);

extern void export_colliders_to_obj(const struct collider_s *collider,
                                    guint num_colliders, const gchar *filename);

extern void
create_collision_partitions(struct collision_partitions_8x8x8_s *partitions,
                            struct collider_s *colliders, guint num_colliders);

extern void
free_collision_partitions(struct collision_partitions_8x8x8_s *partitions);

extern gboolean
sweep_collision_partitions(struct collision_partitions_8x8x8_s *partitions,
                           struct collision_partitions_8x8x8_sweeper_s *sweeper,
                           struct vec3_s p0, struct vec3_s p1, guint ignore_id);

#endif // _MESH_
