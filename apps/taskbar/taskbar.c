#include "voxel/taskbar.h"
#include "voxel/classic_ui.h"
#include <stdint.h>

void voxel_taskbar_draw(struct voxel_surface *surface,
                        struct voxel_app_host *host,
                        const struct voxel_app_descriptor *const *apps,
                        const uint8_t *visible, int active_app,
                        int32_t screen_width, int32_t screen_height,
                        int start_pressed)
{
    struct voxel_clock_time t={0};
    char clock[6]={'0','0',':','0','0',0};
    uint32_t i;
    if(!surface||!apps||!visible)return;
    voxel_surface_fill_rect(surface,0,screen_height-30,screen_width,30,VOXEL_UI_FACE);
    voxel_ui_taskbar(surface,screen_width,screen_height,start_pressed);
    for(i=0;i<VOXEL_APP_COUNT;i++) {
        int32_t x=76+(int32_t)i*68;
        if(visible[i]&&apps[i])
            voxel_ui_button(surface,(struct voxel_ui_rect){x,screen_height-27,64,24},apps[i]->name,active_app==(int)i);
    }
    if(host&&host->get_time&&host->get_time(host->userdata,&t)==0) {
        clock[0]=(char)('0'+t.hour/10);clock[1]=(char)('0'+t.hour%10);
        clock[3]=(char)('0'+t.minute/10);clock[4]=(char)('0'+t.minute%10);
    }
    voxel_ui_bevel(surface,(struct voxel_ui_rect){screen_width-66,screen_height-26,62,22},1);
    voxel_ui_text(surface,screen_width-58,screen_height-21,clock,VOXEL_UI_TEXT);
}
