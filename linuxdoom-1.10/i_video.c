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
    [KEY_Y] = 'y'
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

            event_t doomEvent;
            doomEvent.data1 = doomKeyCode;
            doomEvent.data2 = 0;
            doomEvent.data3 = 0;
            doomEvent.type = event_.value ? ev_keydown : ev_keyup;
            D_PostEvent(&doomEvent); // This gets copied. We're fine.
        }
    }
}


void I_InitHardware (void) {
    printf("I_InitHardware: enter\n");

    // Map FPGA virtual address ranges
    if ((mmap_fd = open_physical (mmap_fd)) == -1) {
        printf("I_InitHardware: fail to open file to /dev/mem");
        return;
    }
    if (!(lw_v_addr = map_physical (mmap_fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN))){
        printf("I_InitHardware: fail to map FPGA LW bridge");
        return;
    }
    if (!(sram_v_addr = map_physical (mmap_fd, FPGA_ONCHIP_BASE, FPGA_ONCHIP_SPAN))){
        printf("I_InitHardware: fail to map FPGA SRAM bridge");
        return;
    }

    FILE *fp = fopen("/sys/kernel/fpga_space/phys", "r");
    if (!fp) {
        printf("I_InitHardware: failed to open /sys/kernel/fpga_space/phys\n");
        exit(1);
    }
    char buf[64];
    if (!fgets(buf, sizeof(buf), fp)) {
        printf("I_InitHardware: failed to read from /sys/kernel/fpga_space/phys\n");
        fclose(fp);
        exit(1);
    }
    fclose(fp); printf("File /sys/kernel/fpga_space/phys contents: %s", buf);

    ddr_p_addr = atoi(buf);
    if (ddr_p_addr == 0 && buf[0] != '0') {
        printf("I_InitHardware: invalid contents in /sys/kernel/fpga_space/phys\n");
        exit(1);
    }

    // phys_addr now holds the converted value
    if (!(ddr_v_addr = map_physical(mmap_fd, 0, 7*1024*1024))) {
        printf("I_InitHardware: fail to map DDR3 SDRAM\n");
        exit(1);
    }

    printf("Initialized shared RAM space: 0x%x (physical: 0x%x\n", ddr_v_addr, ddr_p_addr);

    led_ptr = (int *) ( (int)lw_v_addr + LEDR_BASE);
    doom_ptr = (int *) ( (int)lw_v_addr + DOOM_DRIVER_BASE);

    printf("Resetting FPGA\n");
    HAL_Reset();
    printf("Checking FPGA memory access capability\n");
    HAL_SelfCheck(); 

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
    HAL_I_SetPalette(palette);
}


void I_UpdateNoBlit (void) { }


void I_FinishUpdate (void) {
    HAL_I_FinishUpdate(screens[0]);
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
}


//
// I_StartTic
//
void I_StartTic (void)
{
    GetAndSendUpdates();
}
