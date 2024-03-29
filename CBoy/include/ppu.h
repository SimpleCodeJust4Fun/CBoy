#pragma once

#include <common.h>

static const int LINES_PER_FRAME = 154;
static const int TICKS_PER_LINE = 456; //tick means dot (pandocs)
static const int YRES = 144; //resolution
static const int XRES = 160;

typedef enum {
    FS_TILE,
    FS_DATA0,
    FS_DATA1,
    FS_IDLE, //sleep state
    FS_PUSH
} fetch_state;

typedef struct _fifo_entry {
    struct _fifo_entry *next;
    u32 value;
} fifo_entry;

typedef struct {
    fifo_entry *head;
    fifo_entry *tail;
    u32 size;
} fifo;

typedef struct {
    fetch_state cur_fetch_state;
    fifo pixel_fifo;
    u8 line_x;
    u8 pushed_x;
    u8 fetch_x; //cur fetching
    u8 bgw_fetch_data[3]; //different temp tiles fetching
    u8 fetch_entry_data[6]; //temp oam data fetching
    u8 map_y;
    u8 map_x;
    u8 tile_y;
    u8 fifo_x;
} pixel_fifo_context;

typedef struct {
    u8 y;
    u8 x;
    u8 tile;

    unsigned f_cgb_pn : 3;
    unsigned f_cgb_vram_bank : 1;
    unsigned f_ncgb_pn : 1;
    unsigned f_x_flip : 1;
    unsigned f_y_flip : 1;
    unsigned f_bg_priority : 1;
} oam_entry;

/*
 Bit7   BG and Window over OBJ (0=No, 1=BG and Window colors 1-3 over the OBJ)
 Bit6   Y flip          (0=Normal, 1=Vertically mirrored)
 Bit5   X flip          (0=Normal, 1=Horizontally mirrored)
 Bit4   Palette number  **Non CGB Mode Only** (0=OBP0, 1=OBP1)
 Bit3   Tile VRAM-Bank  **CGB Mode Only**     (0=Bank 0, 1=Bank 1)
 Bit2-0 Palette number  **CGB Mode Only**     (OBP0-7)
 */

typedef struct _oam_line_entry {
    oam_entry entry;
    struct _oam_line_entry *next;
} oam_line_entry;

typedef struct {
    oam_entry oam_ram[40];
    u8 vram[0x2000];

    u8 line_sprite_count; //0 to 10 sprites per line
    oam_line_entry *line_sprites; //linked list of current sprites on line 
    oam_line_entry line_entry_array[10]; //allocate memory to use for list

    u8 fetched_entry_count;
    oam_entry fetched_entries[3]; //entries fetched during pipeline
    u8 window_line; //current window line

    pixel_fifo_context pfc;

    u32 current_frame;
    u32 line_ticks; //how many ticks (dots) there are in cur frame
    u32 *video_buffer;
} ppu_context;

void ppu_init();
void ppu_tick();

void ppu_oam_write(u16 address, u8 value);
u8 ppu_oam_read(u16 address);

void ppu_vram_write(u16 address, u8 value);
u8 ppu_vram_read(u16 address);

ppu_context *ppu_get_context();

void pipeline_process();
void pipeline_fifo_reset();

bool window_visible();
