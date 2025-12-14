#ifndef _MYGLTF_
#define _MYGLTF_

#include <glib.h>

#include "mesh.h"

void export_mesh_to_gltf(const struct mesh_s *mesh, gfloat scale,
                         const gchar *output_path, GError **err);
#endif // _MYGLTF_
