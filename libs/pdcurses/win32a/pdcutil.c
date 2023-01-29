/* Public Domain Curses */

#include "pdcwin.h"

RCSID("$Id: pdcutil.c,v 1.14 2008/07/14 04:24:52 wmcbrine Exp $")

void PDC_beep(void)
{
    PDC_LOG(("PDC_beep() - called\n"));

/*  MessageBeep(MB_OK); */
    MessageBeep(0XFFFFFFFF);
}

// This used to call Sleep() hence "nap" but that has been removed
// It is abused to process the event loop
void PDC_napms(int ms)
{
    (void)ms;
    /* RR: keep GUI window responsive while PDCurses sleeps */
    MSG msg;

    PDC_LOG(("PDC_napms() - called: ms=%d\n", ms));

    while( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

const char *PDC_sysname(void)
{
    return "Win32a";
}
