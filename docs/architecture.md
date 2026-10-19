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

- `include/voxel/surface.h`, `src/surface.c`: caller-owned pixel buffers, explicit pixel stride, clipped rectangle and point operations. Initialization checks the requested storage capacity.
- `include/voxel/compositor.h`, `src/compositor.c`: caller-owned output buffer and up to `VOXEL_MAX_LAYERS` ordered surfaces. Array order is back-to-front; `voxel_compositor_raise()` moves the selected layer to the last/front position. Composition starts from a dark opaque background and blends straight-alpha ARGB layers over the result. A clip rectangle restricts output writes.

The core owns neither surface pixels nor output memory. Callers must keep storage alive and must not mutate surface metadata to invalid values after initialization.

### NEWOS backend

- `include/voxel/abi.h`: fixed-width syscall records and compile-time layout guards.
- `src/newos_backend.c`: obtains framebuffer geometry and submits bounded XRGB8888 rectangles with `SYS_FBWRITE`.
- `src/input.c`: polls the latest mouse snapshot and exposes a duration-based sleep helper.

The kernel's display memory is not directly mapped by VOXEL. `SYS_FBWRITE` accepts contiguous 32-bit `0x00RRGGBB` words and performs conversion to the framebuffer's native RGB channel masks. Panel pitch is therefore checked as geometry metadata but is not used to stride the source buffer.

No keyboard event API is implemented because the current inspected NEWOS userspace syscall interface does not expose one. Do not interpret `voxel_sleep_ms()` as event polling.

### Demos

- `demo/voxel_demo.c` draws a fixed-size 640×480 software desktop and windows, centers it in the detected framebuffer, polls the PS/2 pointer, raises the cursor over windows, and permits title-bar dragging and close-square clicks.
- `demo/tinygl_demo.c` optionally initializes TinyGL with a bounded arena allocator, renders a triangle into a 32-bit software Z-buffer, and presents the pixel words through the same framebuffer backend. It is a rendering smoke demo, not a reusable GL window system.

## ABI and upstream basis

VOXEL targets the current NEWOS x86_64 syscall ABI using `int $0x80`: `SYS_FBINFO=453`, `SYS_FBWRITE=454`, `SYS_MOUSE_GET=455`, and `SYS_SLEEP=35`. The framebuffer info record is 40 bytes, FBWRITE request is 48 bytes, mouse snapshot is 32 bytes, and timespec is 16 bytes; compile-time assertions in `abi.h` guard these sizes.

The TinyGL optional dependency is a submodule pinned to `jserv/tinygl` commit `c2e48591a6bfba1a85f1b87a78dcf1abf7dff57a`. Its 32-bit `PIXEL`/RGBA configuration uses `0x00RRGGBB`, matching the NEWOS write interface. The TinyGL source inspection does not establish that its target build or runtime works in NEWOS.
