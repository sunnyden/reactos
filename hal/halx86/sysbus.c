/* $Id: sysbus.c,v 1.2 2002/09/07 15:12:10 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/sysbus.c
 * PURPOSE:         System bus handler functions
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  09/04/2000 Created
 */

/* INCLUDES *****************************************************************/

#include <hal.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS ****************************************************************/

ULONG STDCALL
HalpGetSystemInterruptVector(PVOID BusHandler,
			     ULONG BusNumber,
			     ULONG BusInterruptLevel,
			     ULONG BusInterruptVector,
			     PKIRQL Irql,
			     PKAFFINITY Affinity)
{
   *Irql = HIGH_LEVEL - BusInterruptVector;
   *Affinity = 0xFFFFFFFF;
   return BusInterruptVector;
}


BOOLEAN STDCALL
HalpTranslateSystemBusAddress(PBUS_HANDLER BusHandler,
			      ULONG BusNumber,
			      PHYSICAL_ADDRESS BusAddress,
			      PULONG AddressSpace,
			      PPHYSICAL_ADDRESS TranslatedAddress)
{
   ULONG BaseAddress = 0;

   if (*AddressSpace == 0)
     {
	/* memory space */

     }
   else if (*AddressSpace == 1)
     {
	/* io space */

     }
   else
     {
	/* other */
	return FALSE;
     }

   TranslatedAddress->QuadPart = BusAddress.QuadPart + BaseAddress;

   return TRUE;
}

/* EOF */
