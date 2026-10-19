#ifndef VOXEL_ABI_H
#define VOXEL_ABI_H

/* NEWOS x86_64 userspace ABI, verified against NEWOS-kernel/abi/syscall_abi.h.
 * Keep these fixed-width layouts in sync with that public kernel ABI. */
#include <stddef.h>
#include <stdint.h>

#define VOXEL_SYS_READ       0
#define VOXEL_SYS_WRITE      1
#define VOXEL_SYS_SLEEP     35
#define VOXEL_SYS_FBINFO   453
#define VOXEL_SYS_FBWRITE  454
#define VOXEL_SYS_MOUSE_GET 455

struct voxel_fbinfo {
    uint64_t present;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint32_t bpp;
    uint32_t _pad;
};

struct voxel_fbwrite {
    uint64_t x, y, w, h;
    uint64_t pixels;
    uint64_t pixlen;
};

struct voxel_mouse_state {
    int64_t x;
    int64_t y;
    uint64_t seq;
    uint32_t buttons;
    int32_t wheel;
};

struct voxel_timespec {
    int64_t tv_sec;
    int64_t tv_nsec;
};

/* Compile-time checks catch accidental ABI drift on both host tests and target. */
typedef char voxel_abi_fbinfo_size[(sizeof(struct voxel_fbinfo) == 40) ? 1 : -1];
typedef char voxel_abi_fbwrite_size[(sizeof(struct voxel_fbwrite) == 48) ? 1 : -1];
typedef char voxel_abi_mouse_size[(sizeof(struct voxel_mouse_state) == 32) ? 1 : -1];
typedef char voxel_abi_timespec_size[(sizeof(struct voxel_timespec) == 16) ? 1 : -1];

#endif
