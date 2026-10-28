#include "voxel/app.h"
#include "voxel/classic_ui.h"

void voxel_settings_draw(struct voxel_app_context *a)
{
    struct voxel_theme_state *t=a->host?a->host->theme:0;
    voxel_surface_clear(a->surface,VOXEL_UI_FACE);
    voxel_ui_window_frame(a->surface, (struct voxel_ui_rect){2,2,(int32_t)a->surface->width-4,(int32_t)a->surface->height-4}, "Display Properties", 1);
    voxel_ui_text(a->surface,16,38,"DESKTOP COLOR",VOXEL_UI_TEXT);
    voxel_ui_button(a->surface, (struct voxel_ui_rect){16,54,130,24}, "TEAL CLASSIC", 0);
    voxel_ui_text(a->surface,16,90,"WINDOW THEME",VOXEL_UI_TEXT);
    voxel_ui_button(a->surface, (struct voxel_ui_rect){16,106,130,24}, "GRAY / NAVY", 0);
    voxel_ui_text(a->surface,16,148,"Settings are session-local.",VOXEL_UI_TEXT);
    if(t){voxel_ui_text(a->surface,160,58,"DESKTOP",t->desktop);voxel_ui_text(a->surface,160,110,"WINDOWS",t->face);}
}
int voxel_settings_pointer(struct voxel_app_context *a,int32_t x,int32_t y)
{
    struct voxel_theme_state*t=a->host?a->host->theme:0;(void)x;
    if(!t)return 0;
    if(y>=54&&y<78){t->desktop=t->desktop==VOXEL_UI_DESKTOP?0x00008000u:VOXEL_UI_DESKTOP;return 1;}
    if(y>=106&&y<130){t->face=t->face==VOXEL_UI_FACE?0x00a8a8a8u:VOXEL_UI_FACE;return 1;}
    return 0;
}
