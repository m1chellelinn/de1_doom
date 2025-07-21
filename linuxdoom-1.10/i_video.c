// Emacs style mode select   -*- C -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for X11, UNIX.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_x.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";

#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <fcntl.h>
#include <linux/input.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>

#include "doomdef.h"
#include "doomstat.h"
#include "i_consts.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

#include "i_address_map.h"
#include "i_peripherals.h"

#define POINTER_WARP_COUNTDOWN	1

int keys_fd = -1;
const char *dev = LINUX_KEYBOARD_EVENT_PATH;
struct input_event event_;
ssize_t num_bytes_read;

byte local_palette[256*3];

static int keyMap[256] = {
    // Weapons
    [KEY_1] = '1',
    [KEY_2] = '2',
    [KEY_3] = '3',
    [KEY_4] = '4',
    [KEY_5] = '5',
    [KEY_6] = '6',
    [KEY_7] = '7',

    // Map
    [KEY_F] = 'F',
    [KEY_M] = 'M',
    [KEY_C] = 'C',
    [KEY_O] = 'O',
    [KEY_MINUS] = '-',
    [KEY_EQUAL] = '+',

    // Movement
    [KEY_W] = DOOM_KEY_UPARROW,
    [KEY_A] = DOOM_KEY_LEFTARROW,
    [KEY_S] = DOOM_KEY_DOWNARROW,
    [KEY_D] = DOOM_KEY_RIGHTARROW,
    [KEY_UP] = DOOM_KEY_UPARROW,
    [KEY_LEFT] = DOOM_KEY_LEFTARROW,
    [KEY_DOWN] = DOOM_KEY_DOWNARROW,
    [KEY_RIGHT] = DOOM_KEY_RIGHTARROW,
    [KEY_LEFTALT] = DOOM_KEY_LALT,

    // Fire
    [KEY_SPACE] = DOOM_KEY_RCTRL,

    // Interact
    [KEY_E] = ' ',

    // Specially-defined keys
    [KEY_ENTER] = DOOM_KEY_ENTER,
    [KEY_ESC] = DOOM_KEY_ESCAPE,
    [KEY_TAB] = DOOM_KEY_TAB,

    // Quitting the game
    [KEY_Y] = 'Y'
};



inline void WriteVgaPixel(int x, int y, byte r, byte g, byte b) {
    volatile uint16_t *vga_ptr = (uint16_t *) (
        (int)sram_v_addr + 
        ((x+5) << VGA_ADDR_X_OFFSET) + 
        ((y+5) << VGA_ADDR_Y_OFFSET)
    );
    *vga_ptr = (r << VGA_R_OFFSET) | (g << VGA_G_OFFSET) | (b << VGA_B_OFFSET);
}

// NOTE: void D_PostEvent (event_t* ev);
void GetAndSendUpdates() {

    while ((num_bytes_read = read(keys_fd, &event_, sizeof event_)) > 0) {
        if (num_bytes_read != sizeof event_) {
            printf("GetAndSendUpdates: invalid input read size\n");
            continue;
        }

        if (event_.type == EV_KEY && 
            event_.value >= 0 && 
            event_.value <= 2) 
        {
            int doomKeyCode = 0;
            if (event_.code >= 0 && event_.code < 256) {
                doomKeyCode = keyMap[event_.code];
            }
            printf("Got new event: %d-%s\n  -->Doom interpretation: %d\n", 
                event_.code, 
                event_.value ? "ev_keydown" : "ev_keyup", 
                doomKeyCode);

            event_t doomEvent;
            doomEvent.data1 = doomKeyCode;
            doomEvent.data2 = 0;
            doomEvent.data3 = 0;
            doomEvent.type = event_.value ? ev_keydown : ev_keyup;
            D_PostEvent(&doomEvent); // This gets copied. We're fine.
        }
    }
}


void I_InitGraphics (void) {
    printf("I_InitGraphics: enter\n");

    // Map FPGA virtual address ranges
    if ((mmap_fd = open_physical (mmap_fd)) == -1) {
        printf("I_InitGraphics: fail to open file to /dev/mem");
        return;
    }
    if (!(lw_v_addr = map_physical (mmap_fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN))){
        printf("I_InitGraphics: fail to map FPGA LW bridge");
        return;
    }
    if (!(sram_v_addr = map_physical (mmap_fd, FPGA_ONCHIP_BASE, FPGA_ONCHIP_SPAN))){
        printf("I_InitGraphics: fail to map FPGA SRAM bridge");
        return;
    }
    if (!(ddr_v_addr = map_physical (mmap_fd, DDR_BASE, DDR_SPAN))){
        printf("I_InitGraphics: fail to map DDR3 SDRAM");
        exit(1);
    }

    
    led_ptr = (int *) ( (int)lw_v_addr + LEDR_BASE);
    doom_ptr = (int *) ( (int)lw_v_addr + DOOM_DRIVER_BASE);

    int i;

    printf("Calling FPGA debug function\n");
    *(int*)((int)doom_ptr+1) = DDR_BASE + 0x30000000;
    *(doom_ptr) = CMD_V_Init;

    printf("Getting FPGA-written data??\n");
    while (1) {
        printf("Contents of 0x3000_0000: ");
        for (i = 0; i < 20; i++) {
            printf("%d, ", *(ddr_v_addr + i));
        }
        printf("\n");
        sleep(1);
    }


    
    keys_fd = open(dev, O_RDONLY);
    if (keys_fd == -1) {
        printf("Cannot open %s: %s.\n", dev, strerror(errno));
        return;
    }
    // Use non-blocking read to process all available events and then exit
    int flags = fcntl(keys_fd, F_GETFL, 0);
    fcntl(keys_fd, F_SETFL, flags | O_NONBLOCK);

    printf("Opened %s for reading", LINUX_KEYBOARD_EVENT_PATH);
}


void I_ShutdownGraphics(void) {
    printf("I_ShutdownGraphics: enter\n");
    unmap_physical (lw_v_addr, LW_BRIDGE_SPAN);
    unmap_physical (sram_v_addr, FPGA_ONCHIP_SPAN);
    close_physical (mmap_fd);

    lw_v_addr = NULL;
    sram_v_addr = NULL;
    led_ptr = NULL;
    doom_ptr = NULL;
    mmap_fd = -1;
    keys_fd = -1; // let Linux auto-close this file
}


void I_SetPalette (byte* palette) {
    // printf("I_SetPalette: enter w/ usegamma=%d\n", usegamma);
    byte c;
    int i;

    usegamma = 3;
    for (i = 0 ; i<256 ; i++) {
        c = gammatable[usegamma][*palette++];
        local_palette[(i*3)+PALETTE_R_OFFSET] = c >> 3; // VGA R 
        c = gammatable[usegamma][*palette++];
        local_palette[(i*3)+PALETTE_G_OFFSET] = c >> 2;
        c = gammatable[usegamma][*palette++];
        local_palette[(i*3)+PALETTE_B_OFFSET] = c >> 3;
    }
}

void I_UpdateNoBlit (void) { }


// boolean printedScreenData = false;

void I_FinishUpdate (void) {
    // printf("I_FinishUpdate invoke\n");
    // printf("I_FinishUpdate: invoke\n");
    
    int x, y;
    for (y = 0; y < SCREENHEIGHT; y++) {
        for (x = 0; x < SCREENWIDTH; x++) {
            byte index = screens[0][y * SCREENWIDTH + x];
            byte r = local_palette[(index*3)+PALETTE_R_OFFSET];
            byte g = local_palette[(index*3)+PALETTE_G_OFFSET];
            byte b = local_palette[(index*3)+PALETTE_B_OFFSET];
            WriteVgaPixel(x, y, r, g, b);
            
        }
    }

    // Debug pixels
    x = 50; y = 220;
    WriteVgaPixel(x++, y, 0xFF, 0xFF, 0xFF);
    WriteVgaPixel(x++, y, 0xFF, 0x00, 0x00);
    WriteVgaPixel(x++, y, 0x00, 0xFF, 0x00);
    WriteVgaPixel(x++, y, 0x00, 0x00, 0xFF);
    WriteVgaPixel(x++, y, 0xFF, 0xFF, 0x00);
    WriteVgaPixel(x++, y, 0x00, 0xFF, 0xFF);
    WriteVgaPixel(x++, y, 0xFF, 0x00, 0xFF);
    
    // printedScreenData = true;
    GetAndSendUpdates();
}


void I_ReadScreen (byte* scr) {
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}



//
// I_StartFrame
//
void I_StartFrame (void)
{
    // printf("I_StartFrame invoke\n");
    // er?
}


//
// I_StartTic
//
void I_StartTic (void)
{
    GetAndSendUpdates();
}
