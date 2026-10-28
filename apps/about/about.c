#include "voxel/app.h"
#include "voxel/classic_ui.h"

void voxel_about_draw(struct voxel_app_context *a)
{
    voxel_surface_clear(a->surface,VOXEL_UI_FACE);
    voxel_ui_window_frame(a->surface, (struct voxel_ui_rect){2,2,(int32_t)a->surface->width-4,(int32_t)a->surface->height-4}, "About VOXEL", 1);
    voxel_ui_icon(a->surface,18,48,0,"VOXEL",0);
    voxel_ui_text(a->surface,94,58,"VOXEL CLASSIC DESKTOP",VOXEL_UI_TEXT);
    voxel_ui_text(a->surface,94,76,"SOFTWARE UI / NEWOS",VOXEL_UI_TEXT);
    voxel_ui_text(a->surface,18,116,"Version 0.2.0",VOXEL_UI_TEXT);
    voxel_ui_text(a->surface,18,132,"Independent graphical shell demo",VOXEL_UI_TEXT);
    voxel_ui_text(a->surface,18,148,"Built with the VOXEL surface compositor",VOXEL_UI_TEXT);
}
