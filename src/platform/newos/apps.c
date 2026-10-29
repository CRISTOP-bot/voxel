#include "voxel/app.h"
#include "voxel/abi.h"
#include "syscall.h"
#include <stddef.h>
#include <stdint.h>

#define O_READ 0
#define O_WRITE 1
#define O_CREATE 0100
#define SYS_FTRUNCATE 77
#define VOXEL_SYS_GETTIME 458
struct newos_time { uint16_t year;uint8_t mon,day,hour,min,sec,pad; };
struct newos_dirent { char name[64]; uint32_t mode; uint64_t size; };
struct newos_stat { uint64_t dev,ino,nlink;uint32_t mode,uid,gid,pad;uint64_t rdev;int64_t size,blksize,blocks,atime_sec,atime_nsec,mtime_sec,mtime_nsec,ctime_sec,ctime_nsec;uint64_t spare[3]; };
typedef char stat_size_must_be_144[(sizeof(struct newos_stat)==144)?1:-1];
typedef char dirent_size_must_be_80[(sizeof(struct newos_dirent)==80)?1:-1];
static long syscall1(long n,long a){return voxel_newos_syscall1(n,a);}
static long syscall2(long n,long a,long b){long r;__asm__ __volatile__("int $0x80":"=a"(r):"a"(n),"D"(a),"S"(b):"memory");return r;}
static long syscall3(long n,long a,long b,long c){long r;__asm__ __volatile__("int $0x80":"=a"(r):"a"(n),"D"(a),"S"(b),"d"(c):"memory");return r;}
static long syscall5(long n,long a,long b,long c,long d,long e){long r;register long r10 __asm__("r10")=d;register long r8 __asm__("r8")=e;__asm__ __volatile__("int $0x80":"=a"(r):"a"(n),"D"(a),"S"(b),"d"(c),"r"(r10),"r"(r8):"memory");return r;}

static int list_dir(void *u,const char *path,struct voxel_fs_entry *out,uint32_t cap)
{
    struct newos_dirent raw[16];long count,i;(void)u;
    if(!path||!out||!cap)return -1;if(cap>16)cap=16;
    count=syscall3(VOXEL_SYS_READDIR,(long)(uintptr_t)path,(long)(uintptr_t)raw,(long)(cap*sizeof(raw[0])));
    if(count<0)return -1;if((uint64_t)count>cap)count=cap;
    for(i=0;i<count;i++){uint32_t j;for(j=0;j<VOXEL_FS_NAME_MAX-1&&raw[i].name[j];j++)out[i].name[j]=raw[i].name[j];out[i].name[j]=0;out[i].mode=raw[i].mode;out[i].size=raw[i].size;}
    return (int)count;
}
static int read_file(void*u,const char*path,void*buffer,uint32_t capacity,uint32_t*bytes_read)
{
    long fd,n;(void)u;if(!path||!buffer||!bytes_read)return -1;*bytes_read=0;
    fd=syscall2(VOXEL_SYS_OPEN,(long)(uintptr_t)path,O_READ);if(fd<0)return -1;
    n=syscall3(VOXEL_SYS_READ,fd,(long)(uintptr_t)buffer,capacity);(void)syscall1(VOXEL_SYS_CLOSE,fd);
    if(n<0)return -1;*bytes_read=(uint32_t)n;return 0;
}
static int open_image(void*u,const char*path){(void)u;(void)path;return -2;}

static int write_file(void*u,const char*path,const void*buffer,uint32_t bytes)
{
    long fd,n;(void)u;if(!path||(!buffer&&bytes))return -1;
    fd=syscall2(VOXEL_SYS_OPEN,(long)(uintptr_t)path,O_WRITE|O_CREATE);if(fd<0)return -1;
    if(syscall2(SYS_FTRUNCATE,fd,0)<0){(void)syscall1(VOXEL_SYS_CLOSE,fd);return -1;}
    n=syscall3(VOXEL_SYS_WRITE,fd,(long)(uintptr_t)buffer,bytes);(void)syscall1(VOXEL_SYS_CLOSE,fd);
    return n==(long)bytes?0:-1;
}
static int stat_path(void*u,const char*path,uint32_t*mode)
{
    struct newos_stat st;(void)u;if(!path||!mode)return -1;
    if(syscall2(VOXEL_SYS_STAT,(long)(uintptr_t)path,(long)(uintptr_t)&st)<0)return -1;
    *mode=st.mode;return 0;
}
static int get_time(void*u,struct voxel_clock_time*t){struct newos_time n;(void)u;if(!t||syscall1(VOXEL_SYS_GETTIME,(long)(uintptr_t)&n)<0)return -1;t->year=n.year;t->month=n.mon;t->day=n.day;t->hour=n.hour;t->minute=n.min;t->second=n.sec;t->pad=0;return 0;}
static int run_command(void*u,const char*path,char*out,uint32_t cap,uint32_t*readn)
{
    int pipes[2];char*argv[2];long pid,n;uint32_t used=0;(void)u;
    if(!path||!out||!cap||!readn)return -1;*readn=0;
    if(syscall1(VOXEL_SYS_PIPE,(long)(uintptr_t)pipes)<0)return -1;
    argv[0]=(char*)path;argv[1]=0;
    pid=syscall5(VOXEL_SYS_SPAWN3,(long)(uintptr_t)path,(long)(uintptr_t)argv,1,0,pipes[1]);
    if(pid<0){(void)syscall1(VOXEL_SYS_CLOSE,pipes[0]);(void)syscall1(VOXEL_SYS_CLOSE,pipes[1]);return -1;}
    (void)syscall1(VOXEL_SYS_CLOSE,pipes[1]);
    while(used<cap){n=syscall3(VOXEL_SYS_READ,pipes[0],(long)(uintptr_t)(out+used),cap-used);if(n<=0)break;used+=(uint32_t)n;}
    (void)syscall1(VOXEL_SYS_CLOSE,pipes[0]);(void)syscall2(VOXEL_SYS_WAITPID,pid,0);*readn=used;return 0;
}
static int spawn(void*u,const char*path)
{
    char *argv[2];long r;(void)u;if(!path||!path[0])return -1;
    argv[0]=(char*)path;argv[1]=0;r=syscall3(VOXEL_SYS_SPAWN2,(long)(uintptr_t)path,(long)(uintptr_t)argv,1);
    return r<0?-1:0;
}
void voxel_newos_app_host_init(struct voxel_app_host*host)
{
    if(!host)return;host->userdata=0;host->list_dir=list_dir;host->read_file=read_file;host->write_file=write_file;host->spawn=spawn;host->stat_path=stat_path;host->getcwd=0;host->get_time=get_time;host->theme=0;host->open_path=0;host->open_image=open_image;host->open_text=0;host->run_command=run_command;
}
