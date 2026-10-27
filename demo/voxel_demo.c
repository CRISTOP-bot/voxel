#include "voxel/classic_ui.h"
#include "voxel/compositor.h"
#include "voxel/input.h"

#include <stdint.h>
#include <stddef.h>

#define VIEW_W 640u
#define VIEW_H 480u
#define TASKBAR_H 30
#define TITLE_H 22
#define APP_COUNT 3
#define POPUP_W VIEW_W
#define POPUP_H VIEW_H
#define MENU_W 180u
#define MENU_H 112u
#define CURSOR_W 12u
#define CURSOR_H 18u

struct app_window {
    struct voxel_surface surface;
    uint32_t *pixels;
    uint32_t width, height;
    int32_t x, y, restore_x, restore_y, restore_w, restore_h;
    int visible, minimized, maximized;
    const char *title;
};

static uint32_t output_pixels[VIEW_W * VIEW_H];
static uint32_t desktop_pixels[VIEW_W * VIEW_H];
static uint32_t explorer_pixels[VIEW_W * (VIEW_H - TASKBAR_H)];
static uint32_t settings_pixels[VIEW_W * (VIEW_H - TASKBAR_H)];
static uint32_t about_pixels[VIEW_W * (VIEW_H - TASKBAR_H)];
static uint32_t menu_pixels[POPUP_W * POPUP_H];
static uint32_t cursor_pixels[CURSOR_W * CURSOR_H];
static struct voxel_surface desktop, menu_surface, cursor;
static struct app_window apps[APP_COUNT];
static struct voxel_compositor compositor;
static struct voxel_framebuffer fb;
static int active_app=-1, menu_open=0, file_menu_open=0, about_dialog=0;
static struct app_window *dragging=0;
static int32_t drag_dx,drag_dy;
static const char *dialog_title="About VOXEL";
static const char *dialog_message="CLASSIC DESKTOP PREVIEW";
static uint32_t previous_buttons=0;
static const char no_fb[]="VOXEL: no supported NEWOS framebuffer; boot with Limine graphics.\n";
static const char no_space[]="VOXEL: unable to initialize classic desktop surfaces.\n";
static const char present_error[]="VOXEL: framebuffer presentation failed.\n";
static const char ready[]="VOXEL: classic desktop ready; mouse supports Start, windows, menus and title-bar dragging.\n";
static const char completed[]="VOXEL: classic desktop demo completed successfully.\n";

static long os_write(const char *text,size_t length)
{
    long result;
    __asm__ __volatile__("int $0x80":"=a"(result):"a"((long)VOXEL_SYS_WRITE),"D"((long)1),"S"((long)(uintptr_t)text),"d"((long)length):"memory");
    return result;
}

static uint32_t parse_frame_limit(int argc,char **argv)
{
    int i;
    for(i=1;i+1<argc;++i) {
        const char *p=argv[i];
        if(p[0]=='-'&&p[1]=='-'&&p[2]=='f'&&p[3]=='r'&&p[4]=='a'&&p[5]=='m'&&p[6]=='e'&&p[7]=='s'&&p[8]=='\0') {
            uint32_t value=0;
            p=argv[i+1];
            if(!*p) return 0;
            for(;*p>='0'&&*p<='9';++p) {
                uint32_t digit=(uint32_t)(*p-'0');
                if(value>(UINT32_MAX-digit)/10u) return UINT32_MAX;
                value=value*10u+digit;
            }
            return *p?0:value;
        }
    }
    return 0;
}

static void draw_desktop(void)
{
    size_t i;
    (void)voxel_surface_init(&desktop,VIEW_W,VIEW_H,VIEW_W,VOXEL_XRGB8888,desktop_pixels,VIEW_W*VIEW_H);
    voxel_surface_clear(&desktop,VOXEL_UI_DESKTOP);
    voxel_ui_icon(&desktop,12,14,0,"My Computer",0);
    voxel_ui_icon(&desktop,12,66,1,"Documents",0);
    voxel_ui_icon(&desktop,12,118,2,"Settings",0);
    /* Pixel-art desktop bitmap accent: intentionally flat, not a gradient. */
    voxel_ui_taskbar(&desktop,VIEW_W,VIEW_H,menu_open);
}

static void draw_explorer(struct app_window *a)
{
    static const char *const menus[]={"File","View","Help"};
    struct voxel_ui_rect inner={4,4,(int32_t)a->width-8,(int32_t)a->height-8};
    voxel_surface_clear(&a->surface,VOXEL_UI_FACE);
    voxel_ui_window_frame(&a->surface,inner,a->title,active_app==0);
    voxel_ui_menu_bar(&a->surface,inner.x+4,inner.y+TITLE_H+4,inner.width-8,menus,3);
    voxel_ui_icon(&a->surface,inner.x+22,inner.y+TITLE_H+31,0,"Computer",0);
    voxel_ui_icon(&a->surface,inner.x+108,inner.y+TITLE_H+31,1,"Documents",0);
    voxel_ui_icon(&a->surface,inner.x+194,inner.y+TITLE_H+31,2,"Settings",0);
    (void)voxel_surface_fill_rect(&a->surface,inner.x+7,inner.y+inner.height-23,inner.width-14,15,VOXEL_UI_FACE);
    voxel_ui_bevel(&a->surface,(struct voxel_ui_rect){inner.x+7,inner.y+inner.height-23,inner.width-14,15},1);
    voxel_ui_text(&a->surface,inner.x+14,inner.y+inner.height-19,"3 OBJECTS",VOXEL_UI_TEXT);
}

static void draw_settings(struct app_window *a)
{
    static const char *const menus[]={"File","Edit","Help"};
    struct voxel_ui_rect inner={4,4,(int32_t)a->width-8,(int32_t)a->height-8};
    voxel_surface_clear(&a->surface,VOXEL_UI_FACE);
    voxel_ui_window_frame(&a->surface,inner,a->title,active_app==1);
    voxel_ui_menu_bar(&a->surface,inner.x+4,inner.y+TITLE_H+4,inner.width-8,menus,3);
    voxel_ui_text(&a->surface,inner.x+17,inner.y+TITLE_H+34,"DISPLAY PROPERTIES",VOXEL_UI_TEXT);
    voxel_ui_text(&a->surface,inner.x+17,inner.y+TITLE_H+54,"COLOR SCHEME:",VOXEL_UI_TEXT);
    voxel_ui_button(&a->surface,(struct voxel_ui_rect){inner.x+17,inner.y+TITLE_H+68,138,24},"CLASSIC 3D",1);
    voxel_ui_text(&a->surface,inner.x+17,inner.y+TITLE_H+105,"800 X 600  -  256 COLORS",VOXEL_UI_TEXT);
    voxel_ui_button(&a->surface,(struct voxel_ui_rect){inner.x+inner.width-174,inner.y+inner.height-35,75,23},"APPLY",0);
    voxel_ui_button(&a->surface,(struct voxel_ui_rect){inner.x+inner.width-91,inner.y+inner.height-35,75,23},"CLOSE",0);
}

static void draw_about(struct app_window *a)
{
    struct voxel_ui_rect inner={4,4,(int32_t)a->width-8,(int32_t)a->height-8};
    voxel_surface_clear(&a->surface,VOXEL_UI_FACE);
    voxel_ui_window_frame(&a->surface,inner,a->title,active_app==2);
    voxel_ui_icon(&a->surface,inner.x+14,inner.y+TITLE_H+24,0,"VOXEL",0);
    voxel_ui_text(&a->surface,inner.x+63,inner.y+TITLE_H+28,"CLASSIC DESKTOP",VOXEL_UI_TEXT);
    voxel_ui_text(&a->surface,inner.x+63,inner.y+TITLE_H+45,"SOFTWARE GUI DEMO",VOXEL_UI_TEXT);
    voxel_ui_text(&a->surface,inner.x+16,inner.y+TITLE_H+78,"FRAMEBUFFER / MOUSE BACKEND",VOXEL_UI_TEXT);
    voxel_ui_button(&a->surface,(struct voxel_ui_rect){inner.x+inner.width-82,inner.y+inner.height-35,68,23},"OK",0);
}

static void paint_app(int id)
{
    if(id==0) draw_explorer(&apps[id]);
    else if(id==1) draw_settings(&apps[id]);
    else draw_about(&apps[id]);
}

static uint32_t layer_for_surface(const struct voxel_surface *surface)
{
    size_t i;
    for(i=0;i<compositor.layer_count;++i)
        if(compositor.layers[i].surface==surface) return (uint32_t)i;
    return UINT32_MAX;
}

static void raise_app(int id)
{
    uint32_t index;
    if(id<0||id>=APP_COUNT) return;
    apps[id].visible=1; apps[id].minimized=0; active_app=id;
    index=layer_for_surface(&apps[id].surface);
    if(index!=UINT32_MAX) (void)voxel_compositor_raise(&compositor,index);
    /* Keep cursor above all application windows. */
    index=layer_for_surface(&cursor);
    if(index!=UINT32_MAX) (void)voxel_compositor_raise(&compositor,index);
}

static void set_visible(struct app_window *a,int visible)
{
    uint32_t i=layer_for_surface(&a->surface);
    a->visible=visible;
    if(i!=UINT32_MAX) voxel_compositor_set_visible(&compositor,i,visible);
}

static void draw_popup(void)
{
    static const char *const file_items[]={"Open","Close","About VOXEL"};
    voxel_surface_clear(&menu_surface,0);
    if(menu_open) voxel_ui_start_menu(&menu_surface,0,(int32_t)VIEW_H-TASKBAR_H-(int32_t)MENU_H+1,-1);
    if(file_menu_open&&apps[0].visible&&!apps[0].minimized)
        voxel_ui_menu(&menu_surface,(struct voxel_ui_rect){apps[0].x+7,apps[0].y+49,112,65},file_items,3,-1);
    if(about_dialog) voxel_ui_dialog(&menu_surface,(struct voxel_ui_rect){170,159,300,150},dialog_title,dialog_message);
}

static void draw_pointer(void)
{
    uint32_t y;
    voxel_surface_clear(&cursor,0);
    for(y=0;y<CURSOR_H;++y) {
        uint32_t x;
        for(x=0;x<CURSOR_W;++x) if(x<=y/2u&&y<15u)
            cursor_pixels[(size_t)y*CURSOR_W+x]=(x==0u||x==y/2u||y==14u)?0xffffffffu:0xff000000u;
    }
}

static int point_in(int32_t x,int32_t y,int32_t rx,int32_t ry,int32_t rw,int32_t rh)
{
    return x>=rx&&y>=ry&&x<rx+rw&&y<ry+rh;
}

static int app_at(int32_t x,int32_t y)
{
    int i;
    /* compositor order is back-to-front; inspect front-to-back. */
    size_t layer;
    for(layer=compositor.layer_count;layer>1;--layer) {
        const struct voxel_layer *l=&compositor.layers[layer-1];
        for(i=0;i<APP_COUNT;++i) if(l->surface==&apps[i].surface&&l->visible&&apps[i].visible&&!apps[i].minimized&&
            point_in(x,y,apps[i].x,apps[i].y,(int32_t)apps[i].width,(int32_t)apps[i].height)) return i;
    }
    return -1;
}

static void draw_task_buttons(void)
{
    static const char *const labels[APP_COUNT]={"My Computer","Settings","About VOXEL"};
    int i;
    for(i=0;i<APP_COUNT;++i) {
        int32_t x=76+i*145;
        struct voxel_ui_rect r={x,(int32_t)VIEW_H-TASKBAR_H+3,140,TASKBAR_H-6};
        if(apps[i].visible) voxel_ui_button(&desktop,r,labels[i],active_app==i&&!apps[i].minimized);
    }
}

static void pointer_click(int32_t x,int32_t y,uint32_t buttons)
{
    int id;
    if(about_dialog) {
        if(!point_in(x,y,170,159,300,150)||point_in(x,y,392,278,66,23)) { about_dialog=0; dialog_title="About VOXEL"; dialog_message="CLASSIC DESKTOP PREVIEW"; }
        return;
    }
    if(file_menu_open) {
        int32_t mx=apps[0].x+8,my=apps[0].y+49;
        if(point_in(x,y,mx,my,112,65)) {
            int item=(y-my-4)/20;
            file_menu_open=0;
            if(item==0) raise_app(0);
            else if(item==1) { set_visible(&apps[0],0); apps[0].minimized=0; active_app=-1; }
            else if(item==2) { dialog_title="About VOXEL"; dialog_message="CLASSIC DESKTOP PREVIEW"; about_dialog=1; }
            return;
        }
        file_menu_open=0;
    }
    if(y>=(int32_t)VIEW_H-TASKBAR_H) {
        if(x<70) { menu_open=!menu_open; return; }
        if(x>=76) {
            int slot=(x-76)/145;
            if(slot>=0&&slot<APP_COUNT&&apps[slot].visible) {
                if(active_app==slot&&!apps[slot].minimized) {
                    apps[slot].minimized=1; set_visible(&apps[slot],0); active_app=-1;
                } else raise_app(slot);
            }
        }
        return;
    }
    if(menu_open) {
        int menu_y=(int32_t)VIEW_H-TASKBAR_H-(int32_t)MENU_H+1;
        if(point_in(x,y,0,menu_y,(int32_t)MENU_W,(int32_t)MENU_H)) {
            int item=(y-menu_y-4)/20;
            menu_open=0;
            if(item==0) raise_app(0);
            else if(item==1) raise_app(0);
            else if(item==2) raise_app(1);
            else if(item==3) raise_app(2);
            return;
        }
        menu_open=0;
    }
    id=app_at(x,y);
    if(id>=0) {
        file_menu_open=0;
        struct app_window *a=&apps[id];
        int32_t lx=x-a->x,ly=y-a->y;
        raise_app(id);
        if(ly>=7&&ly<7+TITLE_H) {
            /* Frame inset + titlebar inset put the close button at width-24. */
            int close_x=(int32_t)a->width-25;
            if(lx>=close_x&&lx<close_x+14) {
                set_visible(a,0); a->minimized=0; active_app=-1; dragging=0; return;
            }
            if(lx>=close_x-16&&lx<close_x-2) {
                if(!a->maximized) {
                    a->restore_x=a->x; a->restore_y=a->y;
                    a->restore_w=(int32_t)a->width; a->restore_h=(int32_t)a->height;
                    a->x=0;a->y=0;a->width=VIEW_W;a->height=VIEW_H-TASKBAR_H;a->maximized=1;
                    (void)voxel_surface_init(&a->surface,a->width,a->height,a->width,VOXEL_XRGB8888,a->pixels,VIEW_W*(VIEW_H-TASKBAR_H));
                } else {
                    a->x=a->restore_x;a->y=a->restore_y;a->width=(uint32_t)a->restore_w;a->height=(uint32_t)a->restore_h;a->maximized=0;
                    (void)voxel_surface_init(&a->surface,a->width,a->height,a->width,VOXEL_XRGB8888,a->pixels,VIEW_W*(VIEW_H-TASKBAR_H));
                }
                paint_app(id); return;
            }
            if(lx>=close_x-32&&lx<close_x-18) {
                a->minimized=1;set_visible(a,0);active_app=-1;dragging=0;return;
            }
            if(buttons&1u) { dragging=a;drag_dx=lx;drag_dy=ly; }
        } else if(id==1&&ly>=30&&ly<49) {
            if(lx<60) {
                file_menu_open=1;
                menu_open=0;
            } else if(lx<108) {
                dialog_title="View Options";
                dialog_message="NO WINDOW EFFECTS ENABLED";
                about_dialog=1;
            } else {
                dialog_title="About VOXEL";
                dialog_message="CLASSIC DESKTOP PREVIEW";
                about_dialog=1;
            }
        } else if(id==1) {
            int32_t inner_x=4, inner_y=4;
            if(point_in(lx,ly,inner_x+300-8-174,inner_y+190-8-35,75,23)) {
                dialog_title="Display Properties";
                dialog_message="SETTINGS ARE DEMO ONLY";
                about_dialog=1;
            } else if(point_in(lx,ly,inner_x+300-8-91,inner_y+190-8-35,75,23)) {
                set_visible(a,0); a->minimized=0; active_app=-1;
            }
        } else if(id==0&&ly>=30&&ly<49) {
            if(lx<60) { file_menu_open=!file_menu_open; menu_open=0; }
            else if(lx<108) {
                file_menu_open=0;
                dialog_title="View Options"; dialog_message="NO WINDOW EFFECTS ENABLED";
                about_dialog=1;
            } else {
                file_menu_open=0;
                dialog_title="About VOXEL"; dialog_message="CLASSIC DESKTOP PREVIEW";
                about_dialog=1;
            }
        }
        return;
    }
    if(x<55&&y<165) {
        if(y<62||y<114) raise_app(0); else raise_app(1);
    }
}

int main(int argc,char **argv)
{
    uint32_t frame_limit=parse_frame_limit(argc,argv),frame=0;
    uint32_t origin_x,origin_y;
    struct voxel_pointer pointer;
    struct voxel_rect full={0,0,VIEW_W,VIEW_H};
    if(voxel_framebuffer_open(&fb)!=0||fb.info.width<VIEW_W||fb.info.height<VIEW_H) {
        (void)os_write(no_fb,sizeof(no_fb)-1u);return 1;
    }
    if(voxel_surface_init(&desktop,VIEW_W,VIEW_H,VIEW_W,VOXEL_XRGB8888,desktop_pixels,VIEW_W*VIEW_H)||
       voxel_surface_init(&menu_surface,POPUP_W,POPUP_H,POPUP_W,VOXEL_ARGB8888,menu_pixels,POPUP_W*POPUP_H)||
       voxel_surface_init(&cursor,CURSOR_W,CURSOR_H,CURSOR_W,VOXEL_ARGB8888,cursor_pixels,CURSOR_W*CURSOR_H)||
       voxel_compositor_init(&compositor,VIEW_W,VIEW_H,VIEW_W,output_pixels,VIEW_W*VIEW_H)) {
        (void)os_write(no_space,sizeof(no_space)-1u);return 1;
    }
    apps[0]=(struct app_window){.pixels=explorer_pixels,.width=320,.height=220,.x=60,.y=45,.visible=1,.title="My Computer"};
    apps[1]=(struct app_window){.pixels=settings_pixels,.width=300,.height=190,.x=198,.y=115,.visible=1,.title="Settings"};
    apps[2]=(struct app_window){.pixels=about_pixels,.width=280,.height=160,.x=166,.y=208,.visible=0,.title="About VOXEL"};
    if(voxel_surface_init(&apps[0].surface,320,220,320,VOXEL_XRGB8888,explorer_pixels,VIEW_W*(VIEW_H-TASKBAR_H))||
       voxel_surface_init(&apps[1].surface,300,190,300,VOXEL_XRGB8888,settings_pixels,VIEW_W*(VIEW_H-TASKBAR_H))||
       voxel_surface_init(&apps[2].surface,280,160,280,VOXEL_XRGB8888,about_pixels,VIEW_W*(VIEW_H-TASKBAR_H))) {
        (void)os_write(no_space,sizeof(no_space)-1u);return 1;
    }
    draw_desktop();draw_pointer();paint_app(0);paint_app(1);paint_app(2);draw_popup();
    if(voxel_compositor_add(&compositor,&desktop,0,0,0)||
       voxel_compositor_add(&compositor,&apps[0].surface,apps[0].x,apps[0].y,0)||
       voxel_compositor_add(&compositor,&apps[1].surface,apps[1].x,apps[1].y,0)||
       voxel_compositor_add(&compositor,&apps[2].surface,apps[2].x,apps[2].y,0)||
       voxel_compositor_add(&compositor,&menu_surface,0,0,0)||
       voxel_compositor_add(&compositor,&cursor,0,0,0)) {
        (void)os_write(no_space,sizeof(no_space)-1u);return 1;
    }
    voxel_compositor_set_visible(&compositor,3,0);voxel_compositor_set_visible(&compositor,4,0);
    active_app=1;
    (void)voxel_compositor_raise(&compositor,2);
    {
        uint32_t cursor_layer=layer_for_surface(&cursor);
        if(cursor_layer!=UINT32_MAX)
            (void)voxel_compositor_raise(&compositor,cursor_layer);
    }
    origin_x=(uint32_t)((fb.info.width-VIEW_W)/2u);origin_y=(uint32_t)((fb.info.height-VIEW_H)/2u);
    (void)os_write(ready,sizeof(ready)-1u);
    while(frame_limit==0||frame<frame_limit) {
        if(voxel_pointer_poll(&pointer)==0) {
            int32_t mx=pointer.x-(int32_t)origin_x,my=pointer.y-(int32_t)origin_y;
            uint32_t ci=layer_for_surface(&cursor);
            int pressed=(pointer.buttons&1u)&&!(previous_buttons&1u);
            if(ci==UINT32_MAX)return 1;
            compositor.layers[ci].x=mx;compositor.layers[ci].y=my;(void)voxel_compositor_raise(&compositor,ci);
            if(pressed) pointer_click(mx,my,pointer.buttons);
            if(!(pointer.buttons&1u)) dragging=0;
            if(dragging&&!dragging->maximized) {
                dragging->x=mx-drag_dx;dragging->y=my-drag_dy;
                if(dragging->x<0)dragging->x=0;
                if(dragging->y<0)dragging->y=0;
                if(dragging->x+(int32_t)dragging->width>(int32_t)VIEW_W)
                    dragging->x=(int32_t)VIEW_W-(int32_t)dragging->width;
                if(dragging->y+(int32_t)dragging->height>(int32_t)VIEW_H-TASKBAR_H)
                    dragging->y=(int32_t)VIEW_H-TASKBAR_H-(int32_t)dragging->height;
            }
            previous_buttons=pointer.buttons;
        }
        draw_desktop();draw_task_buttons();draw_popup();draw_pointer();
        for(int i=0;i<APP_COUNT;++i) paint_app(i);
        for(int i=0;i<APP_COUNT;++i) {
            uint32_t li=layer_for_surface(&apps[i].surface);
            if(li!=UINT32_MAX) {
                compositor.layers[li].x=apps[i].x;compositor.layers[li].y=apps[i].y;
                voxel_compositor_set_visible(&compositor,li,apps[i].visible&&!apps[i].minimized);
            }
        }
        {
            uint32_t mi=layer_for_surface(&menu_surface),ci=layer_for_surface(&cursor);
            if(mi!=UINT32_MAX) {
                compositor.layers[mi].y=0;
                voxel_compositor_set_visible(&compositor,mi,menu_open||file_menu_open||about_dialog);
                (void)voxel_compositor_raise(&compositor,mi);
            }
            if(ci!=UINT32_MAX)(void)voxel_compositor_raise(&compositor,ci);
        }
        voxel_compositor_compose(&compositor,full);
        if(voxel_compositor_present(&compositor,&fb,origin_x,origin_y)!=0) {
            (void)os_write(present_error,sizeof(present_error)-1u);return 1;
        }
        ++frame;if(voxel_sleep_ms(16u)<0)return 1;
    }
    if(frame_limit)(void)os_write(completed,sizeof(completed)-1u);
    return 0;
}
