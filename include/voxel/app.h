#ifndef VOXEL_APP_H
#define VOXEL_APP_H

#include "voxel/surface.h"
#include <stdint.h>

#define VOXEL_APP_PATH_MAX 128u
#define VOXEL_APP_TEXT_MAX 512u
#define VOXEL_APP_STATE_WORDS 2048u
#define VOXEL_FS_NAME_MAX 64u
#define VOXEL_FS_MODE_DIR 0x4000u
#define VOXEL_FS_MODE_REG 0x8000u
#define VOXEL_KEY_ENTER 0x100u
#define VOXEL_KEY_BACKSPACE 0x101u
#define VOXEL_KEY_LEFT 0x102u
#define VOXEL_KEY_RIGHT 0x103u
#define VOXEL_KEY_UP 0x104u
#define VOXEL_KEY_DOWN 0x105u
#define VOXEL_KEY_DELETE 0x106u
#define VOXEL_KEY_TAB 0x107u
#define VOXEL_KEY_ESCAPE 0x108u
#define VOXEL_KEY_CTRL 0x200u
#define VOXEL_KEY_CAPS 0x201u

struct voxel_fs_entry {
    char name[VOXEL_FS_NAME_MAX];
    uint32_t mode;
    uint64_t size;
};

struct voxel_theme_state {
    uint32_t desktop;
    uint32_t face;
    uint32_t title;
    uint32_t desktop_saved;
    uint32_t face_saved;
};

/* Platform callbacks are real syscall adapters on NEWOS and fakeable in tests. */
struct voxel_app_host {
    void *userdata;
    int (*list_dir)(void *userdata, const char *path,
                    struct voxel_fs_entry *entries, uint32_t capacity);
    int (*read_file)(void *userdata, const char *path, void *buffer,
                     uint32_t capacity, uint32_t *bytes_read);
    int (*write_file)(void *userdata, const char *path, const void *buffer,
                      uint32_t bytes);
    int (*spawn)(void *userdata, const char *path);
    int (*stat_path)(void *userdata, const char *path, uint32_t *mode);
    int (*getcwd)(void *userdata, char *buffer, uint32_t capacity);
    struct voxel_theme_state *theme;
    void (*open_path)(void *userdata, const char *path);
    int (*open_image)(void *userdata, const char *path);
};

struct voxel_app_context {
    struct voxel_surface *surface;
    struct voxel_app_host *host;
    uint64_t state_alignment;
    unsigned char state[VOXEL_APP_STATE_WORDS * sizeof(uint64_t)];
};

enum voxel_app_id {
    VOXEL_APP_ABOUT = 0,
    VOXEL_APP_EXPLORER,
    VOXEL_APP_SETTINGS,
    VOXEL_APP_RUN,
    VOXEL_APP_TERMINAL,
    VOXEL_APP_HELLOWORLD,
    VOXEL_APP_EDITOR,
    VOXEL_APP_IMAGEVIEW,
    VOXEL_APP_COUNT
};

struct voxel_app_descriptor {
    const char *name;
    const char *title;
    void (*init)(struct voxel_app_context *app);
    void (*draw)(struct voxel_app_context *app);
    int (*pointer)(struct voxel_app_context *app, int32_t x, int32_t y);
    int (*key)(struct voxel_app_context *app, uint32_t key);
};

const struct voxel_app_descriptor *voxel_app_get(enum voxel_app_id id);
void voxel_app_init(struct voxel_app_context *app, enum voxel_app_id id,
                    struct voxel_surface *surface, struct voxel_app_host *host);
void voxel_app_draw(struct voxel_app_context *app, enum voxel_app_id id);
int voxel_app_pointer(struct voxel_app_context *app, enum voxel_app_id id,
                      int32_t x, int32_t y);
int voxel_app_key(struct voxel_app_context *app, enum voxel_app_id id,
                  uint32_t key);
void voxel_newos_app_host_init(struct voxel_app_host *host);

int voxel_bmp_decode(const void *bytes, uint32_t length, uint32_t *pixels,
                     uint32_t capacity_pixels, uint32_t *width,
                     uint32_t *height);
#endif
