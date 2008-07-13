/*
 * PROJECT:         Ramdisk Class Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/storage/class/ramdisk/ramdisk.c
 * PURPOSE:         Main Driver Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <initguid.h>
#include <ntddk.h>
#include <ntdddisk.h>
#include <scsi.h>
#include <ntddscsi.h>
#include <mountdev.h>
#include <mountmgr.h>
#include <helper.h>
#include <ketypes.h>
#include <iotypes.h>
#include <rtlfuncs.h>
#include <arc/arc.h>
#include <reactos/drivers/ntddrdsk.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

typedef enum _RAMDISK_DEVICE_TYPE
{
    RamdiskFdo,
    RamdiskPdo
} RAMDISK_DEVICE_TYPE;

DEFINE_GUID(RamdiskBusInterface,
		    0x5DC52DF0,
			0x2F8A,
			0x410F,
			0x80, 0xE4, 0x05, 0xF8, 0x10, 0xE7, 0xA8, 0x8A);

typedef struct _RAMDISK_EXTENSION
{
    RAMDISK_DEVICE_TYPE Type;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_OBJECT AttachedDevice;
    IO_REMOVE_LOCK RemoveLock;
    UNICODE_STRING SymbolicLinkName;
    FAST_MUTEX DiskListLock;
    LIST_ENTRY DiskListHead;
} RAMDISK_EXTENSION, *PRAMDISK_EXTENSION;

ULONG MaximumViewLength;
ULONG MaximumPerDiskViewLength;
ULONG ReportDetectedDevice;
ULONG MarkRamdisksAsRemovable;
ULONG MinimumViewCount;
ULONG DefaultViewCount;
ULONG MaximumViewCount;
ULONG MinimumViewLength;
ULONG DefaultViewLength;
UNICODE_STRING DriverRegistryPath;
BOOLEAN ExportBootDiskAsCd;
BOOLEAN IsWinPEBoot;
PDEVICE_OBJECT RamdiskBusFdo;

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
QueryParameters(IN PUNICODE_STRING RegistryPath)
{
    ULONG MinView, DefView, MinViewLength, DefViewLength, MaxViewLength;
    RTL_QUERY_REGISTRY_TABLE QueryTable[10];
    
    //
    // Set defaults
    //
    MaximumViewLength = 0x10000000u;
    MaximumPerDiskViewLength = 0x10000000u;
    ReportDetectedDevice = 0;
    MarkRamdisksAsRemovable = 0;
    MinimumViewCount = 2;
    DefaultViewCount = 16;
    MaximumViewCount = 64;
    MinimumViewLength = 0x10000u;
    DefaultViewLength = 0x100000u;
    
    //
    // Setup the query table and query the registry
    //
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].Flags = 1;
    QueryTable[0].Name = L"Parameters";
    QueryTable[1].Flags = 32;
    QueryTable[1].Name = L"ReportDetectedDevice";
    QueryTable[1].EntryContext = &ReportDetectedDevice;
    QueryTable[2].Flags = 32;
    QueryTable[2].Name = L"MarkRamdisksAsRemovable";
    QueryTable[2].EntryContext = &MarkRamdisksAsRemovable;
    QueryTable[3].Flags = 32;
    QueryTable[3].Name = L"MinimumViewCount";
    QueryTable[3].EntryContext = &MinimumViewCount;
    QueryTable[4].Flags = 32;
    QueryTable[4].Name = L"DefaultViewCount";
    QueryTable[4].EntryContext = &DefaultViewCount;
    QueryTable[5].Flags = 32;
    QueryTable[5].Name = L"MaximumViewCount";
    QueryTable[5].EntryContext = &MaximumViewCount;
    QueryTable[6].Flags = 32;
    QueryTable[6].Name = L"MinimumViewLength";
    QueryTable[6].EntryContext = &MinimumViewLength;
    QueryTable[7].Flags = 32;
    QueryTable[7].Name = L"DefaultViewLength";
    QueryTable[7].EntryContext = &DefaultViewLength;
    QueryTable[8].Flags = 32;
    QueryTable[8].Name = L"MaximumViewLength";
    QueryTable[8].EntryContext = &MaximumViewLength;
    QueryTable[9].Flags = 32;
    QueryTable[9].Name = L"MaximumPerDiskViewLength";
    QueryTable[9].EntryContext = &MaximumPerDiskViewLength;
    RtlQueryRegistryValues(RTL_REGISTRY_OPTIONAL,
                           RegistryPath->Buffer,
                           QueryTable,
                           NULL,
                           NULL);
    
    //
    // Parse minimum view count, cannot be bigger than 256 or smaller than 2
    //
    MinView = MinimumViewCount;
    if (MinimumViewCount >= 2)
    {
        if (MinimumViewCount > 256) MinView = 256;
    }
    else
    {
        MinView = 2;
    }
    MinimumViewCount = MinView;

    //
    // Parse default view count, cannot be bigger than 256 or smaller than minimum
    //
    DefView = DefaultViewCount;
    if (DefaultViewCount >= MinView)
    {
        if (DefaultViewCount > 256) DefView = 256;
    }
    else
    {
        DefView = MinView;
    }
    DefaultViewCount = DefView;
    
    //
    // Parse maximum view count, cannot be bigger than 256 or smaller than default
    //
    if (MaximumViewCount >= DefView)
    {
        if (MaximumViewCount > 256) MaximumViewCount = 256;
    }
    else
    {
        MaximumViewCount = DefView;
    }
    
    //
    // Parse minimum view length, cannot be bigger than 1GB or smaller than 64KB
    //
    MinViewLength = MinimumViewLength;
    if (MinimumViewLength >= 0x10000)
    {
        if (MinimumViewLength > 0x40000000) MinViewLength = 0x40000000u;
    }
    else
    {
        MinViewLength = 0x10000u;
    }
    MinimumViewLength = MinViewLength;

    //
    // Parse default view length, cannot be bigger than 1GB or smaller than minimum
    //
    DefViewLength = DefaultViewLength;
    if (DefaultViewLength >= MinViewLength)
    {
        if (DefaultViewLength > 0x40000000) DefViewLength = 0x40000000u;
    }
    else
    {
        DefViewLength = MinViewLength;
    }
    DefaultViewLength = DefViewLength;

    //
    // Parse maximum view length, cannot be bigger than 1GB or smaller than default
    //
    MaxViewLength = MaximumViewLength;
    if (MaximumViewLength >= DefViewLength)
    {
        if (MaximumViewLength > 0x40000000) MaxViewLength = 0x40000000u;
    }
    else
    {
        MaxViewLength = DefViewLength;
    }
    MaximumViewLength = MaxViewLength;

    //
    // Parse maximum view length per disk, cannot be smaller than 16MB
    //
    if (MaximumPerDiskViewLength >= 0x1000000)
    {
        if (MaxViewLength > 0xFFFFFFFF) MaximumPerDiskViewLength = -1;
    }
    else
    {
        MaximumPerDiskViewLength = 0x1000000u;
    }
}

NTSTATUS
NTAPI
RamdiskOpenClose(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskReadWrite(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskDeviceControl(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskPnp(IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskPower(IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskSystemControl(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskScsi(IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskFlushBuffers(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

VOID
NTAPI
RamdiskUnload(IN PDRIVER_OBJECT DriverObject)
{
    UNIMPLEMENTED;
    while (TRUE);
}

NTSTATUS
NTAPI
RamdiskAddDevice(IN PDRIVER_OBJECT DriverObject, 
                 IN PDEVICE_OBJECT PhysicalDeviceObject)
{
	PRAMDISK_EXTENSION DeviceExtension;
	PDEVICE_OBJECT AttachedDevice;
	NTSTATUS Status; 
	UNICODE_STRING DeviceName;
	PDEVICE_OBJECT DeviceObject;
	
	//
	// Only create the FDO once
	//
	if (RamdiskBusFdo) return STATUS_DEVICE_ALREADY_ATTACHED;
	
	//
	// Create the FDO
	//
	RtlInitUnicodeString(&DeviceName, L"\\Device\\Ramdisk");
	Status = IoCreateDevice(DriverObject,
						    sizeof(RAMDISK_EXTENSION),
							&DeviceName,
							FILE_DEVICE_BUS_EXTENDER,
							FILE_DEVICE_SECURE_OPEN,
							0,
							&DeviceObject);
	if (NT_SUCCESS(Status))
	{
		//
		// Initialize the FDO extension
		//
	    DeviceExtension = (PRAMDISK_EXTENSION)DeviceObject->DeviceExtension;
	    RtlZeroMemory(DeviceObject->DeviceExtension, sizeof(RAMDISK_EXTENSION));

		//
		// Set FDO flags
		//
	    DeviceObject->Flags |= DO_POWER_PAGABLE | DO_DIRECT_IO;

		//
		// Setup the FDO extension
		//
	    DeviceExtension->Type = RamdiskFdo;
		ExInitializeFastMutex(&DeviceExtension->DiskListLock);
	    IoInitializeRemoveLock(&DeviceExtension->RemoveLock,
                               TAG('R', 'a', 'm', 'd'),
                               0,
                               1);
		InitializeListHead(&DeviceExtension->DiskListHead);
	    DeviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;
	    DeviceExtension->DeviceObject = DeviceObject;

		//
		// Register the RAM disk device interface
		//
	    Status = IoRegisterDeviceInterface(PhysicalDeviceObject,
										   &RamdiskBusInterface,
										   NULL,
										   &DeviceExtension->SymbolicLinkName);
	    if (!NT_SUCCESS(Status))
	    {
			//
			// Fail
			//
			IoDeleteDevice(DeviceObject);
			return Status;
	    }

		//
		// Attach us to the device stack
		//
	    AttachedDevice = IoAttachDeviceToDeviceStack(DeviceObject,
													 PhysicalDeviceObject);
	    DeviceExtension->AttachedDevice = AttachedDevice;
	    if (!AttachedDevice)
	    {
			//
			// Fail
			//
			IoSetDeviceInterfaceState(&DeviceExtension->SymbolicLinkName, 0);
			RtlFreeUnicodeString(&DeviceExtension->SymbolicLinkName);
			IoDeleteDevice(DeviceObject);
			return STATUS_NO_SUCH_DEVICE;
	    }

		//
		// FDO is initialized
		//
	    RamdiskBusFdo = DeviceObject;

		//
		// Loop for loader block
		//
	    if (KeLoaderBlock)
	    {
			//
			// Are we being booted from setup? Not yet supported
			//
			ASSERT (!KeLoaderBlock->SetupLdrBlock);
	    }

		//
		// All done
		//
	    DeviceObject->Flags &= DO_DEVICE_INITIALIZING;
	    Status = STATUS_SUCCESS;
	}

	//
	// Return status
	//
	return Status;
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    PCHAR BootDeviceName, CommandLine;
    PDEVICE_OBJECT PhysicalDeviceObject = NULL;
    NTSTATUS Status;
    
    //
    // Query ramdisk parameters
    //
    QueryParameters(RegistryPath);
    
    //
    // Save the registry path
    //
    DriverRegistryPath = *RegistryPath;
    DriverRegistryPath.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                      RegistryPath->Length +
                                                      sizeof(WCHAR),
                                                      TAG('R', 'a', 'm', 'd'));
    if (!DriverRegistryPath.Buffer) return STATUS_INSUFFICIENT_RESOURCES;
    RtlCopyUnicodeString(&DriverRegistryPath, RegistryPath);
    
    //
    // Set device routines
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE] = RamdiskOpenClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = RamdiskOpenClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = RamdiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = RamdiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = RamdiskDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = RamdiskPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = RamdiskPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = RamdiskSystemControl;
    DriverObject->MajorFunction[IRP_MJ_SCSI] = RamdiskScsi;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = RamdiskFlushBuffers;
    DriverObject->DriverExtension->AddDevice = RamdiskAddDevice;
    DriverObject->DriverUnload = RamdiskUnload;
    
    //
    // Check for a loader block
    //
    if (KeLoaderBlock)
    {
        //
        // Get the boot device name
        //
        BootDeviceName = KeLoaderBlock->ArcBootDeviceName;
        if (BootDeviceName)
        {
            //
            // Check if we're booting from ramdisk
            //
            if ((strlen(BootDeviceName) >= 10) &&
                !(_strnicmp(BootDeviceName, "ramdisk(0)", 10)))
            {
                //
                // We'll have to tell the PnP Manager
                //
                ReportDetectedDevice = TRUE;
                
                //
                // Check for a command line
                //
                CommandLine = KeLoaderBlock->LoadOptions;
                if (CommandLine)
                {
                    //
                    // Check if this is an ISO boot
                    //
                    if (strstr(CommandLine, "RDEXPORTASCD"))
                    {
                        //
                        // Remember for later
                        //
                        ExportBootDiskAsCd = TRUE;
                    }
                    
                    //
                    // Check if this is PE boot
                    //
                    if (strstr(CommandLine, "MININT"))
                    {
                        //
                        // Remember for later
                        //
                        IsWinPEBoot = TRUE;
                    }
                }
            }
            
        }
    }
    
    //
    // Installing from Ramdisk isn't supported yet
    //
    ASSERT(!KeLoaderBlock->SetupLdrBlock);
    
    //
    // Are we reporting the device
    //
    if (ReportDetectedDevice)
    {
        //
        // Do it
        //
        Status = IoReportDetectedDevice(DriverObject,
                                        InterfaceTypeUndefined,
                                        0xFFFFFFFF,
                                        0xFFFFFFFF,
                                        NULL,
                                        NULL,
                                        0,
                                        &PhysicalDeviceObject);
        if (NT_SUCCESS(Status))
        {
            //
            // ReactOS Fix
            // The ReactOS Plug and Play Manager is broken and does not create
            // the required keys when reporting a detected device.
            // We hack around this ourselves.
            //
            RtlCreateUnicodeString(&((PEXTENDED_DEVOBJ_EXTENSION)PhysicalDeviceObject->DeviceObjectExtension)->DeviceNode->InstancePath,
                                   L"Root\\UNKNOWN\\0000");
            
            //
            // Create the device object
            //
            Status = RamdiskAddDevice(DriverObject, PhysicalDeviceObject);
            if (NT_SUCCESS(Status))
            {
                //
                // We're done
                //
                PhysicalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
                Status = STATUS_SUCCESS;
            }
        }
    }
    else
    {
        //
        // Done
        //
        Status = STATUS_SUCCESS;
    }

    //
    // Done
    //
    return Status;
}
