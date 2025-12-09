#ifndef _MYGLTF_
#define _MYGLTF_

#include <glib.h>

struct mygltf_material_s {
  GString *name;
  GArray *textures;
};

struct mygltf_mesh_s {
  GString *name;
  GArray *vertices;
  GArray *normals;
  GArray *uvs;
  GArray *indices;
  struct mygltf_material_s *material;
};

struct mygltf_scene_s {
  GArray *meshes;
};

#endif // _MYGLTF_
