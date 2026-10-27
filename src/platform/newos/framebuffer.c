#include "voxel/framebuffer.h"
#include "syscall.h"

#include <stdint.h>


int voxel_framebuffer_open(struct voxel_framebuffer *fb)
{
    uint64_t min_pitch;
    if (!fb)
        return -1;
    fb->info.present = 0;
    fb->info.width = fb->info.height = fb->info.pitch = 0;
    fb->info.bpp = fb->info._pad = 0;
    if (voxel_newos_syscall1(VOXEL_SYS_FBINFO, (long)(uintptr_t)&fb->info) != 0)
        return -1;
    if (!fb->info.present || !fb->info.width || !fb->info.height ||
        fb->info.width > INT32_MAX || fb->info.height > INT32_MAX ||
        (fb->info.bpp != 24u && fb->info.bpp != 32u))
        return -1;
    if (fb->info.width > UINT64_MAX / (fb->info.bpp / 8u))
        return -1;
    min_pitch = fb->info.width * (fb->info.bpp / 8u);
    if (fb->info.pitch < min_pitch)
        return -1;
    return 0;
}

int voxel_framebuffer_blit(struct voxel_framebuffer *fb,
                           uint32_t x, uint32_t y,
                           uint32_t width, uint32_t height,
                           const uint32_t *pixels, size_t stride_pixels)
{
    uint32_t row;
    if (!fb || !pixels || !width || !height || stride_pixels < width ||
        width > 4096u || height > 4096u || !fb->info.present ||
        x > fb->info.width || y > fb->info.height ||
        width > fb->info.width - x || height > fb->info.height - y ||
        stride_pixels > SIZE_MAX / height ||
        stride_pixels * height > SIZE_MAX / sizeof(*pixels))
        return -1;

    /* SYS_FBWRITE consumes XRGB8888 words; the kernel converts to native
     * 24/32-bpp RGB masks. Panel pitch does not describe this source buffer. */
    for (row = 0; row < height; ++row) {
        struct voxel_fbwrite request;
        request.x = x;
        request.y = (uint64_t)y + row;
        request.w = width;
        request.h = 1;
        request.pixels = (uint64_t)(uintptr_t)(pixels + (size_t)row * stride_pixels);
        request.pixlen = (uint64_t)width * sizeof(*pixels);
        if (voxel_newos_syscall1(VOXEL_SYS_FBWRITE,
                           (long)(uintptr_t)&request) != 0)
            return -1;
    }
    return 0;
}
