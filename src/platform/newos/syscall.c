#include "syscall.h"

long voxel_newos_syscall1(long number, long argument)
{
    long result;
    __asm__ __volatile__("int $0x80"
                         : "=a"(result)
                         : "a"(number), "D"(argument)
                         : "memory");
    return result;
}
