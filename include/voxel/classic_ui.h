#ifndef VOXEL_CLASSIC_UI_H
#define VOXEL_CLASSIC_UI_H

#include "voxel/surface.h"
#include <stdint.h>

/* Reusable Win9x-style UI primitives rendered into existing VOXEL surfaces. */
struct voxel_ui_rect { int32_t x, y, width, height; };

#define VOXEL_UI_FACE       0x00c0c0c0u
#define VOXEL_UI_LIGHT      0x00ffffffu
#define VOXEL_UI_MID        0x00808080u
#define VOXEL_UI_DARK       0x00404040u
#define VOXEL_UI_NAVY       0x00000080u
#define VOXEL_UI_DESKTOP    0x00008080u
#define VOXEL_UI_TEXT       0x00000000u
#define VOXEL_UI_WHITE      0x00ffffffu
#define VOXEL_UI_SELECTION  0x00000080u

void voxel_ui_text(struct voxel_surface *surface, int32_t x, int32_t y,
                   const char *text, uint32_t color);
void voxel_ui_bevel(struct voxel_surface *surface, struct voxel_ui_rect rect,
                    int pressed);
void voxel_ui_button(struct voxel_surface *surface, struct voxel_ui_rect rect,
                     const char *label, int pressed);
void voxel_ui_titlebar(struct voxel_surface *surface, struct voxel_ui_rect rect,
                       const char *title, int active);
void voxel_ui_window_frame(struct voxel_surface *surface,
                           struct voxel_ui_rect rect, const char *title,
                           int active);
void voxel_ui_menu_bar(struct voxel_surface *surface, int32_t x, int32_t y,
                       int32_t width, const char *const *labels, uint32_t count);
void voxel_ui_icon(struct voxel_surface *surface, int32_t x, int32_t y,
                   uint32_t kind, const char *label, int selected);
void voxel_ui_taskbar(struct voxel_surface *surface, int32_t screen_width,
                      int32_t screen_height, int start_pressed);
void voxel_ui_start_menu(struct voxel_surface *surface, int32_t x, int32_t y,
                         int selected);
void voxel_ui_menu(struct voxel_surface *surface, struct voxel_ui_rect rect,
                   const char *const *items, uint32_t count, int selected);
void voxel_ui_dialog(struct voxel_surface *surface, struct voxel_ui_rect rect,
                     const char *title, const char *message);

#endif
