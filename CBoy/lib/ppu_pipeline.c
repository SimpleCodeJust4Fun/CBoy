#include <ppu.h>
#include <lcd.h>
#include <bus.h>

void pixel_fifo_push(u32 value) {
    //fifo inner util func, standard linked-list push operation
    fifo_entry *next = malloc(sizeof(fifo_entry));
    next->next = NULL;
    next->value = value;

    if (!ppu_get_context()->pfc.pixel_fifo.head) {
        //first entry
        ppu_get_context()->pfc.pixel_fifo.head = ppu_get_context()->pfc.pixel_fifo.tail = next;
    } else {
        ppu_get_context()->pfc.pixel_fifo.tail->next = next;
        ppu_get_context()->pfc.pixel_fifo.tail = next;
    }

    ppu_get_context()->pfc.pixel_fifo.size++;
}

u32 pixel_fifo_pop() {
    //fifo inner util func, standard linked-list pop operation
    if (ppu_get_context()->pfc.pixel_fifo.size <= 0) {
        fprintf(stderr, "ERROR IN PIXEL FIFO POP");
        exit(-8);
    }

    fifo_entry *popped = ppu_get_context()->pfc.pixel_fifo.head;
    ppu_get_context()->pfc.pixel_fifo.head = popped->next;
    ppu_get_context()->pfc.pixel_fifo.size--;

    u32 val = popped->value;
    free(popped);

    return val;
}

bool pipeline_fifo_add() {
    if (ppu_get_context()->pfc.pixel_fifo.size > 8) {
        //fifo is full
        return false;
    }

    int x = ppu_get_context()->pfc.fetch_x - (8 - (lcd_get_context()->scroll_x % 8)); //x=8 is the 1st location visible on the screen

    for (int i = 0; i < 8; i++) {
        int bit = 7 - i;
        u8 low = !!(ppu_get_context()->pfc.bgw_fetch_data[1] & (1 << bit));
        u8 high = !!(ppu_get_context()->pfc.bgw_fetch_data[2] & (1 << bit)) << 1;
        u32 color = lcd_get_context()->bg_colors[low | high];

        if (x >= 0) {
            //push that color onto the fifo
            pixel_fifo_push(color);
            ppu_get_context()->pfc.fifo_x++;
        }
    }

    return true;
}

void pipeline_fetch() {
    switch(ppu_get_context()->pfc.cur_fetch_state) {
        case FS_TILE: {
            //fetch the cur tile for the location we are on the map
            if (LCDC_BGW_ENABLE) {
                ppu_get_context()->pfc.bgw_fetch_data[0] = bus_read(LCDC_BG_MAP_AREA +
                    ppu_get_context()->pfc.map_x / 8 +
                    (ppu_get_context()->pfc.map_y / 8) * 32);
                
                if (LCDC_BGW_DATA_AREA == 0x8800) {
                    ppu_get_context()->pfc.bgw_fetch_data[0] += 128; //increment id for that tile
                }
            }

            //switch to next state
            ppu_get_context()->pfc.cur_fetch_state = FS_DATA0;
            ppu_get_context()->pfc.fetch_x += 8;
        } break;

        case FS_DATA0: {
            ppu_get_context()->pfc.bgw_fetch_data[1] = bus_read(LCDC_BGW_DATA_AREA +
                ppu_get_context()->pfc.bgw_fetch_data[0] * 16 +
                ppu_get_context()->pfc.tile_y);

                ppu_get_context()->pfc.cur_fetch_state = FS_DATA1;
        } break;

        case FS_DATA1: {
            ppu_get_context()->pfc.bgw_fetch_data[2] = bus_read(LCDC_BGW_DATA_AREA +
                ppu_get_context()->pfc.bgw_fetch_data[0] * 16 +
                ppu_get_context()->pfc.tile_y + 1);

                ppu_get_context()->pfc.cur_fetch_state = FS_IDLE;
        } break;

        case FS_IDLE: {
                ppu_get_context()->pfc.cur_fetch_state = FS_PUSH;
        } break;

        case FS_PUSH: {
            if (pipeline_fifo_add()) {
                //keep pushing until succeed
                ppu_get_context()->pfc.cur_fetch_state = FS_TILE;
            }
        } break;

    }
}


void pipeline_push_pixel() {
    if (ppu_get_context()->pfc.pixel_fifo.size > 8) {
        //if fifo full, start pushing 
        u32 pixel_data = pixel_fifo_pop();

        if (ppu_get_context()->pfc.line_x >= (lcd_get_context()->scroll_x % 8)) {
            ppu_get_context()->video_buffer[ppu_get_context()->pfc.pushed_x + 
                lcd_get_context()->ly * XRES] = pixel_data;

                ppu_get_context()->pfc.pushed_x++;
        }

        ppu_get_context()->pfc.line_x++;
    }
}

void pipeline_process() {
    ppu_get_context()->pfc.map_y = lcd_get_context()->ly + lcd_get_context()->scroll_y;
    ppu_get_context()->pfc.map_x = ppu_get_context()->pfc.fetch_x + lcd_get_context()->scroll_x;
    ppu_get_context()->pfc.tile_y = ((lcd_get_context()->ly + lcd_get_context()->scroll_y) % 8) * 2; //y-value on cur tile

    if (!(ppu_get_context()->line_ticks & 1)) {
        //first bit is not set (on a even line instead of odd line)
        pipeline_fetch();
    }

    pipeline_push_pixel();
}

void pipeline_fifo_reset() {
    //reset when done with fifo
    while (ppu_get_context()->pfc.pixel_fifo.size) {
        pixel_fifo_pop();
    }

    ppu_get_context()->pfc.pixel_fifo.head = 0;
}
