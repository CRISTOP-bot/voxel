#ifndef VOXEL_VIRTUAL_KEYBOARD_H
#define VOXEL_VIRTUAL_KEYBOARD_H
#include "voxel/surface.h"
#include "voxel/app.h"
#include <stdint.h>
#define VOXEL_VK_ROW_H 23
void voxel_vk_draw(struct voxel_surface *s, int32_t y);
/* Adds selected ASCII key, returns 1; special key 8=backspace, 13=enter. */
int voxel_vk_pointer(struct voxel_surface *s, int32_t x, int32_t y,
                     int32_t top, char *text, uint32_t capacity,
                     uint32_t *length, uint32_t *key);
#endif
