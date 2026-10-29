#ifndef VOXEL_TASKBAR_H
#define VOXEL_TASKBAR_H
#include "voxel/app.h"
int voxel_taskbar_hit(int32_t x, const uint8_t *visible);
void voxel_taskbar_draw(struct voxel_surface *surface,
                        struct voxel_app_host *host,
                        const struct voxel_app_descriptor *const *apps,
                        const uint8_t *visible, int active_app,
                        int32_t screen_width, int32_t screen_height,
                        int start_pressed);
void voxel_taskbar_start_menu(struct voxel_surface *surface,
                             const char *const *labels, uint32_t count,
                             int32_t y, int selected);
#endif
