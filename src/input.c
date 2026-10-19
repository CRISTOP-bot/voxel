#include "voxel/input.h"

/* The x86_64 int $0x80 calling convention is the syscall ABI used by NEWOS. */
static long voxel_syscall1(long number, long argument)
{
    long result;
    __asm__ __volatile__("int $0x80"
                         : "=a"(result)
                         : "a"(number), "D"(argument)
                         : "memory");
    return result;
}

int voxel_pointer_poll(struct voxel_pointer *pointer)
{
    struct voxel_mouse_state state;
    if (!pointer || voxel_syscall1(VOXEL_SYS_MOUSE_GET,
                                   (long)(uintptr_t)&state) != 0)
        return -1;
    if (state.x < INT32_MIN || state.x > INT32_MAX ||
        state.y < INT32_MIN || state.y > INT32_MAX)
        return -1;
    pointer->x = (int32_t)state.x;
    pointer->y = (int32_t)state.y;
    pointer->buttons = state.buttons;
    pointer->wheel = state.wheel;
    pointer->sequence = state.seq;
    return 0;
}

int voxel_sleep_ms(uint32_t milliseconds)
{
    struct voxel_timespec delay;
    delay.tv_sec = (int64_t)(milliseconds / 1000u);
    delay.tv_nsec = (int64_t)(milliseconds % 1000u) * 1000000;
    return (int)voxel_syscall1(VOXEL_SYS_SLEEP,
                               (long)(uintptr_t)&delay);
}
