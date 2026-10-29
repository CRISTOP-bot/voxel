#include "virtual_keyboard.h"
#include "voxel/classic_ui.h"

static const char *const lower_rows[] = {"1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm,./"};
static const char *const upper_rows[] = {"!@#$%^&*()", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM<>?"};
static int caps_lock;

void voxel_vk_draw(struct voxel_surface *s, int32_t top)
{
    unsigned r, c;
    if (!s || s->width < 80) return;
    for (r = 0; r < 4; ++r) {
        const char *row = caps_lock ? upper_rows[r] : lower_rows[r];
        unsigned n = 0;
        int32_t slots, w, y = top + (int32_t)r * VOXEL_VK_ROW_H;
        while (row[n]) ++n;
        slots = (int32_t)n + (r == 2 ? 2 : r == 3 ? 2 : 0);
        w = (int32_t)(s->width - 8u) / slots;
        for (c = 0; c < n; ++c) {
            char label[2] = { row[c], 0 };
            voxel_ui_button(s, (struct voxel_ui_rect){4+(int32_t)c*w,y,w-2,20}, label, 0);
        }
        if (r == 2) {
            voxel_ui_button(s,(struct voxel_ui_rect){4+(int32_t)n*w,y,w-2,20},"SP",0);
            voxel_ui_button(s,(struct voxel_ui_rect){4+((int32_t)n+1)*w,y,w-2,20},"CAP",caps_lock);
        } else if (r == 3) {
            voxel_ui_button(s,(struct voxel_ui_rect){4+(int32_t)n*w,y,w-2,20},"<",0);
            voxel_ui_button(s,(struct voxel_ui_rect){4+((int32_t)n+1)*w,y,w-2,20},"OK",0);
        }
    }
}

int voxel_vk_pointer_key(struct voxel_surface *s,int32_t x,int32_t y,int32_t top,uint32_t *key)
{
    unsigned r,n=0;const char*row;int32_t slots,w,col;
    if(!s||!key||y<top||x<4||s->width<80)return 0;
    r=(unsigned)((y-top)/VOXEL_VK_ROW_H);if(r>=4)return 0;
    row=caps_lock?upper_rows[r]:lower_rows[r];while(row[n])++n;
    slots=(int32_t)n+(r==2?2:r==3?2:0);w=(int32_t)(s->width-8u)/slots;if(w<=0)return 0;col=(x-4)/w;if(col<0)return 0;
    if(r==2&&(unsigned)col==n){*key=' ';return 1;}
    if(r==2&&(unsigned)col==n+1){caps_lock=!caps_lock;*key=VOXEL_KEY_CAPS;return 1;}
    if(r==3&&(unsigned)col==n){*key=VOXEL_KEY_BACKSPACE;return 1;}
    if(r==3&&(unsigned)col==n+1){*key=VOXEL_KEY_ENTER;return 1;}
    if((unsigned)col<n){*key=(uint32_t)row[col];return 1;}return 0;
}

int voxel_vk_pointer(struct voxel_surface *s,int32_t x,int32_t y,int32_t top,
                     char *text,uint32_t capacity,uint32_t *length,uint32_t *key)
{
    unsigned r,n=0;
    const char *row;
    int32_t slots,w,col;
    if(!s||!text||!length||!key||y<top||x<4||s->width<80)return 0;
    r=(unsigned)((y-top)/VOXEL_VK_ROW_H);
    if(r>=4)return 0;
    row=caps_lock?upper_rows[r]:lower_rows[r];
    while(row[n])++n;
    slots=(int32_t)n+(r==2?2:r==3?2:0);
    w=(int32_t)(s->width-8u)/slots;
    if(w<=0)return 0;
    col=(x-4)/w;
    if(col<0)return 0;
    if(r==2&&(unsigned)col==n){*key=' ';}
    else if(r==2&&(unsigned)col==n+1){caps_lock=!caps_lock;*key=VOXEL_KEY_CAPS;return 1;}
    else if(r==3&&(unsigned)col==n){if(*length){text[--*length]=0;}*key=VOXEL_KEY_BACKSPACE;return 1;}
    else if(r==3&&(unsigned)col==n+1){*key=VOXEL_KEY_ENTER;return 1;}
    else if((unsigned)col<n)*key=(uint32_t)row[col];
    else return 0;
    if(*length+1u<capacity){text[*length]=(char)*key;++*length;text[*length]=0;}
    return 1;
}
