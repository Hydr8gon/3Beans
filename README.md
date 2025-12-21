# 3Beans
A low-level 3DS emulator.

### Overview
3Beans emulates the 3DS at a low level, which means that it runs the entire OS as if it were on real hardware. It can
boot the home menu and launch some games, but it's still young and has plenty of issues. It has both software and
hardware GPU rendering, but CPUs are still fully interpreted. My goal for this project is to achieve viable speeds with
full low-level emulation, and maybe explore some high-level elements down the road.

### Downloads
3Beans is available for Windows, macOS, and Linux. The latest builds are automatically provided via GitHub Actions,
and can be downloaded from the [releases page](https://github.com/Hydr8gon/3Beans/releases).

### Setup
To function, 3Beans requires files dumped from an old or new 3DS system. Old 3DS is preferable for the best performance
and stability, but either console with homebrew access will work. At minimum, you'll need `boot9.bin`, `boot11.bin`, and
`nand.bin`, all of which can be obtained using [GodMode9](https://github.com/d0k3/GodMode9). You might also want to
create `sd.img`, which can be any [FAT-formatted image file](https://kuribo64.net/get.php?id=mRJJ5GggXOPbKUMZ) to serve
as an SD card. These files can be configured in the path settings.

### Usage
Once the necessary files are present, 3Beans should function like the system they were dumped from. Controls can be
viewed and changed in the Settings menu. You can boot without a cartridge through the System menu, or select one through
the File menu. If you want to skip the home menu and load directly into a game, you can use the Cart Auto-Boot setting.
Note that 3Beans requires encrypted cartridge dumps, unlike high-level emulators which typically expect decrypted ones.
GodMode9 can dump both encrypted and decrypted ROMs, so make sure you get the right one.

### Contributing
This is a personal project, and I've decided to not review or accept pull requests for it. If you want to help, you can
test things and report issues or provide feedback. If you can afford it, you can also donate to motivate me and allow me
to spend more time on things like this. Nothing is mandatory, and I appreciate any interest in my projects, even if
you're just a user!

### Building
**Windows:** Install [MSYS2](https://www.msys2.org) and run the command
`pacman -Syu mingw-w64-x86_64-{gcc,pkg-config,wxwidgets-msw,portaudio,libepoxy,jbigkit} make` to get dependencies.
Navigate to the project root directory and run `make -j$(nproc)` to start building.

**macOS/Linux:** On the target system, install [wxWidgets](https://www.wxwidgets.org),
[PortAudio](https://www.portaudio.com), and [Epoxy](https://github.com/anholt/libepoxy). This can be done with a package
manager like [Homebrew](https://brew.sh) on macOS, or a built-in one on Linux. Run `make` in the project root directory
to start building.

**Notice:** Current wxWidgets releases might not have features that 3Beans requires. As a temporary workaround, you can
install a custom build by running the following commands:
```
git clone --recursive https://github.com/wxWidgets/wxWidgets.git
cd wxWidgets
git reset --hard 1fd12d7bd6987603b8d63000a593a4286f5cad46
./configure --disable-sys-libs --disable-shared --disable-tests --without-libcurl
make -j4
sudo make install
```

### Hardware References
* [GBATEK](https://problemkaputt.de/gbatek.htm) - Incomplete but great reference for the 3DS hardware
* [3DBrew](https://www.3dbrew.org) - Comprehensive wiki covering high- and low-level details
* [Teakra](https://github.com/wwylele/teakra) - The only source for newer aspects of the Teak architecture
* [Corgi3DS](https://github.com/PSI-Rockin/Corgi3DS) - The first LLE 3DS emulator, whose logs helped me debug

### Other Links
* [Hydra's Lair](https://hydr8gon.github.io) - Blog where I may or may not write about things
* [Discord Server](https://discord.gg/JbNz7y4) - A place to chat about my projects and stuff
