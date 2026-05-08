Emulator for https://mapache64.ucsbieee.org/

While I have had moderate success with MinGW or Visual Studio, you will probably have the best time compiling this in a Unix, so if you are on Windows, you should consider installing WSL, which there are instructions for here: https://learn.microsoft.com/en-us/windows/wsl/install

The required packages can be installed in Ubuntu with the following commands:
```
sudo apt update
sudo apt install build-essential libasound2-dev mesa-common-dev libx11-dev libxrandr-dev libxi-dev xorg-dev libgl1-mesa-dev libglu1-mesa-dev cmake
```
Equivalents for any other system should be easy to find

Configure with `cmake .`, compile with `cmake --build .`

Runs with 64k memory dump as argument (`mapache64.bin` in the dump folder if building from the template)


Arrow keys correspond to dpad, z is A, x is B, enter is START, and tab is SELECT

Assembly monitor is opened with escape or upon encountering an STP instruction
