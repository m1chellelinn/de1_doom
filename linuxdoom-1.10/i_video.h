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
// DESCRIPTION:
//	System specific interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __I_VIDEO__
#define __I_VIDEO__


#include "doomtype.h"

#ifdef __GNUG__
#pragma interface
#endif


// TODO: Called by D_DoomMain,
// determines the hardware configuration
// and sets up the video mode
void I_InitHardware (void);

// TODO: last call before program exit
void I_ShutdownGraphics(void);

// TODO:
// Takes full 8 bit values.
// Embedded DOOM implementation:
/*
static byte lpalette[256*3];

void I_SetPalette (byte* palette)
{
	memcpy(lpalette, palette, sizeof( lpalette ));
    //UploadNewPalette(X_cmap, palette);
	
}
*/
void I_SetPalette (byte* palette);

// draw buffered stuff to screen
// no implementation. don't need to change
// 2 locations: d_main.c
void I_UpdateNoBlit (void);

// TODO: flush framebuffer to VGA
// buffer declaration, each screen is [SCREENWIDTH*SCREENHEIGHT]:
/*
byte* screens[5];	
*/
void I_FinishUpdate (void);

// Wait for vertical retrace or pause a bit.
// defined in i_system.c
void I_WaitVBL(int count);

// Updates the "screens" array. No need to change
void I_ReadScreen (byte* scr);
// defined in i_system.c
void I_BeginRead (void);
void I_EndRead (void);



#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
