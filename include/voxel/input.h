#ifndef VOXEL_INPUT_H
#define VOXEL_INPUT_H

#include <stdint.h>
#include "voxel/abi.h"

struct voxel_pointer {
    int32_t x, y;
    uint32_t buttons;
    int32_t wheel;
    uint64_t sequence;
};

/* Polls the kernel's latest PS/2 pointer snapshot. sequence changes per packet. */
int voxel_pointer_poll(struct voxel_pointer *pointer);

/* NEWOS currently exposes no userspace key-event syscall. This waits for a
 * duration using the implemented SYS_SLEEP call; it is not a keyboard API. */
int voxel_sleep_ms(uint32_t milliseconds);

#endif
