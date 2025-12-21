#include "img.h"

void downsample_image(const struct img_s *img_src, struct img_s *img_dst) {
  float h_scale = (float)img_dst->h / (float)img_src->h;
  float w_scale = (float)img_dst->w / (float)img_src->w;

  struct rgbsum_s {
    guint r;
    guint g;
    guint b;
    guint a;
    guint count;
  };
  struct rgbsum_s *vals = g_new0(struct rgbsum_s, img_dst->w * img_dst->h);
  for (guint y = 0; y < img_src->h; y++) {
    for (guint x = 0; x < img_src->w; x++) {

      guint src_idx = y * img_src->w + x;
      guint dst_x = (guint)(x * w_scale);
      guint dst_y = (guint)(y * h_scale);
      guint dst_idx = dst_y * img_dst->w + dst_x;
      struct rgba_s color = img_src->data[src_idx];
      vals[dst_idx].r += color.r;
      vals[dst_idx].g += color.g;
      vals[dst_idx].b += color.b;
      vals[dst_idx].a += color.a;
      vals[dst_idx].count++;
    }
  }
#define RGBA_MAX(a) ((a) > 255 ? 255 : (a))

  for (guint i = 0; i < img_dst->w * img_dst->h; i++) {
    if (vals[i].count > 0) {
      img_dst->data[i].r = RGBA_MAX(vals[i].r / vals[i].count);
      img_dst->data[i].g = RGBA_MAX(vals[i].g / vals[i].count);
      img_dst->data[i].b = RGBA_MAX(vals[i].b / vals[i].count);
      img_dst->data[i].a = RGBA_MAX(vals[i].a / vals[i].count);
    } else {
      img_dst->data[i].r = 0;
      img_dst->data[i].g = 0;
      img_dst->data[i].b = 0;
      img_dst->data[i].a = 0;
    }
  }
#undef RGBA_MAX
  g_free(vals);
}