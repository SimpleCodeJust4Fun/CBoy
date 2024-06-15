dump new game rom:

```
xxd -i your_rom.gb > output.c
```

new build process:
```
cd CBoy
mkdir build
cd build
cmake ..
make
gbemu/gbemu
```

to switch from Emscripten build to C build:
comment first line in CMakeLists.txt:
```
set(CMAKE_TOOLCHAIN_FILE "/Users/liutianyu/Desktop/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
```
this will change the build tool chain to Emscripten's


