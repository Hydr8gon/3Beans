# 3Beans
An experimental low-level 3DS emulator.

### Overview
3Beans emulates the 3DS at a low level, which means that it runs the entire OS as if it were on real hardware. It's in
the early stages of playing commercial games, but the home menu still doesn't fully boot. It's currently fully
interpreted and software-rendered, which makes it portable but slow. If the project matures enough, I might try to
incorporate hardware acceleration or high-level elements to achieve reasonable speeds.

### Downloads
3Beans is available for Windows, macOS, and Linux. The latest builds are automatically provided via GitHub Actions,
and can be downloaded from the [releases page](https://github.com/Hydr8gon/3Beans/releases).

### Setup
Currently, 3Beans is only fully compatible with old 3DS systems that have
[boot9strap](https://github.com/SciresM/boot9strap) installed. New 3DS mode has trouble booting the OS, and bootloaders
like [fastboot3DS](https://github.com/derrekr/fastboot3DS) are completely broken. If you have a suitable setup, you can
use [GodMode9](https://github.com/d0k3/GodMode9) to dump `boot9.bin`, `boot11.bin`, and `nand.bin`, which are required
for 3Beans to do anything. You might also want to create `sd.img`, which can be any
[FAT-formatted image file](https://kuribo64.net/get.php?id=mRJJ5GggXOPbKUMZ) to serve as an SD card. These files can be
configured in the path settings.

### Usage
Once the necessary files are present, 3Beans should function like the system they were dumped from. If you have
[Luma3DS](https://github.com/LumaTeam/Luma3DS) installed, you can open the chainloader for bare-metal homebrew by
holding the start button on boot. If you start the OS normally, you'll find that the home menu never shows up. This
means that digital titles are inaccessible, but cartridges can still be used if the "Cart Auto-Boot" setting is enabled.
Note that 3Beans requires encrypted cartridge dumps, unlike high-level emulators which typically expect decrypted ones.
GodMode9 can dump both encrypted and decrypted ROMs, so make sure you get the right one.

### Contributing
This is a personal project, and I've decided to not review or accept pull requests for it. If you want to help, you can
test things and report issues or provide feedback. If you can afford it, you can also donate to motivate me and allow me
to spend more time on things like this. Nothing is mandatory, and I appreciate any interest in my projects, even if
you're just a user!

### Building
**Windows:** Install [MSYS2](https://www.msys2.org) and run the command
`pacman -Syu mingw-w64-x86_64-{gcc,pkg-config,wxwidgets-msw,portaudio,jbigkit} make` to get dependencies. Navigate to
the project root directory and run `make -j$(nproc)` to start building.

**macOS/Linux:** On the target system, install [wxWidgets](https://www.wxwidgets.org) and
[PortAudio](https://www.portaudio.com). This can be done with the [Homebrew](https://brew.sh) package manager on macOS,
or a built-in package manager on Linux. Run `make -j$(nproc)` in the project root directory to start building.

### Hardware References
* [GBATEK](https://problemkaputt.de/gbatek.htm) - Incomplete but great reference for the 3DS hardware
* [3DBrew](https://www.3dbrew.org) - Comprehensive wiki covering high- and low-level details
* [Teakra](https://github.com/wwylele/teakra) - The only source for newer aspects of the Teak architecture
* [Corgi3DS](https://github.com/PSI-Rockin/Corgi3DS) - The first LLE 3DS emulator, whose logs helped me debug

### Other Links
* [Hydra's Lair](https://hydr8gon.github.io) - Blog where I may or may not write about things
* [Discord Server](https://discord.gg/JbNz7y4) - A place to chat about my projects and stuff
