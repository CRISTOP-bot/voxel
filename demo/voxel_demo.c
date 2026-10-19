#include "voxel/compositor.h"
#include "voxel/input.h"

#include <stdint.h>
#include <stddef.h>

#define VIEW_W 640u
#define VIEW_H 480u
#define W1_W 310u
#define W1_H 210u
#define W2_W 260u
#define W2_H 190u
#define W3_W 300u
#define W3_H 170u
#define CURSOR_W 12u
#define CURSOR_H 18u

static uint32_t output_pixels[VIEW_W * VIEW_H];
static uint32_t desktop_pixels[VIEW_W * VIEW_H];
static uint32_t window1_pixels[W1_W * W1_H];
static uint32_t window2_pixels[W2_W * W2_H];
static uint32_t window3_pixels[W3_W * W3_H];
static uint32_t cursor_pixels[CURSOR_W * CURSOR_H];

static struct voxel_surface desktop, window1, window2, window3, cursor;
static const char no_fb[] = "VOXEL: no supported NEWOS framebuffer; boot with Limine graphics.\n";
static const char no_space[] = "VOXEL: unable to initialize compositor surfaces.\n";
static const char present_error[] = "VOXEL: framebuffer presentation failed.\n";
static const char ready[] = "VOXEL: software compositor ready; drag title bars, click the red square to close.\n";

static long os_write(const char *text, size_t length)
{
    long result;
    __asm__ __volatile__("int $0x80" : "=a"(result)
                         : "a"((long)VOXEL_SYS_WRITE), "D"((long)1),
                           "S"((long)(uintptr_t)text), "d"((long)length)
                         : "memory");
    return result;
}

static uint32_t get_layer_index(const struct voxel_compositor *c,
                                const struct voxel_surface *surface)
{
    size_t i;
    for (i = 0; i < c->layer_count; ++i)
        if (c->layers[i].surface == surface)
            return (uint32_t)i;
    return UINT32_MAX;
}

static void draw_window(struct voxel_surface *s, uint32_t width,
                        uint32_t height, uint32_t tint)
{
    uint32_t i;
    voxel_surface_clear(s, 0xff111827u);
    (void)voxel_surface_fill_rect(s, 0, 0, (int32_t)width, 24, 0xff202b3du);
    (void)voxel_surface_fill_rect(s, 10, 8, 9, 9, 0xff48b8a8u);
    (void)voxel_surface_fill_rect(s, 25, 8, (int32_t)(width - 64), 8,
                                  0xffa9bdd4u);
    (void)voxel_surface_fill_rect(s, (int32_t)width - 22, 6, 14, 14,
                                  0xffe35f68u);
    (void)voxel_surface_fill_rect(s, 12, 38, (int32_t)width - 24, 1,
                                  0xff35445bu);
    for (i = 0; i < 5; ++i) {
        uint32_t bar = 34u + (i * 31u) % (width > 100u ? width - 70u : 1u);
        if (bar > width - 24u)
            bar = width - 24u;
        (void)voxel_surface_fill_rect(s, 14, 54 + (int32_t)i * 17,
                                      (int32_t)bar, 7,
                                      (i == 0) ? tint : 0xff33435au);
    }
    if (height > 140u) {
        (void)voxel_surface_fill_rect(s, 14, (int32_t)height - 34,
                                      (int32_t)width - 28, 18,
                                      0xff182337u);
        (void)voxel_surface_fill_rect(s, 19, (int32_t)height - 29,
                                      (int32_t)(width / 3u), 8, tint);
    }
}

static void draw_desktop(void)
{
    uint32_t y;
    for (y = 0; y < VIEW_H; ++y) {
        uint32_t x;
        uint32_t shade = (y * 40u) / VIEW_H;
        for (x = 0; x < VIEW_W; ++x) {
            uint32_t r = 8u + shade / 3u;
            uint32_t g = 22u + shade;
            uint32_t b = 42u + shade * 2u;
            desktop_pixels[(size_t)y * VIEW_W + x] = (r << 16) | (g << 8) | b;
        }
    }
    /* Subtle geometric bands make the desktop visibly different from a test pattern. */
    for (y = 0; y < VIEW_H; y += 48u)
        (void)voxel_surface_fill_rect(&desktop, 0, (int32_t)y,
                                      (int32_t)VIEW_W, 1, 0xff16314au);
}

static void draw_pointer(void)
{
    uint32_t y;
    voxel_surface_clear(&cursor, 0x00000000u);
    for (y = 0; y < CURSOR_H; ++y) {
        uint32_t x;
        for (x = 0; x < CURSOR_W; ++x) {
            if (x <= y / 2u && y < 15u)
                cursor_pixels[(size_t)y * CURSOR_W + x] =
                    (x == 0u || x == y / 2u || y == 14u) ?
                    0xfff4f7fau : 0xff55d6c2u;
        }
    }
}

static uint32_t parse_frame_limit(int argc, char **argv)
{
    int i;
    for (i = 1; i + 1 < argc; ++i) {
        if (argv[i][0] == '-' && argv[i][1] == '-' &&
            argv[i][2] == 'f' && argv[i][3] == 'r' &&
            argv[i][4] == 'a' && argv[i][5] == 'm' &&
            argv[i][6] == 'e' && argv[i][7] == 's' && argv[i][8] == '\0') {
            uint32_t value = 0;
            const char *p = argv[i + 1];
            if (!*p)
                return 0;
            for (; *p >= '0' && *p <= '9'; ++p) {
                uint32_t digit = (uint32_t)(*p - '0');
                if (value > (UINT32_MAX - digit) / 10u)
                    return UINT32_MAX;
                value = value * 10u + digit;
            }
            if (*p)
                return 0;
            return value;
        }
    }
    return 0;
}

static int hit_title(const struct voxel_layer *layer, int32_t x, int32_t y)
{
    return layer->visible && x >= layer->x && y >= layer->y &&
           (int64_t)x < (int64_t)layer->x + layer->surface->width &&
           (int64_t)y < (int64_t)layer->y + 24;
}

int main(int argc, char **argv)
{
    struct voxel_framebuffer fb;
    struct voxel_compositor compositor;
    struct voxel_pointer pointer;
    struct voxel_surface *dragging = 0;
    int32_t drag_dx = 0, drag_dy = 0;
    uint32_t previous_buttons = 0;
    uint32_t frame_limit = parse_frame_limit(argc, argv);
    uint32_t frame = 0;
    uint32_t origin_x, origin_y;
    struct voxel_rect full_view = { 0, 0, VIEW_W, VIEW_H };

    if (voxel_framebuffer_open(&fb) != 0) {
        (void)os_write(no_fb, sizeof(no_fb) - 1u);
        return 1;
    }
    if (fb.info.width < VIEW_W || fb.info.height < VIEW_H) {
        (void)os_write(no_fb, sizeof(no_fb) - 1u);
        return 1;
    }
    if (voxel_surface_init(&desktop, VIEW_W, VIEW_H, VIEW_W,
                           VOXEL_XRGB8888, desktop_pixels,
                           sizeof(desktop_pixels) / sizeof(desktop_pixels[0])) ||
        voxel_surface_init(&window1, W1_W, W1_H, W1_W, VOXEL_ARGB8888,
                           window1_pixels, sizeof(window1_pixels) / sizeof(window1_pixels[0])) ||
        voxel_surface_init(&window2, W2_W, W2_H, W2_W, VOXEL_ARGB8888,
                           window2_pixels, sizeof(window2_pixels) / sizeof(window2_pixels[0])) ||
        voxel_surface_init(&window3, W3_W, W3_H, W3_W, VOXEL_ARGB8888,
                           window3_pixels, sizeof(window3_pixels) / sizeof(window3_pixels[0])) ||
        voxel_surface_init(&cursor, CURSOR_W, CURSOR_H, CURSOR_W, VOXEL_ARGB8888,
                           cursor_pixels, sizeof(cursor_pixels) / sizeof(cursor_pixels[0])) ||
        voxel_compositor_init(&compositor, VIEW_W, VIEW_H, VIEW_W,
                              output_pixels,
                              sizeof(output_pixels) / sizeof(output_pixels[0]))) {
        (void)os_write(no_space, sizeof(no_space) - 1u);
        return 1;
    }

    draw_desktop();
    draw_window(&window1, W1_W, W1_H, 0xff45c7b4u);
    draw_window(&window2, W2_W, W2_H, 0xff7ba9f6u);
    draw_window(&window3, W3_W, W3_H, 0xfff0b45fu);
    draw_pointer();
    (void)voxel_compositor_add(&compositor, &desktop, 0, 0, 0);
    (void)voxel_compositor_add(&compositor, &window1, 35, 38, 0);
    (void)voxel_compositor_add(&compositor, &window2, 305, 125, 0);
    (void)voxel_compositor_add(&compositor, &window3, 110, 278, 0);
    (void)voxel_compositor_add(&compositor, &cursor, 0, 0, 0);

    origin_x = (uint32_t)((fb.info.width - VIEW_W) / 2u);
    origin_y = (uint32_t)((fb.info.height - VIEW_H) / 2u);
    (void)os_write(ready, sizeof(ready) - 1u);

    while (frame_limit == 0u || frame < frame_limit) {
        if (voxel_pointer_poll(&pointer) == 0) {
            int32_t mx = pointer.x - (int32_t)origin_x;
            int32_t my = pointer.y - (int32_t)origin_y;
            int pressed = (pointer.buttons & 1u) && !(previous_buttons & 1u);
            uint32_t cursor_index = get_layer_index(&compositor, &cursor);
            size_t i;
            if (cursor_index == UINT32_MAX)
                return 1;
            compositor.layers[cursor_index].x = mx;
            compositor.layers[cursor_index].y = my;
            (void)voxel_compositor_raise(&compositor, cursor_index);
            cursor_pixels[0] = 0xfff4f7fau;
      
            if (pressed) {
                for (i = compositor.layer_count - 1u; i > 0; --i) {
                    struct voxel_layer *layer = &compositor.layers[i - 1u];
                    if (layer->surface == &cursor || layer->surface == &desktop ||
                        !hit_title(layer, mx, my))
                        continue;
                    if (mx >= layer->x + (int32_t)layer->surface->width - 22 &&
                        mx < layer->x + (int32_t)layer->surface->width - 8) {
                        voxel_compositor_set_visible(&compositor, i - 1u, 0);
                        dragging = 0;
                    } else {
                        struct voxel_surface *picked = layer->surface;
                        drag_dx = mx - layer->x;
                        drag_dy = my - layer->y;
                        (void)voxel_compositor_raise(&compositor, i - 1u);
                        cursor_index = get_layer_index(&compositor, &cursor);
                        if (cursor_index != UINT32_MAX)
                            (void)voxel_compositor_raise(&compositor, cursor_index);
                        dragging = picked;
                    }
                    break;
                }
            }
            if (!(pointer.buttons & 1u))
                dragging = 0;
            if (dragging) {
                uint32_t index = get_layer_index(&compositor, dragging);
                if (index != UINT32_MAX) {
                    compositor.layers[index].x = mx - drag_dx;
                    compositor.layers[index].y = my - drag_dy;
                }
            }
            previous_buttons = pointer.buttons;
        }

        voxel_compositor_compose(&compositor, full_view);
        if (voxel_compositor_present(&compositor, &fb, origin_x, origin_y) != 0) {
            (void)os_write(present_error, sizeof(present_error) - 1u);
            return 1;
        }
        ++frame;
        if (voxel_sleep_ms(16u) < 0)
            return 1;
    }
    return 0;
}
