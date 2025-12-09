#include "mesh.h"

#define VERTEX_CHUNK_SIZE 16
#define INDEX_BUFFER_CHUNK_SIZE 64

void init_index_buffer(struct index_buffer_s *buffer, guint chunk_size) {
  buffer->indices = g_new(guint, chunk_size);
  buffer->num_indices = 0;
  buffer->capacity = chunk_size;
  buffer->chunk_size = chunk_size;
}

void free_index_buffer(struct index_buffer_s *buffer) {
  g_free(buffer->indices);
  buffer->indices = NULL;
  buffer->num_indices = 0;
  buffer->capacity = 0;
  buffer->chunk_size = 0;
}

void add_index_to_buffer(struct index_buffer_s *buffer, guint index) {
  if (buffer->num_indices >= buffer->capacity) {
    buffer->capacity += buffer->chunk_size;
    buffer->indices =
        g_realloc(buffer->indices, sizeof(guint) * buffer->capacity);
  }
  buffer->indices[buffer->num_indices++] = index;
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
}

void triangulate_poly(struct poly_s *poly) {
  if (poly->num_vertices < 3) {
    return;
  }
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

gboolean material_eq_fn(gconstpointer a, gconstpointer b) {
  const struct material_s *ma = a;
  const struct material_s *mb = b;

  if (ma == mb)
    return TRUE;

  if (!ma || !mb || !ma->name || !mb->name)
    return FALSE;

  return g_str_equal(ma->name->str, mb->name->str);
}
*/

void free_mat(struct mat_s *mat) {
  if (!mat)
    return;
  g_print("freeing material '%s' (%d polys)\n", mat->name,
          mat->polys->num_indices);
  g_free(mat->name);
  free_index_buffer(mat->polys);
  g_free(mat->polys);
}

struct mat_s *mesh_add_get_material(struct mesh_s *mesh,
                                    const gchar *material_name) {
  if (!mesh->material_map) {
    mesh->material_map = g_hash_table_new_full(
        g_str_hash, (GEqualFunc)g_str_equal, g_free, NULL);
  }
  if (!mesh->mats) {
    mesh->mats = g_ptr_array_new_with_free_func((GDestroyNotify)free_mat);
  }

  gpointer val = g_hash_table_lookup(mesh->material_map, material_name);
  if (val != NULL) {
    return (struct mat_s *)val;
  }

  struct mat_s *stored = g_new(struct mat_s, 1);
  stored->name = g_strdup(material_name);
  stored->polys = g_new(struct index_buffer_s, 1);
  init_index_buffer(stored->polys, INDEX_BUFFER_CHUNK_SIZE);
  g_hash_table_insert(mesh->material_map, g_strdup(material_name), stored);
  g_ptr_array_add(mesh->mats, stored);
  return stored;
}

void init_mesh(struct mesh_s *mesh) {
  mesh->vertex_map =
      g_hash_table_new_full(vertex_hash_fn, vertex_eq_fn, g_free, NULL);
  mesh->vertices = g_array_new(FALSE, FALSE, sizeof(struct vertex_s));
  mesh->polys = g_array_new(FALSE, FALSE, sizeof(struct poly_s));
  mesh->material_map = NULL;
  mesh->mats = NULL;
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

guint mesh_add_get_vertex(struct mesh_s *mesh, struct vec3_s position,
                          struct vec2_s uv) {
  // 1) Look up using stack vertex as probe key
  struct vertex_s v_tmp;
  v_tmp.position = position;
  v_tmp.uv = uv;
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

void free_mesh(struct mesh_s **mesh) {
  g_hash_table_destroy((*mesh)->vertex_map);
  g_array_free((*mesh)->vertices, TRUE);
  for (guint i = 0; i < (*mesh)->polys->len; i++) {
    struct poly_s *poly = &g_array_index((*mesh)->polys, struct poly_s, i);
    g_free(poly->vertices);
    poly->vertices = NULL;
    poly->num_vertices = 0;
  }
  g_hash_table_destroy((*mesh)->material_map);
  if ((*mesh)->mats) {
    g_ptr_array_free((*mesh)->mats, TRUE);
  }
  g_array_free((*mesh)->polys, TRUE);
  g_free(*mesh);
  *mesh = NULL;
}
