#include <ppu.h>
#include <lcd.h>
#include <cpu.h>
#include <interrupts.h>

void increment_ly() {
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

void ppu_mode_oam() {
    if (ppu_get_context()->line_ticks >= 80) {
        LCDS_MODE_SET(MODE_XFER);

        ppu_get_context()->pfc.cur_fetch_state = FS_TILE;
        ppu_get_context()->pfc.line_x = 0;
        ppu_get_context()->pfc.fetch_x = 0;
        ppu_get_context()->pfc.pushed_x = 0;
        ppu_get_context()->pfc.fifo_x = 0;
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
        }

        ppu_get_context()->line_ticks = 0; //reset tick for a new line
    }
}


