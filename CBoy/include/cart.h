#pragma once

#include <common.h>

typedef struct {
    u8 entry[4];
    u8 logo[0x30];

    char title[16];
    u16 new_lic_code;
    u8 sgb_flag;
    u8 type;
    u8 rom_size;
    u8 ram_size;
    u8 dest_code;
    u8 lic_code;
    u8 version;
    u8 checksum;
    u16 global_checksum;
} rom_header;

typedef enum {
    GAME_TETRIS,
    GAME_SUPER_MARIO_LAND,
    GAME_ZELDA,
    GAME_SF2,
} Game;

bool cart_load(Game game);

u8 cart_read(u16 address);
void cart_write(u16 address, u8 value);

bool cart_need_save();
void cart_battery_save();
void cart_battery_load();
