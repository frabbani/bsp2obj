/*
 * bsp2obj
 *
 * Copyright (C) 2016 Florian Zwoch <fzwoch@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <math.h>
#include <string.h>

#include "lodepng.h"
#include "mesh.h"
#include "mygltf.h"

#define SWAP(a, b)                                                             \
  do {                                                                         \
    typeof(a) tmp = a;                                                         \
    a = b;                                                                     \
    b = tmp;                                                                   \
  } while (0)

struct entry_s {
  guint32 offset;
  guint32 size;
};

struct header_s {
  guint32 version;
  struct entry_s entities;
  struct entry_s planes;
  struct entry_s miptex;
  struct entry_s vertices;
  struct entry_s visilist;
  struct entry_s nodes;
  struct entry_s texinfo;
  struct entry_s faces;
  struct entry_s lightmaps;
  struct entry_s clipnodes;
  struct entry_s leaves;
  struct entry_s faces_list;
  struct entry_s edges;
  struct entry_s edges_list;
  struct entry_s models;
};

struct boundbox_s {
  struct vec3_s min;
  struct vec3_s max;
};

struct model_s {
  struct boundbox_s bound;
  struct vec3_s origin;
  guint32 node_id0;
  guint32 node_id1;
  guint32 node_id2;
  guint32 node_id3;
  guint32 numleaves;
  guint32 face_id;
  guint32 face_num;
};

struct mipheader_s {
  guint32 numtex;
  guint32 offsets[];
};

struct miptex_s {
  gchar name[16];
  guint32 width;
  guint32 height;
  guint32 offset1;
  guint32 offset2;
  guint32 offset4;
  guint32 offset8;
};

struct surface_s {
  struct vec3_s vectorS;
  gfloat distS;
  struct vec3_s vectorT;
  gfloat distT;
  guint32 texture_id;
  guint32 animated;
};

struct edge_s {
  guint16 vertex0;
  guint16 vertex1;
};

struct face_s {
  guint16 plane_id;
  guint16 side;
  gint32 ledge_id;
  guint16 ledge_num;
  guint16 texinfo_id;
  guint8 typelight;
  guint8 baselight;
  guint8 light[2];
  gint32 lightmap;
};

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

struct lmap_s {
  gint face_id;
  gfloat mins[2];
  gfloat maxs[2];
  gint bmins[2];
  gint bmaxs[2];
  gint tmins[2];
  gint texts[2];
  gint width, height;
  struct rgba_s *data;
  gint atlas_x, atlas_y;
};

void init_lmap(struct lmap_s *lm, gint face_id);
void lmap_addST(struct lmap_s *lm, gfloat s, gfloat t);
void calc_lmap(struct lmap_s *lm);
void lmap_getUV(struct lmap_s *lm, gfloat s, gfloat t, gfloat *u, gfloat *v);
int compare_lmap_fn(const gpointer a, const gpointer b);

struct ivec2_s {
  gint x;
  gint y;
};

struct ivec2_s pack_lmap_block(guint *skyline, guint atlas_width, guint width,
                               guint height, gboolean padded) {
  guint best_x = G_MAXUINT;
  guint best_y = G_MAXUINT;

  struct ivec2_s uv = {-1, -1};

  gint padding = padded ? 1 : 0;

  width += padding;
  height += padding;
  for (guint x = padding; x < atlas_width - width - padding; x++) {
    if (skyline[x] >= best_y) {
      continue;
    }
    gboolean fits = TRUE;
    for (gint x2 = x; x2 < x + width; x2++) {
      if (skyline[x2] > skyline[x]) {
        fits = FALSE;
        break;
      }
    }
    if (fits) {
      best_y = skyline[x];
      best_x = x;
    }
  }
  if (best_x != G_MAXUINT) {
    for (guint x = best_x; x < best_x + width; x++) {
      skyline[x] = best_y + height;
    }
    uv.x = best_x;
    uv.y = best_y;
  }
  return uv;
}

void pack_lmaps(struct lmap_s *lmaps, guint num_lmaps, guint atlas_width,
                guint atlas_height) {
  guint *skyline = g_new(guint, atlas_width);
  for (guint i = 0; i < atlas_width; i++) {
    skyline[i] = 1;
  }

  g_qsort_with_data(lmaps, num_lmaps, sizeof(struct lmap_s), compare_lmap_fn,
                    NULL);

  g_print("packing...\n");
  for (guint i = 0; i < num_lmaps; i++) {
    struct lmap_s *lm = &lmaps[i];
    struct ivec2_s uv =
        pack_lmap_block(skyline, atlas_width, lm->width, lm->height, FALSE);
    if (uv.x == -1) {
      g_error("Failed to pack lightmaps into atlas (%ux%u)", atlas_width,
              atlas_height);
    } else {
      lm->atlas_x = uv.x;
      lm->atlas_y = uv.y;
    }
  }

  guint max_height = skyline[0];
  for (guint i = 1; i < atlas_width; i++) {
    if (skyline[i] > max_height) {
      max_height = skyline[i];
    }
  }
  g_print("Lightmap atlas size: %ux%u\n", atlas_width, max_height);
  if (max_height <= atlas_height) {
    struct rgba_s *atlas_data =
        g_new(struct rgba_s, atlas_width * atlas_height);
    for (guint i = 0; i < atlas_width * atlas_height; i++) {
      atlas_data[i].r = 255;
      atlas_data[i].g = 0;
      atlas_data[i].b = 255;
      atlas_data[i].a = 255;
    }
    for (guint i = 0; i < num_lmaps; i++) {
      struct lmap_s *lm = &lmaps[i];
      for (gint y = 0; y < lm->height; y++) {
        for (gint x = 0; x < lm->width; x++) {
          guint dest_x = lm->atlas_x + x;
          guint dest_y = lm->atlas_y + y;
          atlas_data[dest_y * atlas_width + dest_x] =
              lm->data[y * lm->width + x];
        }
      }
    }
    unsigned error = lodepng_encode32_file("lightmap.png", atlas_data,
                                           atlas_width, atlas_height);
    if (error) {
      g_error("error %u: %s\n", error, lodepng_error_text(error));
    }
    g_free(atlas_data);
  }
  g_free(skyline);
}

guint *create_lmap_lut(const struct lmap_s *lmaps, guint num_lmaps) {
  guint *lut = g_new(guint, num_lmaps);
  for (guint i = 0; i < num_lmaps; i++) {
    lut[lmaps[i].face_id] = i;
  }
  return lut;
}

void export_mesh_with_lmap_to_obj(struct mesh_s *mesh, gfloat scale) {
  GString *obj = g_string_new(NULL);
  /*
  newmtl lightmap
  Ka 1 1 1
  Kd 1 1 1
  Ks 0 0 0
  Tr 1
  illum 1
  Ns 0
  map_Kd lightmap.png
  */
  // Write material
  g_string_append_printf(obj, "newmtl lightmap\n");
  g_string_append(obj, "Ka 1 1 1\n");
  g_string_append(obj, "Kd 1 1 1\n");
  g_string_append(obj, "Ks 0 0 0\n");
  g_string_append(obj, "Tr 1\n");
  g_string_append(obj, "illum 1\n");
  g_string_append(obj, "Ns 0\n");
  g_string_append(obj, "map_Kd lightmap.png\n");
  g_file_set_contents("lightmap.mtl", obj->str, obj->len, NULL);

  g_string_free(obj, TRUE);
  obj = g_string_new(NULL);
  g_string_append(obj, "mtllib lightmap.mtl\n");
  g_string_append(obj, "usemtl lightmap\n");

  // Write vertices
  for (guint i = 0; i < mesh->vertices->len; i++) {
    struct vertex_s *v = &g_array_index(mesh->vertices, struct vertex_s, i);
    g_string_append_printf(obj, "v %g %g %g\n", v->position.x * scale,
                           v->position.y * scale, v->position.z * scale);
  }

  // Write texture coordinates
  for (guint i = 0; i < mesh->vertices->len; i++) {
    struct vertex_s *v = &g_array_index(mesh->vertices, struct vertex_s, i);
    g_string_append_printf(obj, "vt %g %g\n", v->uvs[1].x, v->uvs[1].y);
  }

  // Write faces
  for (guint i = 0; i < mesh->polys->len; i++) {
    struct poly_s *poly = &g_array_index(mesh->polys, struct poly_s, i);
    for (guint j = 0; j < poly->num_tris; j++) {
      struct tri_s *tri = &poly->tris[j];
      g_string_append(obj, "f");
      g_string_append_printf(obj, " %u/%u", tri->v0 + 1, tri->v0 + 1);
      g_string_append_printf(obj, " %u/%u", tri->v1 + 1, tri->v1 + 1);
      g_string_append_printf(obj, " %u/%u", tri->v2 + 1, tri->v2 + 1);
      g_string_append_c(obj, '\n');
    }
  }

  // Output to file
  g_file_set_contents("output.obj", obj->str, obj->len, NULL);
  g_string_free(obj, TRUE);
}

/*
 * Load a Quake palette (PLAYPAL-style) file. The file is expected to contain
 * 256 RGB triplets (768 bytes). The palette is written into `palette` as
 * palette[index][0..2] = R,G,B.
 *
 * Returns TRUE on success, FALSE on failure and sets `err` via GLib.
 */
gboolean load_palette(const char *path, struct rgb_s **palette, GError **err) {
  gsize len = 0;
  guchar *data = NULL;

  if (!g_file_get_contents(path, (gchar **)&data, &len, err)) {
    return FALSE;
  }

  if (len != 256 * 3) {
    g_set_error(err, G_FILE_ERROR, G_FILE_ERROR_INVAL,
                "palette file '%s' invalid size (%zu bytes instead of 768)",
                path, len);
    g_free(data);
    return FALSE;
  }
  if (*palette != NULL) {
    g_free(*palette);
  }
  *palette = g_new(struct rgb_s, 256);

  for (gint i = 0; i < 256; i++) {
    (*palette)[i].r = data[i * 3 + 0];
    (*palette)[i].g = data[i * 3 + 1];
    (*palette)[i].b = data[i * 3 + 2];
  }

  g_free(data);
  return TRUE;
}

struct material_s {
  gchar name[64];
  GString *faces;
};

void material_free(gpointer data) {
  struct material_s *mat = (struct material_s *)data;
  g_string_free(mat->faces, TRUE);
  g_free(mat);
}

gchar *dedupe_obj_text(const gchar *text) {
  GHashTable *seen =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free, material_free);
  gchar **lines = g_strsplit(text, "\n", 0);
  GString *result = g_string_new(NULL);
  struct material_s *mat = NULL;
  for (gint i = 0; lines[i] != NULL; i++) {
    gchar *line = lines[i];
    if (g_str_has_prefix(line, "usemtl ")) {
      gchar *name = line + 7; // Skip "usemtl "
      name = g_strstrip(name);
      mat = g_hash_table_lookup(seen, name);
      if (mat == NULL) {
        mat = g_new(struct material_s, 1);
        g_strlcpy(mat->name, name, sizeof(mat->name));
        mat->faces = g_string_new(NULL);
        g_hash_table_insert(seen, g_strdup(mat->name), mat);
      }
    } else if (line[0] == 'f' && line[1] == ' ') {
      if (mat) {
        g_string_append(mat->faces, line);
        g_string_append_c(mat->faces, '\n');
      }
      // skip if there is material.
    } else {
      g_string_append(result, line);
      g_string_append_c(result, '\n');
    }
  }

  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init(&iter, seen);
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    struct material_s *mat = (struct material_s *)value;
    g_string_append_printf(result, "usemtl %s\n", mat->name);
    g_string_append(result, mat->faces->str);
  }

  gchar *deduped = g_string_free(result, FALSE);
  g_hash_table_destroy(seen);
  g_strfreev(lines);
  return deduped;
}

static guint map_vertex(GHashTable *map, GString *obj, struct vec3_s *vertices,
                        guint vertex_idx) {
  gpointer value = g_hash_table_lookup(map, GINT_TO_POINTER(vertex_idx));

  if (value != NULL) {
    return GPOINTER_TO_INT(value);
  }

  gint mapped_idx = g_hash_table_size(map) + 1;
  g_hash_table_insert(map, GINT_TO_POINTER(vertex_idx),
                      GINT_TO_POINTER(mapped_idx));

  g_string_append_printf(obj, "v %g %g %g\n", vertices[vertex_idx].x,
                         vertices[vertex_idx].y, vertices[vertex_idx].z);

  return mapped_idx;
}

int main(int argc, char **argv) {
  GError *err = NULL;
  gchar *buf = NULL;
  gsize buf_len = 0;
  struct rgb_s *palette = NULL;

  if (argc != 2) {
    g_print("usage: %s <map.bsp>\n", argv[0]);
    return 0;
  }

  load_palette("palette.lmp", &palette, &err);
  if (err != NULL) {
    g_error("%s", err->message);
    g_error_free(err);
    return 1;
  }

  GHashTable *map = g_hash_table_new(g_direct_hash, g_direct_equal);
  GString *obj = g_string_new(NULL);

  g_file_get_contents(argv[1], &buf, &buf_len, &err);
  if (err != NULL) {
    g_error("%s", err->message);
    g_error_free(err);

    return 1;
  }

  struct header_s *header = buf;
  struct model_s *models = buf + header->models.offset;
  struct mipheader_s *mipheader = buf + header->miptex.offset;
  struct vec3_s *vertices = buf + header->vertices.offset;
  struct surface_s *surfaces = buf + header->texinfo.offset;
  //	gint16 *faces_list = buf + header->faces_list.offset;
  struct face_s *faces = buf + header->faces.offset;
  gint32 *edges_list = buf + header->edges_list.offset;
  struct edge_s *edges = buf + header->edges.offset;

  gchar *map_name = g_path_get_basename(argv[1]);
  map_name[strcspn(map_name, ".")] = '\0';

  // Extract materials and textures
  for (gint i = 0; i < mipheader->numtex; i++) {
    struct miptex_s *miptex =
        buf + header->miptex.offset + mipheader->offsets[i];

    if (miptex->name[0] == '*') {
      miptex->name[0] = '+';
    }

    gboolean has_tex = strlen(miptex->name) > 0;
    if (!has_tex) {
      g_snprintf(miptex->name, sizeof(miptex->name), "unnamed%d", i);
    }

    g_string_append_printf(obj, "newmtl %s\n", miptex->name);
    g_string_append_printf(obj, "Ka 1 1 1\n");
    g_string_append_printf(obj, "Kd 1 1 1\n");
    g_string_append_printf(obj, "Ks 0 0 0\n");
    g_string_append_printf(obj, "Tr 1\n");
    g_string_append_printf(obj, "illum 1\n");
    g_string_append_printf(obj, "Ns 0\n");
    g_string_append_printf(obj, "map_Kd textures/%s.png\n", miptex->name);

    if (has_tex && mipheader->offsets[i] != -1) {
      gsize img_size = miptex->width * miptex->height;
      guchar *img_data = g_new(guchar, img_size * 4);
      g_print("Extracting texture '%s' (%ux%u) (offsets: %d, %d, %d, %d)\n",
              miptex->name, miptex->width, miptex->height, miptex->offset1,
              miptex->offset2, miptex->offset4, miptex->offset8);
      guchar *mip_data = (guchar *)buf + header->miptex.offset +
                         mipheader->offsets[i] + miptex->offset1;

      for (gsize j = 0; j < img_size; j++) {
        struct rgb_s col = palette[mip_data[j]];

        img_data[j * 4 + 0] = col.r;
        img_data[j * 4 + 1] = col.g;
        img_data[j * 4 + 2] = col.b;
        img_data[j * 4 + 3] = 255;
      }
      gchar *img_file = g_strdup_printf("export/textures/%s.png", miptex->name);
      unsigned error = lodepng_encode32_file(img_file, img_data, miptex->width,
                                             miptex->height);
      if (error) {
        g_error("error %u: %s\n", error, lodepng_error_text(error));
      }

      g_free(img_file);
      g_free(img_data);
    }
  }

  gchar *out_file = g_strdup_printf("%s.mtl", map_name);

  g_file_set_contents(out_file, obj->str, obj->len, &err);
  if (err != NULL) {
    g_error("%s", err->message);
    g_error_free(err);

    return 1;
  }

  g_free(out_file);

  guint face_count = header->faces.size / sizeof(struct face_s);
  struct lmap_s *lmaps = g_new(struct lmap_s, face_count);

  // Extract models and lightmap info
  for (gint k = 0; k < header->models.size / sizeof(struct model_s); k++) {
    struct model_s *model = &models[k];
    GString *obj_uvs = g_string_new(NULL);
    GString *obj_faces = g_string_new(NULL);
    gint count = 1;

    g_string_printf(obj, "mtllib %s.mtl\n", map_name);

    for (gint i = 0; i < model->face_num; i++) {
      struct lmap_s *lm = &lmaps[model->face_id + i];
      struct face_s *face = &faces[model->face_id + i];
      struct edge_s *edge = edges + ABS(edges_list[face->ledge_id]);

      struct miptex_s *miptex =
          buf + header->miptex.offset +
          mipheader->offsets[surfaces[face->texinfo_id].texture_id];
      init_lmap(lm, model->face_id + i);
      g_string_append_printf(obj_faces, "usemtl %s\n", miptex->name);

      gint vtx = edges_list[face->ledge_id] < 0 ? edge->vertex1 : edge->vertex0;
      for (gint j = 1; j < face->ledge_num - 1; j++) {
        struct edge_s *edge1 = edges + ABS(edges_list[face->ledge_id + j]);
        struct edge_s *edge2 = edges + ABS(edges_list[face->ledge_id + j + 1]);

        gint vtx1 = edges_list[face->ledge_id + j] < 0 ? edge1->vertex1
                                                       : edge1->vertex0;
        gint vtx2 = edges_list[face->ledge_id + j + 1] < 0 ? edge2->vertex1
                                                           : edge2->vertex0;

        g_string_append_printf(obj_faces, "f %u/%d %u/%d %u/%d\n",
                               map_vertex(map, obj, vertices, vtx), count,
                               map_vertex(map, obj, vertices, vtx2), count + 2,
                               map_vertex(map, obj, vertices, vtx1), count + 1);

        count += 3;

        float u[3];
        float v[3];

        struct surface_s *surface = &surfaces[face->texinfo_id];
        u[0] = vec3_dot(surface->vectorS, vertices[vtx]) + surface->distS;
        v[0] = vec3_dot(surface->vectorT, vertices[vtx]) + surface->distT;

        u[1] = vec3_dot(surface->vectorS, vertices[vtx1]) + surface->distS;
        v[1] = vec3_dot(surface->vectorT, vertices[vtx1]) + surface->distT;

        u[2] = vec3_dot(surface->vectorS, vertices[vtx2]) + surface->distS;
        v[2] = vec3_dot(surface->vectorT, vertices[vtx2]) + surface->distT;

        g_string_append_printf(obj_uvs, "vt %g %g\nvt %g %g\nvt %g %g\n",
                               u[0] / miptex->width, 1 - v[0] / miptex->height,
                               u[1] / miptex->width, 1 - v[1] / miptex->height,
                               u[2] / miptex->width, 1 - v[2] / miptex->height);
        lmap_addST(lm, u[0], v[0]);
        lmap_addST(lm, u[1], v[1]);
        lmap_addST(lm, u[2], v[2]);
      }
      calc_lmap(lm);
      guint num_luxels = lm->width * lm->height;
      lm->data = g_new(struct rgba_s, num_luxels);
      for (gint j = 0; j < num_luxels; j++) {
        lm->data[j].r = 0;
        lm->data[j].g = 0;
        lm->data[j].b = 0;
        lm->data[j].a = 255;
      }
      if (face->lightmap >= 0) {
        guchar *lm_data =
            (guchar *)buf + header->lightmaps.offset + face->lightmap;
        for (gint j = 0; j < num_luxels; j++) {
          guint8 intensity = lm_data[j];
          lm->data[j].r = intensity;
          lm->data[j].g = intensity;
          lm->data[j].b = intensity;
        }
        /*
        gchar* img_file = g_strdup_printf("lightmaps/lm_%d.png", lm->face_id);
        unsigned error =
            lodepng_encode32_file(img_file, lm->data, lm->width, lm->height);
        if (error) {
          g_error("error %u: %s\n", error, lodepng_error_text(error));
        }
        g_free(img_file);
        */
      }
    }
    obj = g_string_append(obj, obj_uvs->str);
    obj = g_string_append(obj, obj_faces->str);

    g_string_free(obj_uvs, TRUE);
    g_string_free(obj_faces, TRUE);

    g_hash_table_remove_all(map);

    if (k == 0) {
      out_file = g_strdup_printf("export/%s.obj", map_name);
    } else {
      out_file = g_strdup_printf("export/%s_%d.obj", map_name, k);
    }

    /* Deduplicate consecutive identical usemtl lines to reduce file bloat. */
    gchar *deduped = dedupe_obj_text(obj->str);
    if (deduped != NULL) {
      g_file_set_contents(out_file, deduped, strlen(deduped), &err);
      g_free(deduped);
    } else {
      g_file_set_contents(out_file, obj->str, obj->len, &err);
    }
    if (err != NULL) {
      g_error("%s", err->message);
      g_error_free(err);

      return 1;
    }

    g_free(out_file);
  }

  guint atlas_width = 512;
  guint atlas_height = 512;
  pack_lmaps(lmaps, face_count, atlas_width, atlas_height);
  guint *lmap_lut = create_lmap_lut(lmaps, face_count);
  // Extract a single model containing lightmap UVs
  struct model_s *model = &models[0];
  struct mesh_s *mesh = g_new(struct mesh_s, 1);
  init_mesh(mesh);
  for (gint i = 0; i < model->face_num; i++) {
    gint face_id = model->face_id + i;
    struct face_s *face = &faces[face_id];
    struct surface_s *surface = &surfaces[face->texinfo_id];
    struct miptex_s *miptex =
        buf + header->miptex.offset + mipheader->offsets[surface->texture_id];

    struct lmap_s *lm = &lmaps[lmap_lut[face_id]];
    struct poly_s *poly = mesh_add_poly(mesh, miptex->name);

    for (gint j = 0; j < face->ledge_num; j++) {
      struct edge_s *edge = edges + ABS(edges_list[face->ledge_id + j]);
      gint vtx =
          edges_list[face->ledge_id + j] < 0 ? edge->vertex1 : edge->vertex0;
      struct vec3_s position = vertices[vtx];
      struct vec2_s st, uv;
      st.x = vec3_dot(surface->vectorS, position) + surface->distS;
      st.y = vec3_dot(surface->vectorT, position) + surface->distT;
      lmap_getUV(lm, st.x, st.y, &uv.x, &uv.y);
      uv.x = (lm->atlas_x + uv.x) / atlas_width;
      uv.y = 1.0f - (lm->atlas_y + uv.y) / atlas_height;
      st.x /= miptex->width;
      st.y = 1.0f - (st.y / miptex->height);
      guint vertex_idx = mesh_add_get_vertex(mesh, position, st, uv);
      poly_add_vertex(poly, vertex_idx);
    }
    triangulate_poly(poly);
  }
  build_mesh(mesh);
  export_mesh_with_lmap_to_obj(mesh, 0.025f);
  export_mesh_with_mats_to_obj(mesh, 0.025f);
  export_mesh_to_gltf(mesh, 0.025f, "mesh.gltf", &err);
  if (err != NULL) {
    g_error("%s", err->message);
    g_error_free(err);
  }

  g_hash_table_unref(map);
  g_string_free(obj, TRUE);
  g_free(buf);
  for (guint i = 0; i < face_count; i++) {
    g_free(lmaps[i].data);
  }
  g_free(lmaps);
  g_free(lmap_lut);
  g_free(palette);
  free_mesh(&mesh);
  g_free(mesh);
  g_print("Done. Goodbye!\n");
  return 0;
}

void init_lmap(struct lmap_s *lm, gint face_id) {
  lm->face_id = face_id;
  lm->mins[0] = lm->mins[1] = G_MAXFLOAT;
  lm->maxs[0] = lm->maxs[1] = -G_MAXFLOAT;
}

void lmap_addST(struct lmap_s *lm, gfloat s, gfloat t) {
  if (s < lm->mins[0])
    lm->mins[0] = s;
  if (t < lm->mins[1])
    lm->mins[1] = t;
  if (s > lm->maxs[0])
    lm->maxs[0] = s;
  if (t > lm->maxs[1])
    lm->maxs[1] = t;
}

void calc_lmap(struct lmap_s *lm) {
  for (gint i = 0; i < 2; i++) {
    lm->bmins[i] = (gint)floor(lm->mins[i] / 16.0f);
    lm->bmaxs[i] = (gint)ceil(lm->maxs[i] / 16.0f);
    lm->tmins[i] = lm->bmins[i] * 16;
    lm->texts[i] = (lm->bmaxs[i] - lm->bmins[i]) * 16;
  }
  lm->width = lm->texts[0] / 16 + 1;
  lm->height = lm->texts[1] / 16 + 1;
}

void lmap_getUV(struct lmap_s *lm, gfloat s, gfloat t, gfloat *u, gfloat *v) {
  if (u != NULL) {
    *u = (s - lm->tmins[0]) / 16.0f + 0.5f;
  }
  if (v != NULL) {
    *v = (t - lm->tmins[1]) / 16.0f + 0.5f;
  }
}

int compare_lmap_fn(const gpointer a, const gpointer b) {
  const struct lmap_s *lm_a = (const struct lmap_s *)a;
  const struct lmap_s *lm_b = (const struct lmap_s *)b;
  if (lm_a->height != lm_b->height) {
    return lm_b->height - lm_a->height;
  }
  if (lm_a->width != lm_b->width) {
    return lm_b->width - lm_a->width;
  }
  return 0;
}
