/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            keyboard.c
 * PURPOSE:         Keyboard emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "keyboard.h"
#include "ps2.h"

/* PRIVATE VARIABLES **********************************************************/

static BYTE PS2Port = 0;

/* PRIVATE FUNCTIONS **********************************************************/

static VOID WINAPI KeyboardCommand(LPVOID Param, BYTE Command)
{
    // FIXME: UNIMPLEMENTED; just return ACKnowledge.
    // This unblocks some programs that want to initialize
    // the keyboard by sending keyboard commands and then
    // performing polling on the port until "valid" data
    // comes out.
    DPRINT1("KeyboardCommand(0x%02X) NOT IMPLEMENTED\n", Command);
    PS2QueuePush(PS2Port, 0xFA);
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID KeyboardEventHandler(PKEY_EVENT_RECORD KeyEvent)
{
    WORD i;
    BYTE ScanCode = (BYTE)KeyEvent->wVirtualScanCode;

    /* If this is a key release, set the highest bit in the scan code */
    if (!KeyEvent->bKeyDown) ScanCode |= 0x80;

    /* Push the scan code into the PS/2 queue */
    for (i = 0; i < KeyEvent->wRepeatCount; i++)
    {
        if (KeyEvent->dwControlKeyState & ENHANCED_KEY) PS2QueuePush(PS2Port, 0xE0);
        PS2QueuePush(PS2Port, ScanCode);
    }

    DPRINT("Press 0x%X\n", ScanCode);
}

BOOLEAN KeyboardInit(BYTE PS2Connector)
{
    /* Finish to plug the keyboard to the specified PS/2 port */
    PS2Port = PS2Connector;
    PS2SetDeviceCmdProc(PS2Port, NULL, KeyboardCommand);

    return TRUE;
}