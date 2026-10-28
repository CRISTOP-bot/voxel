#include "voxel/app.h"
#include "voxel/classic_ui.h"
#include "voxel/framebuffer.h"
int voxel_framebuffer_blit(struct voxel_framebuffer *fb,uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint32_t*p,size_t stride){(void)fb;(void)x;(void)y;(void)w;(void)h;(void)p;(void)stride;return 0;}
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static int spawn_calls;
static int fake_spawn(void*u,const char*p){(void)u;assert(p[0]=='/');spawn_calls++;return 0;}
static int fake_list(void*u,const char*p,struct voxel_fs_entry*e,uint32_t cap){(void)u;(void)p;if(cap<2)return -1;e[0]=(struct voxel_fs_entry){"docs",VOXEL_FS_MODE_DIR,0};e[1]=(struct voxel_fs_entry){"readme.txt",VOXEL_FS_MODE_REG,12};return 2;}
static void test_app_registry_and_draw(void){uint32_t pixels[240*200];struct voxel_surface s;struct voxel_app_context a;struct voxel_app_host h={0};int i;h.spawn=fake_spawn;h.list_dir=fake_list;assert(voxel_surface_init(&s,240,200,240,VOXEL_XRGB8888,pixels,240u*200u)==0);for(i=0;i<VOXEL_APP_COUNT;i++){assert(voxel_app_get((enum voxel_app_id)i));voxel_app_init(&a,(enum voxel_app_id)i,&s,&h);voxel_app_draw(&a,(enum voxel_app_id)i);}assert(!voxel_app_get((enum voxel_app_id)99));}
static void test_explorer_and_run(void){uint32_t pixels[240*200];struct voxel_surface s;struct voxel_app_context a;struct voxel_app_host h={0};h.spawn=fake_spawn;h.list_dir=fake_list;assert(voxel_surface_init(&s,240,200,240,VOXEL_XRGB8888,pixels,240u*200u)==0);voxel_app_init(&a,VOXEL_APP_EXPLORER,&s,&h);voxel_app_draw(&a,VOXEL_APP_EXPLORER);assert(voxel_app_pointer(&a,VOXEL_APP_EXPLORER,20,55)==1);voxel_app_init(&a,VOXEL_APP_RUN,&s,&h);for(const char*p="/bin/about";*p;p++)assert(voxel_app_key(&a,VOXEL_APP_RUN,(uint32_t)*p));assert(voxel_app_key(&a,VOXEL_APP_RUN,13));assert(spawn_calls==1);}
static void test_bmp_decode(void){const uint8_t bmp[58]={'B','M',58,0,0,0,0,0,0,0,54,0,0,0,40,0,0,0,1,0,0,0,1,0,0,0,1,0,24,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,2,1,0};uint32_t px=0,w=0,h=0;assert(voxel_bmp_decode(bmp,sizeof(bmp),&px,1,&w,&h)==0);assert(w==1&&h==1&&px==0x00010203u);assert(voxel_bmp_decode(bmp,sizeof(bmp),&px,0,&w,&h)<0);}
int main(void){test_app_registry_and_draw();test_explorer_and_run();test_bmp_decode();puts("VOXEL app tests: PASS");return 0;}
