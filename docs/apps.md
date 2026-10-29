# VOXEL applications: implementation status

The application modules are hosted in-process by the VOXEL demonstration. They are not independent binaries or isolated processes.

| Module | Implemented | Limits |
|---|---|---|
| About | Reports VOXEL identity and implementation scope. | Static view. |
| Explorer | Reads real directory entries, enters directories, and goes to parent. | Eight rows; files are not opened automatically by the desktop yet. |
| Settings | Toggles desktop and window palette in memory. | Session-only; no config persistence. |
| Run | On-screen mouse keyboard enters an executable path and launches via NEWOS `SPAWN2`. | Path only; no arguments or child-output capture. |
| Console | On-screen keyboard, bounded history, and one absolute executable per command; output is collected through NEWOS pipe + `SPAWN3`. | Not a shell; no command parser, pipelines, or arguments. |
| Hello World | Renders a small app view. | In-process demo. |
| Editor | Bounded text buffer, on-screen key input and NEWOS read/write callbacks. | Fixed-size buffer; Save creates `/notes.txt` initially; Open reads current path or `/etc/version`. |
| Image Viewer | Loads and renders BMP via a bounded decoder. | Strict uncompressed 24-bit BMP, maximum decoded storage 64×48. |

The pinned NEWOS ABI verified for VOXEL has framebuffer, mouse, directory/file, and spawn syscalls, but no GUI keyboard read/grab or console-ownership calls. Text entry therefore uses a mouse-operated virtual keyboard. The app registry is built into the VOXEL process; separate `.new` packages, a real terminal session/window service, process output redirection, and desktop session integration remain unimplemented.
