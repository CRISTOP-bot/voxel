#!/usr/bin/env python3
"""Lightweight source contract for environments without a host C compiler."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "include/voxel/classic_ui.h").read_text()
source = (root / "src/theme/classic.c").read_text()
demo = (root / "demo/voxel_demo.c").read_text()
makefile = (root / "Makefile").read_text()

for api in (
    "voxel_ui_bevel", "voxel_ui_button", "voxel_ui_titlebar",
    "voxel_ui_window_frame", "voxel_ui_menu_bar", "voxel_ui_icon",
    "voxel_ui_taskbar", "voxel_ui_start_menu", "voxel_ui_menu",
    "voxel_ui_dialog",
):
    assert api in header, f"missing public API: {api}"
    assert api in source, f"missing implementation: {api}"

for interaction in ("pointer_click", "dragging", "maximized", "minimized",
                    "file_menu_open", "about_dialog"):
    assert interaction in demo, f"missing desktop interaction state: {interaction}"
assert "src/theme/classic.c" in makefile, "classic theme implementation is not in the portable core build"
print("VOXEL classic UI source contract: PASS")

platform_syscall = (root / "src/platform/newos/syscall.c").read_text()
assert 'int $0x80' in platform_syscall
assert 'voxel_newos_syscall1' in (root / "src/platform/newos/framebuffer.c").read_text()
assert 'voxel_newos_syscall1' in (root / "src/platform/newos/input.c").read_text()
