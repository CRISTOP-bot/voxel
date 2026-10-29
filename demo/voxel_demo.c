#include "voxel/classic_ui.h"
#include "voxel/compositor.h"
#include "voxel/input.h"
#include "voxel/app.h"
#include "voxel/taskbar.h"

#include <stdint.h>
#include <stddef.h>

#define VIEW_W 640u
#define VIEW_H 480u
#define TASKBAR_H 30
#define TITLE_H 22
#define APP_COUNT VOXEL_APP_COUNT
#define POPUP_W VIEW_W
#define POPUP_H VIEW_H
#define MENU_W 188u
#define MENU_H (APP_COUNT * 20u + 8u)
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
static uint32_t app_pixels[APP_COUNT][VIEW_W * (VIEW_H - TASKBAR_H)];
static struct voxel_app_context app_contexts[APP_COUNT];
static struct voxel_app_host app_host;
static struct voxel_theme_state theme = { VOXEL_UI_DESKTOP, VOXEL_UI_FACE, VOXEL_UI_NAVY, 0, 0 };
static uint32_t menu_pixels[POPUP_W * POPUP_H];
static uint32_t cursor_pixels[CURSOR_W * CURSOR_H];
static struct voxel_surface desktop, menu_surface, cursor;
static struct app_window apps[APP_COUNT];
static struct voxel_compositor compositor;
static struct voxel_framebuffer fb;
static int active_app=-1, menu_open=0;
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
    voxel_surface_clear(&desktop,theme.desktop);
    voxel_ui_icon(&desktop,12,14,0,"My Computer",0);
    voxel_ui_icon(&desktop,12,66,1,"Documents",0);
    voxel_ui_icon(&desktop,12,118,2,"Settings",0);
    /* Pixel-art desktop bitmap accent: intentionally flat, not a gradient. */
    { const struct voxel_app_descriptor *d[APP_COUNT]; uint8_t vis[APP_COUNT]; int i; for(i=0;i<APP_COUNT;i++){d[i]=voxel_app_get((enum voxel_app_id)i);vis[i]=(uint8_t)(apps[i].visible&&!apps[i].minimized);} voxel_taskbar_draw(&desktop,&app_host,d,vis,active_app,VIEW_W,VIEW_H,menu_open); }
}







static void paint_app(int id)
{
    if(id>=0&&id<APP_COUNT) voxel_app_draw(&app_contexts[id],(enum voxel_app_id)id);
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

int voxel_editor_load(struct voxel_app_context *app,const char *path);
int voxel_imageview_open(struct voxel_app_context *app,const char *path);
static int open_text_path(void *userdata,const char *path)
{
    (void)userdata;if(voxel_editor_load(&app_contexts[VOXEL_APP_EDITOR],path)==0){raise_app(VOXEL_APP_EDITOR);return 1;}return -1;
}
static int open_image_path(void *userdata,const char *path)
{
    (void)userdata;if(voxel_imageview_open(&app_contexts[VOXEL_APP_IMAGEVIEW],path)==0){raise_app(VOXEL_APP_IMAGEVIEW);return 1;}return -1;
}
static void request_text_path(void *userdata,const char *path){(void)open_text_path(userdata,path);}
static void request_image_path(void *userdata,const char *path){(void)open_image_path(userdata,path);}
static void set_visible(struct app_window *a,int visible)
{
    uint32_t i=layer_for_surface(&a->surface);
    a->visible=visible;
    if(i!=UINT32_MAX) voxel_compositor_set_visible(&compositor,i,visible);
}

static void draw_popup(void)
{
    voxel_surface_clear(&menu_surface,0);
    if(menu_open) {
        static const char *const items[APP_COUNT]={"About VOXEL","File Explorer","Settings","Run","Console","Hello World","Text Editor","Image Viewer"};
        voxel_ui_menu(&menu_surface,(struct voxel_ui_rect){0,(int32_t)VIEW_H-TASKBAR_H-(int32_t)MENU_H+1,188,(int32_t)MENU_H},items,APP_COUNT,-1);
    }
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



static void pointer_click(int32_t x,int32_t y,uint32_t buttons)
{
    int id;
    if(y>=(int32_t)VIEW_H-TASKBAR_H) {
        if(x<70) { menu_open=!menu_open; return; }
        if(x>=76) {
            uint8_t visible[APP_COUNT];int slot;for(int i=0;i<APP_COUNT;i++)visible[i]=(uint8_t)(apps[i].visible&&!apps[i].minimized);
            slot=voxel_taskbar_hit(x,visible);
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
            if(item>=0&&item<APP_COUNT) raise_app(item);
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
        } else {
            (void)voxel_app_pointer(&app_contexts[id],(enum voxel_app_id)id,lx,ly);
        }
        return;
    }
    if(x<55&&y<165) {
        if(y<62||y<114) raise_app(VOXEL_APP_EXPLORER); else raise_app(VOXEL_APP_SETTINGS);
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
    voxel_newos_app_host_init(&app_host);
    app_host.theme=&theme;
    app_host.open_text=open_text_path;
    app_host.open_image=open_image_path;
    app_host.request_open_text=request_text_path;
    app_host.request_open_image=request_image_path;
    for(int i=0;i<APP_COUNT;++i) {
        const struct voxel_app_descriptor *d=voxel_app_get((enum voxel_app_id)i);
        uint32_t w=(i==VOXEL_APP_EXPLORER?380u:320u),h=(i==VOXEL_APP_EXPLORER?300u:240u);
        apps[i]=(struct app_window){.pixels=app_pixels[i],.width=w,.height=h,.x=35+(i%3)*46,.y=35+(i%3)*38,.visible=(i==VOXEL_APP_EXPLORER||i==VOXEL_APP_SETTINGS),.title=d?d->title:"Application"};
        if(voxel_surface_init(&apps[i].surface,w,h,VIEW_W,VOXEL_XRGB8888,app_pixels[i],VIEW_W*(VIEW_H-TASKBAR_H))) {
            (void)os_write(no_space,sizeof(no_space)-1u);return 1;
        }
        voxel_app_init(&app_contexts[i],(enum voxel_app_id)i,&apps[i].surface,&app_host);
    }
    draw_desktop();draw_pointer();for(int i=0;i<APP_COUNT;++i)paint_app(i);draw_popup();
    if(voxel_compositor_add(&compositor,&desktop,0,0,0)) {
        (void)os_write(no_space,sizeof(no_space)-1u);return 1;
    }
    for(int i=0;i<APP_COUNT;++i) {
        if(voxel_compositor_add(&compositor,&apps[i].surface,apps[i].x,apps[i].y,0)) {
            (void)os_write(no_space,sizeof(no_space)-1u);return 1;
        }
    }
    if(voxel_compositor_add(&compositor,&menu_surface,0,0,0)||voxel_compositor_add(&compositor,&cursor,0,0,0)) {
        (void)os_write(no_space,sizeof(no_space)-1u);return 1;
    }
    for(int i=0;i<APP_COUNT;++i) {
        voxel_compositor_set_visible(&compositor,(uint32_t)(i+1),apps[i].visible);
        if(apps[i].visible) (void)voxel_compositor_raise(&compositor,(uint32_t)(i+1));
    }
    voxel_compositor_set_visible(&compositor,(uint32_t)(APP_COUNT+1),0);
    active_app=VOXEL_APP_EXPLORER;
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
        draw_desktop();draw_popup();draw_pointer();
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
                voxel_compositor_set_visible(&compositor,mi,menu_open);
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
