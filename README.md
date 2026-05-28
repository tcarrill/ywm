# ywm — Wayland Window Manager

A bare-bones Wayland compositor written in C using wlroots 0.17.

## Features (milestone 1)

- Server-side window decorations: title bar, four borders
- Drag windows by the title bar
- Close button with hover highlight
- Double-click title bar to shade/unshade (collapses to title bar only)
- Centred window title rendered via Cairo
- Focused window: light grey — unfocused: darker grey
- `Alt+Escape` to quit

---

## Directory structure

```
ywm/
├── meson.build
├── README.md
├── protocols/          ← empty; for future custom protocol XML
├── include/
│   ├── server.h        ← ywm_server, cursor mode, public server API
│   ├── view.h          ← ywm_view, ywm_decoration, colour constants
│   ├── output.h        ← ywm_output (per monitor)
│   └── keyboard.h      ← ywm_keyboard (per device)
└── src/
    ├── main.c          ← entry point, -s <cmd> flag
    ├── server.c        ← all wl_listener callbacks, input, focus, init/run
    └── view.c          ← decoration layout, Cairo title, shade/unshade
```

---

## Cleaning up the wlroots source build (if you built from source)

If you followed earlier instructions and built wlroots 0.17.4 from source
into `/usr/local`, you can remove it cleanly before switching back to the
Ubuntu package:

```bash
# 1. Go to your wlroots source directory
cd ~/wlroots

# 2. Uninstall everything ninja installed into /usr/local
sudo ninja -C build uninstall

# 3. Update the dynamic linker cache
sudo ldconfig

# 4. Optionally remove the source tree entirely
cd ~
rm -rf ~/wlroots

# 5. Verify /usr/local/lib has no leftover wlroots files
ls /usr/local/lib/aarch64-linux-gnu/libwlroots* 2>/dev/null || echo "clean"
```

---

## Installing the correct wlroots (Ubuntu package)

ywm is written against the **Ubuntu 24.04 wlroots 0.17.1** API, so the
system package works perfectly with no workarounds needed.

```bash
sudo apt update
sudo apt install -y \
    libwlroots-dev \
    libwayland-dev \
    libxkbcommon-dev \
    libcairo2-dev \
    libdrm-dev \
    libinput-dev \
    libgbm-dev \
    meson \
    ninja-build \
    pkg-config \
    wayland-protocols \
    wayland-scanner
```

Verify:

```bash
pkg-config --modversion wlroots
# should print: 0.17.1
```

---

## Building

```bash
cd ywm
meson setup build
ninja -C build
```

The build generates `xdg-shell-protocol.h` automatically via `wayland-scanner`.

### Install (optional)

```bash
sudo ninja -C build install
# installs to /usr/local/bin/ywm
```

---

## Running

### From a TTY (recommended for real hardware / VM)

Switch to a free TTY first (e.g. `Ctrl+Alt+F2`), log in, then:

```bash
cd ~/ywm
./build/ywm
```

Launch Wayland clients from another TTY or a terminal multiplexer:

```bash
WAYLAND_DISPLAY=wayland-1 foot &
WAYLAND_DISPLAY=wayland-1 weston-terminal &
```

### With a startup command

```bash
./build/ywm -s "foot"
```

### Nested (inside an existing X11 or Wayland session)

wlroots auto-detects the parent display and uses the X11 or Wayland backend:

```bash
./build/ywm -s "foot"
# A window appears containing ywm with foot running inside it
```

No extra flags needed. Great for development and testing.

### Exiting

**Alt+Escape** terminates ywm.

---

## Key bindings

| Shortcut      | Action      |
|---------------|-------------|
| Alt + Escape  | Quit ywm    |

---

## Architecture

```
main()
  └─ server_init()
       ├─ wlr_backend_autocreate()     DRM/KMS, x11, headless, wayland
       ├─ wlr_renderer_autocreate()    GLES2 / Vulkan / pixman
       ├─ wlr_scene_create()           retained-mode scene graph
       ├─ wlr_xdg_shell_create()       app windows via xdg-shell protocol
       ├─ wlr_xdg_decoration_manager   forces server-side decorations
       ├─ wlr_cursor + xcursor_manager pointer + cursor images
       └─ wlr_seat                     keyboard + pointer focus

  server_run()  →  wl_display_run()   Wayland event loop

  Per window (ywm_view):
    scene_tree (wlr_scene_tree)
      ├─ deco.border_top      wlr_scene_rect
      ├─ deco.titlebar        wlr_scene_rect
      ├─ deco.title_buf       wlr_scene_buffer (Cairo pixels)
      ├─ deco.close_btn       wlr_scene_rect
      ├─ deco.border_left     wlr_scene_rect
      ├─ deco.border_right    wlr_scene_rect
      ├─ deco.border_bottom   wlr_scene_rect
      └─ surface_tree         wlr_scene_xdg_surface (client pixels)
```

---

## Planned milestones

| # | Feature |
|---|---------|
| 1 | ✅ SSD decorations, move, close, shade/unshade |
| 2 | Resize handles (edge + corner dragging) |
| 3 | Multi-monitor support & DPI awareness |
| 4 | Workspace / virtual desktop management |
| 5 | Alt+Tab focus cycling |
| 6 | Config file (colours, keybinds, gaps) |
| 7 | Panel/taskbar via layer-shell protocol |
| 8 | XWayland (legacy X11 app support) |
| 9 | Animations |
| 10 | IPC socket for scripting |
