#include <emu.h>
// #include <emscripten.h>

int argc_global;
char **argv_global;

void main_loop() {
    emu_run(argc_global, argv_global);
}

int main(int argc, char **argv) {
    argc_global = argc;
    argv_global = argv;
    // emscripten_set_main_loop(main_loop, 0, 1);
    main_loop();
    return 0;
}
