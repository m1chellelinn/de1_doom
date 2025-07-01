// Emacs style mode select   -*- C++ -*- 
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

byte local_palette[256*3];

inline void WriteVgaPixel(int x, int y, byte r, byte g, byte b) {
    volatile uint16_t *vga_ptr = (uint16_t *) (
        (int)sram_v_addr + 
        ((x+5) << VGA_ADDR_X_OFFSET) + 
        ((y+5) << VGA_ADDR_Y_OFFSET)
    );
    *vga_ptr = (r << VGA_R_OFFSET) + (g << VGA_G_OFFSET) + (b << VGA_B_OFFSET);
}


void I_InitGraphics (void) {
    printf("I_InitGraphics: enter");
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
}


void I_ShutdownGraphics(void) {
    printf("I_ShutdownGraphics: enter");
    unmap_physical (lw_v_addr, LW_BRIDGE_SPAN);
    unmap_physical (sram_v_addr, FPGA_ONCHIP_SPAN);
    close_physical (fpga_fd);

    lw_v_addr = NULL;
    sram_v_addr = NULL;
    led_ptr = NULL;
    fpga_fd = -1;
}


void I_SetPalette (byte* palette) {
    printf("I_SetPalette: enter");
    byte c;
    int i;
    for (i = 0 ; i<256 ; i++) {
        c = gammatable[usegamma][*palette++];
        local_palette[i+PALETTE_R_OFFSET] = c >> 3; // VGA R 
        c = gammatable[usegamma][*palette++];
        local_palette[i+PALETTE_G_OFFSET] = c >> 3;
        c = gammatable[usegamma][*palette++];
        local_palette[i+PALETTE_B_OFFSET] = c >> 3;
    }
}


void I_UpdateNoBlit (void) { }


void I_FinishUpdate (void) {
    int x, y;
    for (y = 0; y < SCREENHEIGHT; y++) {
        for (x = 0; x < SCREENWIDTH; x++) {
            byte index = screens[0][y * SCREENWIDTH + x];
            byte r = local_palette[index+PALETTE_R_OFFSET];
            byte g = local_palette[index+PALETTE_G_OFFSET];
            byte b = local_palette[index+PALETTE_B_OFFSET];
            WriteVgaPixel(x, y, r, g, b);
        }
    }
}


void I_ReadScreen (byte* scr) {
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}



//
// I_StartFrame
//
void I_StartFrame (void)
{
    // er?
}


//
// I_StartTic
//
void I_StartTic (void)
{

    // if (!X_display)
	// return;

    // while (XPending(X_display))
	// I_GetEvent();

    // // Warp the pointer back to the middle of the window
    // //  or it will wander off - that is, the game will
    // //  loose input focus within X11.
    // if (grabMouse)
    // {
	// if (!--doPointerWarp)
	// {
	//     XWarpPointer( X_display,
	// 		  None,
	// 		  X_mainWindow,
	// 		  0, 0,
	// 		  0, 0,
	// 		  X_width/2, X_height/2);

	//     doPointerWarp = POINTER_WARP_COUNTDOWN;
	// }
    // }

    // mousemoved = false;

}
