#include "voxel/app.h"
#include "voxel/classic_ui.h"
#include "../common/virtual_keyboard.h"

struct run_state { char path[VOXEL_APP_PATH_MAX]; };
static struct run_state *state(struct voxel_app_context *a) { return (struct run_state *)a->state; }
void voxel_run_init(struct voxel_app_context *a) { state(a)->path[0] = 0; }
void voxel_run_draw(struct voxel_app_context *a)
{
    struct run_state *s = state(a);
    voxel_surface_clear(a->surface, VOXEL_UI_FACE);
    voxel_ui_window_frame(a->surface, (struct voxel_ui_rect){2,2,(int32_t)a->surface->width-4,(int32_t)a->surface->height-4}, "Run", 1);
    voxel_ui_text(a->surface, 14, 34, "PROGRAM PATH:", VOXEL_UI_TEXT);
    voxel_ui_bevel(a->surface, (struct voxel_ui_rect){14,47,(int32_t)a->surface->width-28,24}, 1);
    voxel_ui_text(a->surface, 20, 55, s->path, VOXEL_UI_TEXT);
    voxel_vk_draw(a->surface, (int32_t)a->surface->height-100);
}
int voxel_run_key(struct voxel_app_context *a, uint32_t k)
{
    struct run_state *s=state(a); unsigned n=0;
    while(s->path[n]&&n<VOXEL_APP_PATH_MAX-1)++n;
    if(k==VOXEL_KEY_BACKSPACE){if(n)s->path[n-1]=0;return 1;}
    if(k>=32&&k<127&&n<VOXEL_APP_PATH_MAX-1){s->path[n]=(char)k;s->path[n+1]=0;return 1;}
    return 0;
}
int voxel_run_launch(struct voxel_app_context *a)
{
    struct run_state *s=state(a);
    if(!a->host||!a->host->spawn||!s->path[0])return -1;
    return a->host->spawn(a->host->userdata,s->path);
}
int voxel_run_pointer(struct voxel_app_context *a,int32_t x,int32_t y)
{
    struct run_state*s=state(a);uint32_t key=0,len=0;
    while(len<VOXEL_APP_PATH_MAX-1&&s->path[len])++len;
    if(y>=(int32_t)a->surface->height-100 && voxel_vk_pointer(a->surface,x,y,(int32_t)a->surface->height-100,s->path,sizeof(s->path),&len,&key)) {
        if(key==13u)return voxel_run_launch(a);
        return 1;
    }
    return 0;
}
