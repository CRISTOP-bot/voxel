#ifndef VOXEL_TASKBAR_H
#define VOXEL_TASKBAR_H
#include "voxel/app.h"
void voxel_taskbar_buttons(struct voxel_surface *surface,
                           const struct voxel_app_descriptor *const *apps,
                           const uint8_t *visible, int active_app,
                           int32_t screen_height);
void voxel_taskbar_start_menu(struct voxel_surface *surface,
                             const char *const *labels, uint32_t count,
                             int32_t y, int selected);
#endif
