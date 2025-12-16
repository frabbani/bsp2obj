#ifndef _MESH_
#define _MESH_

#include "vec.h"
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

struct rgb_s {
  union {
    struct {
      guint8 b;
      guint8 g;
      guint8 r;
    };
    guint8 rgb[3];
  };
};

struct rgba_s {
  union {
    struct {
      guint8 b;
      guint8 g;
      guint8 r;
      guint8 a;
    };
    guint8 rgba[4];
  };
};

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
  guint num_vertices;
  guint *vertices; // array of guint (indices into mesh->vertices)
  guint num_tris;
  struct tri_s *tris;
};

extern void poly_add_vertex(struct poly_s *poly, guint vertex_index);
extern void init_poly(struct poly_s *poly, gint face_id);
extern void triangulate_poly(struct poly_s *poly);
extern void free_poly(struct poly_s *poly);

struct texinfo_s {
  gchar name[64];
  guint width;
  guint height;
};

struct mat_s {
  gchar name[64];
  LISTOF(index) * polys;
  LISTOF(tri) * tris;
  guint width, height;
};

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
extern void build_mesh(struct mesh_s *mesh, const struct texinfo_s *texinfos,
                       guint num_texinfos);

extern void free_mesh(struct mesh_s **mesh);

extern void export_mesh_with_mats_to_obj(struct mesh_s *mesh, gfloat scale);

#endif // _MESH_
