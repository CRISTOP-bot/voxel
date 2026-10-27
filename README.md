# VOXEL

**VOXEL** is a small, modular software graphics stack and NEWOS framebuffer demonstration. It currently implements CPU-backed XRGB/ARGB surfaces, clipping and alpha compositing, a NEWOS framebuffer presenter, and a classic desktop demo with reusable gray/navy widgets, pixel-art icons, Start menu, taskbar, menus, dialogs, and mouse-driven windows. It lives in its own repository and does not modify NEWOS source repositories.

This is an early graphics foundation—not a desktop environment or a complete graphics API. The public surface/compositor code is kept separate from the NEWOS syscall backend so the core can be tested on a host.

## What is implemented

- Stride-aware software surfaces with pixel, rectangle-fill and clear operations.
- A bounded 16-layer compositor, per-layer visibility, ordering, clipping and straight-alpha blending.
- A NEWOS x86_64 `int $0x80` framebuffer backend using `SYS_FBINFO` (453) and `SYS_FBWRITE` (454). Frames are submitted as XRGB8888 words; NEWOS converts these to the active 24- or 32-bpp panel layout.
- A NEWOS mouse snapshot reader using `SYS_MOUSE_GET` (455), and a sleep helper using `SYS_SLEEP` (35).
- Reusable classic UI primitives in `include/voxel/classic_ui.h` and `src/classic_ui.c`: bitmap text, bevels, square buttons, titlebars, window frames, menu bars and popups, icons, taskbar, Start menu, and dialogs.
- `voxel.elf`, a mouse-driven classic desktop demo with task switching, Settings/About windows, caption controls, and title-bar dragging.
- Optional `voxel-tinygl.elf`, a TinyGL triangle rendering smoke demo. TinyGL is pinned as a submodule at `c2e48591a6bfba1a85f1b87a78dcf1abf7dff57a`; its allocator is a bounded 16 MiB single-threaded arena.
- A host-side core test suite and a NEWOS package spec.

## Not implemented / limitations

- **VOXEL has no keyboard-event input.** Current NEWOS userspace exposes no keyboard-event syscall to the program; the sleep helper is not an input API.
- **No GPU acceleration, window server, or full font rendering.** VOXEL is a CPU software compositor; its tiny 5×7 bitmap font is for this demo, not a complete desktop or general-purpose OpenGL implementation.
- **Target behavior needs target-specific validation.** A host-side core-test pass does not demonstrate that the NEWOS executable builds correctly or runs in QEMU; cross-build, package installation, and QEMU runtime evidence must be checked separately.
- The demo requires a NEWOS boot with a supported Limine framebuffer of at least 640×480 and a working PS/2 mouse for interaction. It is a standalone program, not integrated into NEWOS's shell/session startup.
- The optional TinyGL target cross-compiled successfully and its ELF entry was verified in target run [39196790955](https://github.com/CRISTOP-bot/voxel/actions/runs/39196790955), but TinyGL itself has **not** been run in QEMU. Its build still reports compiler warnings and an RWX LOAD-segment warning.
- The layer array and demo are single-threaded. The allocator is not thread-safe.

The desktop uses a flat teal background, compact gray controls, navy active title bars and raised/sunken bevels—no neon, glass effects, rounded cards or decorative gradients. See [`docs/classic-desktop.md`](docs/classic-desktop.md).

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

The resulting executable is `build/voxel.elf`. Target run [39194826040](https://github.com/CRISTOP-bot/voxel/actions/runs/39194826040) exposed that the link wrapper checked for `crt0.o` but did not pass it to the linker; the ELF had no defined `_start` and faulted in QEMU. This was corrected in [commit `0be80a9`](https://github.com/CRISTOP-bot/voxel/commit/0be80a975154951eaa23b247b714a52390ddc332). In [run 39196790955](https://github.com/CRISTOP-bot/voxel/actions/runs/39196790955), the ELF entry matched a defined `_start`, the package installed, and the one-frame demo completed in QEMU. A non-fatal RWX LOAD-segment linker warning remains.

### Optional TinyGL demo

Initialize the pinned submodule and build the regular NEWOS demo/toolchain prerequisites:

```sh
git submodule update --init --recursive
make NEWOS_ROOT=/path/to/NEWOS TINYGL_ROOT=vendor/tinygl tinygl
```

The target sets `NEWOS_FPU=1` for the floating-point TinyGL build and uses a custom 32-bit render configuration. The result is `build/voxel-tinygl.elf`.

### Cross-build and QEMU validation

The manual GitHub Actions workflow `target-validation.yml` checks out a pinned NEWOS revision, builds its real `x86_64-elf` toolchain, cross-compiles the regular demo, builds the `.new` package, and runs the optional TinyGL build as a separate step. For QEMU it places that actual package in NEWOS's disposable built-in sample slot and scripts `nsh.rc` to install it with `newpkg` and execute one frame. This changes only the Actions checkout, never the NEWOS repositories. The workflow checks the ELF entry point, serial boot, framebuffer report, package installation and VOXEL completion marker, then uploads the binaries, package and image. A green host-core workflow alone does not imply the target workflow passed.

### NEWOS package

With `NEWOS_ROOT` set to a complete NEWOS checkout:

```sh
make NEWOS_ROOT=/path/to/NEWOS package
```

This stages the built `voxel.elf` in NEWOS's userland build directory and calls `tools/newpkg/newpkg-build.py` to create `build/packages/voxel-0.1.0-x86_64.new`. Install on NEWOS with:

```sh
newpkg install voxel-0.1.0-x86_64.new
```

Package construction and QEMU installation are checked by the manual target workflow, not by the host-core job. Run 39194826040 installed the package but exposed the invalid entry-point bug. After correction, run 39196790955 verified the package install and VOXEL completion marker; this is a headless one-frame runtime check, not a visual screenshot or interactive mouse test.

## Evidence and validation status

The pinned TinyGL ABI and NEWOS syscall layouts were inspected directly. The 16-byte allocator alignment covers the scalar and pointer types used in the pinned TinyGL build, and the code statically verifies the block-header alignment invariant. In run 39196790955, the regular compositor demo completed its one-frame QEMU run after reporting the NEWOS framebuffer; that verifies target execution and presentation logic, but not visual appearance or interactive mouse dragging. TinyGL was cross-built and its entry checked, but it was not run. See [`docs/architecture.md`](docs/architecture.md) and [`docs/build-and-validation.md`](docs/build-and-validation.md) for exact results and remaining limits.

## License

VOXEL's own code is MIT licensed; the optional TinyGL submodule is separately MIT licensed upstream. See [`LICENSE`](LICENSE).
