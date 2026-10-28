#include "voxel/app.h"
#include "voxel/classic_ui.h"
void voxel_helloworld_draw(struct voxel_app_context*a){voxel_surface_clear(a->surface,VOXEL_UI_FACE);voxel_ui_window_frame(a->surface, (struct voxel_ui_rect){2,2,(int32_t)a->surface->width-4,(int32_t)a->surface->height-4}, "Hello, World!", 1);voxel_ui_text(a->surface,20,58,"HELLO FROM VOXEL USERSPACE",VOXEL_UI_TEXT);voxel_ui_text(a->surface,20,78,"This app is an in-process UI module.",VOXEL_UI_TEXT);}
