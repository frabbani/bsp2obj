#include "mesh.h"
#include "clock.h"
#include "lodepng.h"
#include <math.h>

#define VERTEX_CHUNK_SIZE 16
#define INDEX_BUFFER_CHUNK_SIZE 16

#define SCALE 10000.0f

// quantize float to int
static inline gint qf(gfloat x) { return (gint)lrintf(x * SCALE); }

/*
void init_index_list(struct indexlist_s *list) {
  list->data = g_new(guint, INDEX_BUFFER_CHUNK_SIZE);
  list->num_indices = 0;
  list->capacity = INDEX_BUFFER_CHUNK_SIZE;
}

void free_index_list(struct indexlist_s *list) {
  g_free(list->indices);
  list->indices = NULL;
  list->num_indices = 0;
  list->capacity = 0;
}

void add_index_to_list(struct indexlist_s *list, guint index) {
  if (list->num_indices >= list->capacity) {
    list->capacity += list->capacity;
    list->indices = g_realloc(list->indices, sizeof(guint) * list->capacity);
  }
  list->indices[list->num_indices++] = index;
}
*/

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
  poly->plane_normal = vec3_set(0.0f, 0.0f, 0.0f);
  poly->plane_dist = 0.0f;
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

static void rotate_poly(struct poly_s *poly,
                        const struct mat3_s rotation_matrix) {
  // vertices are already rotated, rotate the plane normal
  poly->plane_normal = vec3_transf(rotation_matrix, poly->plane_normal);
}

static void rotate_poly_region(struct poly_region_s *region,
                               const struct mat3_s rotation_matrix) {
  region->o = vec3_transf(rotation_matrix, region->o);
  region->s_axis = vec3_transf(rotation_matrix, region->s_axis);
  region->t_axis = vec3_transf(rotation_matrix, region->t_axis);
}

struct vec3_s poly_region_coord_to_3d(const struct poly_region_s *region,
                                      struct ivec2_s co) {
  gfloat u = ((gfloat)(co.x) + 0.5f) / (gfloat)(region->w);
  gfloat v = ((gfloat)(co.y) + 0.5f) / (gfloat)(region->h);

  gfloat S = region->scale.x * u + region->bias.x;
  gfloat T = region->scale.y * v + region->bias.y;
  struct vec3_s s = vec3_mul(region->s_axis, S);
  struct vec3_s t = vec3_mul(region->t_axis, T);
  return vec3_add(vec3_add(region->o, s), t);
}

struct ivec2_s poly_region_coord_from_3d(const struct poly_region_s *region,
                                         struct vec3_s p) {
  struct vec3_s v = vec3_sub(p, region->o);
  gfloat S = vec3_dot(v, region->s_axis);
  gfloat T = vec3_dot(v, region->t_axis);
  float U = (S - region->bias.x) / region->scale.x;
  float V = (T - region->bias.y) / region->scale.y;
  return (struct ivec2_s){(gint)(U * region->w - 0.5f),
                          (gint)(V * region->h - 0.5f)};
}

gint mat_cmp_fn(gconstpointer a, gconstpointer b) {
  const struct mat_s *ma = *(const struct mat_s *const *)a;
  const struct mat_s *mb = *(const struct mat_s *const *)b;
  if (ma == mb)
    return 0;
  return g_strcmp0(ma->name, mb->name);
}

void free_mat(struct mat_s *mat) {
  if (!mat)
    return;
  g_print("freeing material '%s' - %d polys (reserved %d), %d triangles\n",
          mat->name, mat->polys->len, mat->polys->capacity, mat->tris->len);
  memset(mat->name, 0, sizeof(mat->name));
  LIST_FREE(mat->polys);
  g_free(mat->polys);
  LIST_FREE(mat->tris);
  g_free(mat->tris);
  g_free(mat->texture_data);
  mat->tris = NULL;
}

struct mat_s *mesh_add_get_material(struct mesh_s *mesh,
                                    const gchar *material_name) {
  gpointer val = g_hash_table_lookup(mesh->material_map, material_name);
  if (val != NULL) {
    return (struct mat_s *)val;
  }
  struct mat_s *stored = g_new(struct mat_s, 1);
  g_strlcpy(stored->name, material_name, sizeof(stored->name) - 1);
  stored->polys = g_new(LISTOF(index), 1);
  LIST_INIT(stored->polys, 32);
  stored->texture_data = NULL;
  stored->tris = NULL;
  g_hash_table_insert(mesh->material_map, g_strdup(material_name), stored);
  g_ptr_array_add(mesh->mats, stored);
  g_print("adding material '%s @ %u'\n", stored->name, mesh->mats->len - 1);
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
  mesh->texture_atlas = g_new(struct atlas_s, 1);
  mesh->min = vec3_set(FLT_MAX, FLT_MAX, FLT_MAX);
  mesh->max = vec3_set(FLT_MIN, FLT_MIN, FLT_MIN);
}

struct poly_s *mesh_add_poly(struct mesh_s *mesh, const gchar *material_name) {
  struct poly_s poly;
  guint face_id = mesh->polys->len;
  init_poly(&poly, face_id);
  g_array_append_val(mesh->polys, poly);
  struct mat_s *mat = mesh_add_get_material(mesh, material_name);
  LIST_APPEND(mat->polys, face_id);
  return &g_array_index(mesh->polys, struct poly_s, face_id);
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
  mesh->min = vec3_min(mesh->min, position);
  mesh->max = vec3_max(mesh->max, position);
  g_array_append_val(mesh->vertices, *stored);

  // 3) Store a heap key in the hash table
  g_hash_table_insert(mesh->vertex_map, stored, GUINT_TO_POINTER(index));

  return index;
}

static struct mat_s *find_mat(GPtrArray *sorted_mats, const gchar *name,
                              gint *step_count) {
  gint left = 0;
  gint right = sorted_mats->len - 1;
  if (step_count)
    *step_count = 0;
  while (left <= right) {
    gint mid = left + (right - left) / 2;
    struct mat_s *mat = g_ptr_array_index(sorted_mats, mid);
    gint cmp = g_strcmp0(mat->name, name);
    if (cmp == 0) {
      return mat; // Found
    } else if (cmp < 0) {
      left = mid + 1;
    } else {
      right = mid - 1;
    }
    if (step_count)
      (*step_count)++;
  }
  return NULL; // Not found
}

void build_mesh(struct mesh_s *mesh, const struct texinfo_s *texinfos,
                guint num_texinfos, guint atlas_width, guint atlas_height) {
  // Create texture atlas
  g_ptr_array_sort(mesh->mats, (GCompareFunc)mat_cmp_fn);

  mesh->texture_atlas->width = atlas_width;
  mesh->texture_atlas->height = atlas_height;
  mesh->texture_atlas->basis = mat3_ident();

  g_print("mesh min/max: <%f, %f, %f> / <%f, %f, %f>\n", mesh->min.x,
          mesh->min.y, mesh->min.z, mesh->max.x, mesh->max.y, mesh->max.z);
  for (guint i = 0; i < mesh->mats->len; i++) {
    struct mat_s *mat = g_ptr_array_index(mesh->mats, i);
    g_print("triangulating material %u of %u (%s)\n", i + 1, mesh->mats->len,
            mat->name);
    guint num_tris = 0;
    for (guint j = 0; j < mat->polys->len; j++) {
      guint poly_idx = mat->polys->data[j];
      struct poly_s *poly =
          &g_array_index(mesh->polys, struct poly_s, poly_idx);
      if (0 == poly->num_tris) {
        triangulate_poly(poly);
      }
      num_tris += poly->num_tris;
    }
    g_print("# of triangle: %u\n", num_tris);
    mat->tris = g_new(LISTOF(tri), 1);
    LIST_INIT(mat->tris, num_tris);
    for (guint j = 0; j < mat->polys->len; j++) {
      guint poly_idx = mat->polys->data[j];
      struct poly_s *poly =
          &g_array_index(mesh->polys, struct poly_s, poly_idx);
      for (guint k = 0; k < poly->num_tris; k++) {
        LIST_APPEND(mat->tris, poly->tris[k]);
      }
    }
  }
  if (texinfos && num_texinfos > 0) {
    const guint max_res = 256 + 128;
    guint *tex_res = g_new(guint, max_res * max_res);
    for (guint i = 0; i < max_res; i++) {
      for (guint j = 0; j < max_res; j++) {
        tex_res[i * max_res + j] = 0;
      }
    }
    for (guint i = 0; i < num_texinfos; i++) {
      const struct texinfo_s *texinfo = &texinfos[i];
      g_print(" texinfo[%u]: name='%s' size=%ux%u\n", i, texinfo->name,
              texinfo->width, texinfo->height);
      gint searches = 0;
      struct mat_s *mat = find_mat(mesh->mats, texinfo->name, &searches);
      if (mat) {
        g_print(" * found matching material '%s' (after %d searches)\n",
                mat->name, searches);
        mat->width = texinfo->width;
        mat->height = texinfo->height;
        mat->texture_data =
            g_memdup2(texinfo->data,
                      texinfo->width * texinfo->height * sizeof(struct rgba_s));
        guint count;
        double r, g, b, a;
        guint minr = 255, ming = 255, minb = 255;
        guint maxr = 0, maxg = 0, maxb = 0;
        r = g = b = a = 0.0;
        count = 0;
        for (guint j = 0; j < mat->width * mat->height; j++) {
          struct rgba_s *pixel = &mat->texture_data[j];
          r += SQ((double)pixel->r / 255.0);
          g += SQ((double)pixel->g / 255.0);
          b += SQ((double)pixel->b / 255.0);
          a += SQ((double)pixel->a / 255.0);
          count++;

          if (pixel->r < minr)
            minr = pixel->r;
          if (pixel->r > maxr)
            maxr = pixel->r;
          if (pixel->g < ming)
            ming = pixel->g;
          if (pixel->g > maxg)
            maxg = pixel->g;
          if (pixel->b < minb)
            minb = pixel->b;
          if (pixel->b > maxb)
            maxb = pixel->b;
        }
        r /= (double)count;
        g /= (double)count;
        b /= (double)count;
        a /= (double)count;
        r = sqrt(r) * 255.0;
        g = sqrt(g) * 255.0;
        b = sqrt(b) * 255.0;
        a = sqrt(a) * 255.0;
        mat->avg_color.r = (guint8)CLAMP_COLOR_COMPONENT(r);
        mat->avg_color.g = (guint8)CLAMP_COLOR_COMPONENT(g);
        mat->avg_color.b = (guint8)CLAMP_COLOR_COMPONENT(b);
        mat->avg_color.a = (guint8)CLAMP_COLOR_COMPONENT(a);

        g_print("   avg color: R=%u G=%u B=%u A=%u\n", mat->avg_color.r,
                mat->avg_color.g, mat->avg_color.b, mat->avg_color.a);
        g_print("   min color: R=%u G=%u B=%u\n", minr, ming, minb);
        g_print("   max color: R=%u G=%u B=%u\n", maxr, maxg, maxb);
        tex_res[mat->height * max_res + mat->width]++;

      } else {
        g_print(" * (ignored)\n");
      }
    }
    // Print texture resolution histogram
    for (guint i = 0; i < max_res; i++) {
      for (guint j = 0; j < max_res; j++) {
        if (tex_res[i * max_res + j] > 0) {
          g_print("# of textures with resolution %u x %u: %u\n", j, i,
                  tex_res[i * max_res + j]);
        }
      }
    }
    g_free(tex_res);
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
  g_free((*mesh)->texture_atlas->id_data);
  g_free((*mesh)->texture_atlas->diffuse_data);
  g_free((*mesh)->texture_atlas->normal_data);
  g_free((*mesh)->texture_atlas->position_data);
  g_free((*mesh)->texture_atlas->poly_regions);
  g_free((*mesh)->texture_atlas);
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
    for (guint j = 0; j < mat->polys->len; j++) {
      guint poly_idx = mat->polys->data[j];
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

void export_mesh_with_mats_to_obj_2(struct mesh_s *mesh, gfloat scale,
                                    const char **ignore_mats,
                                    int ignore_count) {
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

  int vertex_offset = 1;
  for (guint i = 0; i < mesh->mats->len; i++) {
    struct mat_s *mat = g_ptr_array_index(mesh->mats, i);
    bool skip = false;
    for (int j = 0; j < ignore_count; j++) {
      if (g_strrstr(mat->name, ignore_mats[j]) != NULL) {
        g_print(" - skipping material '%s'\n", mat->name);
        skip = true;
        break;
      }
    }
    if (skip)
      continue;
    g_string_append_printf(obj, "usemtl %s\n", mat->name);

    // Write faces
    for (guint j = 0; j < mat->polys->len; j++) {
      guint poly_idx = mat->polys->data[j];
      struct poly_s *poly =
          &g_array_index(mesh->polys, struct poly_s, poly_idx);
      for (guint k = 0; k < poly->num_vertices; k++) {
        guint v_idx = poly->vertices[k];
        struct vertex_s *v =
            &g_array_index(mesh->vertices, struct vertex_s, v_idx);
        g_string_append_printf(obj, "v %g %g %g\n", v->position.x * scale,
                               v->position.y * scale, v->position.z * scale);
        g_string_append_printf(obj, "vt %g %g\n", v->uvs[0].x, v->uvs[0].y);
      }
      for (guint k = 1; k < poly->num_vertices; k++) {
        uint v0 = vertex_offset;
        uint v1 = vertex_offset + k;
        uint v2 = vertex_offset + ((k + 1) % poly->num_vertices);
        g_string_append(obj, "f");
        g_string_append_printf(obj, " %u/%u", v0, v0);
        g_string_append_printf(obj, " %u/%u", v1, v1);
        g_string_append_printf(obj, " %u/%u", v2, v2);
        g_string_append_c(obj, '\n');
      }
      vertex_offset += poly->num_vertices;
    }
  }

  g_file_set_contents("mesh.obj", obj->str, obj->len, NULL);
  g_string_free(obj, TRUE);
}

static gboolean sweep_plane(struct GridTr_plane_s plane,
                            const struct GridTr_rayseg_s *rayseg,
                            struct vec3_s *hit_p) {
  if (rayseg->len < 1e-6f) {
    return FALSE;
  }

  gfloat ddotn = vec3_dot(rayseg->d, plane.n);
  if (fabsf(ddotn) < 1e-6f) {
    return FALSE; // parallel
  }
  gfloat dp0 = eval_plane(plane, rayseg->o);
  gfloat dp1 = eval_plane(plane, rayseg->e);
  if (dp0 > 1e-6f && dp1 > 1e-6f) {
    return FALSE;
  }
  if (dp0 < -1e-6f && dp1 < -1e-6f) {
    return FALSE;
  }
  // n . (p + t*d) = dist
  // n.p + t * n.d = dist
  // t = (dist - n.p) / n.d
  gfloat t = (plane.dist - vec3_dot(plane.n, rayseg->o)) / ddotn;
  if (t < -1e-6f || t > rayseg->len + 1e-6f) {
    return FALSE;
  }
  if (hit_p) {
    *hit_p = vec3_add(rayseg->o, vec3_mul(rayseg->d, t));
  }
  return TRUE;
}

gboolean sweep_collider(const struct GridTr_collider_s *collider,
                        const struct GridTr_rayseg_s *rayseg,
                        struct vec3_s *hit_p) {
  struct vec3_s p;
  if (!sweep_plane(collider->plane, rayseg, &p)) {
    return FALSE;
  }
  for (guint i = 0; i < collider->edge_count; i++) {
    float s = eval_plane(collider->edge_planes[i], p);
    if (s > 1e-6f) {
      return FALSE;
    }
  }
  if (hit_p) {
    *hit_p = p;
  }
  return TRUE;
}

struct trace_data_s {
  uint32 id;
  gboolean hit;
  struct vec3_s hit_p;
};

bool same_planes(const struct GridTr_plane_s *a,
                 const struct GridTr_plane_s *b) {
  float dot = vec3_dot(a->n, b->n);
  if (dot < 1.0f - 1e-12f) {
    return FALSE;
  }
  return fabsf(a->dist - b->dist) < 1e-3f;
}

bool trace_cb(const struct GridTr_grid_cell_s *cell, struct ivec3_s crl,
              const struct GridTr_rayseg_s *rayseg,
              const struct GridTr_collider_s *colliders, void *user_data) {
  if (rayseg->len < 1e-5f || !cell || cell->num_colliders == 0) {
    return false;
  }

  struct trace_data_s *data = user_data;
  data->hit = FALSE;
  for (uint i = 0; i < cell->num_colliders; i++) {
    const struct GridTr_collider_s *collider = &colliders[cell->colliders[i]];
    if (collider->poly_id == data->id) {
      continue;
    }
    if (sweep_collider(collider, rayseg, &data->hit_p)) {
      data->hit = TRUE;
      return true; // stop traversal
    }
  }
  return false;
}

void export_colliders_to_obj(const struct GridTr_collider_s *colliders,
                             guint num_colliders, const gchar *filename) {
  GString *obj = g_string_new(NULL);

  g_print("# of colliders: %u\n", num_colliders);
  for (guint i = 0; i < num_colliders; i++) {
    const struct GridTr_collider_s *collider = &colliders[i];
    for (guint j = 0; j < collider->edge_count; j++) {
      struct vec3_s p = vec3_mul(collider->ps[j], 0.025);
      g_string_append_printf(obj, "v %f %f %f\n", p.x, p.y, p.z);
    }
  }
  guint vertex_offset = 1;
  for (guint i = 0; i < num_colliders; i++) {
    const struct GridTr_collider_s *collider = &colliders[i];
    g_string_append_printf(obj, "f ");
    for (guint j = 0; j < collider->edge_count; j++) {
      guint vi = vertex_offset + j;
      g_string_append_printf(obj, "%u ", vi);
    }
    g_string_append_c(obj, '\n');
    vertex_offset += collider->edge_count;
  }

  g_file_set_contents(filename, obj->str, -1, NULL);
  g_print("exported colliders to OBJ file '%s'\n", filename);
  g_string_free(obj, TRUE);
}

void create_mesh_g_buffer(struct mesh_s *mesh) {
  struct atlas_s *atlas = mesh->texture_atlas;
  atlas->id_data = g_new(guint, atlas->width * atlas->height);
  atlas->diffuse_data = g_new(struct rgba_s, atlas->width * atlas->height);
  atlas->normal_data = g_new(struct vec3_s, atlas->width * atlas->height);
  atlas->position_data = g_new(struct vec3_s, atlas->width * atlas->height);

  g_print("creating g-buffer atlas %ux%u...\n", atlas->width, atlas->height);
  for (guint i = 0; i < atlas->width * atlas->height; i++) {
    atlas->id_data[i] = (guint)(-1);
    atlas->diffuse_data[i] = (struct rgba_s){{{0, 0, 0, 255}}};
    atlas->normal_data[i] = vec3_zero();
    atlas->position_data[i] = vec3_set(0.0f, 0.0f, 0.0f);
  }

  struct rgba_s *poly_colors = g_new(struct rgba_s, mesh->polys->len);
  for (guint i = 0; i < mesh->mats->len; i++) {
    struct mat_s *mat = g_ptr_array_index(mesh->mats, i);
    for (guint j = 0; j < mat->polys->len; j++) {
      guint poly_idx = mat->polys->data[j];
      poly_colors[poly_idx] = mat->avg_color;
    }
  }

  struct GridTr_grid_s grid;
  struct vec3_s *ps = GridTr_new(sizeof(struct vec3_s) * 256);

  GridTr_create_grid(&grid, 500.0f);
  for (guint i = 0; i < mesh->polys->len; i++) {
    struct poly_s *poly = &g_array_index(mesh->polys, struct poly_s, i);
    for (guint i = 0; i < poly->num_vertices; i++) {
      struct vertex_s *v =
          &g_array_index(mesh->vertices, struct vertex_s, poly->vertices[i]);
      ps[i] = v->position;
    }
    struct GridTr_collider_s coll;
    struct GridTr_plane_s plane =
        GridTr_create_plane(vec3_mul(poly->plane_normal, -1.0f), ps[0]);
    GridTr_create_collider(&coll, poly->face_id, ps, poly->num_vertices, plane);
    GridTr_add_collider_to_grid(&grid, &coll);
    GridTr_destroy_collider(&coll);
  }
  GridTr_free(ps);
  export_colliders_to_obj(grid.colliders->data, grid.colliders->num_elems,
                          "mesh_colliders.obj");
  for (guint i = 0; i < mesh->polys->len; i++) {
    struct poly_s *poly = &g_array_index(mesh->polys, struct poly_s, i);
    struct poly_region_s *region = &atlas->poly_regions[i];
    for (gint y = 0; y < region->h; y++) {
      for (gint x = 0; x < region->w; x++) {
        gint dst_x = region->x + x;
        gint dst_y = region->y + y;
        // if (dst_x >= (gint)atlas->width || dst_y >= (gint)atlas->height) {
        //   g_print("skipping oob pixel at (%d, %d) for poly %u\n", dst_x,
        //   dst_y,
        //           i);
        //   continue;
        // }
        guint dst_idx = dst_y * atlas->width + dst_x;
        atlas->id_data[dst_idx] = poly->face_id;
        atlas->diffuse_data[dst_idx] = poly_colors[i];
        atlas->normal_data[dst_idx] = poly->plane_normal;
        atlas->position_data[dst_idx] =
            poly_region_coord_to_3d(region, (struct ivec2_s){x, y});
      }
    }
  }

  struct vec3_s lightpos = vec3_zero();
  gint64 t0 = now_us();
  for (guint i = 0; i < atlas->width * atlas->height; i++) {
    guint id = atlas->id_data[i];
    if (id == (guint)(-1)) {
      continue;
    }
    struct vec3_s n = vec3_mul(atlas->normal_data[i], +1.0f);
    if (vec3_dot(n, n) < 1e-12f) {
      continue; // not a valid point
    }

    struct vec3_s p0 = atlas->position_data[i];
    p0 = vec3_add(p0, vec3_mul(n, 1e-3f));
    //   (t*d).n = 1e-3
    //   t = 1e-3 / d.n
    // struct vec3_s d = vec3_norm(point_vec(p0, lightpos));
    // float ddotn = fabsf(vec3_dot(n, d));
    // p0 = vec3_add(p0, vec3_mul(d, 1e-3f / ddotn));
    struct GridTr_rayseg_s rayseg = GridTr_create_rayseg(p0, lightpos);
    gfloat ndotl = 1.0f; //-vec3_dot(n, rayseg.d);
    ndotl = MAX(ndotl, 0.0f);

    struct trace_data_s data = {0};
    data.hit = false;
    data.id = id;
    if (GridTr_trace_ray_through_grid(&grid, &rayseg, trace_cb, &data)) {
      ndotl = 0.0f;
    }

    gfloat intensity = sqrtf(ndotl); // linear -> srgb approx
    atlas->diffuse_data[i].r =
        (guint8)CLAMP_COLOR_COMPONENT(atlas->diffuse_data[i].r * intensity);
    atlas->diffuse_data[i].g =
        (guint8)CLAMP_COLOR_COMPONENT(atlas->diffuse_data[i].g * intensity);
    atlas->diffuse_data[i].b =
        (guint8)CLAMP_COLOR_COMPONENT(atlas->diffuse_data[i].b * intensity);
  }
  GridTr_export_grid_boxes_to_obj(&grid, "grid_boxes.obj");
  GridTr_destroy_grid(&grid);

  gint64 t1 = now_us();
  g_print("Processing time: %lf seconds\n", (double)(t1 - t0) * 1e-6);
  unsigned error = lodepng_encode32_file("diffuse.png", atlas->diffuse_data,
                                         atlas->width, atlas->height);
  if (error) {
    g_error("error %u: %s\n", error, lodepng_error_text(error));
  }
  g_free(poly_colors);
}
