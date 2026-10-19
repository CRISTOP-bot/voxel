# VOXEL

**VOXEL** is a small, modular software graphics stack and NEWOS framebuffer demonstration. It currently implements CPU-backed XRGB/ARGB surfaces, clipping and alpha compositing, a NEWOS framebuffer presenter, and a movable-window demo with PS/2 mouse pointer polling. It lives in its own repository and does not modify NEWOS source repositories.

This is an early graphics foundation—not a desktop environment or a complete graphics API. The public surface/compositor code is kept separate from the NEWOS syscall backend so the core can be tested on a host.

## What is implemented

- Stride-aware software surfaces with pixel, rectangle-fill and clear operations.
- A bounded 16-layer compositor, per-layer visibility, ordering, clipping and straight-alpha blending.
- A NEWOS x86_64 `int $0x80` framebuffer backend using `SYS_FBINFO` (453) and `SYS_FBWRITE` (454). Frames are submitted as XRGB8888 words; NEWOS converts these to the active 24- or 32-bpp panel layout.
- A NEWOS mouse snapshot reader using `SYS_MOUSE_GET` (455), and a sleep helper using `SYS_SLEEP` (35).
- `voxel.elf`, a software-composited desktop demo with three draggable windows, close controls and a software cursor.
- Optional `voxel-tinygl.elf`, a TinyGL triangle rendering smoke demo. TinyGL is pinned as a submodule at `c2e48591a6bfba1a85f1b87a78dcf1abf7dff57a`; its allocator is a bounded 16 MiB single-threaded arena.
- A host-side core test suite and a NEWOS package spec.

## Not implemented / limitations

- **Keyboard events are unsupported.** Current NEWOS userspace has no keyboard-event syscall exposed to VOXEL. The sleep helper is not an input API.
- There is no GPU acceleration, window server protocol, font/text renderer, input focus model, general-purpose OpenGL implementation, or persistent window manager.
- The demo requires a NEWOS boot with a supported Limine framebuffer of at least 640×480 and a working PS/2 mouse for interaction. It is a standalone program, not integrated into NEWOS's shell/session startup.
- The optional TinyGL target has not been claimed to compile or run on NEWOS merely because the upstream sources and API were inspected. It depends on a working NEWOS floating-point toolchain configuration.
- The layer array and demo are single-threaded. The allocator is not thread-safe.

## Build and tests

### Host core tests

Requires a C11 compiler and GNU Make:

```sh
make test
```

This builds and runs the portable surface/compositor tests only; it does not build NEWOS executables or test syscalls.

### NEWOS demo

Requires a NEWOS checkout with its `toolchain/newos-gcc` wrapper and `user/lib/app.ld` SDK file available. Build NEWOS and its toolchain first, then:

```sh
make NEWOS_ROOT=/path/to/NEWOS demo
```

The resulting executable is `build/voxel.elf`. It has not been validated by this repository's CI against a NEWOS cross-toolchain. Use the target kernel and QEMU/hardware for runtime testing.

### Optional TinyGL demo

Initialize the pinned submodule and build the regular NEWOS demo/toolchain prerequisites:

```sh
git submodule update --init --recursive
make NEWOS_ROOT=/path/to/NEWOS TINYGL_ROOT=vendor/tinygl tinygl
```

The target sets `NEWOS_FPU=1` for the floating-point TinyGL build and uses a custom 32-bit render configuration. The result is `build/voxel-tinygl.elf`.

### Cross-build and QEMU validation

The manual GitHub Actions workflow `target-validation.yml` checks out a pinned NEWOS revision, builds its real `x86_64-elf` toolchain, cross-compiles both VOXEL demos, builds the package, and boots the normal demo for one frame in QEMU. It adds VOXEL to NEWOS's initramfs only in the disposable Actions checkout; it never commits changes to NEWOS. The workflow uploads the target binaries, package and boot image as artifacts. A green host-core workflow alone does not imply this target workflow passed.

### NEWOS package

With `NEWOS_ROOT` set to a complete NEWOS checkout:

```sh
make NEWOS_ROOT=/path/to/NEWOS package
```

This stages the built `voxel.elf` in NEWOS's userland build directory and calls `tools/newpkg/newpkg-build.py` to create `build/packages/voxel-0.1.0-x86_64.new`. Install on NEWOS with:

```sh
newpkg install voxel-0.1.0-x86_64.new
```

Package construction/install and actual framebuffer runtime behavior are not validated by the host CI job.

## Evidence and validation status

The NEWOS syscall numbers, request layouts, framebuffer pixel transfer semantics, mouse state layout and TinyGL pixel format were inspected against the public upstream sources at the pinned/project revisions. This source inspection is not equivalent to compiling the target code. CI runs the portable C core tests only; it does not boot NEWOS, exercise the framebuffer syscalls, or validate a QEMU session. See [`docs/architecture.md`](docs/architecture.md) and [`docs/build-and-validation.md`](docs/build-and-validation.md) for the boundaries and commands.

## License

VOXEL's own code is MIT licensed; the optional TinyGL submodule is separately MIT licensed upstream. See [`LICENSE`](LICENSE).
