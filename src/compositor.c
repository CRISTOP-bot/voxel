#include "voxel/compositor.h"

#include <limits.h>
#include <stdint.h>

static int valid_surface(const struct voxel_surface *s)
{
    return s && s->pixels && s->width && s->height &&
           s->stride_pixels >= s->width &&
           (s->format == VOXEL_XRGB8888 || s->format == VOXEL_ARGB8888);
}

int voxel_compositor_init(struct voxel_compositor *compositor,
                          uint32_t width, uint32_t height,
                          size_t stride_pixels, uint32_t *output,
                          size_t output_pixels)
{
    size_t need;
    if (!compositor || !output || !width || !height ||
        stride_pixels < width || stride_pixels > SIZE_MAX / height)
        return -1;
    need = stride_pixels * (size_t)height;
    if (need > output_pixels || need > SIZE_MAX / sizeof(*output))
        return -1;
    compositor->width = width;
    compositor->height = height;
    compositor->stride_pixels = stride_pixels;
    compositor->output = output;
    compositor->layer_count = 0;
    return 0;
}

int voxel_compositor_add(struct voxel_compositor *compositor,
                         struct voxel_surface *surface, int32_t x, int32_t y,
                         size_t *layer_index)
{
    size_t i;
    if (!compositor || !valid_surface(surface) ||
        compositor->layer_count >= VOXEL_MAX_LAYERS)
        return -1;
    for (i = 0; i < compositor->layer_count; ++i)
        if (compositor->layers[i].surface == surface)
            return -1;
    i = compositor->layer_count++;
    compositor->layers[i].surface = surface;
    compositor->layers[i].x = x;
    compositor->layers[i].y = y;
    compositor->layers[i].visible = 1;
    if (layer_index)
        *layer_index = i;
    return 0;
}

int voxel_compositor_remove(struct voxel_compositor *compositor, size_t index)
{
    size_t i;
    if (!compositor || index >= compositor->layer_count)
        return -1;
    for (i = index + 1; i < compositor->layer_count; ++i)
        compositor->layers[i - 1] = compositor->layers[i];
    --compositor->layer_count;
    return 0;
}

int voxel_compositor_raise(struct voxel_compositor *compositor, size_t index)
{
    struct voxel_layer selected;
    size_t i;
    if (!compositor || index >= compositor->layer_count)
        return -1;
    selected = compositor->layers[index];
    for (i = index + 1; i < compositor->layer_count; ++i)
        compositor->layers[i - 1] = compositor->layers[i];
    compositor->layers[compositor->layer_count - 1] = selected;
    return 0;
}

void voxel_compositor_set_visible(struct voxel_compositor *compositor,
                                  size_t index, int visible)
{
    if (compositor && index < compositor->layer_count)
        compositor->layers[index].visible = (uint8_t)(visible != 0);
}

static uint32_t blend(uint32_t dst, uint32_t src, uint32_t alpha)
{
    uint32_t inv = 255u - alpha;
    uint32_t r = ((((src >> 16) & 255u) * alpha +
                   ((dst >> 16) & 255u) * inv + 127u) / 255u);
    uint32_t g = ((((src >> 8) & 255u) * alpha +
                   ((dst >> 8) & 255u) * inv + 127u) / 255u);
    uint32_t b = (((src & 255u) * alpha + (dst & 255u) * inv + 127u) / 255u);
    return (r << 16) | (g << 8) | b;
}

void voxel_compositor_compose(struct voxel_compositor *compositor,
                              struct voxel_rect clip)
{
    int64_t x0, y0, x1, y1;
    uint32_t y;
    if (!compositor || !compositor->output || !clip.width || !clip.height)
        return;
    x0 = clip.x;
    y0 = clip.y;
    x1 = x0 + (int64_t)clip.width;
    y1 = y0 + (int64_t)clip.height;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > compositor->width) x1 = compositor->width;
    if (y1 > compositor->height) y1 = compositor->height;
    if (x0 >= x1 || y0 >= y1)
        return;

    for (y = (uint32_t)y0; y < (uint32_t)y1; ++y) {
        uint32_t x;
        uint32_t *out = compositor->output + (size_t)y * compositor->stride_pixels;
        for (x = (uint32_t)x0; x < (uint32_t)x1; ++x) {
            uint32_t color = 0x00101828u;
            size_t i;
            for (i = 0; i < compositor->layer_count; ++i) {
                const struct voxel_layer *layer = &compositor->layers[i];
                const struct voxel_surface *s = layer->surface;
                int64_t sx, sy;
                uint32_t source;
                if (!layer->visible || !valid_surface(s))
                    continue;
                sx = (int64_t)x - layer->x;
                sy = (int64_t)y - layer->y;
                if (sx < 0 || sy < 0 || sx >= s->width || sy >= s->height)
                    continue;
                source = s->pixels[(size_t)sy * s->stride_pixels + (size_t)sx];
                if (s->format == VOXEL_XRGB8888)
                    color = source & 0x00ffffffu;
                else
                    color = blend(color, source & 0x00ffffffu, source >> 24);
            }
            out[x] = color;
        }
    }
}

int voxel_compositor_present(struct voxel_compositor *compositor,
                             struct voxel_framebuffer *fb,
                             uint32_t origin_x, uint32_t origin_y)
{
    if (!compositor || !fb || compositor->width > UINT32_MAX - origin_x ||
        compositor->height > UINT32_MAX - origin_y ||
        origin_x > fb->info.width || origin_y > fb->info.height ||
        compositor->width > fb->info.width - origin_x ||
        compositor->height > fb->info.height - origin_y)
        return -1;
    return voxel_framebuffer_blit(fb, origin_x, origin_y,
                                  compositor->width, compositor->height,
                                  compositor->output,
                                  compositor->stride_pixels);
}
