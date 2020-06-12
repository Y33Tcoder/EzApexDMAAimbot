# EzApexDMAAimbot


EzApexDMAAimbot
Simple Apex Legends Aimbot with Glow test project using KVM DMA module

Credits:

- 90% aimbot code by TheCruz ported to use with KVM DMA https://www.unknowncheats.me/forum/apex-legends/369786-apex-directx-wallhack-smooth-aimbot-source.html

- vmread KVM DMA module by h33p https://github.com/h33p/vmread

Added Features:

- Offsets updated to Apex Legends season 5

- Aimbot has added recoil randomization,non-linear smoothing and target bone randomization to avoid possible serverside detections (Violation-CA thingy)

- By default an additional keyboard is required to be plugged into the linux host as the aimbot activation button is bound to SPACEBAR on linux host, activation using buttons in windows guest is possible, but require updating code and offset in example.cpp to point ADS-ing offset to be read in cheat (note! not sure if this is a detection vector)

- Glow by default has team color differentiation.

How to install:

- install meson and ninja on your linux host
- run "meson builddir" inside vmread folder
- run "ninja" command in builddir to build example.cpp
- run "./example"