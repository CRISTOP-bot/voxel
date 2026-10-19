#ifndef VOXEL_SURFACE_H
#define VOXEL_SURFACE_H

#include <stddef.h>
#include <stdint.h>

enum voxel_pixel_format {
    VOXEL_XRGB8888 = 0, /* 0x00RRGGBB, opaque */
    VOXEL_ARGB8888 = 1  /* 0xAARRGGBB, straight alpha */
};

struct voxel_surface {
    uint32_t width;
    uint32_t height;
    size_t stride_pixels;
    size_t capacity_pixels;
    enum voxel_pixel_format format;
    uint32_t *pixels;
};

int voxel_surface_init(struct voxel_surface *surface, uint32_t width,
                       uint32_t height, size_t stride_pixels,
                       enum voxel_pixel_format format, uint32_t *storage,
                       size_t storage_pixels);
void voxel_surface_clear(struct voxel_surface *surface, uint32_t color);
int voxel_surface_fill_rect(struct voxel_surface *surface, int32_t x, int32_t y,
                            int32_t width, int32_t height, uint32_t color);
int voxel_surface_put_pixel(struct voxel_surface *surface, int32_t x, int32_t y,
                            uint32_t color);

#endif
