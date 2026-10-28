#include "voxel/app.h"
#include <stddef.h>
#include <stdint.h>

static uint16_t u16(const uint8_t *p) { return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
static uint32_t u32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24); }

int voxel_bmp_decode(const void *data, uint32_t length, uint32_t *out,
                     uint32_t capacity, uint32_t *width, uint32_t *height)
{
    const uint8_t *b=(const uint8_t *)data;
    uint32_t offset,dib,w,h,rowbytes,y;
    int32_t signed_h;
    if(!b||!out||!width||!height||length<54u||b[0]!='B'||b[1]!='M') return -1;
    offset=u32(b+10);dib=u32(b+14);w=u32(b+18);signed_h=(int32_t)u32(b+22);
    if(dib<40u||signed_h==0||signed_h==INT32_MIN||u16(b+26)!=1u||u16(b+28)!=24u||u32(b+30)!=0u) return -1;
    h=(uint32_t)(signed_h<0?-signed_h:signed_h);
    if(!w||!h||w>4096u||h>4096u||w>UINT32_MAX/3u) return -1;
    rowbytes=(w*3u+3u)&~3u;
    if((uint64_t)w*h>capacity||offset>length||(uint64_t)offset+(uint64_t)rowbytes*h>length) return -1;
    for(y=0;y<h;y++) {
        uint32_t sy=signed_h>0?h-1u-y:y,x;
        const uint8_t *row=b+offset+(size_t)sy*rowbytes;
        for(x=0;x<w;x++) out[(size_t)y*w+x]=((uint32_t)row[x*3u+2u]<<16)|((uint32_t)row[x*3u+1u]<<8)|row[x*3u];
    }
    *width=w;*height=h;return 0;
}
