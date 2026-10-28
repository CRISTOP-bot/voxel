#include "voxel/app.h"
#include "voxel/classic_ui.h"

struct explorer_state { char path[VOXEL_APP_PATH_MAX]; struct voxel_fs_entry entries[8]; int count; };
static struct explorer_state *st(struct voxel_app_context *a){ return (struct explorer_state *)a->state; }
static void refresh(struct voxel_app_context *a){ struct explorer_state *s=st(a); s->count=-1; if(a->host&&a->host->list_dir) s->count=a->host->list_dir(a->host->userdata,s->path,s->entries,8u); }
void voxel_explorer_init(struct voxel_app_context *a){ struct explorer_state *s=st(a); s->path[0]='/';s->path[1]=0;refresh(a); }
int voxel_explorer_parent(struct voxel_app_context*a){struct explorer_state*s=st(a);unsigned n=0;if(!s->path[0]||(s->path[0]=='/'&&!s->path[1]))return 0;while(s->path[n]&&n<VOXEL_APP_PATH_MAX-1)n++;if(n>1&&s->path[n-1]=='/')--n;while(n>1&&s->path[n-1]!='/')--n;if(n>1)--n;s->path[n]=0;refresh(a);return 1;}
int voxel_explorer_open_selected(struct voxel_app_context *a, int row)
{
    struct explorer_state *s=st(a);char full[VOXEL_APP_PATH_MAX];unsigned b=0,n=0;
    if(row<0||row>=s->count||row>=8)return -1;
    while(s->path[b]&&b<VOXEL_APP_PATH_MAX-2){full[b]=s->path[b];b++;}
    if(b>1&&full[b-1]!='/')full[b++]='/';
    while(s->entries[row].name[n]&&n<VOXEL_FS_NAME_MAX-1&&b<VOXEL_APP_PATH_MAX-1){full[b++]=s->entries[row].name[n++];}
    full[b]=0;
    if(s->entries[row].mode&VOXEL_FS_MODE_DIR){
        if(n==2&&s->entries[row].name[0]=='.'&&s->entries[row].name[1]=='.'){
            if(b>1){if(full[b-1]=='/')--b;while(b>1&&full[b-1]!='/')--b;if(b>1)--b;full[b]=0;}
        }
        for(b=0;b<VOXEL_APP_PATH_MAX-1&&full[b];++b)s->path[b]=full[b];
        s->path[b]=0;refresh(a);return 1;
    }
    if(a->host&&a->host->open_image){return a->host->open_image(a->host->userdata,full);}if(a->host&&a->host->open_path){a->host->open_path(a->host->userdata,full);return 1;}
    return -1;
    return -1;
}

void voxel_explorer_draw(struct voxel_app_context *a)
{
    struct explorer_state *s=st(a);int i;voxel_surface_clear(a->surface,VOXEL_UI_FACE);
    voxel_ui_window_frame(a->surface, (struct voxel_ui_rect){2,2,(int32_t)a->surface->width-4,(int32_t)a->surface->height-4}, "File Explorer", 1);
    voxel_ui_button(a->surface,(struct voxel_ui_rect){10,30,58,18},"UP",0);
    voxel_ui_text(a->surface,74,34,s->path,VOXEL_UI_TEXT);
    voxel_ui_bevel(a->surface,(struct voxel_ui_rect){10,48,(int32_t)a->surface->width-20,(int32_t)a->surface->height-61},1);
    if(s->count<0){voxel_ui_text(a->surface,18,58,"DIRECTORY LIST UNAVAILABLE",VOXEL_UI_TEXT);return;}
    if(!s->count){voxel_ui_text(a->surface,18,58,"THIS DIRECTORY IS EMPTY",VOXEL_UI_TEXT);return;}
    for(i=0;i<s->count&&i<8;i++){
        char label[VOXEL_FS_NAME_MAX+4];unsigned j;label[0]=(s->entries[i].mode&VOXEL_FS_MODE_DIR)?'[':' ';label[1]=' ';
        for(j=0;j<VOXEL_FS_NAME_MAX-3&&s->entries[i].name[j];j++)label[j+2]=s->entries[i].name[j];
        label[j+2]=0;voxel_ui_text(a->surface,18,58+i*18,label,VOXEL_UI_TEXT);
    }
}
int voxel_explorer_pointer(struct voxel_app_context *a,int32_t x,int32_t y)
{
    int row;if(y>=30&&y<48&&x>=10&&x<68)return voxel_explorer_parent(a);if(x<12||x>(int32_t)a->surface->width-12||y<54)return 0;row=(y-54)/18;return voxel_explorer_open_selected(a,row);
}
