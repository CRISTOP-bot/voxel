CC ?= cc
NEWOS_ROOT ?= ../NEWOS
NEWOS_GCC ?= $(NEWOS_ROOT)/toolchain/newos-gcc
TINYGL_ROOT ?= vendor/tinygl
BUILD_DIR ?= build

WARNINGS := -Wall -Wextra -Werror
PORT_CFLAGS := -std=gnu11 -O2 -ffreestanding -fno-pic -fno-pie \
               -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables \
               -fno-unwind-tables -mgeneral-regs-only $(WARNINGS) -Iinclude
CORE_SRCS := src/surface.c src/compositor.c src/classic_ui.c
BACKEND_SRCS := src/newos_backend.c src/input.c
VOXEL_DEMO := $(BUILD_DIR)/voxel.elf
TINYGL_DEMO := $(BUILD_DIR)/voxel-tinygl.elf
TINYGL_SRCS := \
  $(TINYGL_ROOT)/src/api.c $(TINYGL_ROOT)/src/arrays.c \
  $(TINYGL_ROOT)/src/clear.c $(TINYGL_ROOT)/src/clip.c \
  $(TINYGL_ROOT)/src/get.c $(TINYGL_ROOT)/src/image_util.c \
  $(TINYGL_ROOT)/src/init.c $(TINYGL_ROOT)/src/light.c \
  $(TINYGL_ROOT)/src/list.c $(TINYGL_ROOT)/src/matrix.c \
  $(TINYGL_ROOT)/src/misc.c \
  $(TINYGL_ROOT)/src/msghandling.c $(TINYGL_ROOT)/src/select.c \
  $(TINYGL_ROOT)/src/specbuf.c $(TINYGL_ROOT)/src/texture.c \
  $(TINYGL_ROOT)/src/vertex.c $(TINYGL_ROOT)/src/zbuffer.c \
  $(TINYGL_ROOT)/src/zline.c $(TINYGL_ROOT)/src/zmath.c \
  $(TINYGL_ROOT)/src/zpostprocess.c $(TINYGL_ROOT)/src/zraster.c \
  $(TINYGL_ROOT)/src/ztext.c $(TINYGL_ROOT)/src/ztriangle.c

.PHONY: all demo tinygl test clean check-newos check-tinygl package test-source
all: demo

demo: $(VOXEL_DEMO)

check-newos:
	@test -x "$(NEWOS_GCC)" || { echo "NEWOS cross compiler not found: $(NEWOS_GCC); build the NEWOS toolchain first" >&2; exit 1; }
	@test -f "$(NEWOS_ROOT)/user/lib/app.ld" || { echo "NEWOS_ROOT is not a complete NEWOS checkout" >&2; exit 1; }

$(VOXEL_DEMO): demo/voxel_demo.c $(CORE_SRCS) $(BACKEND_SRCS) | check-newos
	@mkdir -p $(dir $@)
	"$(NEWOS_GCC)" $(PORT_CFLAGS) -o "$@" $^

check-tinygl: check-newos
	@test -f "$(TINYGL_ROOT)/include/TGL/gl.h" || { echo "TinyGL submodule missing; run git submodule update --init --recursive" >&2; exit 1; }
	@for f in $(TINYGL_SRCS); do test -f "$$f" || { echo "TinyGL source missing: $$f" >&2; exit 1; }; done

tinygl: $(TINYGL_DEMO)

$(TINYGL_DEMO): demo/tinygl_demo.c src/tinygl_allocator.c $(BACKEND_SRCS) $(TINYGL_SRCS) | check-tinygl
	@mkdir -p $(dir $@)
	NEWOS_FPU=1 "$(NEWOS_GCC)" -std=gnu11 -O2 -ffreestanding -fno-pic -fno-pie \
	  -fno-stack-protector -fno-builtin -fno-asynchronous-unwind-tables \
	  -fno-unwind-tables -Wall -Wextra \
	  -DTGL_FEATURE_RENDER_BITS=32 -DNO_DEBUG_OUTPUT \
	  -Iinclude -I"$(TINYGL_ROOT)/include" -I"$(TINYGL_ROOT)/src" \
	  -o "$@" $^ -lm

test: test-source
	@mkdir -p $(BUILD_DIR)/tests
	$(CC) -std=c11 $(WARNINGS) -pedantic -Iinclude \
	  tests/test_core.c $(CORE_SRCS) -o $(BUILD_DIR)/tests/test_core
	$(BUILD_DIR)/tests/test_core

test-source:
	python3 tests/test_classic_ui_source.py

package: demo
	@test -n "$(NEWOS_ROOT)" || { echo "Set NEWOS_ROOT to the NEWOS checkout" >&2; exit 1; }
	@mkdir -p "$(NEWOS_ROOT)/build/userland" "$(NEWOS_ROOT)/build/packages"
	cp "$(VOXEL_DEMO)" "$(NEWOS_ROOT)/build/userland/voxel.elf"
	cd "$(NEWOS_ROOT)" && python3 tools/newpkg/newpkg-build.py \
	  "$(abspath packages/voxel.newspec)" \
	  -o build/packages/voxel-0.1.0-x86_64.new

clean:
	rm -rf $(BUILD_DIR)
