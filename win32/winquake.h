#ifndef __WINQUAKE_INCLUDED__
#define __WINQUAKE_INCLUDED__

//#define WINVER 0x0500				// to allow some additional stuff in windows headers
#define WIN32_LEAN_AND_MEAN			// exclude rarely-used services from windown headers
#include <windows.h>
#include <mmsystem.h>				// time services, joystick

#include "../qcommon/qcommon.h"

// stuff from Win98+ and Win2000+

#define SPI_GETMOUSESPEED         112
#define SPI_SETMOUSESPEED         113


extern	HINSTANCE	global_hInstance;

extern HWND		cl_hwnd;
extern bool	ActiveApp, Minimized;

// in_win.c
void IN_Activate (bool active);	//?? declared in cl_input.h too
void IN_MouseEvent (int mstate);
extern bool in_needRestart;		//?? used from vid_dll.c::Vid_NewVindow()


// Remove some MS defines from <windows.h>

#undef SHIFT_PRESSED
#undef MOD_ALT


#endif
