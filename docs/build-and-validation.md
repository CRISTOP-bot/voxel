# Build and validation

## Local commands

```sh
# Portable surface/compositor unit tests (host compiler)
make test

# Regular NEWOS demo (complete NEWOS checkout, real x86_64-elf toolchain and SDK)
make NEWOS_ROOT=/path/to/NEWOS demo

# Optional TinyGL demo (pinned vendor/tinygl submodule initialized)
make NEWOS_ROOT=/path/to/NEWOS TINYGL_ROOT=vendor/tinygl tinygl

# Build the regular demo's NEWOS .new package
make NEWOS_ROOT=/path/to/NEWOS package
```

`make test` compiles `src/surface.c`, `src/compositor.c`, and `tests/test_core.c` with the host compiler. It checks the portable core only; it does not execute NEWOS syscalls or either demo. `demo`, `package`, and `tinygl` are separate Make targets. The TinyGL build opts into FPU support (`NEWOS_FPU=1`) and links the pinned TinyGL source with VOXEL's bounded allocator.

## Functional limitations and validation scope

- VOXEL has no keyboard-event input: current NEWOS userspace exposes no keyboard-event syscall to it. Polling the mouse snapshot and sleeping are not substitutes for keyboard events.
- VOXEL has no GPU acceleration, window server, or font/text rendering; it is a standalone, CPU-rendered demonstration rather than a complete desktop environment.
- Passing the host-side core tests does **not** demonstrate that a NEWOS target executable builds correctly, installs, or runs in QEMU. Those require target-specific cross-compilation, package/install validation, and QEMU runtime evidence.

## CI workflows

- **Core tests:** [run 39194820404](https://github.com/CRISTOP-bot/voxel/actions/runs/39194820404) passed on commit `bae9d00de341130b63daf800f716c4b480474f3`; [run 39191482997](https://github.com/CRISTOP-bot/voxel/actions/runs/39191482997) also passed on `de7040047b61560803a052df79d2ed915af9dfb2`. These results cover only the portable host-core tests.
- **First target attempt:** [run 39191482210](https://github.com/CRISTOP-bot/voxel/actions/runs/39191482210) built NEWOS and booted its Limine image in QEMU. NEWOS reported a `1280x800`, 32-bpp framebuffer (pitch 5120) and passed its 7 kernel and 9 phase-2 self-tests, but the shell could not find `/bin/voxel`; this attempt did not run VOXEL.
- **Package-injection target attempt:** [run 39194826040](https://github.com/CRISTOP-bot/voxel/actions/runs/39194826040) booted NEWOS, reported the same framebuffer, passed both self-test suites, and verified and installed `voxel 0.1.0`. It still **did not run the demo successfully**: the process faulted at address `0x23`, with `RIP=0x400985`, before either VOXEL readiness or completion marker appeared. [The uploaded artifact](https://github.com/CRISTOP-bot/voxel/actions/runs/39194826040/artifacts/12160964666) contains the serial and build logs plus both ELFs.
- Inspection of that run's regular `voxel.elf` found no defined `_start`; its entry address `0x400000` landed in `draw_window`, not startup code. The temporary wrapper checked for `crt0.o` but omitted it from the linker inputs. The bad entry passed the process argument count (`3`) as a surface pointer, explaining the fault in `valid_surface`. The optional TinyGL step likewise returned success while warning that `_start` was missing, and also reported an RWX load segment. Neither ELF is accepted as a valid runnable target.
- Earlier TinyGL attempt [39190307402](https://github.com/CRISTOP-bot/voxel/actions/runs/39190307402) exposed the SDK's missing `max_align_t`; `src/tinygl_allocator.c` now uses explicit 16-byte x86_64 alignment. This does not establish TinyGL runtime success.

The latest workflow revision explicitly passes NEWOS's `crt0.o` to the real cross-link and checks that each ELF's entry address resolves to a defined `_start` before packaging/running it. The QEMU sample install still uses the actual `.new` package in the disposable NEWOS image; no NEWOS repository is modified. The corrected entry-point check and QEMU run remain pending, so do not claim that the compositor or TinyGL works on NEWOS yet.

## Evidence boundaries

1. **Inspected:** API, ABI, source and build definitions reviewed. This is not a build result.
2. **Compiled:** name the exact target and compiler; a zero exit status is not enough when the linker reports a missing entry point.
3. **Unit-tested:** report the exact test command and its result.
4. **Boot-tested:** report the actual image, bootloader and serial evidence.
5. **Application runtime:** require the package-install and VOXEL completion markers from the QEMU serial log. A kernel boot alone does not count.
6. **Visual/interactive behavior:** the headless one-frame QEMU test does not provide a screenshot or exercise mouse dragging; do not claim those behaviors from its success.

The authoring sandbox lacks a working local `make`, cross-compiler and QEMU. Target build and QEMU evidence therefore comes from the linked GitHub Actions run, not a local execution.
