# 3Beans
A low-level 3DS emulator (maybe).

### Overview
3Beans is an experimental low-level 3DS emulator. It's a while away from booting the home menu or playing games, but it
can run custom FIRMs like [GodMode9](https://github.com/d0k3/GodMode9) using a NAND hacked with
[boot9strap](https://github.com/SciresM/boot9strap). If it ever matures enough, I might experiment with writing a JIT or
incorporating HLE elements to achieve playable speeds.

### Usage
Files dumped from a hacked 3DS are required for 3Beans to boot anything. At a minimum you need `boot9.bin`,
`boot11.bin`, and `nand.bin`, all of which can be obtained using GodMode9. You likely also want `sd.img`, which can be
any FAT-formatted image file to serve as an SD card. These files can be configured in the path settings.

### Contributing
This is a personal project, and I've decided to not review or accept pull requests for it. If you want to help, you can
test things and report issues or provide feedback. If you can afford it, you can also donate to motivate me and allow me
to spend more time on things like this. Nothing is mandatory, and I appreciate any interest in my projects, even if
you're just a user!

### Building
**Windows:** Install [MSYS2](https://www.msys2.org) and run the command
`pacman -Syu mingw-w64-x86_64-{gcc,pkg-config,wxWidgets,portaudio,jbigkit} make` to get dependencies. Navigate to the
project root directory and run `make -j$(nproc)` to start building.

**macOS/Linux:** On the target system, install [wxWidgets](https://www.wxwidgets.org) and
[PortAudio](https://www.portaudio.com). This can be done with the [Homebrew](https://brew.sh) package manager on macOS,
or a built-in package manager on Linux. Run `make -j$(nproc)` in the project root directory to start building.

### Hardware References
* [GBATEK](https://problemkaputt.de/gbatek.htm) - Incomplete but great reference for the 3DS hardware
* [3DBrew](https://www.3dbrew.org) - Comprehensive wiki covering high- and low-level details
* [Teakra](https://github.com/wwylele/teakra) - The only source for newer aspects of the Teak architecture

### Other Links
* [Hydra's Lair](https://hydr8gon.github.io) - Blog where I may or may not write about things
* [Discord Server](https://discord.gg/JbNz7y4) - A place to chat about my projects and stuff
