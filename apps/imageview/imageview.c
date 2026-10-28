#include "voxel/app.h"
#include "voxel/classic_ui.h"
struct image_state{uint32_t pixels[64*48];uint32_t width,height,ready;};
void voxel_imageview_init(struct voxel_app_context*a){struct image_state*s=(struct image_state*)a->state;s->width=s->height=s->ready=0;}
int voxel_imageview_load(struct voxel_app_context*a,const void*bytes,uint32_t length){struct image_state*s=(struct image_state*)a->state;if(voxel_bmp_decode(bytes,length,s->pixels,64u*48u,&s->width,&s->height))return -1;s->ready=1;return 0;}
int voxel_imageview_open(struct voxel_app_context*a,const char*path){uint8_t bytes[64u*48u*4u+4096u];uint32_t n=0;if(!a->host||!a->host->read_file||!path)return -1;if(a->host->read_file(a->host->userdata,path,bytes,sizeof(bytes),&n))return -1;return voxel_imageview_load(a,bytes,n);}
int voxel_imageview_pointer(struct voxel_app_context*a,int32_t x,int32_t y){(void)a;(void)x;(void)y;return 0;}
void voxel_imageview_draw(struct voxel_app_context*a){struct image_state*s=(struct image_state*)a->state;uint32_t y,x;voxel_surface_clear(a->surface,VOXEL_UI_FACE);voxel_ui_window_frame(a->surface, (struct voxel_ui_rect){2,2,(int32_t)a->surface->width-4,(int32_t)a->surface->height-4}, "Image Viewer", 1);if(!s->ready){voxel_ui_text(a->surface,14,48,"Open a supported 24-bit BMP through app host.",VOXEL_UI_TEXT);return;}for(y=0;y<s->height;y++)for(x=0;x<s->width;x++)voxel_surface_put_pixel(a->surface,12+(int32_t)x,44+(int32_t)y,s->pixels[y*s->width+x]);}
