#include "voxel/app.h"
#include <stddef.h>

void voxel_about_draw(struct voxel_app_context *);
void voxel_explorer_init(struct voxel_app_context *);
void voxel_explorer_draw(struct voxel_app_context *);
int voxel_explorer_pointer(struct voxel_app_context *, int32_t, int32_t);
void voxel_settings_draw(struct voxel_app_context *);
int voxel_settings_pointer(struct voxel_app_context *, int32_t, int32_t);
void voxel_run_init(struct voxel_app_context *);
void voxel_run_draw(struct voxel_app_context *);
int voxel_run_key(struct voxel_app_context *, uint32_t);
int voxel_run_pointer(struct voxel_app_context *, int32_t, int32_t);
void voxel_terminal_init(struct voxel_app_context *);
void voxel_terminal_draw(struct voxel_app_context *);
int voxel_terminal_key(struct voxel_app_context *, uint32_t);
int voxel_terminal_pointer(struct voxel_app_context *, int32_t, int32_t);
void voxel_helloworld_draw(struct voxel_app_context *);
void voxel_editor_init(struct voxel_app_context *);
void voxel_editor_draw(struct voxel_app_context *);
int voxel_editor_key(struct voxel_app_context *, uint32_t);
int voxel_editor_pointer(struct voxel_app_context *, int32_t, int32_t);
void voxel_imageview_init(struct voxel_app_context *);
void voxel_imageview_draw(struct voxel_app_context *);
int voxel_imageview_pointer(struct voxel_app_context *, int32_t, int32_t);

static const struct voxel_app_descriptor apps[VOXEL_APP_COUNT] = {
    [VOXEL_APP_ABOUT] = {"about", "About VOXEL", 0, voxel_about_draw, 0, 0},
    [VOXEL_APP_EXPLORER] = {"explorer", "File Explorer", voxel_explorer_init, voxel_explorer_draw, voxel_explorer_pointer, 0},
    [VOXEL_APP_SETTINGS] = {"settings", "Display Properties", 0, voxel_settings_draw, voxel_settings_pointer, 0},
    [VOXEL_APP_RUN] = {"run", "Run", voxel_run_init, voxel_run_draw, voxel_run_pointer, voxel_run_key},
    [VOXEL_APP_TERMINAL] = {"terminal", "VOXEL Console", voxel_terminal_init, voxel_terminal_draw, voxel_terminal_pointer, voxel_terminal_key},
    [VOXEL_APP_HELLOWORLD] = {"helloworld", "Hello, World!", 0, voxel_helloworld_draw, 0, 0},
    [VOXEL_APP_EDITOR] = {"editor", "Text Editor", voxel_editor_init, voxel_editor_draw, voxel_editor_pointer, voxel_editor_key},
    [VOXEL_APP_IMAGEVIEW] = {"imageview", "Image Viewer", voxel_imageview_init, voxel_imageview_draw, voxel_imageview_pointer, 0}
};

const struct voxel_app_descriptor *voxel_app_get(enum voxel_app_id id)
{
    return id >= 0 && id < VOXEL_APP_COUNT ? &apps[id] : 0;
}

void voxel_app_init(struct voxel_app_context *app, enum voxel_app_id id,
                    struct voxel_surface *surface, struct voxel_app_host *host)
{
    const struct voxel_app_descriptor *d = voxel_app_get(id);
    if (!app) return;
    *app = (struct voxel_app_context){0};
    app->surface = surface;
    app->host = host;
    if (d && d->init) d->init(app);
}

void voxel_app_draw(struct voxel_app_context *app, enum voxel_app_id id)
{
    const struct voxel_app_descriptor *d = voxel_app_get(id);
    if (app && app->surface && d && d->draw) d->draw(app);
}

int voxel_app_pointer(struct voxel_app_context *app, enum voxel_app_id id,
                      int32_t x, int32_t y)
{
    const struct voxel_app_descriptor *d = voxel_app_get(id);
    return app && d && d->pointer ? d->pointer(app, x, y) : 0;
}

int voxel_app_key(struct voxel_app_context *app, enum voxel_app_id id,
                  uint32_t key)
{
    const struct voxel_app_descriptor *d = voxel_app_get(id);
    return app && d && d->key ? d->key(app, key) : 0;
}
