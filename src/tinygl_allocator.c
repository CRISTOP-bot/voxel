/* A bounded, reclaiming allocator for TinyGL when no host allocator is
 * available. Blocks are naturally aligned and coalesced on free. */
#include <TGL/gl.h>
#include <stddef.h>
#include <stdint.h>

#define ARENA_BYTES (16u * 1024u * 1024u)
#define VOXEL_TGL_ALIGNMENT 16u

typedef union voxel_tgl_block voxel_tgl_block;
union __attribute__((aligned(VOXEL_TGL_ALIGNMENT))) voxel_tgl_block {
    struct {
        size_t size;
        voxel_tgl_block *next;
        unsigned int is_free;
    } m;
    uint64_t alignment[2];
};

/* NEWOS's current C SDK does not expose max_align_t. TinyGL on NEWOS is
 * x86_64, whose ABI requires at most 16-byte alignment for general objects. */
static unsigned char arena[ARENA_BYTES]
    __attribute__((aligned(VOXEL_TGL_ALIGNMENT)));
_Static_assert(sizeof(voxel_tgl_block) % VOXEL_TGL_ALIGNMENT == 0u,
               "allocator headers must preserve payload alignment");
static voxel_tgl_block *first_block;

static size_t aligned_size(size_t size)
{
    const size_t alignment = VOXEL_TGL_ALIGNMENT;
    if (size > SIZE_MAX - (alignment - 1u))
        return 0;
    return (size + alignment - 1u) / alignment * alignment;
}

static void init_arena(void)
{
    if (first_block)
        return;
    first_block = (voxel_tgl_block *)(void *)arena;
    first_block->m.size = ARENA_BYTES - sizeof(*first_block);
    first_block->m.next = 0;
    first_block->m.is_free = 1;
}

void *gl_malloc(GLint requested)
{
    size_t need;
    voxel_tgl_block *block;
    if (requested <= 0)
        return 0;
    need = aligned_size((size_t)requested);
    if (!need || need > ARENA_BYTES - sizeof(voxel_tgl_block))
        return 0;
    init_arena();
    for (block = first_block; block; block = block->m.next) {
        if (!block->m.is_free || block->m.size < need)
            continue;
        if (block->m.size >= need + sizeof(*block) + VOXEL_TGL_ALIGNMENT) {
            voxel_tgl_block *remainder = (voxel_tgl_block *)(void *)
                ((unsigned char *)(void *)(block + 1) + need);
            remainder->m.size = block->m.size - need - sizeof(*block);
            remainder->m.next = block->m.next;
            remainder->m.is_free = 1;
            block->m.next = remainder;
            block->m.size = need;
        }
        block->m.is_free = 0;
        return (void *)(block + 1);
    }
    return 0;
}

void *gl_zalloc(GLint requested)
{
    unsigned char *memory = (unsigned char *)gl_malloc(requested);
    size_t i;
    if (!memory)
        return 0;
    for (i = 0; i < (size_t)requested; ++i)
        memory[i] = 0;
    return memory;
}

void gl_free(void *memory)
{
    voxel_tgl_block *block;
    uintptr_t address = (uintptr_t)memory;
    uintptr_t begin = (uintptr_t)(void *)arena;
    uintptr_t end = begin + ARENA_BYTES;
    if (!memory || address < begin + sizeof(voxel_tgl_block) || address >= end)
        return;
    init_arena();
    for (block = first_block; block; block = block->m.next) {
        if ((void *)(block + 1) != memory)
            continue;
        if (block->m.is_free)
            return;
        block->m.is_free = 1;
        while (block->m.next && block->m.next->m.is_free) {
            voxel_tgl_block *next = block->m.next;
            block->m.size += sizeof(*next) + next->m.size;
            block->m.next = next->m.next;
        }
        return;
    }
}
