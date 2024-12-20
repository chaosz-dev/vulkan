# VKCourse

To build:
```sh
$ cmake -Bbuild -H.
$ make -C build/ -j4
```

To run `01_window`:
```sh
$ ./build/bin/01_window
```

# Required packages

Linux (ubuntu package names):
* vulkan-validationlayers
* libvulkan-dev
* libvulkan1
* glslang-tools
* cmake

* (recommended): libglfw3-dev
* If libglfw3-dev is not installed then additionally: libxkbcommon-dev xorg-dev

Windows:
* Vulkan SDK (eg.: https://sdk.lunarg.com/sdk/download/1.3.296.0/windows/VulkanSDK-1.3.296.0-Installer.exe )
* Visual Studio or some kind of compiler
* cmake
