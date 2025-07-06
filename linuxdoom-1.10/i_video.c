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

#include "doomstat.h"
#include "i_address_map.h"
#include "i_consts.h"
#include "i_peripherals.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

#include "doomdef.h"
#include "i_address_map.h"

#define POINTER_WARP_COUNTDOWN	1

int fpga_fd = -1;
void *lw_v_addr = NULL;
void *sram_v_addr = NULL;
volatile int *led_ptr = NULL;

int keys_fd = -1;
const char *dev = LINUX_KEYBOARD_EVENT_PATH;
struct input_event event_;
ssize_t num_bytes_read;

byte local_palette[256*3];

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
            printf("Got new event: %d-%s\n", event_.code, event_.value ? "ev_keydown" : "ev_keyup");
            int doomKeyCode;
            switch (event_.code) {
                case LINUX_DOOM_KEY_UP:
                    doomKeyCode = DOOM_KEY_UPARROW;
                case LINUX_DOOM_KEY_DOWN:
                    doomKeyCode = DOOM_KEY_DOWNARROW;
                case LINUX_DOOM_KEY_LEFT:
                    doomKeyCode = DOOM_KEY_LEFTARROW;
                case LINUX_DOOM_KEY_RIGHT:
                    doomKeyCode = DOOM_KEY_RIGHTARROW;
                case LINUX_DOOM_KEY_ENTER:
                    doomKeyCode = DOOM_KEY_ENTER;
                case LINUX_DOOM_KEY_ESC:
                    doomKeyCode = DOOM_KEY_ESCAPE;
                case LINUX_DOOM_KEY_TAB:
                    doomKeyCode = DOOM_KEY_TAB;
            }

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
    if ((fpga_fd = open_physical (fpga_fd)) == -1) {
        printf("I_InitGraphics: fail to open file to /dev/mem");
        return;
    }
    if (!(lw_v_addr = map_physical (fpga_fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN))){
        printf("I_InitGraphics: fail to map FPGA LW bridge");
        return;
    }
    if (!(sram_v_addr = map_physical (fpga_fd, FPGA_ONCHIP_BASE, FPGA_ONCHIP_SPAN))){
        printf("I_InitGraphics: fail to map FPGA SRAM bridge");
        return;
    }
    
    led_ptr = (int *) ( (int)lw_v_addr + LEDR_BASE);

    
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
    close_physical (fpga_fd);

    lw_v_addr = NULL;
    sram_v_addr = NULL;
    led_ptr = NULL;
    fpga_fd = -1;
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

    // printf("\n\nINPUT PALETTE: \n");
    // for (i = 0; i < 256; i++) {
    //     printf("%02X ", palette[i]);
    // }

    
    // printf("\n\nLOCAL PALETTE: \n");
    // for (i = 0; i < 256; i++) {
    //     printf("%02X-", local_palette[(i*3)+PALETTE_R_OFFSET]);
    //     printf("%02X-", local_palette[(i*3)+PALETTE_G_OFFSET]);
    //     printf("%02X ", local_palette[(i*3)+PALETTE_B_OFFSET]);
    // }


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
            
            // if (y==0 && !printedScreenData) {
            //     printf("x=%d index=%d, r=%d, g=%d, b=%d\n", x, index, r, g, b);
            // }
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
