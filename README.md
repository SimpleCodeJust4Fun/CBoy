# How to use on macOS
Deploy Env
- Machine: MacBook Air m1
- OS: macOS 14.2
- Compiler: Apple clang version 15.0.0 (clang-1500.1.0.2.5) from Apple's default CommandLineTools
- Other requirements: Homebrew 4.2.0
  
setup:
1. clone the repo to your local directory 
```bash
git clone https://github.com/SimpleCodeJust4Fun/cBoy.git
cd LLD_GEBEMU
cd part16
```
2. use cmake to construct the project
```bash
mkdir build
cd build
cmake ..
make
```
install the essesntial libraries with homebrew if `cmake` or `make` is not successful:
```bash
brew install sdl2
brew install sdl2_ttf
brew install check
```
remember to delete the cmake cache before you start `cmake` or `make` again
```bash
rm -rf *
```
3. run the emulator
```bash
cd gbemu
./gbemu <rom_file>
```
**key modification that enable success build on mac**
- add `include_directories("/opt/homebrew/include")` in CMakeLists.txt
  
**explaination**:
- the dependencies sdl2, sdl2_ttf and check are installed with homebrew, the original repo was desined to run on Linux and Windows therefore did not add homebrew include path in CMakeLists.txt
# Appendix

Important References:

https://gbdev.io/pandocs/

https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html

https://archive.org/details/GameBoyProgManVer1.1/page/n85/mode/2up

https://github.com/rockytriton/LLD_gbemu/raw/main/docs/The%20Cycle-Accurate%20Game%20Boy%20Docs.pdf

https://github.com/rockytriton/LLD_gbemu/raw/main/docs/gbctr.pdf


NOTE: Designed to run on Linux, but you can build on Windows with MSYS2 and mingw-w64

Windows Environment Setup:

1. Install MSYS2:  https://www.msys2.org/

2. Follow instructions 1 through 7 on the MSYS2 page.

3. pacman -S cmake

4. pacman -S mingw64/mingw-w64-x86_64-SDL2 mingw64/mingw-w64-x86_64-SDL2_mixer mingw64/mingw-w64-x86_64-SDL2_image mingw64/mingw-w64-x86_64-SDL2_ttf mingw64/mingw-w64-x86_64-SDL2_net

5. pacman -S mingw-w64-x86_64-check

After above steps you should be able to build from Windows using MSYS2 just like in the videos.


