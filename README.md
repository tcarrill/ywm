###ywm
Ywm is a stacking window manager for X11.  Its UI takes inspiration from Apple's Rhapsody operating system, which grew from NeXTSTEP and would eventually become macOS. 

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
