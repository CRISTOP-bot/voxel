#include "voxel/taskbar.h"
#include "voxel/classic_ui.h"

void voxel_taskbar_buttons(struct voxel_surface *s,
                           const struct voxel_app_descriptor *const *apps,
                           const uint8_t *visible,int active,int32_t height)
{
    uint32_t i;
    if(!s||!apps||!visible)return;
    for(i=0;i<VOXEL_APP_COUNT&&i<4u;i++) {
        int32_t x=76+(int32_t)i*140;
        if(visible[i]&&apps[i])
            voxel_ui_button(s,(struct voxel_ui_rect){x,height-27,136,24},apps[i]->title,active==(int)i);
    }
}

void voxel_taskbar_start_menu(struct voxel_surface *s,const char *const *labels,
                              uint32_t count,int32_t y,int selected)
{
    struct voxel_ui_rect r={0,y,188,(int32_t)count*20+8};
    voxel_ui_menu(s,r,labels,count,selected);
}
