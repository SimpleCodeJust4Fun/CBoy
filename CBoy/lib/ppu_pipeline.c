#include <ppu.h>
#include <lcd.h>
#include <bus.h>

bool window_visible() {
    return LCDC_WIN_ENABLE && 
        lcd_get_context()->win_x >=0 && lcd_get_context()->win_x <= 166 &&
        lcd_get_context()->win_x >=0 && lcd_get_context()->win_y <= YRES; 
}

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

u32 fetch_sprite_pixels(int bit, u32 color, u8 bg_color) {
    for (int i = 0; i < ppu_get_context()->fetched_entry_count; i++) {
        int sp_x = (ppu_get_context()->fetched_entries[i].x - 8) +
            (lcd_get_context()->scroll_x % 8);

        if (sp_x + 8 < ppu_get_context()->pfc.fifo_x) {
            //past pixel point already
            continue;
        }

        int offset = ppu_get_context()->pfc.fifo_x - sp_x;

        if (offset < 0 || offset > 7) {
            //out of bounds
            continue;
        }

        //revert the sprite in mirror
        bit  = 7 - offset;
        if (ppu_get_context()->fetched_entries[i].f_x_flip) {
            bit = offset;
        }

        u8 high = !!(ppu_get_context()->pfc.fetch_entry_data[i * 2] & (1 << bit));
        u8 low = !!(ppu_get_context()->pfc.fetch_entry_data[i * 2 + 1] & (1 << bit)) << 1;
    
        bool bg_priority = ppu_get_context()->fetched_entries[i].f_bg_priority;

        if (!(high|low)) {
            //transparent color
            continue;
        }

        if (!bg_priority || bg_color == 0) {
            //sprite is in front of bg
            color = (ppu_get_context()->fetched_entries[i].f_ncgb_pn) ? 
                lcd_get_context()->sp2_colors[high|low] : lcd_get_context()->sp1_colors[high|low];
        
            if (high|low) {
                //check again if the fetched color is not transparent
                break;
            }
        }
    }
    return color;
}

bool pipeline_fifo_add() {
    if (ppu_get_context()->pfc.pixel_fifo.size > 8) {
        //fifo is full
        return false;
    }

    int x = ppu_get_context()->pfc.fetch_x - (8 - (lcd_get_context()->scroll_x % 8)); //x=8 is the 1st location visible on the screen

    for (int i = 0; i < 8; i++) {
        int bit = 7 - i;
        u8 high = !!(ppu_get_context()->pfc.bgw_fetch_data[1] & (1 << bit));
        u8 low = !!(ppu_get_context()->pfc.bgw_fetch_data[2] & (1 << bit)) << 1;
        u32 color = lcd_get_context()->bg_colors[high|low];

        if (!LCDC_BGW_ENABLE) {
            color = lcd_get_context()->bg_colors[0];

        }

        if (LCDC_OBJ_ENABLE) {
            color = fetch_sprite_pixels(bit, color, high|low);
        }

        if (x >= 0) {
            //push that color onto the fifo
            pixel_fifo_push(color);
            ppu_get_context()->pfc.fifo_x++;
        }
    }

    return true;
}

void pipeline_load_sprite_tile() {
    oam_line_entry *le = ppu_get_context()->line_sprites;
    
    while(le) {
        int sp_x = (le->entry.x - 8) + (lcd_get_context()->scroll_x % 8);

        //bound check
        if ((sp_x >= ppu_get_context()->pfc.fetch_x && sp_x < ppu_get_context()->pfc.fetch_x + 8) ||
            ((sp_x + 8) >= ppu_get_context()->pfc.fetch_x && (sp_x + 8) < ppu_get_context()->pfc.fetch_x + 8)) {
            //need to add entry
            ppu_get_context()->fetched_entries[ppu_get_context()->fetched_entry_count++] = le->entry;
        }

        le = le->next;

        if (!le || ppu_get_context()->fetched_entry_count >= 3) {
            //max 3 sprites per 8-pixel section
            break;
        }
    }
}

void pipeline_load_sprite_data(u8 offset) {
    int cur_y = lcd_get_context()->ly;
    u8 sprite_height = LCDC_OBJ_HEIGHT;

    for (int i = 0; i < ppu_get_context()->fetched_entry_count; i++) {
        u8 tile_y = ((cur_y + 16) - ppu_get_context()->fetched_entries[i].y) * 2;
    
        if (ppu_get_context()->fetched_entries[i].f_y_flip) {
            //flipped upside down
            tile_y = (sprite_height * 2 - 2) - tile_y;
        }

        u8 tile_index = ppu_get_context()->fetched_entries[i].tile;

        if (sprite_height == 16) {
            //trick for 8x16 sprite 
            tile_index &= ~(1); //remove the last bit
        }

        ppu_get_context()->pfc.fetch_entry_data[i * 2 + offset] = 
            bus_read(0x8000 + tile_index * 16 + tile_y + offset);
    }
}

void pipeline_load_window_tile() {
    if (!window_visible()) {
        return;
    }

    u8 window_y = lcd_get_context()->win_y;

    if (ppu_get_context()->pfc.fetch_x + 7 >= lcd_get_context()->win_x && 
            ppu_get_context()->pfc.fetch_x + 7 < lcd_get_context()->win_x + YRES + 14) {
        if (lcd_get_context()->ly >= window_y && lcd_get_context()->ly < window_y + XRES) {
            u8 w_tile_y = ppu_get_context()->window_line / 8;

            ppu_get_context()->pfc.bgw_fetch_data[0] = bus_read(LCDC_WIN_MAP_AREA +
                ((ppu_get_context()->pfc.fetch_x + 7 - lcd_get_context()->win_x) / 8) +
                w_tile_y * 32);

            if (LCDC_BGW_DATA_AREA == 0x8800) {
                ppu_get_context()->pfc.bgw_fetch_data[0] += 128; //increment id for that tile
            }
        }
    }
}

void pipeline_fetch() {
    switch(ppu_get_context()->pfc.cur_fetch_state) {
        case FS_TILE: {
            ppu_get_context()->fetched_entry_count = 0;

            //fetch the cur tile for the location we are on the map
            if (LCDC_BGW_ENABLE) {
                ppu_get_context()->pfc.bgw_fetch_data[0] = bus_read(LCDC_BG_MAP_AREA +
                    ppu_get_context()->pfc.map_x / 8 +
                    (ppu_get_context()->pfc.map_y / 8) * 32);
                
                if (LCDC_BGW_DATA_AREA == 0x8800) {
                    ppu_get_context()->pfc.bgw_fetch_data[0] += 128; //increment id for that tile
                }
            }

            pipeline_load_window_tile();

            if (LCDC_OBJ_ENABLE && ppu_get_context()->line_sprites) {
                pipeline_load_sprite_tile();
            }

            //switch to next state
            ppu_get_context()->pfc.cur_fetch_state = FS_DATA0;
            ppu_get_context()->pfc.fetch_x += 8;
        } break;

        case FS_DATA0: {
            ppu_get_context()->pfc.bgw_fetch_data[1] = bus_read(LCDC_BGW_DATA_AREA +
                ppu_get_context()->pfc.bgw_fetch_data[0] * 16 +
                ppu_get_context()->pfc.tile_y);

            pipeline_load_sprite_data(0);
            ppu_get_context()->pfc.cur_fetch_state = FS_DATA1;
        } break;

        case FS_DATA1: {
            ppu_get_context()->pfc.bgw_fetch_data[2] = bus_read(LCDC_BGW_DATA_AREA +
                ppu_get_context()->pfc.bgw_fetch_data[0] * 16 +
                ppu_get_context()->pfc.tile_y + 1);

            pipeline_load_sprite_data(1);

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
