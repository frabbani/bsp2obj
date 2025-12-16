#include "mygltf.h"

#define CGLTF_IMPLEMENTATION
#define CGLTF_WRITE_IMPLEMENTATION

#include "cgltf_write.h"

void export_mesh_to_gltf(const struct mesh_s *mesh, gfloat scale,
                         const gchar *output_path, GError **err) {
  const GArray *vertices = mesh->vertices;
  const GPtrArray *mats = mesh->mats;
  cgltf_options options = {0};
  options.type = cgltf_file_type_gltf; // write .gltf
  size_t allocs_size = 256;
  void **allocs = g_malloc0(allocs_size * sizeof(void *));
  size_t alloc_count = 0;
#define ALLOC(count, size)                                                     \
  ({                                                                           \
    if (alloc_count == allocs_size) {                                          \
      allocs_size *= 2;                                                        \
      allocs = g_realloc(allocs, allocs_size * sizeof(void *));                \
      g_assert(allocs != NULL);                                                \
    }                                                                          \
    void *ptr = g_malloc0((count) * (size));                                   \
    g_assert(ptr != NULL);                                                     \
    allocs[alloc_count++] = ptr;                                               \
    ptr;                                                                       \
  })

  cgltf_data *data = ALLOC(1, sizeof(cgltf_data));
  data->asset.version = "2.0";

  // -------- 1) Counts --------
  cgltf_size vertex_count = vertices->len;
  cgltf_size material_count = mats->len;

  cgltf_size index_total_count = 0;
  for (guint i = 0; i < material_count; ++i) {
    struct mat_s *m = g_ptr_array_index(mats, i);
    index_total_count += (cgltf_size)m->tris.num_tris * 3;
  }

  // -------- 2) Buffer layout: [VERTICES][INDICES] --------
  const cgltf_size vertex_stride = sizeof(struct vertex_s);
  const cgltf_size vertex_buffer_size = vertex_count * vertex_stride;
  const cgltf_size index_buffer_size = index_total_count * sizeof(uint32_t);
  const cgltf_size total_buffer_size = vertex_buffer_size + index_buffer_size;

  uint8_t *buffer_data = ALLOC(1, total_buffer_size);

  // copy vertices (interleaved, already in the right layout)
  memcpy(buffer_data, vertices->data, vertex_buffer_size);
  struct vertex_s *vs = (struct vertex_s *)buffer_data;
  for (guint i = 0; i < vertex_count; i++) {
    vs[i].uvs[0].y = 1.0f - vs[i].uvs[0].y;
    vs[i].uvs[1].y = 1.0f - vs[i].uvs[1].y;
  }

  // flatten indices per material into one big index buffer
  uint32_t *index_ptr = (uint32_t *)(buffer_data + vertex_buffer_size);
  cgltf_size running_index_offset = 0;

  // store per-material index ranges
  cgltf_size *per_mat_index_count = ALLOC(material_count, sizeof(cgltf_size));
  cgltf_size *per_mat_index_offset = ALLOC(material_count, sizeof(cgltf_size));

  for (guint i = 0; i < material_count; ++i) {
    struct mat_s *m = g_ptr_array_index(mats, i);
    per_mat_index_count[i] = (cgltf_size)m->tris.num_tris * 3;
    per_mat_index_offset[i] = running_index_offset;

    for (guint t = 0; t < m->tris.num_tris; ++t) {
      struct tri_s *tri = &m->tris.tris[t];
      index_ptr[running_index_offset++] = tri->v2;
      index_ptr[running_index_offset++] = tri->v1;
      index_ptr[running_index_offset++] = tri->v0;
    }
  }

  // -------- 3) Buffer --------
  data->buffers_count = 1;
  data->buffers = ALLOC(1, sizeof(cgltf_buffer));
  data->buffers[0].data = buffer_data;
  data->buffers[0].size = total_buffer_size;
  data->buffers[0].uri = "mesh.bin";
  // uri stays NULL for .glb

  // -------- 4) BufferViews --------
  data->buffer_views_count = 2;
  data->buffer_views = ALLOC(2, sizeof(cgltf_buffer_view));

  // Vertex bufferView (interleaved)
  cgltf_buffer_view *bv_vertices = &data->buffer_views[0];
  bv_vertices->buffer = &data->buffers[0];
  bv_vertices->offset = 0;
  bv_vertices->size = vertex_buffer_size;
  bv_vertices->stride = vertex_stride;
  bv_vertices->type = cgltf_buffer_view_type_vertices;
  // bv_vertices->target = 34962; // ARRAY_BUFFER

  // Index bufferView
  cgltf_buffer_view *bv_indices = &data->buffer_views[1];
  bv_indices->buffer = &data->buffers[0];
  bv_indices->offset = vertex_buffer_size;
  bv_indices->size = index_buffer_size;
  bv_indices->stride = 0; // tightly packed
  bv_indices->type = cgltf_buffer_view_type_indices;
  // bv_indices->target = 34963; // ELEMENT_ARRAY_BUFFER

  // -------- 5) Accessors --------
  // 0: POSITION
  // 1: TEXCOORD_0
  // 2: TEXCOORD_1
  // 3..(2+material_count): index accessors per material
  data->accessors_count = 3 + material_count;
  data->accessors = ALLOC(data->accessors_count, sizeof(cgltf_accessor));

  // POSITION accessor
  cgltf_accessor *acc_pos = &data->accessors[0];
  acc_pos->buffer_view = bv_vertices;
  acc_pos->offset = offsetof(struct vertex_s, position);
  acc_pos->component_type = cgltf_component_type_r_32f;
  acc_pos->count = vertex_count;
  acc_pos->type = cgltf_type_vec3;

  // Optional: compute min/max for POSITION (helps viewers)
  {
    if (vertex_count > 0) {
      struct vertex_s *v0 = &g_array_index(vertices, struct vertex_s, 0);
      struct vec3_s min =
          vec3_set(v0->position.x, v0->position.y, v0->position.z);
      struct vec3_s max =
          vec3_set(v0->position.x, v0->position.y, v0->position.z);
      for (guint i = 1; i < vertex_count; ++i) {
        struct vertex_s *v = &g_array_index(vertices, struct vertex_s, i);
        struct vec3_s p = vec3_set(v->position.x, v->position.y, v->position.z);
        min = vec3_min(min, p);
        max = vec3_max(max, p);
      }
      acc_pos->has_min = acc_pos->has_max = 1;
      memcpy(acc_pos->min, min.xyz, sizeof(min.xyz));
      memcpy(acc_pos->max, max.xyz, sizeof(max.xyz));
    }
  }

  // TEXCOORD_0 (diffuse UVs)
  cgltf_accessor *acc_uv0 = &data->accessors[1];
  acc_uv0->buffer_view = bv_vertices;
  acc_uv0->offset = offsetof(struct vertex_s, uvs[0]);
  acc_uv0->component_type = cgltf_component_type_r_32f;
  acc_uv0->count = vertex_count;
  acc_uv0->type = cgltf_type_vec2;
  acc_uv0->normalized = 0;

  // TEXCOORD_1 (lightmap UVs)
  cgltf_accessor *acc_uv1 = &data->accessors[2];
  acc_uv1->buffer_view = bv_vertices;
  acc_uv1->offset = offsetof(struct vertex_s, uvs[1]);
  acc_uv1->component_type = cgltf_component_type_r_32f;
  acc_uv1->count = vertex_count;
  acc_uv1->type = cgltf_type_vec2;
  acc_uv1->normalized = 0;

  // Index accessors: one per material
  for (guint i = 0; i < material_count; ++i) {
    cgltf_accessor *acc = &data->accessors[3 + i];
    acc->buffer_view = bv_indices;
    acc->component_type = cgltf_component_type_r_32u; // UNSIGNED_INT
    acc->type = cgltf_type_scalar;
    acc->count = per_mat_index_count[i];
    acc->offset = per_mat_index_offset[i] * sizeof(uint32_t);
  }

  // -------- 6) Materials --------
  data->materials_count = material_count;
  data->materials = ALLOC(material_count, sizeof(cgltf_material));

  // Create images and textures for materials (diffuse PNGs at
  // export/textures/<name>.png)
  data->images_count = material_count;
  data->images = ALLOC(material_count, sizeof(cgltf_image));
  data->textures_count = material_count;
  data->textures = ALLOC(material_count, sizeof(cgltf_texture));
  // Register KHR_materials_unlit so materials render as pure albedo (no
  // lighting)
  data->extensions_used_count = 1;
  data->extensions_used = ALLOC(1, sizeof(char *));
  data->extensions_used[0] = "KHR_materials_unlit";

  for (guint i = 0; i < material_count; ++i) {
    cgltf_material *mat = &data->materials[i];
    struct mat_s *src = g_ptr_array_index(mats, i);
    mat->name = ALLOC(1, 256);
    strncpy(mat->name, src->name, 255);
    // Wire a simple diffuse texture using the material name ->
    // export/textures/<name>.png
    cgltf_image *img = &data->images[i];
    cgltf_texture *tex = &data->textures[i];
    img->uri = ALLOC(1, 128);
    g_snprintf(img->uri, 128, "export/textures/%s.png", src->name);
    tex->image = img;
    // Hook into the material's baseColorTexture (simple diffuse)
    mat->pbr_metallic_roughness.base_color_texture.texture = tex;
    mat->pbr_metallic_roughness.base_color_texture.texcoord = 0;
    // Use a minimal material that references a base color texture only.
    // We still use the pbr_metallic_roughness struct to attach baseColorTexture
    // but we avoid setting metallic/roughness or base color factors.
    mat->has_pbr_metallic_roughness = 1;
    mat->pbr_metallic_roughness.roughness_factor = 1.0f;
    mat->pbr_metallic_roughness.metallic_factor = 0.0f;
    mat->pbr_metallic_roughness.base_color_factor[0] = 1.0f;
    mat->pbr_metallic_roughness.base_color_factor[1] = 1.0f;
    mat->pbr_metallic_roughness.base_color_factor[2] = 1.0f;
    mat->pbr_metallic_roughness.base_color_factor[3] = 1.0f;
    // mat->extensions_count = 0;
    // mat->extensions = NULL;
    // Mark material as unlit so the baseColorTexture is shown as-is (pure
    // albedo)
    mat->alpha_mode = cgltf_alpha_mode_opaque;
    mat->extensions_count = 1;
    mat->extensions = ALLOC(1, sizeof(cgltf_extension));
    mat->extensions[0].name = "KHR_materials_unlit";
    mat->extensions[0].data = NULL;
  }

  // -------- 7) Mesh + primitives (one per material) --------
  data->meshes_count = 1;
  data->meshes = ALLOC(1, sizeof(cgltf_mesh));
  cgltf_mesh *gltf_mesh = &data->meshes[0];

  gltf_mesh->primitives_count = material_count;
  gltf_mesh->primitives = ALLOC(material_count, sizeof(cgltf_primitive));

  for (guint i = 0; i < material_count; ++i) {
    cgltf_primitive *prim = &gltf_mesh->primitives[i];

    prim->type = cgltf_primitive_type_triangles;
    prim->material = &data->materials[i];
    prim->indices = &data->accessors[3 + i];

    prim->attributes_count = 3;
    prim->attributes = ALLOC(3, sizeof(cgltf_attribute));

    prim->attributes[0].name = "POSITION";
    prim->attributes[0].data = acc_pos;
    prim->attributes[0].type = cgltf_attribute_type_position;

    prim->attributes[1].name = "TEXCOORD_0";
    prim->attributes[1].data = acc_uv0;
    prim->attributes[1].type = cgltf_attribute_type_texcoord;

    prim->attributes[2].name = "TEXCOORD_1";
    prim->attributes[2].data = acc_uv1;
    prim->attributes[2].type = cgltf_attribute_type_texcoord;
  }

  // -------- 8) Node --------
  data->nodes_count = 1;
  data->nodes = ALLOC(1, sizeof(cgltf_node));
  data->nodes[0].mesh = gltf_mesh;
  data->nodes[0].has_translation = 1;
  data->nodes[0].has_scale = 1;
  data->nodes[0].translation[0] = 0.0f;
  data->nodes[0].translation[1] = 0.0f;
  data->nodes[0].translation[2] = 0.0f;
  data->nodes[0].scale[0] = scale;
  data->nodes[0].scale[1] = scale;
  data->nodes[0].scale[2] = scale;

  // -------- 9) Scene --------
  data->scenes_count = 1;
  data->scenes = ALLOC(1, sizeof(cgltf_scene));
  data->scenes[0].nodes = ALLOC(1, sizeof(cgltf_node *));
  data->scenes[0].nodes[0] = &data->nodes[0];
  data->scenes[0].nodes_count = 1;
  data->scene = 0;

  // -------- 10) Write file --------
  cgltf_result res = cgltf_write_file(&options, output_path, data);
  if (res != cgltf_result_success) {
    fprintf(stderr, "cgltf_write_file failed: %d\n", (int)res);
    g_set_error(err, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                "cgltf_write_file failed: %d", (int)res);
  }
  g_file_set_contents("mesh.bin", (const gchar *)buffer_data, total_buffer_size,
                      NULL);

  // -------- 11) Cleanup --------
  for (size_t i = 0; i < alloc_count; ++i) {
    g_free(allocs[i]);
  }
  g_free(allocs);
}