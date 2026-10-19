# Build and validation

## Local commands

```sh
# Host-only portable core tests
make test

# NEWOS demo (requires the NEWOS SDK and real cross-toolchain)
make NEWOS_ROOT=/path/to/NEWOS demo

# Optional TinyGL demo (requires initialized vendor/tinygl submodule)
make NEWOS_ROOT=/path/to/NEWOS TINYGL_ROOT=vendor/tinygl tinygl

# Package the regular demo in a NEWOS checkout
make NEWOS_ROOT=/path/to/NEWOS package
```

`make test` uses the host `CC` and compiles `surface.c`, `compositor.c`, and `tests/test_core.c`. A small mock `voxel_framebuffer_blit()` in the test file checks the compositor's presentation bounds and arguments without issuing a syscall. It does not compile inline `int $0x80` backend code or either demo. `demo` and `tinygl` use NEWOS's `toolchain/newos-gcc`; a host compiler is not a substitute for that target ABI. The TinyGL target opts into floating point with `NEWOS_FPU=1`, avoiding the wrapper's default general-register-only restriction.

## CI scope

The GitHub Actions workflow runs the portable host core tests with the compiler provided by Ubuntu. A green run demonstrates that this host test target compiled and its assertions passed in that runner. It does not validate cross-compilation, the NEWOS syscall ABI at runtime, package installation, TinyGL target compatibility, framebuffer output, mouse interaction, or boot/QEMU behavior.

## Evidence vocabulary

When reporting results, distinguish these activities:

1. **Inspected:** source/API/ABI definitions reviewed. Inspection can support a compatibility rationale but does not prove a build.
2. **Compiled:** compiler completed a specified target. Name the actual target and toolchain.
3. **Tests run:** report the exact test command and observed pass/fail result.
4. **Runtime tested:** report the actual NEWOS image, QEMU/hardware environment and interactions observed. Do not infer this from CI or a successful compile.

At initial publication, the verified evidence is source inspection and the CI host-core job's actual result (once observed). No local compiler/Make/QEMU was available in the authoring environment; no local target build or NEWOS runtime test is claimed.
