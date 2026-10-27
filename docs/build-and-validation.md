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

`make test` runs a source contract and compiles `src/surface.c`, `src/compositor.c`, `src/classic_ui.c`, and `tests/test_core.c` with the host compiler. It checks the portable core only; it does not execute NEWOS syscalls or either demo. `demo`, `package`, and `tinygl` are separate Make targets. The TinyGL build opts into FPU support (`NEWOS_FPU=1`) and links the pinned TinyGL source with VOXEL's bounded allocator.

## Functional limitations and validation scope

- VOXEL has no keyboard-event input: current NEWOS userspace exposes no keyboard-event syscall to it. Polling the mouse snapshot and sleeping are not substitutes for keyboard events.
- VOXEL has no GPU acceleration, window server, or font/text rendering; it is a standalone, CPU-rendered demonstration rather than a complete desktop environment.
- Passing the host-side core tests does **not** demonstrate that a NEWOS target executable builds correctly, installs, or runs in QEMU. Those require target-specific cross-compilation, package/install validation, and QEMU runtime evidence.

## CI workflows

- **Core tests:** [run 39194820404](https://github.com/CRISTOP-bot/voxel/actions/runs/39194820404) passed on commit `bae9d00de341130b63daf800f716c4b480474f3`; [run 39191482997](https://github.com/CRISTOP-bot/voxel/actions/runs/39191482997) also passed on `de7040047b61560803a052df79d2ed915af9dfb2`. These results cover only the portable host-core tests.
- **First target attempt:** [run 39191482210](https://github.com/CRISTOP-bot/voxel/actions/runs/39191482210) built NEWOS and booted its Limine image in QEMU. NEWOS reported a `1280x800`, 32-bpp framebuffer (pitch 5120) and passed its 7 kernel and 9 phase-2 self-tests, but the shell could not find `/bin/voxel`; this attempt did not run VOXEL.
- **Entry-point failure:** [run 39194826040](https://github.com/CRISTOP-bot/voxel/actions/runs/39194826040) installed the package but launched an ELF without `_start`; QEMU faulted at `0x23` (`RIP=0x400985`) before the demo was ready. Inspection showed entry `0x400000` landed in `draw_window`; the workflow had checked for `crt0.o` without linking it. [Its artifact](https://github.com/CRISTOP-bot/voxel/actions/runs/39194826040/artifacts/12160964666) contains the logs and ELFs.
- **Corrected target validation:** [run 39196790955](https://github.com/CRISTOP-bot/voxel/actions/runs/39196790955) passed on commit `0be80a975154951eaa23b247b714a52390ddc332`, against NEWOS `7cb7d01a2798224c9e850868b35e60b4943425ba`. It confirmed the regular ELF entry `0x400000` matches a defined `_start`, built and installed `voxel 0.1.0`, booted NEWOS with the `1280x800`, pitch-5120, 32-bpp framebuffer, and passed the NEWOS 7-test and 9-test suites. The serial log contains both `VOXEL: software compositor ready` and `VOXEL: demo completed successfully`, with no page-fault marker. [The uploaded artifact](https://github.com/CRISTOP-bot/voxel/actions/runs/39196790955/artifacts/12162401876) includes the ELF files, package, ISO, and logs.
- The optional TinyGL build in run 39196790955 also passed its ELF-entry check, but TinyGL was **not executed in QEMU**. Its build logged 111 compiler warnings (mainly unused variables/parameters and implicit fallthrough) and an RWX LOAD-segment linker warning. The regular VOXEL ELF and NEWOS userland image also emitted RWX warnings; these did not fail the entry, install, boot, or one-frame demo checks, but remain noted.
- Earlier TinyGL attempt [39190307402](https://github.com/CRISTOP-bot/voxel/actions/runs/39190307402) exposed the SDK's missing `max_align_t`; `src/tinygl_allocator.c` now uses explicit 16-byte x86_64 alignment. The successful regular demo run does not establish TinyGL runtime behavior.

The target workflow now passes NEWOS's `crt0.o` to the real cross-link and verifies each ELF entry against a defined `_start`. It stages the actual `.new` package only in the disposable NEWOS test image; no NEWOS repository is modified. The one-frame, headless QEMU result verifies target execution and framebuffer presentation code, not visual appearance or interactive mouse dragging.

## Evidence boundaries

1. **Inspected:** API, ABI, source and build definitions reviewed. This is not a build result.
2. **Compiled:** name the exact target and compiler; a zero exit status is not enough when the linker reports a missing entry point.
3. **Unit-tested:** report the exact test command and its result.
4. **Boot-tested:** report the actual image, bootloader and serial evidence.
5. **Application runtime:** require the package-install and VOXEL completion markers from the QEMU serial log. A kernel boot alone does not count.
6. **Visual/interactive behavior:** the headless one-frame QEMU test does not provide a screenshot or exercise mouse dragging; do not claim those behaviors from its success.

The authoring sandbox lacks a working local `make`, cross-compiler and QEMU. Target build and QEMU evidence therefore comes from the linked GitHub Actions run, not a local execution.

## Classic widget tests

Portable core tests check raised/sunken button pixels, active navy titlebar,
menu-bar output, popup rendering and clipping at the surface boundary. These
host tests do not claim target visual or interactive validation.
