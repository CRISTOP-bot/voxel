# Classic desktop widgets and demo

VOXEL now includes a reusable, CPU-rendered Win95/98-inspired widget layer.
It is intentionally built on the project's existing `voxel_surface` and
compositor APIs. It adds no kernel calls, device interfaces, font packages,
window protocol, or claims of keyboard support.

## Components

- `include/voxel/classic_ui.h`, `src/classic_ui.c`: clipped 5×7 bitmap text,
  neutral gray/navy color constants, raised/sunken bevels, square buttons,
  active/inactive title bars, window frames, compact menu bars and popup menus,
  three pixel-art icons, Start menu, taskbar, and a dialog helper.
- `demo/voxel_demo.c`: teal desktop with desktop shortcuts, My Computer,
  Settings and About windows, File/View/Help menus, task buttons, Start menu,
  modal About dialog, draggable title bars, and close/minimize/maximize controls.
- `tests/test_core.c`: pixel assertions for raised/sunken button edges, navy
  titlebar, menu layout and clipping at the surface edge.

The 3D bevels use flat pixels (white/light edge and medium/dark edge), not
shadows, gradient fills, transparency effects or rounded corners. Surface
buffers are caller-owned as before; VOXEL remains a single-process demo, not a
persistent window manager. The Settings window is illustrative and does not
change kernel settings. The interface is mouse-driven because NEWOS currently
exposes no keyboard-event syscall to userspace.

## Controls

- Click **Start** to open the classic menu. Programs/Documents opens My Computer;
  Settings opens Display Properties; About VOXEL opens an information dialog.
- Click a taskbar button to activate its window; click the active button again
  to minimize it.
- Drag a titlebar to move a window. The caption controls minimize, maximize /
  restore and close. The File menu includes Open, Close and About VOXEL.
- Click desktop shortcuts to open their corresponding demo window.

The existing NEWOS framebuffer and mouse syscalls remain the only target
backend. This code still needs actual cross-build and QEMU visual/interactive
validation before any claims about appearance or runtime on the target.
