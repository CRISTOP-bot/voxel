#include "voxel/app.h"
#include "voxel/classic_ui.h"
#include "../common/virtual_keyboard.h"
#define TERM_TEXT 384u
struct term_state { char line[96]; char history[TERM_TEXT]; uint32_t length; };
static struct term_state*state(struct voxel_app_context*a){return(struct term_state*)a->state;}
void voxel_terminal_init(struct voxel_app_context*a){state(a)->length=0;state(a)->line[0]=0;state(a)->history[0]=0;}
static void append(struct term_state*s,const char*p){unsigned n=0;while(s->history[n]&&n<TERM_TEXT-2)n++;while(*p&&n<TERM_TEXT-2)s->history[n++]=*p++;s->history[n++]='\n';s->history[n]=0;}
static void execute(struct voxel_app_context*a){struct term_state*s=state(a);if(!s->line[0])return;append(s,s->line);if(a->host&&a->host->spawn&&s->line[0]=='/'&&a->host->spawn(a->host->userdata,s->line)==0)append(s,"Program launched; output remains on NEWOS console.");else append(s,"Only absolute executable paths can be launched here.");s->length=0;s->line[0]=0;}
void voxel_terminal_draw(struct voxel_app_context*a){struct term_state*s=state(a);voxel_surface_clear(a->surface,0x00000080u);voxel_ui_window_frame(a->surface, (struct voxel_ui_rect){2,2,(int32_t)a->surface->width-4,(int32_t)a->surface->height-4}, "VOXEL Console", 1);voxel_ui_bevel(a->surface,(struct voxel_ui_rect){10,36,(int32_t)a->surface->width-20,(int32_t)a->surface->height-142},1);voxel_ui_text(a->surface,16,44,s->history,VOXEL_UI_TEXT);voxel_ui_text(a->surface,16,(int32_t)a->surface->height-130,">",VOXEL_UI_TEXT);voxel_ui_text(a->surface,28,(int32_t)a->surface->height-130,s->line,VOXEL_UI_TEXT);voxel_vk_draw(a->surface,(int32_t)a->surface->height-108);}
int voxel_terminal_key(struct voxel_app_context*a,uint32_t k){struct term_state*s=state(a);if(k==13){execute(a);return 1;}if(k==8&&s->length){s->line[--s->length]=0;return 1;}if(k>=32&&k<127&&s->length+1<sizeof(s->line)){s->line[s->length++]=(char)k;s->line[s->length]=0;return 1;}return 0;}
int voxel_terminal_pointer(struct voxel_app_context*a,int32_t x,int32_t y){struct term_state*s=state(a);uint32_t key=0,len=s->length;if(y>=(int32_t)a->surface->height-108&&voxel_vk_pointer(a->surface,x,y,(int32_t)a->surface->height-108,s->line,sizeof(s->line),&len,&key)){s->length=len;if(key==13)execute(a);return 1;}return 0;}
