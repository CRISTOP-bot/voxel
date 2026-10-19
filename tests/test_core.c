#include "voxel/compositor.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void test_surface_bounds_and_stride(void)
{
    uint32_t storage[12];
    struct voxel_surface surface;
    size_t i;
    for (i = 0; i < 12; ++i)
        storage[i] = 0xdeadbeefu;
    assert(voxel_surface_init(&surface, 3, 2, 4, VOXEL_XRGB8888,
                              storage, 8) == 0);
    voxel_surface_clear(&surface, 0x00112233u);
    assert(storage[0] == 0x00112233u && storage[2] == 0x00112233u);
    assert(storage[3] == 0xdeadbeefu); /* row padding is not surface pixels */
    assert(voxel_surface_fill_rect(&surface, -1, 0, 2, 2,
                                   0x00abcdefu) == 0);
    assert(storage[0] == 0x00abcdefu);
    assert(storage[1] == 0x00112233u);
    assert(voxel_surface_put_pixel(&surface, 3, 0, 0) == -1);
    assert(voxel_surface_init(&surface, 3, 2, 4, VOXEL_XRGB8888,
                              storage, 7) == -1);
}

static void test_composition_clip_alpha_order(void)
{
    uint32_t output[60];
    uint32_t bottom_pixels[12];
    uint32_t top_pixels[4];
    uint32_t cover_pixels[1] = { 0xff00ff00u };
    struct voxel_surface bottom, top, cover;
    struct voxel_compositor compositor;
    struct voxel_rect clip = { 1, 1, 2, 2 };
    size_t i, cover_index;

    for (i = 0; i < 60; ++i)
        output[i] = 0xdeadbeefu;
    for (i = 0; i < 12; ++i)
        bottom_pixels[i] = 0x00112233u;
    for (i = 0; i < 4; ++i)
        top_pixels[i] = 0x80ff0000u;
    assert(voxel_surface_init(&bottom, 4, 3, 4, VOXEL_XRGB8888,
                              bottom_pixels, 12) == 0);
    assert(voxel_surface_init(&top, 2, 2, 2, VOXEL_ARGB8888,
                              top_pixels, 4) == 0);
    assert(voxel_surface_init(&cover, 1, 1, 1, VOXEL_XRGB8888,
                              cover_pixels, 1) == 0);
    assert(voxel_compositor_init(&compositor, 8, 6, 10, output, 60) == 0);
    assert(voxel_compositor_add(&compositor, &bottom, 0, 0, 0) == 0);
    assert(voxel_compositor_add(&compositor, &top, 1, 1, 0) == 0);
    voxel_compositor_compose(&compositor, clip);
    assert(output[1 + 1 * 10] == 0x00891119u);
    assert(output[2 + 2 * 10] == 0x00891119u);
    assert(output[0] == 0xdeadbeefu); /* outside requested clip untouched */
    assert(output[7 + 1 * 10] == 0xdeadbeefu); /* stride padding untouched */

    assert(voxel_compositor_add(&compositor, &cover, 2, 2,
                                &cover_index) == 0);
    assert(cover_index == 2);
    voxel_compositor_compose(&compositor, (struct voxel_rect){0, 0, 8, 6});
    assert(output[2 + 2 * 10] == 0x0000ff00u);
    assert(voxel_compositor_raise(&compositor, 1) == 0);
    /* Raising top shifts cover from slot 2 to slot 1. */
    assert(compositor.layers[0].surface == &bottom);
    assert(compositor.layers[1].surface == &cover);
    assert(compositor.layers[2].surface == &top);
    voxel_compositor_compose(&compositor, (struct voxel_rect){0, 0, 8, 6});
    assert(output[2 + 2 * 10] == 0x00807f00u);
    voxel_compositor_set_visible(&compositor, 2, 0);
    voxel_compositor_compose(&compositor, (struct voxel_rect){0, 0, 8, 6});
    assert(output[2 + 2 * 10] == 0x0000ff00u);
    assert(voxel_compositor_remove(&compositor, 1) == 0);
    assert(compositor.layer_count == 2);
    assert(compositor.layers[0].surface == &bottom);
    assert(compositor.layers[1].surface == &top);
}

static void test_invalid_compositor_arguments(void)
{
    uint32_t output[4];
    struct voxel_compositor compositor;
    assert(voxel_compositor_init(&compositor, 2, 2, 1, output, 4) == -1);
    assert(voxel_compositor_init(&compositor, 2, 2, 2, output, 3) == -1);
}

int main(void)
{
    test_surface_bounds_and_stride();
    test_composition_clip_alpha_order();
    test_invalid_compositor_arguments();
    puts("VOXEL core tests: PASS");
    return 0;
}
