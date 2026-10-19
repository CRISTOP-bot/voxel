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

- **VOXEL has no keyboard-event input.** Current NEWOS userspace exposes no keyboard-event syscall to the program; the sleep helper is not an input API.
- **No GPU acceleration, window server, or font/text rendering.** VOXEL is a CPU software compositor, not a complete desktop or general-purpose OpenGL implementation; it also has no input-focus model or persistent window manager.
- **Target behavior needs target-specific validation.** A host-side core-test pass does not demonstrate that the NEWOS executable builds correctly or runs in QEMU; cross-build, package installation, and QEMU runtime evidence must be checked separately.
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

The resulting executable is `build/voxel.elf`. A previous target CI command returned success but its temporary link wrapper omitted NEWOS's `crt0.o`, so the linker reported a missing `_start`; that artifact is not considered a valid executable. The current target workflow uses NEWOS's official wrapper instead. See the evidence section below for the QEMU and framebuffer results.

### Optional TinyGL demo

Initialize the pinned submodule and build the regular NEWOS demo/toolchain prerequisites:

```sh
git submodule update --init --recursive
make NEWOS_ROOT=/path/to/NEWOS TINYGL_ROOT=vendor/tinygl tinygl
```

The target sets `NEWOS_FPU=1` for the floating-point TinyGL build and uses a custom 32-bit render configuration. The result is `build/voxel-tinygl.elf`.

### Cross-build and QEMU validation

The manual GitHub Actions workflow `target-validation.yml` checks out a pinned NEWOS revision, builds its real `x86_64-elf` toolchain, cross-compiles the regular demo, builds the `.new` package, and runs the optional TinyGL build as a separate step. For QEMU it places that actual package in NEWOS's disposable built-in sample slot and scripts `nsh.rc` to install it with `newpkg` and execute one frame. This changes only the Actions checkout, never the NEWOS repositories. The workflow checks the serial boot, framebuffer report, and VOXEL completion marker, then uploads the binaries, package and image. A green host-core workflow alone does not imply the target workflow passed.

### NEWOS package

With `NEWOS_ROOT` set to a complete NEWOS checkout:

```sh
make NEWOS_ROOT=/path/to/NEWOS package
```

This stages the built `voxel.elf` in NEWOS's userland build directory and calls `tools/newpkg/newpkg-build.py` to create `build/packages/voxel-0.1.0-x86_64.new`. Install on NEWOS with:

```sh
newpkg install voxel-0.1.0-x86_64.new
```

Package construction and QEMU installation are checked by the manual target workflow, not by the host-core job. A QEMU boot that prints the NEWOS kernel marker still does not prove that VOXEL's package installed or that its framebuffer call completed; the target test now checks those separately.

## Evidence and validation status

The pinned TinyGL ABI and NEWOS syscall layouts were inspected directly. The 16-byte allocator alignment covers the scalar and pointer types used in the pinned TinyGL build, and the code statically verifies the block-header alignment invariant. See [`docs/architecture.md`](docs/architecture.md) and [`docs/build-and-validation.md`](docs/build-and-validation.md) for exact CI results and the remaining runtime boundaries. A headless one-frame QEMU run confirms framebuffer calls and completion only; it is not a visual screenshot or an interactive mouse test.

## License

VOXEL's own code is MIT licensed; the optional TinyGL submodule is separately MIT licensed upstream. See [`LICENSE`](LICENSE).
