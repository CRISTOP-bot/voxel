/* Optional TinyGL pipeline smoke demo. TinyGL's 32-bit PIXEL is 0x00RRGGBB,
 * matching the word format accepted by NEWOS SYS_FBWRITE. */
#include "voxel/framebuffer.h"
#include "voxel/input.h"

#include <TGL/gl.h>
#include <zbuffer.h>
#include <stdint.h>

#define MAX_W 640u
#define MAX_H 480u

int main(int argc, char **argv)
{
    struct voxel_framebuffer fb;
    ZBuffer *zb;
    uint32_t width, height, origin_x, origin_y;
    uint32_t frames = 240u, frame;
    (void)argc;
    (void)argv;

    if (voxel_framebuffer_open(&fb) != 0 ||
        fb.info.width < 160u || fb.info.height < 120u)
        return 1;
    width = (uint32_t)(fb.info.width < MAX_W ? fb.info.width : MAX_W);
    width &= ~3u; /* TinyGL's 32-bit rasterizer aligns scan lines to 4 pixels. */
    height = (uint32_t)(fb.info.height < MAX_H ? fb.info.height : MAX_H);
    if (width < 160u || height < 120u)
        return 1;
    zb = ZB_open((int)width, (int)height, ZB_MODE_RGBA, 0);
    if (!zb)
        return 1;

    glInit(zb);
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.5, 1.5, -1.5 * (GLdouble)height / (GLdouble)width,
              1.5 * (GLdouble)height / (GLdouble)width, 1.0, 10.0);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.025f, 0.045f, 0.08f, 1.0f);
    origin_x = (uint32_t)((fb.info.width - width) / 2u);
    origin_y = (uint32_t)((fb.info.height - height) / 2u);

    for (frame = 0; frame < frames; ++frame) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, -4.0f);
        glRotatef((GLfloat)frame * 1.5f, 0.0f, 1.0f, 0.0f);
        glRotatef((GLfloat)frame, 0.0f, 0.0f, 1.0f);
        glBegin(GL_TRIANGLES);
        glColor3f(0.25f, 0.95f, 0.75f);
        glVertex3f(0.0f, 1.2f, 0.0f);
        glColor3f(0.25f, 0.55f, 1.0f);
        glVertex3f(-1.0f, -0.8f, 0.7f);
        glColor3f(1.0f, 0.48f, 0.25f);
        glVertex3f(1.0f, -0.8f, 0.7f);
        glEnd();

        /* With TGL_FEATURE_RENDER_BITS=32, pbuf words are 0x00RRGGBB.
         * Do not reinterpret them as packed byte-order ABGR or bpp=pitch. */
        if (voxel_framebuffer_blit(&fb, origin_x, origin_y, width, height,
                                   (const uint32_t *)zb->pbuf, width) != 0) {
            glClose();
            ZB_close(zb);
            return 1;
        }
        if (voxel_sleep_ms(16u) < 0) {
            glClose();
            ZB_close(zb);
            return 1;
        }
    }
    glClose();
    ZB_close(zb);
    return 0;
}
