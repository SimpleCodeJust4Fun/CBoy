#include <ppu.h>
#include <lcd.h>
#include <cpu.h>
#include <interrupts.h>
#include <string.h>

void increment_ly() {
    if (window_visible() && lcd_get_context()->ly >= lcd_get_context()->win_y && 
        lcd_get_context()->ly < lcd_get_context()->win_y + YRES) {
        //if window is visible and current line is within window's y range
        //increment window line
        ppu_get_context()->window_line++;
    }

    lcd_get_context()->ly++;

    if (lcd_get_context()->ly == lcd_get_context()->ly_compare) {
        LCDS_LYC_SET(1); //set LYC=LY flag
        if (LCDC_STAT_INT(SS_LYC)) {
            //check if interrupt needs to be requested
            cpu_request_interrupt(IT_LCD_STAT);
        }
    } else {
        LCDS_LYC_SET(0); //clear LYC=LY flag
    }
}

void load_line_sprites() {
    //load all sprites on given line into a linked list
    int cur_y = lcd_get_context()->ly;

    u8 sprite_height = LCDC_OBJ_HEIGHT;
    //clear line entry array
    memset(ppu_get_context()->line_entry_array, 0,
        sizeof(ppu_get_context()->line_entry_array));

    for (int i = 0; i < 40; i++) {
        //loop through all 40 sprites in oam
        oam_entry e = ppu_get_context()->oam_ram[i];
    
        if (!e.x) {
            //if x is 0, then sprite is not visible
            continue;
        }

        if (ppu_get_context()->line_sprite_count >= 10) {
            //if we already have 10 sprites on line, stop
            break;
        }

        if (e.y <= cur_y + 16 && e.y + sprite_height > cur_y + 16) {
            //this sprite is on cur line

            oam_line_entry *entry = &ppu_get_context()->line_entry_array[
                ppu_get_context()->line_sprite_count++
            ];

            entry->entry = e;
            entry->next = NULL;

            // add the cur entry to linked list 
            if (!ppu_get_context()->line_sprites ||
                    ppu_get_context()->line_sprites->entry.x > e.x) {
                //make sure entry with less x is in front position
                entry->next = ppu_get_context()->line_sprites;
                ppu_get_context()->line_sprites = entry;
                continue;
            }

            //find a correct place for cur entry in the linked list
            //sorted sprites by x position is essential for some game's proper rendering

            oam_line_entry *le = ppu_get_context()->line_sprites;
            oam_line_entry *prev = le;

            while(le) {
                if (le->entry.x > e.x) {
                    //if cur entry has less x than the entry in the list
                    //insert cur entry between prev and le: (prev, entry, le)
                    prev->next = entry;
                    entry->next = le;
                    break;
                }
                
                if (!le->next) {
                    le->next = entry;
                    break;
                }

                prev = le;
                le = le->next;
            }
        }
    }
}

void ppu_mode_oam() {
    if (ppu_get_context()->line_ticks >= 80) {
        LCDS_MODE_SET(MODE_XFER);

        ppu_get_context()->pfc.cur_fetch_state = FS_TILE;
        ppu_get_context()->pfc.line_x = 0;
        ppu_get_context()->pfc.fetch_x = 0;
        ppu_get_context()->pfc.pushed_x = 0;
        ppu_get_context()->pfc.fifo_x = 0;
    }

    if (ppu_get_context()->line_ticks == 1) {
        //read oam data on the first tick only, diff from real CPU (read oam data on every tick)
        ppu_get_context()->line_sprites = 0;
        ppu_get_context()->line_sprite_count = 0;

        load_line_sprites();
    }
}
void ppu_mode_xfer() {
    pipeline_process();

    if (ppu_get_context()->pfc.pushed_x >= XRES) {
        //when we pushed all the pixels that we can push
        pipeline_fifo_reset();

        LCDS_MODE_SET(MODE_HBLANK);

        if (LCDC_STAT_INT(SS_HBLANK)) {
            cpu_request_interrupt(IT_LCD_STAT);
        }
    }
}

static u32 target_frame_time = 1000 / 60; //60 frame per 1000ms
static long prev_frame_time = 0;
static long start_timer = 0;
static long frame_count = 0;

void ppu_mode_hblank() {
    if (ppu_get_context()->line_ticks >= TICKS_PER_LINE) {
        increment_ly();

        if (lcd_get_context()->ly >= YRES) {
            //if at the last visible line, go to vblank
            LCDS_MODE_SET(MODE_VBLANK);

            cpu_request_interrupt(IT_VBLANK);

            if (LCDC_STAT_INT(SS_VBLANK)) {
                //check if interrupt needs to be requested
                cpu_request_interrupt(IT_LCD_STAT);
            }

            ppu_get_context()->current_frame++; //we are at the last hblank of the frame
        
            //calc and adjust to get a constant FPS of 60 FPS
            u32 end = get_ticks(); //get current number of ticks
            u32 frame_time = end - prev_frame_time; //get time elapsed since last frame

            if (frame_time < target_frame_time) {
                delay(target_frame_time - frame_time); //delay to make sure we have a constant frame rate
            }

            if (end - start_timer >= 1000) {
                //if 1 second has passed, print FPS info
                u32 fps = frame_count;
                frame_count = 0;
                start_timer = end;
                printf("FPS: %d\n", fps);
            }

            frame_count++;
            prev_frame_time = get_ticks();

        } else {
            LCDS_MODE_SET(MODE_OAM);
        }

        ppu_get_context()->line_ticks = 0; //reset tick for a new line
    }
}

void ppu_mode_vblank() {
    if (ppu_get_context()->line_ticks >= TICKS_PER_LINE) {
        increment_ly();

        if (lcd_get_context()->ly >= LINES_PER_FRAME) {
            //if ly goes beyond the max line number in a frame, 
            //go back to beginning (OAM)
            LCDS_MODE_SET(MODE_OAM);
            lcd_get_context()->ly = 0; //reset ly to 0
            ppu_get_context()->window_line = 0; //reset window line to 0
        }

        ppu_get_context()->line_ticks = 0; //reset tick for a new line
    }
}


