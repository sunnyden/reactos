/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/intrface/agpintrf.c
 * PURPOSE:         AGP Interface
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PCI_INTERFACE AgpTargetInterface =
{
    &GUID_AGP_TARGET_BUS_INTERFACE_STANDARD,
    sizeof(AGP_BUS_INTERFACE_STANDARD),
    AGP_BUS_INTERFACE_V1,
    AGP_BUS_INTERFACE_V1,
    PCI_INTERFACE_PDO,
    0,
    PciInterface_AgpTarget,
    agpintrf_Constructor,
    agpintrf_Initializer
};

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
agpintrf_Initializer(IN PVOID Instance)
{
    /* PnP Interfaces don't get Initialized */
    ASSERTMSG(FALSE, "PCI agpintrf_Initializer, unexpected call.");
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
agpintrf_Constructor(IN PVOID DeviceExtension,
                     IN PVOID Instance,
                     IN PVOID InterfaceData,
                     IN USHORT Version,
                     IN USHORT Size,
                     IN PINTERFACE Interface)
{
    PPCI_PDO_EXTENSION PdoExtension = (PPCI_PDO_EXTENSION)DeviceExtension;

    /* Only AGP bridges are supported (which are PCI-to-PCI Bridge Devices) */
    if ((PdoExtension->BaseClass != PCI_CLASS_BRIDGE_DEV) ||
        (PdoExtension->SubClass != PCI_SUBCLASS_BR_PCI_TO_PCI))
    {
        /* Fail any other PDO */
        return STATUS_NOT_SUPPORTED;
    }

    /* Not yet implemented */
    UNIMPLEMENTED;
    ASSERT(FALSE); // while (TRUE);
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
