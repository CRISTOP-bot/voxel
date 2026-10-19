#ifndef VOXEL_COMPOSITOR_H
#define VOXEL_COMPOSITOR_H

#include <stddef.h>
#include <stdint.h>
#include "voxel/framebuffer.h"
#include "voxel/surface.h"

#define VOXEL_MAX_LAYERS 16u

struct voxel_rect {
    int32_t x, y;
    uint32_t width, height;
};

struct voxel_layer {
    struct voxel_surface *surface;
    int32_t x, y;
    uint8_t visible;
};

struct voxel_compositor {
    uint32_t width, height;
    size_t stride_pixels;
    uint32_t *output;
    struct voxel_layer layers[VOXEL_MAX_LAYERS];
    size_t layer_count;
};

int voxel_compositor_init(struct voxel_compositor *compositor,
                          uint32_t width, uint32_t height,
                          size_t stride_pixels, uint32_t *output,
                          size_t output_pixels);
int voxel_compositor_add(struct voxel_compositor *compositor,
                         struct voxel_surface *surface, int32_t x, int32_t y,
                         size_t *layer_index);
int voxel_compositor_remove(struct voxel_compositor *compositor, size_t index);
int voxel_compositor_raise(struct voxel_compositor *compositor, size_t index);
void voxel_compositor_set_visible(struct voxel_compositor *compositor,
                                  size_t index, int visible);
void voxel_compositor_compose(struct voxel_compositor *compositor,
                              struct voxel_rect clip);
int voxel_compositor_present(struct voxel_compositor *compositor,
                             struct voxel_framebuffer *fb,
                             uint32_t origin_x, uint32_t origin_y);

#endif
