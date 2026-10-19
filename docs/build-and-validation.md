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

- **Core tests:** [run 39191482997](https://github.com/CRISTOP-bot/voxel/actions/runs/39191482997) passed on commit `de7040047b61560803a052df79d2ed915af9dfb2`. This is evidence for the portable host-core tests only.
- **Target validation:** [run 39191482210](https://github.com/CRISTOP-bot/voxel/actions/runs/39191482210) on the same commit built NEWOS with its `x86_64-elf` toolchain and produced a bootable Limine image. The serial log confirms NEWOS booted, reported a `1280x800`, 32-bpp framebuffer (pitch 5120), and passed its 7 kernel and 9 phase-2 self-tests.
- That target run **did not run VOXEL successfully**: its QEMU shell reported `/bin/voxel: command not found`, because adding a program to NEWOS's build list did not install it in the initramfs. The run therefore does not prove VOXEL's framebuffer behavior.
- The same run's regular/TinyGL link commands returned exit status 0, but both ELFs emitted `cannot find entry symbol _start`; the temporary validation wrapper had omitted NEWOS's `crt0.o`. Those outputs are **not accepted as valid executables**, and the generated package was not installed or run.
- An earlier TinyGL attempt, [run 39190307402](https://github.com/CRISTOP-bot/voxel/actions/runs/39190307402), exposed the missing `max_align_t` declaration in the NEWOS SDK. `src/tinygl_allocator.c` now uses explicit 16-byte x86_64 alignment; that change compiled in the later target run.

The revised `target-validation.yml` uses NEWOS's official `toolchain/newos-gcc` wrapper (including `crt0.o`), builds the regular demo and package separately, stages the actual `.new` package in a disposable NEWOS test image, and runs `newpkg install` followed by a one-frame VOXEL launch in QEMU. The optional TinyGL build is a separate step after the QEMU check. These workflow changes do not edit or push any NEWOS repository. Run [39194826040](https://github.com/CRISTOP-bot/voxel/actions/runs/39194826040) was started on commit `bae9d00de341130b63daf800f716c4b480474f3f`; its final QEMU and TinyGL outcomes still need to be inspected in the step logs before being reported as verified.

## Evidence boundaries

1. **Inspected:** API, ABI, source and build definitions reviewed. This is not a build result.
2. **Compiled:** name the exact target and compiler; a zero exit status is not enough when the linker reports a missing entry point.
3. **Unit-tested:** report the exact test command and its result.
4. **Boot-tested:** report the actual image, bootloader and serial evidence.
5. **Application runtime:** require the package-install and VOXEL completion markers from the QEMU serial log. A kernel boot alone does not count.
6. **Visual/interactive behavior:** the headless one-frame QEMU test does not provide a screenshot or exercise mouse dragging; do not claim those behaviors from its success.

The authoring sandbox lacks a working local `make`, cross-compiler and QEMU. Target build and QEMU evidence therefore comes from the linked GitHub Actions run, not a local execution.
