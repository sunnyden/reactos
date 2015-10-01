/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/hardware/cmos.c
 * PURPOSE:         CMOS Real Time Clock emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"
#include "cmos.h"

#include "io.h"
#include "pic.h"
#include "clock.h"

/* PRIVATE VARIABLES **********************************************************/

static HANDLE hCmosRam = INVALID_HANDLE_VALUE;
static CMOS_MEMORY CmosMemory;

static BOOLEAN NmiEnabled = TRUE;
static CMOS_REGISTERS SelectedRegister = CMOS_REG_STATUS_D;

static PHARDWARE_TIMER ClockTimer;
static PHARDWARE_TIMER PeriodicTimer;

/* PRIVATE FUNCTIONS **********************************************************/

static VOID RtcUpdatePeriodicTimer(VOID)
{
    BYTE RateSelect = CmosMemory.StatusRegA & 0x0F;

    if (RateSelect == 0)
    {
        /* No periodic interrupt */
        DisableHardwareTimer(PeriodicTimer);
        return;
    }

    /* 1 and 2 act like 8 and 9 */
    if (RateSelect <= 2) RateSelect += 7;

    SetHardwareTimerDelay(PeriodicTimer, HZ_TO_NS(1 << (16 - RateSelect)));
    // FIXME: This call keeps EnableCount increasing without compensating it!
    EnableHardwareTimer(PeriodicTimer);
}

static VOID FASTCALL RtcPeriodicTick(ULONGLONG ElapsedTime)
{
    UNREFERENCED_PARAMETER(ElapsedTime);

    /* Set PF */
    CmosMemory.StatusRegC |= CMOS_STC_PF;

    /* Check if there should be an interrupt on a periodic timer tick */
    if (CmosMemory.StatusRegB & CMOS_STB_INT_PERIODIC)
    {
        CmosMemory.StatusRegC |= CMOS_STC_IRQF;

        /* Interrupt! */
        PicInterruptRequest(RTC_IRQ_NUMBER);
    }
}

/* Should be called every second */
static VOID FASTCALL RtcTimeUpdate(ULONGLONG ElapsedTime)
{
    SYSTEMTIME CurrentTime;

    UNREFERENCED_PARAMETER(ElapsedTime);

    /* Get the current time */
    GetLocalTime(&CurrentTime);

    /* Set UF */
    CmosMemory.StatusRegC |= CMOS_STC_UF;

    /* Check if the time matches the alarm time */
    if ((CurrentTime.wHour   == CmosMemory.AlarmHour  ) &&
        (CurrentTime.wMinute == CmosMemory.AlarmMinute) &&
        (CurrentTime.wSecond == CmosMemory.AlarmSecond))
    {
        /* Set the alarm flag */
        CmosMemory.StatusRegC |= CMOS_STC_AF;

        /* Set IRQF if there should be an interrupt */
        if (CmosMemory.StatusRegB & CMOS_STB_INT_ON_ALARM) CmosMemory.StatusRegC |= CMOS_STC_IRQF;
    }

    /* Check if there should be an interrupt on update */
    if (CmosMemory.StatusRegB & CMOS_STB_INT_ON_UPDATE) CmosMemory.StatusRegC |= CMOS_STC_IRQF;

    if (CmosMemory.StatusRegC & CMOS_STC_IRQF)
    {
        /* Interrupt! */
        PicInterruptRequest(RTC_IRQ_NUMBER);
    }
}

static VOID WINAPI CmosWriteAddress(USHORT Port, BYTE Data)
{
    UNREFERENCED_PARAMETER(Port);

    /* Update the NMI enabled flag */
    NmiEnabled = !(Data & CMOS_DISABLE_NMI);

    /* Get the register number */
    Data &= ~CMOS_DISABLE_NMI;

    if (Data < CMOS_REG_MAX)
    {
        /* Select the new register */
        SelectedRegister = Data;
    }
    else
    {
        /* Default to Status Register D */
        SelectedRegister = CMOS_REG_STATUS_D;
    }
}

static BYTE WINAPI CmosReadData(USHORT Port)
{
    BYTE Value;
    SYSTEMTIME CurrentTime;

    UNREFERENCED_PARAMETER(Port);

    /* Get the current time */
    GetLocalTime(&CurrentTime);

    switch (SelectedRegister)
    {
        case CMOS_REG_SECONDS:
        {
            Value = READ_CMOS_DATA(CmosMemory, CurrentTime.wSecond);
            break;
        }

        case CMOS_REG_ALARM_SEC:
        {
            Value = READ_CMOS_DATA(CmosMemory, CmosMemory.AlarmSecond);
            break;
        }

        case CMOS_REG_MINUTES:
        {
            Value = READ_CMOS_DATA(CmosMemory, CurrentTime.wMinute);
            break;
        }

        case CMOS_REG_ALARM_MIN:
        {
            Value = READ_CMOS_DATA(CmosMemory, CmosMemory.AlarmMinute);
            break;
        }

        case CMOS_REG_HOURS:
        {
            BOOLEAN Afternoon = FALSE;
            Value = CurrentTime.wHour;

            if (!(CmosMemory.StatusRegB & CMOS_STB_24HOUR) && (Value >= 12))
            {
                Value -= 12;
                Afternoon = TRUE;
            }

            Value = READ_CMOS_DATA(CmosMemory, Value);

            /* Convert to 12-hour */
            if (Afternoon) Value |= 0x80;

            break;
        }

        case CMOS_REG_ALARM_HRS:
        {
            BOOLEAN Afternoon = FALSE;
            Value = CmosMemory.AlarmHour;

            if (!(CmosMemory.StatusRegB & CMOS_STB_24HOUR) && (Value >= 12))
            {
                Value -= 12;
                Afternoon = TRUE;
            }

            Value = READ_CMOS_DATA(CmosMemory, Value);

            /* Convert to 12-hour */
            if (Afternoon) Value |= 0x80;

            break;
        }

        case CMOS_REG_DAY_OF_WEEK:
        {
            /*
             * The CMOS value is 1-based but the
             * GetLocalTime API value is 0-based.
             * Correct it.
             */
            Value = READ_CMOS_DATA(CmosMemory, CurrentTime.wDayOfWeek + 1);
            break;
        }

        case CMOS_REG_DAY:
        {
            Value = READ_CMOS_DATA(CmosMemory, CurrentTime.wDay);
            break;
        }

        case CMOS_REG_MONTH:
        {
            Value = READ_CMOS_DATA(CmosMemory, CurrentTime.wMonth);
            break;
        }

        case CMOS_REG_YEAR:
        {
            Value = READ_CMOS_DATA(CmosMemory, CurrentTime.wYear % 100);
            break;
        }

        case CMOS_REG_CENTURY:
        {
            Value = READ_CMOS_DATA(CmosMemory, CurrentTime.wYear / 100 + 19);
            break;
        }

        case CMOS_REG_STATUS_C:
        {
            /* Return the old status register value, then clear it */
            Value = CmosMemory.StatusRegC;
            CmosMemory.StatusRegC = 0x00;
            break;
        }

        case CMOS_REG_STATUS_A:
        case CMOS_REG_STATUS_B:
        case CMOS_REG_STATUS_D:
        case CMOS_REG_DIAGNOSTICS:
        case CMOS_REG_SHUTDOWN_STATUS:
        default:
        {
            // ASSERT(SelectedRegister < CMOS_REG_MAX);
            Value = CmosMemory.Regs[SelectedRegister];
        }
    }

    /* Return to Status Register D */
    SelectedRegister = CMOS_REG_STATUS_D;

    return Value;
}

static VOID WINAPI CmosWriteData(USHORT Port, BYTE Data)
{
    BOOLEAN ChangeTime = FALSE;
    SYSTEMTIME CurrentTime;

    UNREFERENCED_PARAMETER(Port);

    /* Get the current time */
    GetLocalTime(&CurrentTime);

    switch (SelectedRegister)
    {
        case CMOS_REG_SECONDS:
        {
            ChangeTime = TRUE;
            CurrentTime.wSecond = WRITE_CMOS_DATA(CmosMemory, Data);
            break;
        }

        case CMOS_REG_ALARM_SEC:
        {
            CmosMemory.AlarmSecond = WRITE_CMOS_DATA(CmosMemory, Data);
            break;
        }

        case CMOS_REG_MINUTES:
        {
            ChangeTime = TRUE;
            CurrentTime.wMinute = WRITE_CMOS_DATA(CmosMemory, Data);
            break;
        }

        case CMOS_REG_ALARM_MIN:
        {
            CmosMemory.AlarmMinute = WRITE_CMOS_DATA(CmosMemory, Data);
            break;
        }

        case CMOS_REG_HOURS:
        {
            BOOLEAN Afternoon = FALSE;

            ChangeTime = TRUE;

            if (!(CmosMemory.StatusRegB & CMOS_STB_24HOUR) && (Data & 0x80))
            {
                Data &= ~0x80;
                Afternoon = TRUE;
            }

            CurrentTime.wHour = WRITE_CMOS_DATA(CmosMemory, Data);

            /* Convert to 24-hour format */
            if (Afternoon) CurrentTime.wHour += 12;

            break;
        }

        case CMOS_REG_ALARM_HRS:
        {
            BOOLEAN Afternoon = FALSE;

            if (!(CmosMemory.StatusRegB & CMOS_STB_24HOUR) && (Data & 0x80))
            {
                Data &= ~0x80;
                Afternoon = TRUE;
            }

            CmosMemory.AlarmHour = WRITE_CMOS_DATA(CmosMemory, Data);

            /* Convert to 24-hour format */
            if (Afternoon) CmosMemory.AlarmHour += 12;

            break;
        }

        case CMOS_REG_DAY_OF_WEEK:
        {
            ChangeTime = TRUE;
            /*
             * The CMOS value is 1-based but the
             * SetLocalTime API value is 0-based.
             * Correct it.
             */
            Data -= 1;
            CurrentTime.wDayOfWeek = WRITE_CMOS_DATA(CmosMemory, Data);
            break;
        }

        case CMOS_REG_DAY:
        {
            ChangeTime = TRUE;
            CurrentTime.wDay = WRITE_CMOS_DATA(CmosMemory, Data);
            break;
        }

        case CMOS_REG_MONTH:
        {
            ChangeTime = TRUE;
            CurrentTime.wMonth = WRITE_CMOS_DATA(CmosMemory, Data);
            break;
        }

        case CMOS_REG_YEAR:
        {
            ChangeTime = TRUE;

            /* Clear everything except the century */
            CurrentTime.wYear  = (CurrentTime.wYear / 100) * 100;
            CurrentTime.wYear += WRITE_CMOS_DATA(CmosMemory, Data);
            break;
        }

        case CMOS_REG_CENTURY:
        {
            UNIMPLEMENTED;
            break;
        }

        case CMOS_REG_STATUS_A:
        {
            CmosMemory.StatusRegA = Data & 0x7F; // Bit 7 is read-only
            RtcUpdatePeriodicTimer();
            break;
        }

        case CMOS_REG_STATUS_B:
        {
            CmosMemory.StatusRegB = Data;
            break;
        }

        case CMOS_REG_STATUS_C:
        case CMOS_REG_STATUS_D:
            // Status registers C and D are read-only
            break;

        /* Is the following correct? */
        case CMOS_REG_EXT_MEMORY_LOW:
        case CMOS_REG_ACTUAL_EXT_MEMORY_LOW:
        {
            /* Sync EMS and UMS */
            CmosMemory.ExtMemoryLow       =
            CmosMemory.ActualExtMemoryLow = Data;
            break;
        }

        /* Is the following correct? */
        case CMOS_REG_EXT_MEMORY_HIGH:
        case CMOS_REG_ACTUAL_EXT_MEMORY_HIGH:
        {
            /* Sync EMS and UMS */
            CmosMemory.ExtMemoryHigh       =
            CmosMemory.ActualExtMemoryHigh = Data;
            break;
        }

        default:
        {
            CmosMemory.Regs[SelectedRegister] = Data;
        }
    }

    if (ChangeTime) SetLocalTime(&CurrentTime);

    /* Return to Status Register D */
    SelectedRegister = CMOS_REG_STATUS_D;
}


/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN IsNmiEnabled(VOID)
{
    return NmiEnabled;
}

VOID CmosInitialize(VOID)
{
    DWORD CmosSize = sizeof(CmosMemory);

    /* File must not be opened before */
    ASSERT(hCmosRam == INVALID_HANDLE_VALUE);

    /* Clear the CMOS memory */
    RtlZeroMemory(&CmosMemory, sizeof(CmosMemory));

    /* Always open (and if needed, create) a RAM file with shared access */
    SetLastError(0); // For debugging purposes
    hCmosRam = CreateFileW(L"cmos.ram",
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
    DPRINT1("CMOS opening %s (Error: %u)\n", hCmosRam != INVALID_HANDLE_VALUE ? "succeeded" : "failed", GetLastError());

    if (hCmosRam != INVALID_HANDLE_VALUE)
    {
        BOOL Success;

        /* Attempt to fill the CMOS memory with the RAM file */
        SetLastError(0); // For debugging purposes
        Success = ReadFile(hCmosRam, &CmosMemory, CmosSize, &CmosSize, NULL);
        if (CmosSize != sizeof(CmosMemory))
        {
            /* Bad CMOS RAM file. Reinitialize the CMOS memory. */
            DPRINT1("Invalid CMOS file, read bytes %u, expected bytes %u\n", CmosSize, sizeof(CmosMemory));
            RtlZeroMemory(&CmosMemory, sizeof(CmosMemory));
        }
        DPRINT1("CMOS loading %s (Error: %u)\n", Success ? "succeeded" : "failed", GetLastError());
        SetFilePointer(hCmosRam, 0, NULL, FILE_BEGIN);
    }

    /* Overwrite some registers with default values */
    CmosMemory.StatusRegA     = CMOS_DEFAULT_STA;
    CmosMemory.StatusRegB     = CMOS_DEFAULT_STB;
    CmosMemory.StatusRegC     = 0x00;
    CmosMemory.StatusRegD     = CMOS_BATTERY_OK; // Our CMOS battery works perfectly forever.
    CmosMemory.Diagnostics    = 0x00;            // Diagnostics must not find any errors.
    CmosMemory.ShutdownStatus = 0x00;
    CmosMemory.EquipmentList  = CMOS_EQUIPMENT_LIST;

    // HACK: For the moment, set the boot sequence to:  1-Floppy, 2-Hard Disk .
    CmosMemory.Regs[CMOS_REG_SYSOP] |= (1 << 5);

    /* Memory settings */

    /*
     * Conventional memory size is 640 kB,
     * see: http://webpages.charter.net/danrollins/techhelp/0184.HTM
     * and see Ralf Brown: http://www.ctyme.com/intr/rb-0598.htm
     * for more information.
     */
    CmosMemory.BaseMemoryLow  = LOBYTE(0x0280);
    CmosMemory.BaseMemoryHigh = HIBYTE(0x0280);

    CmosMemory.ExtMemoryLow        =
    CmosMemory.ActualExtMemoryLow  = LOBYTE((MAX_ADDRESS - 0x100000) / 1024);
    CmosMemory.ExtMemoryHigh       =
    CmosMemory.ActualExtMemoryHigh = HIBYTE((MAX_ADDRESS - 0x100000) / 1024);

    /* Register the I/O Ports */
    RegisterIoPort(CMOS_ADDRESS_PORT,         NULL, CmosWriteAddress);
    RegisterIoPort(CMOS_DATA_PORT   , CmosReadData, CmosWriteData   );

    ClockTimer = CreateHardwareTimer(HARDWARE_TIMER_ENABLED,
                                     HZ_TO_NS(1),
                                     RtcTimeUpdate);
    PeriodicTimer = CreateHardwareTimer(HARDWARE_TIMER_ENABLED | HARDWARE_TIMER_PRECISE,
                                        HZ_TO_NS(1000),
                                        RtcPeriodicTick);
}

VOID CmosCleanup(VOID)
{
    DWORD CmosSize = sizeof(CmosMemory);

    if (hCmosRam == INVALID_HANDLE_VALUE) return;

    DestroyHardwareTimer(PeriodicTimer);
    DestroyHardwareTimer(ClockTimer);

    /* Flush the CMOS memory back to the RAM file and close it */
    SetFilePointer(hCmosRam, 0, NULL, FILE_BEGIN);
    WriteFile(hCmosRam, &CmosMemory, CmosSize, &CmosSize, NULL);

    CloseHandle(hCmosRam);
    hCmosRam = INVALID_HANDLE_VALUE;
}

/* EOF */
