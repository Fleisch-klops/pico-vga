#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/flash.h"
#include "hardware/dma.h"
#include "vga.pio.h"

#define FLASH_TARGET_OFFSET 311200

static const uint8_t* flash_target_contents = (const uint8_t*) (XIP_BASE + FLASH_TARGET_OFFSET);

static const uint32_t ctrl_word_pixels = 0b0000001001111111;                   //640 -1
static const uint32_t ctrl_word_front_porch = 0b0000000000001110;              //16 -2
static const uint32_t ctrl_word_hsync_pulse = 0b0000000000111110;              //64 -2
static const uint32_t ctrl_word_back_porch = 0b0001110001110101;               //bit 7-16: 120 -3     bit 1-6: position of pixels tag (pc)
static const uint32_t ctrl_word_back_porch_blank = 0b0011000001110101;         //bit 7-16: 120 -3     bit 1-6: position of blank tag (pc)
static const uint32_t ctrl_word_back_porch_blank_vsync = 0b0100000001110101;   //bit 7-16: 120 -3     bit 1-6: position of blank_sync tag (pc)

static const uint set_pin = 6;
static const uint set_pin_count = 1;
static const uint sideset_pinbase = 7;
static const uint out_pinbase = 8;
static const uint out_pin_count = 8;
static const float pio_freq = 94500000; //Hz    -> 94.5 MHz
static const float pio_test_freq = 9450; //Hz    -> 9450 Hz



//ToDo: include flash and dma

      /************************************************************/
     /* 4 DMA channels:                                          */
    /*one for ctrl words during/and pixels                      */
   /*the second one for ctrl words during vertical front porch */
  /*the third one for ctrl words during v sync                */
 /* the fouth one for vertical back porch                    */
/************************************************************/


void program_flash() {
    flash_range_erase(FLASH_TARGET_OFFSET, 307200);
}


int main() {
    stdio_init_all();
    //configure test lines 
    uint32_t line_pixels = 0;
    uint32_t blank_no_sync_1 = (uint32_t)((ctrl_word_pixels << 16) + ctrl_word_front_porch);
    uint32_t blank_no_sync_2 = (uint32_t)((ctrl_word_hsync_pulse << 16) + ctrl_word_back_porch_blank);
    uint32_t blank_sync_1 = (uint32_t)((ctrl_word_pixels << 16) + ctrl_word_front_porch);
    uint32_t blank_sync_2 = (uint32_t)((ctrl_word_hsync_pulse << 16) + ctrl_word_back_porch_blank_vsync);

    //configuring the state machine and loading/starting pio program
    PIO pio = pio0;
    uint sm = pio_claim_unused_sm(pio, true);
    uint offset = pio_add_program(pio, &vga_program);
    float div =  (float)clock_get_hz(clk_sys) / pio_test_freq;
    vga_program_init(pio, sm, offset, set_pin, set_pin_count, out_pinbase, out_pin_count, sideset_pinbase, div);
    pio_sm_set_enabled(pio, sm, true);

    //get dma-channels
    int channel_one = dma_claim_unused_channel(true);
    int channel_two = dma_claim_unused_channel(true);
    int channel_three = dma_claim_unused_channel(true);
    int channel_four = dma_claim_unused_channel(true);

    //configure dma channels
    dma_channel_config ctrl_word_pixel_channel = dma_channel_get_default_config(channel_one);
    channel_config_set_dreq(&ctrl_word_pixel_channel, pio_get_dreq(pio, sm, true));
    channel_config_set_chain_to(&ctrl_word_pixel_channel, channel_two);
    channel_config_set_bswap(&ctrl_word_pixel_channel, true);

    dma_channel_config ctrl_word_vertical_front_porch_channel = dma_channel_get_default_config(channel_two);
    channel_config_set_dreq(&ctrl_word_vertical_front_porch_channel, pio_get_dreq(pio, sm, true));
    channel_config_set_chain_to(&ctrl_word_vertical_front_porch_channel, channel_three);
    channel_config_set_bswap(&ctrl_word_vertical_front_porch_channel, true);

    dma_channel_config ctrl_word_v_sync_channel = dma_channel_get_default_config(channel_three);
    channel_config_set_dreq(&ctrl_word_v_sync_channel, pio_get_dreq(pio, sm, true));
    channel_config_set_chain_to(&ctrl_word_v_sync_channel, channel_four);
    channel_config_set_bswap(&ctrl_word_v_sync_channel, true);

    dma_channel_config ctrl_word_vertical_back_porch_channel = dma_channel_get_default_config(channel_four);
    channel_config_set_dreq(&ctrl_word_vertical_back_porch_channel, pio_get_dreq(pio, sm, true));
    channel_config_set_chain_to(&ctrl_word_vertical_back_porch_channel, channel_one);
    channel_config_set_bswap(&ctrl_word_vertical_back_porch_channel, true);

    
    //write the first two ctrl_words to the pio_fifo
    pio_sm_clear_fifos(pio, sm);
    pio_sm_put(pio, sm, (uint32_t)((ctrl_word_back_porch_blank_vsync << 16) + ctrl_word_hsync_pulse));

    //start dma channels

    //continuesly put ctrl_words to pio_fifo
    while (true) {
        pio_sm_put_blocking(pio, sm, (uint32_t)((ctrl_word_front_porch << 16) + ctrl_word_pixels));
        printf("put blank_no_sync_1: %d\n", blank_no_sync_1);
        pio_sm_put_blocking(pio, sm, (uint32_t)((ctrl_word_back_porch_blank << 16) + ctrl_word_hsync_pulse));
        printf("put blank_no_sync_2: %d\n", blank_no_sync_2);
        pio_sm_put_blocking(pio, sm, (uint32_t)((ctrl_word_front_porch << 16) + ctrl_word_pixels));
        printf("put blank_sync_1: %d\n", blank_sync_1);
        pio_sm_put_blocking(pio, sm, (uint32_t)((ctrl_word_back_porch_blank_vsync << 16) + ctrl_word_hsync_pulse));
        printf("put blank_sync_2: %d\n", blank_sync_2);
    }
    return 0;
}