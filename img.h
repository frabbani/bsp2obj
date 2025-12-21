#include <glib.h>

#define CLAMP_COLOR_COMPONENT(a) ((a) > 255 ? 255 : (a) < 0 ? 0 : (a))

struct img_s {
  guint w;
  guint h;
  struct rgba_s *data;
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

void downsample_image(const struct img_s *img_src, struct img_s *img_dst);
