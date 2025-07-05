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
// DESCRIPTION:  none
//
//-----------------------------------------------------------------------------


static const char
rcsid[] = "$Id: s_sound.c,v 1.6 1997/02/03 22:45:12 b1 Exp $";

#include "i_system.h"
#include "i_sound.h"
#include "sounds.h"
#include "s_sound.h"

#include "z_zone.h"
#include "m_random.h"
#include "w_wad.h"

#include "doomdef.h"
#include "p_local.h"

#include "doomstat.h"


int 		snd_SfxVolume = 15;
int 		snd_MusicVolume = 15;
int			numChannels;	

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
//
void S_Init
( int		sfxVolume,
  int		musicVolume )
{  
    (void) sfxVolume;
    (void) musicVolume;
}




//
// Per level startup code.
// Kills playing sounds at start of level,
//  determines music if any, changes music.
//
void S_Start(void) {}	





void
S_StartSoundAtVolume
( void*		origin,
  int		sound_id,
  int		volume )
{
    (void) origin;
    (void) sound_id;
    (void) volume;
}	

void
S_StartSound
( void*		origin,
  int		sound_id )
{
    (void) origin;
    (void) sound_id;
}




void S_StopSound(void *origin)
{
    (void) origin;
}


//
// Stop and resume music, during game PAUSE.
//
void S_PauseSound(void)
{
}

void S_ResumeSound(void)
{
}


//
// Updates music & sounds
//
void S_UpdateSounds(void* listener)
{
    (void) listener;
}


void S_SetMusicVolume(int volume)
{
    (void) volume;
}



void S_SetSfxVolume(int volume)
{
    (void) volume;
}

//
// Starts some music with the music id found in sounds.h.
//
void S_StartMusic(int music_id)
{
    (void) music_id;
}

void S_ChangeMusic 
( int		music_id,
  int		looping )
{
    (void) music_id;
    (void) looping;
}


void S_StopMusic(void){}




void S_StopChannel(int handle)
{
    (void )handle;
}


