#include "voxel/surface.h"

#include <limits.h>
#include <stdint.h>

static int valid_surface(const struct voxel_surface *surface)
{
    return surface && surface->pixels && surface->width && surface->height &&
           surface->stride_pixels >= surface->width &&
           surface->stride_pixels <= SIZE_MAX / surface->height &&
           surface->stride_pixels * surface->height <= surface->capacity_pixels &&
           (surface->format == VOXEL_XRGB8888 ||
            surface->format == VOXEL_ARGB8888);
}

int voxel_surface_init(struct voxel_surface *surface, uint32_t width,
                       uint32_t height, size_t stride_pixels,
                       enum voxel_pixel_format format, uint32_t *storage,
                       size_t storage_pixels)
{
    size_t required;
    if (!surface || !storage || !width || !height ||
        stride_pixels < width ||
        (format != VOXEL_XRGB8888 && format != VOXEL_ARGB8888) ||
        stride_pixels > SIZE_MAX / height)
        return -1;
    required = stride_pixels * (size_t)height;
    if (required > storage_pixels || required > SIZE_MAX / sizeof(*storage))
        return -1;
    surface->width = width;
    surface->height = height;
    surface->stride_pixels = stride_pixels;
    surface->capacity_pixels = storage_pixels;
    surface->format = format;
    surface->pixels = storage;
    return 0;
}

void voxel_surface_clear(struct voxel_surface *surface, uint32_t color)
{
    uint32_t y;
    if (!valid_surface(surface))
        return;
    for (y = 0; y < surface->height; ++y) {
        size_t x;
        uint32_t *row = surface->pixels + (size_t)y * surface->stride_pixels;
        for (x = 0; x < surface->width; ++x)
            row[x] = color;
    }
}

int voxel_surface_fill_rect(struct voxel_surface *surface, int32_t x,
                            int32_t y, int32_t width, int32_t height,
                            uint32_t color)
{
    int64_t left = x, top = y, right, bottom;
    uint32_t cy;
    if (!valid_surface(surface) || width <= 0 || height <= 0)
        return -1;
    right = left + (int64_t)width;
    bottom = top + (int64_t)height;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > surface->width) right = surface->width;
    if (bottom > surface->height) bottom = surface->height;
    if (left >= right || top >= bottom)
        return 0;
    for (cy = (uint32_t)top; cy < (uint32_t)bottom; ++cy) {
        size_t cx;
        uint32_t *row = surface->pixels + (size_t)cy * surface->stride_pixels;
        for (cx = (size_t)left; cx < (size_t)right; ++cx)
            row[cx] = color;
    }
    return 0;
}

int voxel_surface_put_pixel(struct voxel_surface *surface, int32_t x,
                            int32_t y, uint32_t color)
{
    if (!valid_surface(surface) || x < 0 || y < 0 ||
        (uint32_t)x >= surface->width || (uint32_t)y >= surface->height)
        return -1;
    surface->pixels[(size_t)(uint32_t)y * surface->stride_pixels +
                    (uint32_t)x] = color;
    return 0;
}
