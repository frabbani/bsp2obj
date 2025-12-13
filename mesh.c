#include "mesh.h"
#include <math.h>

#define VERTEX_CHUNK_SIZE 16
#define INDEX_BUFFER_CHUNK_SIZE 16

#define SCALE 10000.0f

// quantize float to int
static inline gint qf(gfloat x) { return (gint)lrintf(x * SCALE); }

void init_index_buffer(struct index_buffer_s *buffer) {
  buffer->indices = g_new(guint, INDEX_BUFFER_CHUNK_SIZE);
  buffer->num_indices = 0;
  buffer->capacity = INDEX_BUFFER_CHUNK_SIZE;
}

void free_index_buffer(struct index_buffer_s *buffer) {
  g_free(buffer->indices);
  buffer->indices = NULL;
  buffer->num_indices = 0;
  buffer->capacity = 0;
}

void add_index_to_buffer(struct index_buffer_s *buffer, guint index) {
  if (buffer->num_indices >= buffer->capacity) {
    buffer->capacity += buffer->capacity;
    buffer->indices =
        g_realloc(buffer->indices, sizeof(guint) * buffer->capacity);
  }
  buffer->indices[buffer->num_indices++] = index;
}

gboolean vertex_eq_fn(gconstpointer a, gconstpointer b) {
  const struct vertex_s *va = (const struct vertex_s *)a;
  const struct vertex_s *vb = (const struct vertex_s *)b;
  gint a_u1 = qf(va->uvs[0].x);
  gint a_v1 = qf(va->uvs[0].y);
  gint a_u2 = qf(va->uvs[1].x);
  gint a_v2 = qf(va->uvs[1].y);
  gint a_x = qf(va->position.x);
  gint a_y = qf(va->position.y);
  gint a_z = qf(va->position.z);

  gint b_u1 = qf(vb->uvs[0].x);
  gint b_v1 = qf(vb->uvs[0].y);
  gint b_u2 = qf(vb->uvs[1].x);
  gint b_v2 = qf(vb->uvs[1].y);
  gint b_x = qf(vb->position.x);
  gint b_y = qf(vb->position.y);
  gint b_z = qf(vb->position.z);

  gboolean uv1_equal = (a_u1 == b_u1) && (a_v1 == b_v1);
  gboolean uv2_equal = (a_u2 == b_u2) && (a_v2 == b_v2);
  gboolean pos_equal = (a_x == b_x) && (a_y == b_y) && (a_z == b_z);

  return pos_equal && uv1_equal && uv2_equal;
}

guint vertex_hash_fn(gconstpointer key) {
  const struct vertex_s *v = key;
  guint h = 2166136261u;
#define MIX(i)                                                                 \
  do {                                                                         \
    h ^= (guint)(i);                                                           \
    h *= 16777619u;                                                            \
  } while (0)

  MIX(qf(v->position.x));
  MIX(qf(v->position.y));
  MIX(qf(v->position.z));
  MIX(qf(v->uvs[0].x));
  MIX(qf(v->uvs[0].y));
  // MIX(qf(v->uvs[1].x));
  // MIX(qf(v->uvs[1].y));
#undef MIX
  return h;
}

void poly_add_vertex(struct poly_s *poly, guint vertex_index) {
  if (poly->num_vertices % VERTEX_CHUNK_SIZE == 0) {
    poly->vertices =
        g_realloc(poly->vertices,
                  sizeof(guint) * (poly->num_vertices + VERTEX_CHUNK_SIZE));
  }
  poly->vertices[poly->num_vertices++] = vertex_index;
}

void init_poly(struct poly_s *poly, gint face_id) {
  poly->face_id = face_id;
  poly->num_vertices = 0;
  poly->vertices = NULL;
  poly->num_tris = 0;
  poly->tris = NULL;
}

void triangulate_poly(struct poly_s *poly) {
  if (poly->num_vertices < 3) {
    return;
  }
  g_free(poly->tris);
  poly->num_tris = poly->num_vertices - 2;
  poly->tris = g_new(struct tri_s, poly->num_tris);
  for (guint i = 0; i < poly->num_tris; i++) {
    poly->tris[i].v0 = poly->vertices[0];
    poly->tris[i].v1 = poly->vertices[i + 1];
    poly->tris[i].v2 = poly->vertices[i + 2];
  }
}

void free_poly(struct poly_s *poly) {
  g_free(poly->vertices);
  poly->vertices = NULL;
  poly->num_vertices = 0;
  g_free(poly->tris);
  poly->tris = NULL;
  poly->num_tris = 0;
}

/*
guint material_hash_fn(gconstpointer key) {
  const struct material_s *mat = key;

  // Defensive: allow NULL names (hash to 0)
  if (!mat || !mat->name || !mat->name->str)
    return 0;

  return g_str_hash(mat->name->str);
}
*/
gboolean mat_eq_fn(gconstpointer a, gconstpointer b) {
  const struct mat_s *ma = a;
  const struct mat_s *mb = b;

  if (ma == mb)
    return TRUE;

  if (!ma || !mb || !ma->name || !mb->name)
    return FALSE;

  return g_str_equal(ma->name, mb->name);
}

void free_mat(struct mat_s *mat) {
  if (!mat)
    return;
  g_print("freeing material '%s' - %d polys (reserved %d), %d triangles\n",
          mat->name, mat->polys->num_indices, mat->polys->capacity,
          mat->tris.num_tris);
  g_free(mat->name);
  free_index_buffer(mat->polys);
  g_free(mat->polys);
  mat->tris.num_tris = 0;
  g_free(mat->tris.tris);
  mat->tris.tris = NULL;
}

struct mat_s *mesh_add_get_material(struct mesh_s *mesh,
                                    const gchar *material_name) {
  gpointer val = g_hash_table_lookup(mesh->material_map, material_name);
  if (val != NULL) {
    return (struct mat_s *)val;
  }

  struct mat_s *stored = g_new(struct mat_s, 1);
  stored->name = g_strdup(material_name);
  stored->polys = g_new(struct index_buffer_s, 1);
  init_index_buffer(stored->polys);
  g_hash_table_insert(mesh->material_map, g_strdup(material_name), stored);
  g_ptr_array_add(mesh->mats, stored);
  return stored;
}

void init_mesh(struct mesh_s *mesh) {
  mesh->vertex_map =
      g_hash_table_new_full(vertex_hash_fn, vertex_eq_fn, g_free, NULL);
  mesh->vertices = g_array_new(FALSE, FALSE, sizeof(struct vertex_s));
  mesh->polys = g_array_new(FALSE, FALSE, sizeof(struct poly_s));
  mesh->material_map =
      g_hash_table_new_full(g_str_hash, (GEqualFunc)g_str_equal, g_free, NULL);
  mesh->mats = g_ptr_array_new_with_free_func((GDestroyNotify)free_mat);
}

struct poly_s *mesh_add_poly(struct mesh_s *mesh, const gchar *material_name) {
  struct poly_s poly;
  init_poly(&poly, mesh->polys->len);
  g_array_append_val(mesh->polys, poly);
  guint index = mesh->polys->len - 1;
  struct mat_s *mat = mesh_add_get_material(mesh, material_name);
  add_index_to_buffer(mat->polys, index);
  return &g_array_index(mesh->polys, struct poly_s, mesh->polys->len - 1);
}

// IMPORTANT: Vertices with lightmap UVs will not dedup because lightmap UVs
// differ across faces
guint mesh_add_get_vertex(struct mesh_s *mesh, struct vec3_s position,
                          struct vec2_s uv, struct vec2_s uv2) {
  // 1) Look up using stack vertex as probe key
  struct vertex_s v_tmp;
  v_tmp.position = position;
  v_tmp.uvs[0] = uv;
  v_tmp.uvs[1] = uv2;
  gpointer val = g_hash_table_lookup(mesh->vertex_map, &v_tmp);
  if (val != NULL) {
    // Already present
    return GPOINTER_TO_UINT(val);
  }

  // 2) New vertex: index is current vertex array size
  guint index = mesh->vertices->len;

  // Append to array
  struct vertex_s *stored = g_new(struct vertex_s, 1);
  *stored = v_tmp;
  g_array_append_val(mesh->vertices, *stored);

  // 3) Store a heap key in the hash table
  g_hash_table_insert(mesh->vertex_map, stored, GUINT_TO_POINTER(index));

  return index;
}

void build_mesh(struct mesh_s *mesh) {
  g_ptr_array_sort(mesh->mats, (GCompareFunc)mat_eq_fn);
  for (guint i = 0; i < mesh->mats->len; i++) {
    struct mat_s *mat = g_ptr_array_index(mesh->mats, i);
    guint num_tris = 0;
    for (guint j = 0; j < mat->polys->num_indices; j++) {
      guint poly_idx = mat->polys->indices[j];
      struct poly_s *poly =
          &g_array_index(mesh->polys, struct poly_s, poly_idx);
      if (0 == poly->num_tris) {
        triangulate_poly(poly);
      }
      num_tris += poly->num_tris;
    }
    mat->tris.num_tris = num_tris;
    mat->tris.tris = g_new(struct tri_s, num_tris);
    num_tris = 0;
    for (guint j = 0; j < mat->polys->num_indices; j++) {
      guint poly_idx = mat->polys->indices[j];
      struct poly_s *poly =
          &g_array_index(mesh->polys, struct poly_s, poly_idx);
      for (guint k = 0; k < poly->num_tris; k++) {
        mat->tris.tris[num_tris++] = poly->tris[k];
      }
    }
  }
}

void free_mesh(struct mesh_s **mesh) {
  g_hash_table_destroy((*mesh)->vertex_map);
  g_array_free((*mesh)->vertices, TRUE);
  for (guint i = 0; i < (*mesh)->polys->len; i++) {
    struct poly_s *poly = &g_array_index((*mesh)->polys, struct poly_s, i);
    free_poly(poly);
  }
  g_array_free((*mesh)->polys, TRUE);
  g_hash_table_destroy((*mesh)->material_map);
  g_ptr_array_free((*mesh)->mats, TRUE);
  g_free(*mesh);
  *mesh = NULL;
}

void export_mesh_with_mats_to_obj(struct mesh_s *mesh, gfloat scale) {
  GString *obj = g_string_new(NULL);
  /*
  newmtl lightmap
  Ka 1 1 1
  Kd 1 1 1
  Ks 0 0 0
  Tr 1
  illum 1
  Ns 0
  map_Kd export/textures/name.png
  */

  // Write material
  for (guint i = 0; i < mesh->mats->len; i++) {
    struct mat_s *mat = g_ptr_array_index(mesh->mats, i);
    g_string_append_printf(obj, "newmtl %s\n", mat->name);
    g_string_append(obj, "Ka 1 1 1\n");
    g_string_append(obj, "Kd 1 1 1\n");
    g_string_append(obj, "Ks 0 0 0\n");
    g_string_append(obj, "Tr 1\n");
    g_string_append(obj, "illum 1\n");
    g_string_append(obj, "Ns 0\n");
    g_string_append_printf(obj, "map_Kd export/textures/%s.png\n", mat->name);
  }
  g_file_set_contents("mesh.mtl", obj->str, obj->len, NULL);
  g_string_free(obj, TRUE);

  // Wrote model
  obj = g_string_new(NULL);
  g_string_append(obj, "mtllib mesh.mtl\n");
  g_string_append(obj, "usemtl mesh\n");

  // Write vertices
  for (guint i = 0; i < mesh->vertices->len; i++) {
    struct vertex_s *v = &g_array_index(mesh->vertices, struct vertex_s, i);
    g_string_append_printf(obj, "v %g %g %g\n", v->position.x * scale,
                           v->position.y * scale, v->position.z * scale);
  }

  // Write texture coordinates
  for (guint i = 0; i < mesh->vertices->len; i++) {
    struct vertex_s *v = &g_array_index(mesh->vertices, struct vertex_s, i);
    g_string_append_printf(obj, "vt %g %g\n", v->uvs[0].x, v->uvs[0].y);
  }

  for (guint i = 0; i < mesh->mats->len; i++) {
    struct mat_s *mat = g_ptr_array_index(mesh->mats, i);
    g_string_append_printf(obj, "usemtl %s\n", mat->name);

    // Write faces
    for (guint j = 0; j < mat->polys->num_indices; j++) {
      guint poly_idx = mat->polys->indices[j];
      struct poly_s *poly =
          &g_array_index(mesh->polys, struct poly_s, poly_idx);
      for (guint k = 0; k < poly->num_tris; k++) {
        struct tri_s *tri = &poly->tris[k];
        g_string_append(obj, "f");
        g_string_append_printf(obj, " %u/%u", tri->v0 + 1, tri->v0 + 1);
        g_string_append_printf(obj, " %u/%u", tri->v1 + 1, tri->v1 + 1);
        g_string_append_printf(obj, " %u/%u", tri->v2 + 1, tri->v2 + 1);
        g_string_append_c(obj, '\n');
      }
    }
  }
  g_file_set_contents("mesh.obj", obj->str, obj->len, NULL);
  g_string_free(obj, TRUE);
}