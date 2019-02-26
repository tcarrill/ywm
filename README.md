# ywm
Yutani Window Manager

Initially meant to be a Wayland compositor, hence the name "Yutani" (Weyland-Yutani from the Alien film series). Direction since shifted to traditional Xlib, name stuck because Alien is good and I suck at naming things.

This is under heavy development and will likely be broken at any given time.  I haven't touched Xlib in over a decade and this, more than anything, is just a learning exercise for myself.

![ywm](screenshot.png?raw=true)

## Usage
Right mouse button anywhere but a window will show the root menu.

## Root Menu
The root menu is controlled by the ~/.ywm/ywm.menu.  This is a pipe (|) delimited file in the form of "label|command".  The default file is shown below

```
Xterm|xterm
Xclock|xclock
Xlogo|xlogo
Xeyes|xeyes
Xcalc|xcalc
Xman|xman
Exit ywm|killall ywm
```
