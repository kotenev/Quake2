/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/*
** RW_IMP.C
**
** This file contains ALL Win32 specific stuff having to do with the
** software refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** SWimp_EndFrame
** SWimp_Init
** SWimp_SetPalette
** SWimp_Shutdown
*/

#define WIN32_LEAN_AND_MEAN			// exclude rarely-used services from windown headers
#include <windows.h>
#include <ddraw.h>

#include "../ref_soft/r_local.h"
#include "rw_win.h"


swwstate_t sww_state;


/*
** SWimp_Init
**
** This routine is responsible for initializing the implementation
** specific stuff in a software rendering subsystem.
*/
int SWimp_Init( void )
{
	return true;	//?? empty
}

/*
** SWimp_InitGraphics
**
** This initializes the software refresh's implementation specific
** graphics subsystem.  In the case of Windows it creates DIB or
** DDRAW surfaces.
**
** The necessary width and height parameters are grabbed from
** vid.width and vid.height.
*/
static bool SWimp_InitGraphics (bool fullscreen)
{
	// free resources in use
	SWimp_Shutdown ();

	// create a new window
	sww_state.hWnd = (HWND)Vid_CreateWindow (vid.width, vid.height, fullscreen);

	// initialize the appropriate subsystem
	if (!fullscreen)
	{
		if (!DIB_Init (&vid.buffer, &vid.rowbytes))
		{
			vid.buffer = 0;
			vid.rowbytes = 0;

			return false;
		}
	}
	else
	{
		if (!DDRAW_Init (&vid.buffer, &vid.rowbytes))
		{
			vid.buffer = 0;
			vid.rowbytes = 0;

			return false;
		}
	}

	return true;
}

/*
** SWimp_EndFrame
**
** This does an implementation specific copy from the backbuffer to the
** front buffer.  In the Win32 case it uses BitBlt or BltFast depending
** on whether we're using DIB sections/GDI or DDRAW.
*/
void SWimp_EndFrame (void)
{
	if (!sw_state.fullscreen)
	{
		if (sww_state.palettized)
		{
//			holdpal = SelectPalette(hdcScreen, hpalDIB, FALSE);
//			RealizePalette(hdcScreen);
		}


		BitBlt (sww_state.hDC,
			    0, 0,
				vid.width,
				vid.height,
				sww_state.hdcDIBSection,
				0, 0,
				SRCCOPY);

		if (sww_state.palettized)
		{
//			SelectPalette(hdcScreen, holdpal, FALSE);
		}
	}
	else
	{
		RECT r;
		HRESULT rval;
		DDSURFACEDESC ddsd;

		r.left = 0;
		r.top = 0;
		r.right = vid.width;
		r.bottom = vid.height;

		sww_state.ddsOffScreenBuffer->Unlock (vid.buffer);

		if (sww_state.modex)
		{
			if ((rval = sww_state.ddsBackBuffer->BltFast (0, 0, sww_state.ddsOffScreenBuffer, &r, DDBLTFAST_WAIT)) == DDERR_SURFACELOST)
			{
				sww_state.ddsBackBuffer->Restore ();
				sww_state.ddsBackBuffer->BltFast (0, 0, sww_state.ddsOffScreenBuffer, &r, DDBLTFAST_WAIT);
			}

			if ((rval = sww_state.ddsFrontBuffer->Flip (NULL, DDFLIP_WAIT)) == DDERR_SURFACELOST)
			{
				sww_state.ddsFrontBuffer->Restore ();
				sww_state.ddsFrontBuffer->Flip (NULL, DDFLIP_WAIT);
			}
		}
		else
		{
			if ((rval = sww_state.ddsFrontBuffer->BltFast (0, 0, sww_state.ddsOffScreenBuffer, &r, DDBLTFAST_WAIT)) == DDERR_SURFACELOST)
			{
				sww_state.ddsFrontBuffer->Restore ();
				sww_state.ddsFrontBuffer->BltFast (0, 0, sww_state.ddsOffScreenBuffer, &r, DDBLTFAST_WAIT);
			}
		}

		memset (&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);

		sww_state.ddsOffScreenBuffer->Lock (NULL, &ddsd, DDLOCK_WAIT, NULL);

		vid.buffer = (byte*)ddsd.lpSurface;
		vid.rowbytes = ddsd.lPitch;
	}
}

/*
** SWimp_SetMode
*/
rserr_t SWimp_SetMode( int *pwidth, int *pheight, int mode, bool fullscreen )
{
	const char *win_fs[] = { "W", "FS" };
	rserr_t retval = rserr_ok;

	Com_Printf ("setting mode %d:", mode);

	if ( !Vid_GetModeInfo( pwidth, pheight, mode ) )
	{
		Com_WPrintf (" invalid mode\n");
		return rserr_invalid_mode;
	}

	Com_Printf (" %d %d %s\n", *pwidth, *pheight, win_fs[fullscreen]);

	sww_state.initializing = true;
	if ( fullscreen )
	{
		if ( !SWimp_InitGraphics( 1 ) )
		{
			if ( SWimp_InitGraphics( 0 ) )
			{
				// mode is legal but not as fullscreen
				fullscreen = false;
				retval = rserr_invalid_fullscreen;
			}
			else
				Com_FatalError ("Failed to set a valid mode in windowed mode");
		}
	}
	else
	{
		// failure to set a valid mode in windowed mode
		if ( !SWimp_InitGraphics( fullscreen ) )
		{
			sww_state.initializing = true;
			Com_FatalError ("Failed to set a valid mode in windowed mode");
		}
	}

	sw_state.fullscreen = fullscreen;
	R_GammaCorrectAndSetPalette( ( const unsigned char * ) d_8to24table );
	sww_state.initializing = true;

	return retval;
}

/*
** SWimp_SetPalette
**
** System specific palette setting routine.  A NULL palette means
** to use the existing palette.  The palette is expected to be in
** a padded 4-byte xRGB format.
*/
void SWimp_SetPalette( const unsigned char *palette )
{
	// MGL - what the fuck was kendall doing here?!
	// clear screen to black and change palette
	//	for (i=0 ; i<vid.height ; i++)
	//		memset (vid.buffer + i*vid.rowbytes, 0, vid.width);

	if ( !palette )
		palette = ( const unsigned char * ) sw_state.currentpalette;

	if ( !sw_state.fullscreen )
	{
		DIB_SetPalette( ( const unsigned char * ) palette );
	}
	else
	{
		DDRAW_SetPalette( ( const unsigned char * ) palette );
	}
}

/*
** SWimp_Shutdown
**
** System specific graphics subsystem shutdown routine.  Destroys
** DIBs or DDRAW surfaces as appropriate.
*/
void SWimp_Shutdown( void )
{
	Com_Printf ("Shutting down SW imp\n");
	DIB_Shutdown();
	DDRAW_Shutdown();

	if (sww_state.hWnd)
	{
		Com_Printf ("...destroying window\n");
		ShowWindow( sww_state.hWnd, SW_SHOWNORMAL );	// prevents leaving empty slots in the taskbar (??)
		Vid_DestroyWindow (false);
		sww_state.hWnd = NULL;
	}
}

/*
** SWimp_AppActivate
*/
void SWimp_AppActivate (bool active)
{
//	if (!sww_state.hWnd) return;

	if (active)
	{
		SetForegroundWindow (sww_state.hWnd);
		ShowWindow (sww_state.hWnd, SW_RESTORE);
	}
	else
	{
		if (sww_state.initializing)
			return;
		if (r_fullscreen->integer)
			ShowWindow (sww_state.hWnd, SW_MINIMIZE);
	}
}

//===============================================================================


/*
================
Sys_MakeCodeWriteable
================
*/
void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{
	DWORD  flOldProtect;

	if (!VirtualProtect((LPVOID)startaddr, length, PAGE_READWRITE, &flOldProtect))
 		Com_FatalError ("Protection change failed\n");
}

/*
** Sys_SetFPCW
**
** For reference:
**
** 1
** 5               0
** xxxxRRPP.xxxxxxxx
**
** PP = 00 = 24-bit single precision
** PP = 01 = reserved
** PP = 10 = 53-bit double precision
** PP = 11 = 64-bit extended precision
**
** RR = 00 = round to nearest
** RR = 01 = round down (towards -inf, floor)
** RR = 10 = round up (towards +inf, ceil)
** RR = 11 = round to zero (truncate/towards 0)
**
*/
#if !id386
void Sys_SetFPCW (void)
{
}
#else
unsigned fpu_ceil_cw, fpu_chop_cw, fpu_full_cw, fpu_cw, fpu_pushed_cw;
unsigned fpu_sp24_cw, fpu_sp24_ceil_cw;

void Sys_SetFPCW( void )
{
	__asm xor eax, eax

	__asm fnstcw  word ptr fpu_cw
	__asm mov ax, word ptr fpu_cw

	__asm and ah, 0f0h
	__asm or  ah, 003h          ; round to nearest mode, extended precision
	__asm mov fpu_full_cw, eax

	__asm and ah, 0f0h
	__asm or  ah, 00fh          ; RTZ/truncate/chop mode, extended precision
	__asm mov fpu_chop_cw, eax

	__asm and ah, 0f0h
	__asm or  ah, 00bh          ; ceil mode, extended precision
	__asm mov fpu_ceil_cw, eax

	__asm and ah, 0f0h          ; round to nearest, 24-bit single precision
	__asm mov fpu_sp24_cw, eax

	__asm and ah, 0f0h          ; ceil mode, 24-bit single precision
	__asm or  ah, 008h          ;
	__asm mov fpu_sp24_ceil_cw, eax
}
#endif
