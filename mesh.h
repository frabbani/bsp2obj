#ifndef _MESH_
#define _MESH_

#include "vec.h"
#include <glib.h>

struct index_buffer_s {
  guint *indices;
  guint num_indices;
  guint capacity;
};

void init_index_buffer(struct index_buffer_s *buffer);
void free_index_buffer(struct index_buffer_s *buffer);
void add_index_to_buffer(struct index_buffer_s *buffer, guint index);

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

struct trilist_s {
  guint num_tris;
  struct tri_s *tris;
};

struct poly_s {
  gint face_id;
  guint num_vertices;
  guint *vertices; // array of guint (indices into mesh->vertices)
  guint num_tris;
  struct tri_s *tris;
};

extern void poly_add_vertex(struct poly_s *poly, guint vertex_index);
extern void init_poly(struct poly_s *poly, gint face_id);
extern void triangulate_poly(struct poly_s *poly);
extern void free_poly(struct poly_s *poly);

struct mat_s {
  gchar *name;
  struct index_buffer_s *polys;
  struct trilist_s tris;
};

// guint material_hash_fn(gconstpointer key);
// gboolean material_eq_fn(gconstpointer a, gconstpointer b);

void free_mat(struct mat_s *mat);

struct mesh_s {
  GHashTable
      *vertex_map; // key: struct vertex_s*, value: guint (index into vertices)
  GHashTable *material_map; // key: struct mat_s*, value: guint (index into
                            // mats)
  GArray *vertices;         // array of struct vertex_s
  GPtrArray *mats;          // array of struct mat_s
  GArray *polys;            // array of struct poly_s
};

extern void init_mesh(struct mesh_s *mesh);
extern struct poly_s *mesh_add_poly(struct mesh_s *mesh,
                                    const gchar *material_name);
extern guint mesh_add_get_vertex(struct mesh_s *mesh, struct vec3_s position,
                                 struct vec2_s uv, struct vec2_s uv2);
extern void build_mesh(struct mesh_s *mesh);

extern void free_mesh(struct mesh_s **mesh);

extern void export_mesh_with_mats_to_obj(struct mesh_s *mesh, gfloat scale);

#endif // _MESH_
