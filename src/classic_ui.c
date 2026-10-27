#include "voxel/classic_ui.h"

#include <stddef.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define TITLE_HEIGHT 22
#define TASKBAR_HEIGHT 30

static int valid(const struct voxel_surface *s)
{
    return s && s->pixels && s->width && s->height &&
           s->stride_pixels >= s->width &&
           s->stride_pixels <= SIZE_MAX / s->height &&
           s->stride_pixels * s->height <= s->capacity_pixels;
}

static uint32_t surface_color(const struct voxel_surface *s, uint32_t color)
{
    return s && s->format == VOXEL_ARGB8888 ? color | 0xff000000u : color;
}

static void rect(struct voxel_surface *s, int32_t x, int32_t y,
                 int32_t width, int32_t height, uint32_t color)
{
    if (valid(s) && width > 0 && height > 0)
        (void)voxel_surface_fill_rect(s, x, y, width, height,
                                      surface_color(s, color));
}

static uint8_t row_bits(char ch, int row)
{
    static const uint8_t alpha[26][7] = {
        {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
        {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
        {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
        {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
        {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}
    };
    static const uint8_t numbers[10][7] = {
        {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
        {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
        {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
        {14,17,17,15,1,1,14}
    };
    if (row < 0 || row > 6) return 0;
    if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
    if (ch >= 'A' && ch <= 'Z') return alpha[ch - 'A'][row];
    if (ch >= '0' && ch <= '9') return numbers[ch - '0'][row];
    switch (ch) {
    case ' ': return 0;
    case '.': return row == 6 ? 4 : 0;
    case ',': return row == 5 ? 4 : (row == 6 ? 8 : 0);
    case ':': return (row == 2 || row == 5) ? 4 : 0;
    case '-': return row == 3 ? 14 : 0;
    case '_': return row == 6 ? 31 : 0;
    case '!': return row < 5 ? 4 : (row == 6 ? 4 : 0);
    case '?': { static const uint8_t q[7]={14,17,1,2,4,0,4}; return q[row]; }
    case '+': return row == 3 ? 14 : ((row == 2 || row == 4) ? 4 : 0);
    case '(': return row < 3 ? 2 : (row == 3 ? 4 : 2);
    case ')': return row < 3 ? 8 : (row == 3 ? 4 : 8);
    case '/': return (uint8_t)(1u << (row > 4 ? 0 : 4-row));
    default: return 0;
    }
}

void voxel_ui_text(struct voxel_surface *s, int32_t x, int32_t y,
                   const char *str, uint32_t color)
{
    size_t i;
    int row, col;
    if (!valid(s) || !str) return;
    for (i = 0; str[i]; ++i) {
        for (row = 0; row < 7; ++row) {
            uint8_t bits = row_bits(str[i], row);
            for (col = 0; col < 5; ++col)
                if (bits & (1u << (4-col)))
                    (void)voxel_surface_put_pixel(s, x+(int32_t)i*6+col,
                                                  y+row, surface_color(s, color));
        }
    }
}

void voxel_ui_bevel(struct voxel_surface *s, struct voxel_ui_rect r, int pressed)
{
    uint32_t hi = pressed ? VOXEL_UI_DARK : VOXEL_UI_LIGHT;
    uint32_t hi2 = pressed ? VOXEL_UI_MID : VOXEL_UI_WHITE;
    uint32_t lo = pressed ? VOXEL_UI_WHITE : VOXEL_UI_DARK;
    uint32_t lo2 = pressed ? VOXEL_UI_FACE : VOXEL_UI_MID;
    if (!valid(s) || r.width < 2 || r.height < 2) return;
    rect(s,r.x,r.y,r.width,1,hi); rect(s,r.x,r.y,1,r.height,hi);
    rect(s,r.x+1,r.y+1,r.width-2,1,hi2); rect(s,r.x+1,r.y+1,1,r.height-2,hi2);
    rect(s,r.x,r.y+r.height-1,r.width,1,lo); rect(s,r.x+r.width-1,r.y,1,r.height,lo);
    if (r.width>3 && r.height>3) {
        rect(s,r.x+1,r.y+r.height-2,r.width-2,1,lo2);
        rect(s,r.x+r.width-2,r.y+1,1,r.height-2,lo2);
    }
}

void voxel_ui_button(struct voxel_surface *s, struct voxel_ui_rect r,
                     const char *label, int pressed)
{
    size_t n=0;
    int32_t tx,ty;
    if (!valid(s)) return;
    if (label) while (label[n]) ++n;
    rect(s,r.x,r.y,r.width,r.height,VOXEL_UI_FACE);
    voxel_ui_bevel(s,r,pressed);
    tx=r.x+MAX(4,(r.width-(int32_t)n*6)/2)+(pressed?1:0);
    ty=r.y+MAX(3,(r.height-7)/2)+(pressed?1:0);
    voxel_ui_text(s,tx,ty,label,VOXEL_UI_TEXT);
}

void voxel_ui_titlebar(struct voxel_surface *s, struct voxel_ui_rect r,
                       const char *title, int active)
{
    uint32_t color=active?VOXEL_UI_NAVY:VOXEL_UI_MID;
    struct voxel_ui_rect b;
    if (!valid(s) || r.width<55 || r.height<14) return;
    rect(s,r.x,r.y,r.width,r.height,color);
    voxel_ui_text(s,r.x+5,r.y+MAX(2,(r.height-7)/2),title,VOXEL_UI_WHITE);
    b=(struct voxel_ui_rect){r.x+r.width-17,r.y+3,14,r.height-6};
    voxel_ui_button(s,b,"X",0); b.x-=16; voxel_ui_button(s,b,"+",0);
    b.x-=16; voxel_ui_button(s,b,"_",0);
}

void voxel_ui_window_frame(struct voxel_surface *s, struct voxel_ui_rect r,
                           const char *title, int active)
{
    if (!valid(s) || r.width<58 || r.height<48) return;
    rect(s,r.x,r.y,r.width,r.height,VOXEL_UI_FACE);
    voxel_ui_bevel(s,r,0);
    voxel_ui_titlebar(s,(struct voxel_ui_rect){r.x+3,r.y+3,r.width-6,TITLE_HEIGHT},title,active);
}

void voxel_ui_menu_bar(struct voxel_surface *s,int32_t x,int32_t y,int32_t width,
                       const char *const *labels,uint32_t count)
{
    uint32_t i;
    if (!valid(s)||width<=0) return;
    rect(s,x,y,width,19,VOXEL_UI_FACE);
    for(i=0;i<count;++i) voxel_ui_text(s,x+8+(int32_t)i*48,y+6,labels[i],VOXEL_UI_TEXT);
    rect(s,x,y+18,width,1,VOXEL_UI_MID);
}

static void icon_computer(struct voxel_surface *s,int32_t x,int32_t y)
{
    rect(s,x+3,y+2,27,19,VOXEL_UI_DARK); rect(s,x+5,y+4,23,14,VOXEL_UI_WHITE);
    rect(s,x+7,y+6,19,10,0x00008080u); rect(s,x+14,y+21,5,4,VOXEL_UI_MID);
    rect(s,x+10,y+25,14,3,VOXEL_UI_FACE);
    rect(s,x+10,y+25,14,1,VOXEL_UI_WHITE); rect(s,x+10,y+27,14,1,VOXEL_UI_DARK);
}
static void icon_folder(struct voxel_surface *s,int32_t x,int32_t y)
{
    rect(s,x+4,y+5,13,5,0x00e8c75au); rect(s,x+2,y+8,29,19,0x00b68b25u);
    rect(s,x+4,y+10,25,14,0x00f1d675u); rect(s,x+2,y+8,29,1,VOXEL_UI_DARK);
    rect(s,x+2,y+26,29,1,VOXEL_UI_DARK);
}
static void icon_gear(struct voxel_surface *s,int32_t x,int32_t y)
{
    static const uint8_t gear[7]={0x1c,0x3e,0x7f,0x7f,0x7f,0x3e,0x1c};
    int row,col;
    for(row=0;row<7;++row) for(col=0;col<7;++col)
        if(gear[row]&(1u<<(6-col))) rect(s,x+col*3,y+row*3,3,3,VOXEL_UI_DARK);
    rect(s,x+9,y+9,3,3,VOXEL_UI_FACE);
}

void voxel_ui_icon(struct voxel_surface *s,int32_t x,int32_t y,uint32_t kind,
                   const char *label,int selected)
{
    size_t n=0;
    if(!valid(s)) return;
    if(label) while(label[n]) ++n;
    if(selected) rect(s,x,y,MAX(38,(int32_t)n*6+8),42,VOXEL_UI_SELECTION);
    if(kind==0) icon_computer(s,x+3,y);
    else if(kind==1) icon_folder(s,x+3,y);
    else icon_gear(s,x+9,y+1);
    voxel_ui_text(s,x+2,y+31,label,VOXEL_UI_WHITE);
}

void voxel_ui_taskbar(struct voxel_surface *s,int32_t width,int32_t height,int pressed)
{
    struct voxel_ui_rect start, tray;
    if(!valid(s)||width<80||height<TASKBAR_HEIGHT) return;
    rect(s,0,height-TASKBAR_HEIGHT,width,TASKBAR_HEIGHT,VOXEL_UI_FACE);
    rect(s,0,height-TASKBAR_HEIGHT,width,1,VOXEL_UI_WHITE);
    start=(struct voxel_ui_rect){3,height-TASKBAR_HEIGHT+3,66,TASKBAR_HEIGHT-6};
    voxel_ui_button(s,start,"START",pressed);
    tray=(struct voxel_ui_rect){width-77,height-TASKBAR_HEIGHT+4,72,TASKBAR_HEIGHT-8};
    voxel_ui_bevel(s,tray,1);
    voxel_ui_text(s,tray.x+8,tray.y+6,"READY",VOXEL_UI_TEXT);
}

void voxel_ui_start_menu(struct voxel_surface *s,int32_t x,int32_t y,int selected)
{
    static const char *const items[]={"Programs","Documents","Settings","About VOXEL"};
    int i;
    struct voxel_ui_rect r={x,y,174,108};
    if(!valid(s)) return;
    rect(s,r.x,r.y,r.width,r.height,VOXEL_UI_FACE); voxel_ui_bevel(s,r,0);
    for(i=0;i<4;++i) {
        int32_t iy=y+4+i*20;
        if(i==selected) rect(s,x+3,iy,r.width-6,19,VOXEL_UI_SELECTION);
        voxel_ui_text(s,x+10,iy+6,items[i],i==selected?VOXEL_UI_WHITE:VOXEL_UI_TEXT);
    }
}

void voxel_ui_menu(struct voxel_surface *s,struct voxel_ui_rect r,
                   const char *const *items,uint32_t count,int selected)
{
    uint32_t i;
    if(!valid(s)||r.width<8||r.height<8) return;
    rect(s,r.x,r.y,r.width,r.height,VOXEL_UI_FACE);
    voxel_ui_bevel(s,r,0);
    for(i=0;i<count;++i) {
        int32_t iy=r.y+4+(int32_t)i*20;
        if(iy+19>=r.y+r.height) break;
        if((int)i==selected) rect(s,r.x+3,iy,r.width-6,19,VOXEL_UI_SELECTION);
        voxel_ui_text(s,r.x+10,iy+6,items[i],(int)i==selected?VOXEL_UI_WHITE:VOXEL_UI_TEXT);
    }
}

void voxel_ui_dialog(struct voxel_surface *s,struct voxel_ui_rect r,
                     const char *title,const char *message)
{
    voxel_ui_window_frame(s,r,title,1);
    rect(s,r.x+15,r.y+43,2,18,VOXEL_UI_NAVY);
    voxel_ui_text(s,r.x+25,r.y+48,message,VOXEL_UI_TEXT);
    voxel_ui_button(s,(struct voxel_ui_rect){r.x+r.width-78,r.y+r.height-31,66,23},"OK",0);
}
