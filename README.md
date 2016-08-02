sh3proxy
========

sh3proxy is a d3d8 wrapper for Silent Hill 3. Similarly to https://github.com/emoose/sh2proxy, it provides some fixes for the game:

* Setting arbitrary video mode without conflict with the game's settings
* Setting arbitrary horizontal FOV (field-of-view) angle
* The engine no longer relies on resource paths in Windows registry
* Silent Hill 2 references can be unlocked without having to complete the game
* Safe mode warnings are disabled
* Fixes white border around game window on Windows
* Disabling Depth of Field effect
* Disabling black borders during cutscenes
* Setting arbitrary shadow resolution

Installation
-------

Copy d3d8.dll and sh3proxy.ini to Silent Hill 3 root directory. If needed, adjust settings in sh3proxy.ini.

Builds are available on [releases](https://github.com/07151129/sh3proxy/releases) page.

Building
-------

Set path to mingw in `config.mk`. Run `make`.

sh3proxy can also be built to wrap dinput8 instead of d3d8:
````bash
WRAP_DINPUT=1 make # -B if your previous target was different
````

Caveats
-------

* Fullscreen mode might not work at some resolutions
* sh3proxy relies on hardcoded memory addresses. This means it might not be compatible with your version of
`sh3.exe`. sh3proxy has only been tested with `sh3.exe` with `372ae792` CRC32 sum
* DEP has to be disabled for `sh3.exe`
