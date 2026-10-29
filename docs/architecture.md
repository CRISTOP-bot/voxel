# VOXEL architecture

## Layers

```text
Demo programs
  voxel_demo.c / tinygl_demo.c
        │
        ├── portable software core: surfaces + compositor
        │       └── XRGB8888 / straight-alpha ARGB8888 buffers
        │
        └── NEWOS backend: framebuffer + mouse + sleep syscalls
                └── NEWOS int $0x80 ABI
```

### Portable core

- `include/voxel/surface.h`, `src/graphics/surface.c`: caller-owned pixel buffers, explicit pixel stride, clipped rectangle and point operations. Initialization checks the requested storage capacity.
- `include/voxel/compositor.h`, `src/graphics/compositor.c`: caller-owned output buffer and up to `VOXEL_MAX_LAYERS` ordered surfaces. Array order is back-to-front; `voxel_compositor_raise()` moves the selected layer to the last/front position. Composition starts from a dark opaque background and blends straight-alpha ARGB layers over the result. A clip rectangle restricts output writes.

The core owns neither surface pixels nor output memory. Callers must keep storage alive and must not mutate surface metadata to invalid values after initialization.

### NEWOS backend

- `include/voxel/abi.h`: fixed-width syscall records and compile-time layout guards.
- `src/platform/newos/framebuffer.c`: obtains framebuffer geometry and submits bounded XRGB8888 rectangles with `SYS_FBWRITE`.
- `src/platform/newos/input.c`: polls the latest mouse snapshot and exposes a duration-based sleep helper.

The kernel's display memory is not directly mapped by VOXEL. `SYS_FBWRITE` accepts contiguous 32-bit `0x00RRGGBB` words and performs conversion to the framebuffer's native RGB channel masks. Panel pitch is therefore checked as geometry metadata but is not used to stride the source buffer.

No keyboard event API is implemented because the current inspected NEWOS userspace syscall interface does not expose one. Do not interpret `voxel_sleep_ms()` as event polling.

### Demos

- `demo/voxel_demo.c` draws a fixed-size 640×480 software desktop and windows, centers it in the detected framebuffer, polls the PS/2 pointer, raises the cursor over windows, and permits title-bar dragging and close-square clicks.
- `demo/tinygl_demo.c` optionally initializes TinyGL with a bounded arena allocator, renders a triangle into a 32-bit software Z-buffer, and presents the pixel words through the same framebuffer backend. It is a rendering smoke demo, not a reusable GL window system.

The pinned TinyGL ABI exposes `GLint`/`GLuint`, `GLfloat`, `GLdouble`, and ordinary pointers for the structures and buffers used here; `ZB_open()` allocates these through `gl_malloc` and releases them through `gl_free`. Its default `TGL_FEATURE_ALIGNAS` is 0, and the target build does not compile the separate example files that declare aligned vector types. `src/graphics/tinygl_allocator.c` uses the x86_64 ABI's 16-byte general-allocation alignment instead of relying on `max_align_t`, which the NEWOS SDK headers do not declare. Both the arena and block-header union are aligned to 16 bytes, the header size is statically required to be a multiple of 16, and allocation sizes are rounded to that boundary. This meets TinyGL's configured 16-byte alignment option too, but should be revisited if the target ABI or allocator contract changes.

## Current limitations

- VOXEL cannot receive keyboard events; the current NEWOS userspace ABI exposes no keyboard-event syscall to it. Only mouse-state polling is implemented.
- Rendering is CPU/software-only. There is no GPU acceleration, window-server protocol, or font/text rendering, and the demo is not a complete or integrated desktop environment.
- A successful host-core test run proves only the portable surface/compositor logic. It does not prove that the target executable links correctly or runs in NEWOS/QEMU; those need specific cross-build, package, and runtime checks.

## ABI and upstream basis

VOXEL targets the current NEWOS x86_64 syscall ABI using `int $0x80`: `SYS_FBINFO=453`, `SYS_FBWRITE=454`, `SYS_MOUSE_GET=455`, and `SYS_SLEEP=35`. The framebuffer info record is 40 bytes, FBWRITE request is 48 bytes, mouse snapshot is 32 bytes, and timespec is 16 bytes; compile-time assertions in `abi.h` guard these sizes.

The TinyGL optional dependency is a submodule pinned to `jserv/tinygl` commit `c2e48591a6bfba1a85f1b87a78dcf1abf7dff57a`. Its 32-bit `PIXEL`/RGBA configuration uses `0x00RRGGBB`, matching the NEWOS write interface. In target run [39196790955](https://github.com/CRISTOP-bot/voxel/actions/runs/39196790955), TinyGL cross-compiled and its ELF entry matched a defined `_start`; it was not executed in QEMU. The build still reports compiler warnings and an RWX LOAD-segment warning, so runtime and warning cleanup remain separate work.


### Reusable classic UI

`include/voxel/classic_ui.h` and `src/theme/classic.c` add clipped 5×7 bitmap text,
gray/navy palette constants, raised/sunken bevels, square buttons, active and
inactive title bars, window frames, menu bars/popups, pixel-art icons, taskbar,
Start menu, and a compact message dialog. They draw into caller-owned VOXEL
surfaces and use no invented kernel API or display protocol. The demo uses only
the documented framebuffer and mouse interfaces. Settings is illustrative.

The desktop remains a mouse-driven single-process demo, not a persistent system
window manager. NEWOS currently exposes no keyboard-event syscall to the program;
keyboard navigation is not claimed. Host tests cannot prove target cross-build,
QEMU runtime, visual output, or interactive behavior.


## Source layout and implementation status

The source tree is layered by responsibility: `src/graphics/` owns software
surfaces/composition and the TinyGL allocator; `src/theme/classic.c` implements
the reusable Win9x-style drawing primitives; and `src/platform/newos/` contains
the framebuffer presenter, mouse/sleep backend, and the single `int $0x80` ABI
bridge. Public drawing/input/framebuffer headers remain under `include/voxel/`.
The Makefile lists these translation units explicitly, so moving a file cannot
quietly omit it from host tests or target linking.

The larger proposed desktop tree is not yet implemented. In particular, this
repository does not currently contain independent app processes, filesystem or
process APIs, a window server, persistent desktop/session services, keyboard
events, text input, font/image loaders, or several proposed applications. Those
need verified NEWOS interfaces and real implementations before corresponding
source modules or app claims are added; no placeholder modules are provided.


### Application modules

`apps/` contains registered in-process app modules backed by `voxel_app_host`:
File Explorer (NEWOS `READDIR`), Settings (session palette), Run and the console
launcher (`SPAWN2`), a bounded text editor (NEWOS file read/write), Hello World,
and a BMP image viewer (strict 24-bit uncompressed decoder). They are compiled
into the demonstration process; they are not independent binaries or sandboxed
applications. The app registry and portable app tests live alongside the core.
The pinned NEWOS ABI exposes mouse polling but no GUI keyboard input syscall, so
the text-entry views include a mouse-operated virtual keyboard. The console
launches one absolute executable per command and collects output through the
existing pipe and `SPAWN3` descriptor-redirection interface. It does not provide
a shell parser or pipeline syntax.
