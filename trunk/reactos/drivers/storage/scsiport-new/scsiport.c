/*
 *  ReactOS kernel
 *  Copyright (C) 2001, 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/storage/scsiport/scsiport.c
 * PURPOSE:         SCSI port driver
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#include <ntddk.h>
#include <srb.h>
#include <scsi.h>
#include <ntddscsi.h>
#include <ntddstor.h>
#include <ntdddisk.h>
#include <stdio.h>
#include <stdarg.h>

//#define NDEBUG
#include <debug.h>

#include "scsiport_int.h"

/* TYPES *********************************************************************/

#define IRP_FLAG_COMPLETE	0x00000001
#define IRP_FLAG_NEXT		0x00000002
#define IRP_FLAG_NEXT_LU	0x00000004


/* GLOBALS *******************************************************************/

static BOOLEAN
SpiGetPciConfigData (IN struct _HW_INITIALIZATION_DATA *HwInitializationData,
		     IN OUT PPORT_CONFIGURATION_INFORMATION PortConfig,
		     IN ULONG BusNumber,
		     IN OUT PPCI_SLOT_NUMBER NextSlotNumber);

static NTSTATUS STDCALL
ScsiPortCreateClose(IN PDEVICE_OBJECT DeviceObject,
		    IN PIRP Irp);

static NTSTATUS STDCALL
ScsiPortDispatchScsi(IN PDEVICE_OBJECT DeviceObject,
		     IN PIRP Irp);

static NTSTATUS STDCALL
ScsiPortDeviceControl(IN PDEVICE_OBJECT DeviceObject,
		      IN PIRP Irp);

static VOID STDCALL
ScsiPortStartIo(IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp);

static BOOLEAN STDCALL
ScsiPortStartPacket(IN OUT PVOID Context);


static PSCSI_PORT_LUN_EXTENSION
SpiAllocateLunExtension (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension);

static PSCSI_PORT_LUN_EXTENSION
SpiGetLunExtension (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		    IN UCHAR PathId,
		    IN UCHAR TargetId,
		    IN UCHAR Lun);

static NTSTATUS
SpiSendInquiry (IN PDEVICE_OBJECT DeviceObject,
		IN PSCSI_LUN_INFO LunInfo);

static VOID
SpiScanAdapter (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension);

static NTSTATUS
SpiGetInquiryData (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		   IN PIRP Irp);

static PSCSI_REQUEST_BLOCK_INFO
SpiGetSrbData(IN PVOID DeviceExtension,
              IN UCHAR PathId,
              IN UCHAR TargetId,
              IN UCHAR Lun,
              IN UCHAR QueueTag);

static BOOLEAN STDCALL
ScsiPortIsr(IN PKINTERRUPT Interrupt,
	    IN PVOID ServiceContext);

static VOID STDCALL
ScsiPortDpcForIsr(IN PKDPC Dpc,
		  IN PDEVICE_OBJECT DpcDeviceObject,
		  IN PIRP DpcIrp,
		  IN PVOID DpcContext);

static VOID STDCALL
ScsiPortIoTimer(PDEVICE_OBJECT DeviceObject,
		PVOID Context);

#if 0
static PSCSI_REQUEST_BLOCK
ScsiPortInitSenseRequestSrb(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
			    PSCSI_REQUEST_BLOCK OriginalSrb);

static VOID
ScsiPortFreeSenseRequestSrb(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension);
#endif

static NTSTATUS
SpiBuildDeviceMap (PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		   PUNICODE_STRING RegistryPath);

static NTSTATUS
SpiStatusSrbToNt(UCHAR SrbStatus);

static VOID
SpiSendRequestSense(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                    IN PSCSI_REQUEST_BLOCK Srb);

NTSTATUS STDCALL
SpiCompletionRoutine(PDEVICE_OBJECT DeviceObject,
                     PIRP Irp,
                     PVOID Context);

static VOID
STDCALL
SpiProcessCompletedRequest(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                           IN PSCSI_REQUEST_BLOCK_INFO SrbInfo,
                           OUT PBOOLEAN NeedToCallStartIo);

VOID
STDCALL
SpiGetNextRequestFromLun(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                         IN PSCSI_PORT_LUN_EXTENSION LunExtension);


/* FUNCTIONS *****************************************************************/

/**********************************************************************
 * NAME							EXPORTED
 *	DriverEntry
 *
 * DESCRIPTION
 *	This function initializes the driver.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DriverObject
 *		System allocated Driver Object for this driver.
 *
 *	RegistryPath
 *		Name of registry driver service key.
 *
 * RETURN VALUE
 * 	Status.
 */

NTSTATUS STDCALL
DriverEntry(IN PDRIVER_OBJECT DriverObject,
	    IN PUNICODE_STRING RegistryPath)
{
  DPRINT("ScsiPort Driver %s\n", VERSION);
  return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 *	ScsiDebugPrint
 *
 * DESCRIPTION
 *	Prints debugging messages.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DebugPrintLevel
 *		Debug level of the given message.
 *
 *	DebugMessage
 *		Pointer to printf()-compatible format string.
 *
 *	...
  		Additional output data (see printf()).
 *
 * RETURN VALUE
 * 	None.
 *
 * @implemented
 */

VOID
ScsiDebugPrint(IN ULONG DebugPrintLevel,
	       IN PCHAR DebugMessage,
	       ...)
{
  char Buffer[256];
  va_list ap;

#if 0
  if (DebugPrintLevel > InternalDebugLevel)
    return;
#endif

  va_start(ap, DebugMessage);
  vsprintf(Buffer, DebugMessage, ap);
  va_end(ap);

  DbgPrint(Buffer);
}


/*
 * @unimplemented
 */
VOID STDCALL
ScsiPortCompleteRequest(IN PVOID HwDeviceExtension,
			IN UCHAR PathId,
			IN UCHAR TargetId,
			IN UCHAR Lun,
			IN UCHAR SrbStatus)
{
  DPRINT("ScsiPortCompleteRequest()\n");
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID STDCALL
ScsiPortFlushDma(IN PVOID HwDeviceExtension)
{
  DPRINT("ScsiPortFlushDma()\n");
  UNIMPLEMENTED;
}


/*
 * @implemented
 */
VOID STDCALL
ScsiPortFreeDeviceBase(IN PVOID HwDeviceExtension,
		       IN PVOID MappedAddress)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  PSCSI_PORT_DEVICE_BASE DeviceBase;
  PLIST_ENTRY Entry;

  //DPRINT("ScsiPortFreeDeviceBase() called\n");

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      SCSI_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);
  if (IsListEmpty(&DeviceExtension->DeviceBaseListHead))
    return;

  Entry = DeviceExtension->DeviceBaseListHead.Flink;
  while (Entry != &DeviceExtension->DeviceBaseListHead)
    {
      DeviceBase = CONTAINING_RECORD(Entry,
				     SCSI_PORT_DEVICE_BASE,
				     List);
      if (DeviceBase->MappedAddress == MappedAddress)
	{
	  MmUnmapIoSpace(DeviceBase->MappedAddress,
			 DeviceBase->NumberOfBytes);
	  RemoveEntryList(Entry);
	  ExFreePool(DeviceBase);

	  return;
	}

      Entry = Entry->Flink;
    }
}


/*
 * @implemented
 */
ULONG STDCALL
ScsiPortGetBusData(IN PVOID DeviceExtension,
		   IN ULONG BusDataType,
		   IN ULONG SystemIoBusNumber,
		   IN ULONG SlotNumber,
		   IN PVOID Buffer,
		   IN ULONG Length)
{
  return(HalGetBusData(BusDataType,
		       SystemIoBusNumber,
		       SlotNumber,
		       Buffer,
		       Length));
}


/*
 * @implemented
 */
PVOID STDCALL
ScsiPortGetDeviceBase(IN PVOID HwDeviceExtension,
		      IN INTERFACE_TYPE BusType,
		      IN ULONG SystemIoBusNumber,
		      IN SCSI_PHYSICAL_ADDRESS IoAddress,
		      IN ULONG NumberOfBytes,
		      IN BOOLEAN InIoSpace)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  PHYSICAL_ADDRESS TranslatedAddress;
  PSCSI_PORT_DEVICE_BASE DeviceBase;
  ULONG AddressSpace;
  PVOID MappedAddress;

  //DPRINT ("ScsiPortGetDeviceBase() called\n");

  AddressSpace = (ULONG)InIoSpace;
  if (HalTranslateBusAddress(BusType,
			     SystemIoBusNumber,
			     IoAddress,
			     &AddressSpace,
			     &TranslatedAddress) == FALSE)
    return NULL;

  /* i/o space */
  if (AddressSpace != 0)
    return((PVOID)TranslatedAddress.u.LowPart);

  MappedAddress = MmMapIoSpace(TranslatedAddress,
			       NumberOfBytes,
			       FALSE);

  DeviceBase = ExAllocatePool(NonPagedPool,
			      sizeof(SCSI_PORT_DEVICE_BASE));
  if (DeviceBase == NULL)
    return(MappedAddress);

  DeviceBase->MappedAddress = MappedAddress;
  DeviceBase->NumberOfBytes = NumberOfBytes;
  DeviceBase->IoAddress = IoAddress;
  DeviceBase->SystemIoBusNumber = SystemIoBusNumber;

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      SCSI_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);

  InsertHeadList(&DeviceExtension->DeviceBaseListHead,
		 &DeviceBase->List);

  return(MappedAddress);
}


/*
 * @implemented
 */
PVOID STDCALL
ScsiPortGetLogicalUnit(IN PVOID HwDeviceExtension,
		       IN UCHAR PathId,
		       IN UCHAR TargetId,
		       IN UCHAR Lun)
{
    UNIMPLEMENTED;
#if 0
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  PSCSI_PORT_LUN_EXTENSION LunExtension;
  PLIST_ENTRY Entry;

  DPRINT("ScsiPortGetLogicalUnit() called\n");

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      SCSI_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);
  if (IsListEmpty(&DeviceExtension->LunExtensionListHead))
    return NULL;

  Entry = DeviceExtension->LunExtensionListHead.Flink;
  while (Entry != &DeviceExtension->LunExtensionListHead)
    {
      LunExtension = CONTAINING_RECORD(Entry,
				       SCSI_PORT_LUN_EXTENSION,
				       List);
      if (LunExtension->PathId == PathId &&
	  LunExtension->TargetId == TargetId &&
	  LunExtension->Lun == Lun)
	{
	  return (PVOID)&LunExtension->MiniportLunExtension;
	}

      Entry = Entry->Flink;
    }
#endif
  return NULL;
}


/*
 * @unimplemented
 */
SCSI_PHYSICAL_ADDRESS STDCALL
ScsiPortGetPhysicalAddress(IN PVOID HwDeviceExtension,
			   IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
			   IN PVOID VirtualAddress,
			   OUT ULONG *Length)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  SCSI_PHYSICAL_ADDRESS PhysicalAddress;
  SCSI_PHYSICAL_ADDRESS NextPhysicalAddress;
  ULONG BufferLength = 0;
  ULONG Offset;
  PVOID EndAddress;

  DPRINT("ScsiPortGetPhysicalAddress(%p %p %p %p)\n",
	 HwDeviceExtension, Srb, VirtualAddress, Length);

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      SCSI_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);

  *Length = 0;

  if (Srb == NULL)
    {
      if ((ULONG_PTR)DeviceExtension->VirtualAddress > (ULONG_PTR)VirtualAddress)
	{
	  PhysicalAddress.QuadPart = 0ULL;
	  return PhysicalAddress;
	}

      Offset = (ULONG_PTR)VirtualAddress - (ULONG_PTR)DeviceExtension->VirtualAddress;
      if (Offset >= DeviceExtension->CommonBufferLength)
	{
	  PhysicalAddress.QuadPart = 0ULL;
	  return PhysicalAddress;
	}

      PhysicalAddress.QuadPart =
	DeviceExtension->PhysicalAddress.QuadPart + (ULONGLONG)Offset;
      BufferLength = DeviceExtension->CommonBufferLength - Offset;
    }
  else
    {
      EndAddress = (PVOID)((ULONG_PTR)Srb->DataBuffer + Srb->DataTransferLength);
      if (VirtualAddress == NULL)
	{
	  VirtualAddress = Srb->DataBuffer;
	}
      else if (VirtualAddress < Srb->DataBuffer || VirtualAddress >= EndAddress)
	{
	  PhysicalAddress.QuadPart = 0LL;
	  return PhysicalAddress;
	}

      PhysicalAddress = MmGetPhysicalAddress(VirtualAddress);
      if (PhysicalAddress.QuadPart == 0LL)
	{
	  return PhysicalAddress;
	}

      Offset = (ULONG_PTR)VirtualAddress & (PAGE_SIZE - 1);
#if 1
      /* 
       * FIXME:
       *   MmGetPhysicalAddress doesn't return the offset within the page.
       *   We must set the correct offset.
       */
      PhysicalAddress.u.LowPart = (PhysicalAddress.u.LowPart & ~(PAGE_SIZE - 1)) + Offset;
#endif
      BufferLength += PAGE_SIZE - Offset;
      while ((ULONG_PTR)VirtualAddress + BufferLength < (ULONG_PTR)EndAddress)
	{
	  NextPhysicalAddress = MmGetPhysicalAddress((PVOID)((ULONG_PTR)VirtualAddress + BufferLength));
	  if (PhysicalAddress.QuadPart + (ULONGLONG)BufferLength != NextPhysicalAddress.QuadPart)
	    {
	      break;
	    }
	  BufferLength += PAGE_SIZE;
	}
      if ((ULONG_PTR)VirtualAddress + BufferLength >= (ULONG_PTR)EndAddress)
	{
	  BufferLength = (ULONG_PTR)EndAddress - (ULONG_PTR)VirtualAddress;
	}
    }

  *Length = BufferLength;

  return PhysicalAddress;
}


/*
 * @unimplemented
 */
PSCSI_REQUEST_BLOCK STDCALL
ScsiPortGetSrb(IN PVOID DeviceExtension,
	       IN UCHAR PathId,
	       IN UCHAR TargetId,
	       IN UCHAR Lun,
	       IN LONG QueueTag)
{
  DPRINT1("ScsiPortGetSrb() unimplemented\n");
  UNIMPLEMENTED;
  return NULL;
}


/*
 * @implemented
 */
PVOID STDCALL
ScsiPortGetUncachedExtension(IN PVOID HwDeviceExtension,
			     IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
			     IN ULONG NumberOfBytes)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  DEVICE_DESCRIPTION DeviceDescription;

  DPRINT("ScsiPortGetUncachedExtension(%p %p %lu)\n",
	 HwDeviceExtension, ConfigInfo, NumberOfBytes);

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      SCSI_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);

  /* Check for allocated common DMA buffer */
  if (DeviceExtension->VirtualAddress != NULL)
    {
      DPRINT1("The HBA has already got a common DMA buffer!\n");
      return NULL;
    }

  /* Check for DMA adapter object */
  if (DeviceExtension->AdapterObject == NULL)
    {
      /* Initialize DMA adapter description */
      RtlZeroMemory(&DeviceDescription,
		    sizeof(DEVICE_DESCRIPTION));
      DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
      DeviceDescription.Master = ConfigInfo->Master;
      DeviceDescription.ScatterGather = ConfigInfo->ScatterGather;
      DeviceDescription.DemandMode = ConfigInfo->DemandMode;
      DeviceDescription.Dma32BitAddresses = ConfigInfo->Dma32BitAddresses;
      DeviceDescription.BusNumber = ConfigInfo->SystemIoBusNumber;
      DeviceDescription.DmaChannel = ConfigInfo->DmaChannel;
      DeviceDescription.InterfaceType = ConfigInfo->AdapterInterfaceType;
      DeviceDescription.DmaWidth = ConfigInfo->DmaWidth;
      DeviceDescription.DmaSpeed = ConfigInfo->DmaSpeed;
      DeviceDescription.MaximumLength = ConfigInfo->MaximumTransferLength;
      DeviceDescription.DmaPort = ConfigInfo->DmaPort;

      /* Get a DMA adapter object */
      DeviceExtension->AdapterObject = HalGetAdapter(&DeviceDescription,
						     &DeviceExtension->MapRegisterCount);
      if (DeviceExtension->AdapterObject == NULL)
	{
	  DPRINT1("HalGetAdapter() failed\n");
	  return NULL;
	}
    }

  /* Allocate a common DMA buffer */
  DeviceExtension->CommonBufferLength =
    NumberOfBytes + DeviceExtension->SrbExtensionSize;
  DeviceExtension->VirtualAddress =
    HalAllocateCommonBuffer(DeviceExtension->AdapterObject,
			    DeviceExtension->CommonBufferLength,
			    &DeviceExtension->PhysicalAddress,
			    FALSE);
  if (DeviceExtension->VirtualAddress == NULL)
    {
      DPRINT1("HalAllocateCommonBuffer() failed!\n");
      DeviceExtension->CommonBufferLength = 0;
      return NULL;
    }

  return (PVOID)((ULONG_PTR)DeviceExtension->VirtualAddress +
                 DeviceExtension->SrbExtensionSize);
}


/*
 * @implemented
 */
PVOID STDCALL
ScsiPortGetVirtualAddress(IN PVOID HwDeviceExtension,
			  IN SCSI_PHYSICAL_ADDRESS PhysicalAddress)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  ULONG Offset;

  DPRINT("ScsiPortGetVirtualAddress(%p %I64x)\n",
	 HwDeviceExtension, PhysicalAddress.QuadPart);

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      SCSI_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);

  if (DeviceExtension->PhysicalAddress.QuadPart > PhysicalAddress.QuadPart)
    return NULL;

  Offset = (ULONG)(PhysicalAddress.QuadPart - DeviceExtension->PhysicalAddress.QuadPart);
  if (Offset >= DeviceExtension->CommonBufferLength)
    return NULL;

  return (PVOID)((ULONG_PTR)DeviceExtension->VirtualAddress + Offset);
}


/**********************************************************************
 * NAME							EXPORTED
 *	ScsiPortInitialize
 *
 * DESCRIPTION
 *	Initializes SCSI port driver specific data.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	Argument1
 *		Pointer to the miniport driver's driver object.
 *
 *	Argument2
 *		Pointer to the miniport driver's registry path.
 *
 *	HwInitializationData
 *		Pointer to port driver specific configuration data.
 *
 *	HwContext
  		Miniport driver specific context.
 *
 * RETURN VALUE
 * 	Status.
 *
 * @implemented
 */

ULONG STDCALL
ScsiPortInitialize(IN PVOID Argument1,
		   IN PVOID Argument2,
		   IN struct _HW_INITIALIZATION_DATA *HwInitializationData,
		   IN PVOID HwContext)
{
    PDRIVER_OBJECT DriverObject = (PDRIVER_OBJECT)Argument1;
    //  PUNICODE_STRING RegistryPath = (PUNICODE_STRING)Argument2;
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    PCONFIGURATION_INFORMATION SystemConfig;
    PPORT_CONFIGURATION_INFORMATION PortConfig;
    ULONG DeviceExtensionSize;
    ULONG PortConfigSize;
    BOOLEAN Again;
    BOOLEAN DeviceFound = FALSE;
    ULONG i;
    ULONG Result;
    NTSTATUS Status;
    ULONG MaxBus;
    ULONG BusNumber;
    PCI_SLOT_NUMBER SlotNumber;

    PDEVICE_OBJECT PortDeviceObject;
    WCHAR NameBuffer[80];
    UNICODE_STRING DeviceName;
    WCHAR DosNameBuffer[80];
    UNICODE_STRING DosDeviceName;
    PIO_SCSI_CAPABILITIES PortCapabilities;
    ULONG MappedIrq;
    KIRQL Dirql;
    KAFFINITY Affinity;


    DPRINT ("ScsiPortInitialize() called!\n");

    /* Check params for validity */
    if ((HwInitializationData->HwInitialize == NULL) ||
        (HwInitializationData->HwStartIo == NULL) ||
        (HwInitializationData->HwInterrupt == NULL) ||
        (HwInitializationData->HwFindAdapter == NULL) ||
        (HwInitializationData->HwResetBus == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Set handlers */
    DriverObject->DriverStartIo = ScsiPortStartIo;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = ScsiPortCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = ScsiPortCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ScsiPortDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SCSI] = ScsiPortDispatchScsi;

    /* Obtain configuration information */
    SystemConfig = IoGetConfigurationInformation();

    /* Calculate sizes of DeviceExtension and PortConfig */
    DeviceExtensionSize = sizeof(SCSI_PORT_DEVICE_EXTENSION) +
        HwInitializationData->DeviceExtensionSize;
    PortConfigSize = sizeof(PORT_CONFIGURATION_INFORMATION) + 
        HwInitializationData->NumberOfAccessRanges * sizeof(ACCESS_RANGE);


  MaxBus = (HwInitializationData->AdapterInterfaceType == PCIBus) ? 8 : 1;
  DPRINT("MaxBus: %lu\n", MaxBus);

  PortDeviceObject = NULL;
  BusNumber = 0;
  SlotNumber.u.AsULONG = 0;
  while (TRUE)
    {
      /* Create a unicode device name */
      swprintf (NameBuffer,
		L"\\Device\\ScsiPort%lu",
		SystemConfig->ScsiPortCount);
      RtlInitUnicodeString (&DeviceName,
			    NameBuffer);

      DPRINT("Creating device: %wZ\n", &DeviceName);

      /* Create the port device */
      Status = IoCreateDevice (DriverObject,
			       DeviceExtensionSize,
			       &DeviceName,
			       FILE_DEVICE_CONTROLLER,
			       0,
			       FALSE,
			       &PortDeviceObject);
      if (!NT_SUCCESS(Status))
	{
	  DbgPrint ("IoCreateDevice call failed! (Status 0x%lX)\n", Status);
	  PortDeviceObject = NULL;
	  goto ByeBye;
	}

      DPRINT ("Created device: %wZ (%p)\n", &DeviceName, PortDeviceObject);

      /* Set the buffering strategy here... */
      PortDeviceObject->Flags |= DO_DIRECT_IO;
      PortDeviceObject->AlignmentRequirement = FILE_WORD_ALIGNMENT;

      DeviceExtension = PortDeviceObject->DeviceExtension;
      DeviceExtension->Length = DeviceExtensionSize;
      DeviceExtension->DeviceObject = PortDeviceObject;
      DeviceExtension->PortNumber = SystemConfig->ScsiPortCount;

      DeviceExtension->MiniPortExtensionSize = HwInitializationData->DeviceExtensionSize;
      DeviceExtension->LunExtensionSize = HwInitializationData->SpecificLuExtensionSize;
      DeviceExtension->SrbExtensionSize = HwInitializationData->SrbExtensionSize;
      DeviceExtension->HwStartIo = HwInitializationData->HwStartIo;
      DeviceExtension->HwInterrupt = HwInitializationData->HwInterrupt;

#if 0
      DeviceExtension->AdapterObject = NULL;
      DeviceExtension->MapRegisterCount = 0;
      DeviceExtension->PhysicalAddress.QuadPart = 0ULL;
      DeviceExtension->VirtualAddress = NULL;
      DeviceExtension->CommonBufferLength = 0;
#endif

      /* Initialize the device base list */
      InitializeListHead (&DeviceExtension->DeviceBaseListHead);

      /* Initialize array of LUNs */
      RtlZeroMemory(DeviceExtension->LunExtensionList,
          sizeof(PSCSI_PORT_LUN_EXTENSION) * LUS_NUMBER);

      /* Initialize the spin lock in the controller extension */
      KeInitializeSpinLock (&DeviceExtension->IrpLock);
      KeInitializeSpinLock (&DeviceExtension->SpinLock);

      /* Initialize the DPC object */
      IoInitializeDpcRequest (PortDeviceObject,
			      ScsiPortDpcForIsr);

      /* Initialize the device timer */
      DeviceExtension->TimerState = IDETimerIdle;
      DeviceExtension->TimerCount = -1;
      IoInitializeTimer (PortDeviceObject,
			 ScsiPortIoTimer,
			 DeviceExtension);

      /* Allocate and initialize port configuration info */
      DeviceExtension->PortConfig = ExAllocatePool (NonPagedPool,
						    PortConfigSize);
      if (DeviceExtension->PortConfig == NULL)
	{
	  Status = STATUS_INSUFFICIENT_RESOURCES;
	  goto ByeBye;
	}
      RtlZeroMemory (DeviceExtension->PortConfig,
		     PortConfigSize);

      PortConfig = DeviceExtension->PortConfig;
      PortConfig->Length = sizeof(PORT_CONFIGURATION_INFORMATION);
      PortConfig->SystemIoBusNumber = BusNumber;
      PortConfig->AdapterInterfaceType = HwInitializationData->AdapterInterfaceType;
      PortConfig->InterruptMode =
	(PortConfig->AdapterInterfaceType == PCIBus) ? LevelSensitive : Latched;
      PortConfig->MaximumTransferLength = SP_UNINITIALIZED_VALUE;
      PortConfig->NumberOfPhysicalBreaks = SP_UNINITIALIZED_VALUE;
      PortConfig->DmaChannel = SP_UNINITIALIZED_VALUE;
      PortConfig->DmaPort = SP_UNINITIALIZED_VALUE;
//  PortConfig->DmaWidth =
//  PortConfig->DmaSpeed =
//  PortConfig->AlignmentMask =
      PortConfig->NumberOfAccessRanges = HwInitializationData->NumberOfAccessRanges;
    PortConfig->NumberOfBuses = MaxBus;

      for (i = 0; i < SCSI_MAXIMUM_BUSES; i++)
	PortConfig->InitiatorBusId[i] = 255;

//  PortConfig->ScatterGather =
//  PortConfig->Master =
//  PortConfig->CachesData =
//  PortConfig->AdapterScansDown =
      PortConfig->AtdiskPrimaryClaimed = SystemConfig->AtDiskPrimaryAddressClaimed;
      PortConfig->AtdiskSecondaryClaimed = SystemConfig->AtDiskSecondaryAddressClaimed;
//  PortConfig->Dma32BitAddresses =
//  PortConfig->DemandMode =
      PortConfig->MapBuffers = HwInitializationData->MapBuffers;
      PortConfig->NeedPhysicalAddresses = HwInitializationData->NeedPhysicalAddresses;
      PortConfig->TaggedQueuing = HwInitializationData->TaggedQueuing;
      PortConfig->AutoRequestSense = HwInitializationData->AutoRequestSense;
      PortConfig->MultipleRequestPerLu = HwInitializationData->MultipleRequestPerLu;
      PortConfig->ReceiveEvent = HwInitializationData->ReceiveEvent;
//  PortConfig->RealModeInitialized =
//  PortConfig->BufferAccessScsiPortControlled =
      PortConfig->MaximumNumberOfTargets = SCSI_MAXIMUM_TARGETS;
//  PortConfig->MaximumNumberOfLogicalUnits = SCSI_MAXIMUM_LOGICAL_UNITS;

      PortConfig->SlotNumber = SlotNumber.u.AsULONG;

      PortConfig->AccessRanges = (ACCESS_RANGE(*)[])(PortConfig + 1);

      /* Search for matching PCI device */
      if ((HwInitializationData->AdapterInterfaceType == PCIBus) &&
	  (HwInitializationData->VendorIdLength > 0) &&
	  (HwInitializationData->VendorId != NULL) &&
	  (HwInitializationData->DeviceIdLength > 0) &&
	  (HwInitializationData->DeviceId != NULL))
	{
	  /* Get PCI device data */
	  DPRINT("VendorId '%.*s'  DeviceId '%.*s'\n",
		 HwInitializationData->VendorIdLength,
		 HwInitializationData->VendorId,
		 HwInitializationData->DeviceIdLength,
		 HwInitializationData->DeviceId);

	  if (!SpiGetPciConfigData (HwInitializationData,
				    PortConfig,
				    BusNumber,
				    &SlotNumber))
	    {
	      Status = STATUS_UNSUCCESSFUL;
	      goto ByeBye;
	    }
	}

      /* Note: HwFindAdapter is called once for each bus */
      Again = FALSE;
      DPRINT("Calling HwFindAdapter() for Bus %lu\n", PortConfig->SystemIoBusNumber);
      Result = (HwInitializationData->HwFindAdapter)(&DeviceExtension->MiniPortDeviceExtension,
						     HwContext,
						     0,  /* BusInformation */
						     "", /* ArgumentString */
						     PortConfig,
						     &Again);
      DPRINT("HwFindAdapter() Result: %lu  Again: %s\n",
	     Result, (Again) ? "True" : "False");

      if (Result == SP_RETURN_FOUND)
	{
	  DPRINT("ScsiPortInitialize(): Found HBA! (%x)\n", PortConfig->BusInterruptVector);

	  /* Register an interrupt handler for this device */
	  MappedIrq = HalGetInterruptVector(PortConfig->AdapterInterfaceType,
					    PortConfig->SystemIoBusNumber,
					    PortConfig->BusInterruptLevel,
					    PortConfig->BusInterruptVector,
					    &Dirql,
					    &Affinity);
	  Status = IoConnectInterrupt(&DeviceExtension->Interrupt,
				      ScsiPortIsr,
				      DeviceExtension,
				      &DeviceExtension->SpinLock,
				      MappedIrq,
				      Dirql,
				      Dirql,
				      PortConfig->InterruptMode,
				      TRUE,
				      Affinity,
				      FALSE);
	  if (!NT_SUCCESS(Status))
	    {
	      DbgPrint("Could not connect interrupt %d\n",
		       PortConfig->BusInterruptVector);
	      goto ByeBye;
	    }

          /* Set flag that it's allowed to disconnect during this command */
          DeviceExtension->Flags |= SCSI_PORT_DISCONNECT_ALLOWED;

          /* Initialize counter of active requests (-1 means there are none) */
          DeviceExtension->ActiveRequestCounter = -1;

	  if (!(HwInitializationData->HwInitialize)(&DeviceExtension->MiniPortDeviceExtension))
	    {
	      DbgPrint("HwInitialize() failed!");
	      Status = STATUS_UNSUCCESSFUL;
	      goto ByeBye;
	    }

	  /* Initialize port capabilities */
	  DeviceExtension->PortCapabilities = ExAllocatePool(NonPagedPool,
							     sizeof(IO_SCSI_CAPABILITIES));
	  if (DeviceExtension->PortCapabilities == NULL)
	    {
	      DbgPrint("Failed to allocate port capabilities!\n");
	      Status = STATUS_INSUFFICIENT_RESOURCES;
	      goto ByeBye;
	    }

	  PortCapabilities = DeviceExtension->PortCapabilities;
	  PortCapabilities->Length = sizeof(IO_SCSI_CAPABILITIES);
	  PortCapabilities->MaximumTransferLength =
	    PortConfig->MaximumTransferLength;
	  PortCapabilities->MaximumPhysicalPages =
	    PortCapabilities->MaximumTransferLength / PAGE_SIZE;
	  PortCapabilities->SupportedAsynchronousEvents = 0; /* FIXME */
	  PortCapabilities->AlignmentMask =
	    PortConfig->AlignmentMask;
	  PortCapabilities->TaggedQueuing =
	    PortConfig->TaggedQueuing;
	  PortCapabilities->AdapterScansDown =
	    PortConfig->AdapterScansDown;
	  PortCapabilities->AdapterUsesPio = TRUE; /* FIXME */

          /* Initialize bus scanning information */
          DeviceExtension->BusesConfig = ExAllocatePool(PagedPool,
              sizeof(PSCSI_BUS_SCAN_INFO) * DeviceExtension->PortConfig->NumberOfBuses
              + sizeof(ULONG));

          if (!DeviceExtension->BusesConfig)
          {
              DPRINT1("Out of resources!\n");
              Status = STATUS_INSUFFICIENT_RESOURCES;
              goto ByeBye;
          }

          /* Zero it */
          RtlZeroMemory(DeviceExtension->BusesConfig,
              sizeof(PSCSI_BUS_SCAN_INFO) * DeviceExtension->PortConfig->NumberOfBuses
              + sizeof(ULONG));

          /* Store number of buses there */
          DeviceExtension->BusesConfig->NumberOfBuses = DeviceExtension->PortConfig->NumberOfBuses;

	  /* Scan the adapter for devices */
	  SpiScanAdapter (DeviceExtension);

	  /* Build the registry device map */
	  SpiBuildDeviceMap (DeviceExtension,
			     (PUNICODE_STRING)Argument2);

	  /* Create the dos device link */
	  swprintf(DosNameBuffer,
		   L"\\??\\Scsi%lu:",
		   SystemConfig->ScsiPortCount);
	  RtlInitUnicodeString(&DosDeviceName,
			       DosNameBuffer);
	  IoCreateSymbolicLink(&DosDeviceName,
			       &DeviceName);

	  /* Update the system configuration info */
	  if (PortConfig->AtdiskPrimaryClaimed == TRUE)
	    SystemConfig->AtDiskPrimaryAddressClaimed = TRUE;
	  if (PortConfig->AtdiskSecondaryClaimed == TRUE)
	    SystemConfig->AtDiskSecondaryAddressClaimed = TRUE;

	  SystemConfig->ScsiPortCount++;
	  PortDeviceObject = NULL;
	  DeviceFound = TRUE;
	}
      else
	{
	  DPRINT("HwFindAdapter() Result: %lu\n", Result);

	  ExFreePool (PortConfig);
	  IoDeleteDevice (PortDeviceObject);
	  PortDeviceObject = NULL;
	}

      DPRINT("Bus: %lu  MaxBus: %lu\n", BusNumber, MaxBus);
      if (BusNumber >= MaxBus)
	{
	  DPRINT("Scanned all buses!\n");
	  Status = STATUS_SUCCESS;
	  goto ByeBye;
	}

      if (Again == FALSE)
	{
	  BusNumber++;
	  SlotNumber.u.AsULONG = 0;
	}
    }

ByeBye:
  /* Clean up the mess */
  if (PortDeviceObject != NULL)
    {
      DPRINT("Delete device: %p\n", PortDeviceObject);

      DeviceExtension = PortDeviceObject->DeviceExtension;

      if (DeviceExtension->PortCapabilities != NULL)
	{
	  IoDisconnectInterrupt (DeviceExtension->Interrupt);
	  ExFreePool (DeviceExtension->PortCapabilities);
	}

      if (DeviceExtension->PortConfig != NULL)
	{
	  ExFreePool (DeviceExtension->PortConfig);
	}

      IoDeleteDevice (PortDeviceObject);
    }

  DPRINT("ScsiPortInitialize() done, Status = 0x%08X, DeviceFound = %b!\n",
      Status, DeviceFound);

  return (DeviceFound == FALSE) ? Status : STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
VOID STDCALL
ScsiPortIoMapTransfer(IN PVOID HwDeviceExtension,
		      IN PSCSI_REQUEST_BLOCK Srb,
		      IN PVOID LogicalAddress,
		      IN ULONG Length)
{
  DPRINT1("ScsiPortIoMapTransfer()\n");
  UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
VOID STDCALL
ScsiPortLogError(IN PVOID HwDeviceExtension,
		 IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
		 IN UCHAR PathId,
		 IN UCHAR TargetId,
		 IN UCHAR Lun,
		 IN ULONG ErrorCode,
		 IN ULONG UniqueId)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;

  DPRINT1("ScsiPortLogError() called\n");

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      SCSI_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);


  DPRINT("ScsiPortLogError() done\n");
}


/*
 * @implemented
 */
VOID STDCALL
ScsiPortMoveMemory(OUT PVOID Destination,
		   IN PVOID Source,
		   IN ULONG Length)
{
  RtlMoveMemory(Destination,
		Source,
		Length);
}


/*
 * @implemented
 */
VOID
ScsiPortNotification(IN SCSI_NOTIFICATION_TYPE NotificationType,
		     IN PVOID HwDeviceExtension,
		     ...)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    va_list ap;

    DPRINT("ScsiPortNotification() called\n");

    DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
        SCSI_PORT_DEVICE_EXTENSION,
        MiniPortDeviceExtension);

    DPRINT("DeviceExtension %p\n", DeviceExtension);

    va_start(ap, HwDeviceExtension);

    switch (NotificationType)
    {
    case RequestComplete:
        {
            PSCSI_REQUEST_BLOCK Srb;
            PSCSI_REQUEST_BLOCK_INFO SrbData;

            Srb = (PSCSI_REQUEST_BLOCK) va_arg (ap, PSCSI_REQUEST_BLOCK);

            DPRINT("Notify: RequestComplete (Srb %p)\n", Srb);
            //	  DeviceExtension->IrpFlags |= IRP_FLAG_COMPLETE;

            /* Make sure Srb is allright */
            ASSERT(Srb->SrbStatus != SRB_STATUS_PENDING);
            ASSERT(Srb->Function != SRB_FUNCTION_EXECUTE_SCSI || Srb->SrbStatus != SRB_STATUS_SUCCESS || Srb->ScsiStatus == SCSISTAT_GOOD);

            if (!(Srb->SrbFlags & SRB_FLAGS_IS_ACTIVE))
            {
                /* It's been already completed */
                va_end(ap);
                return;
            }

            /* It's not active anymore */
            Srb->SrbFlags &= ~SRB_FLAGS_IS_ACTIVE;

            if (Srb->Function == SRB_FUNCTION_ABORT_COMMAND)
            {
                /* TODO: Treat it specially */
                ASSERT(FALSE);
            }
            else
            {
                /* Get the SRB data */
                SrbData = SpiGetSrbData(DeviceExtension,
                                        Srb->PathId,
                                        Srb->TargetId,
                                        Srb->Lun,
                                        Srb->QueueTag);

                /* Make sure there are no CompletedRequests and there is a Srb */
                ASSERT(SrbData->CompletedRequests == NULL && SrbData->Srb != NULL);

                /* If it's a read/write request, make sure it has data inside it */
                if ((Srb->SrbStatus == SRB_STATUS_SUCCESS) &&
                    ((Srb->Cdb[0] == SCSIOP_READ) || (Srb->Cdb[0] == SCSIOP_WRITE)))
                {
                        ASSERT(Srb->DataTransferLength);
                }

                SrbData->CompletedRequests = DeviceExtension->InterruptData.CompletedRequests;
                DeviceExtension->InterruptData.CompletedRequests = SrbData;
            }
        }
        break;

    case NextRequest:
        DPRINT("Notify: NextRequest\n");
        DeviceExtension->InterruptData.Flags |= SCSI_PORT_NEXT_REQUEST_READY;
        break;

      case NextLuRequest:
	{
	  UCHAR PathId;
	  UCHAR TargetId;
	  UCHAR Lun;

	  PathId = (UCHAR) va_arg (ap, int);
	  TargetId = (UCHAR) va_arg (ap, int);
	  Lun = (UCHAR) va_arg (ap, int);

	  DPRINT1 ("Notify: NextLuRequest(PathId %u  TargetId %u  Lun %u)\n",
		   PathId, TargetId, Lun);
	  /* FIXME: Implement it! */

	  DeviceExtension->IrpFlags |= IRP_FLAG_NEXT;
//	  DeviceExtension->IrpFlags |= IRP_FLAG_NEXT_LU;

	  /* Hack! */
	  DeviceExtension->IrpFlags |= IRP_FLAG_NEXT;
	}
	break;

      case ResetDetected:
	DPRINT1("Notify: ResetDetected\n");
	/* FIXME: ??? */
	break;

      default:
	DPRINT1 ("Unsupported notification %lu\n", NotificationType);
	break;
    }

  va_end(ap);

    /* Request a DPC after we're done with the interrupt */
    DeviceExtension->InterruptData.Flags |= SCSI_PORT_NOTIFICATION_NEEDED;
}


/*
 * @implemented
 */
ULONG STDCALL
ScsiPortSetBusDataByOffset(IN PVOID DeviceExtension,
			   IN ULONG BusDataType,
			   IN ULONG SystemIoBusNumber,
			   IN ULONG SlotNumber,
			   IN PVOID Buffer,
			   IN ULONG Offset,
			   IN ULONG Length)
{
  DPRINT("ScsiPortSetBusDataByOffset()\n");
  return(HalSetBusDataByOffset(BusDataType,
			       SystemIoBusNumber,
			       SlotNumber,
			       Buffer,
			       Offset,
			       Length));
}


/*
 * @implemented
 */
BOOLEAN STDCALL
ScsiPortValidateRange(IN PVOID HwDeviceExtension,
		      IN INTERFACE_TYPE BusType,
		      IN ULONG SystemIoBusNumber,
		      IN SCSI_PHYSICAL_ADDRESS IoAddress,
		      IN ULONG NumberOfBytes,
		      IN BOOLEAN InIoSpace)
{
  DPRINT("ScsiPortValidateRange()\n");
  return(TRUE);
}


/* INTERNAL FUNCTIONS ********************************************************/


static BOOLEAN
SpiGetPciConfigData (IN struct _HW_INITIALIZATION_DATA *HwInitializationData,
		     IN OUT PPORT_CONFIGURATION_INFORMATION PortConfig,
		     IN ULONG BusNumber,
		     IN OUT PPCI_SLOT_NUMBER NextSlotNumber)
{
  PCI_COMMON_CONFIG PciConfig;
  PCI_SLOT_NUMBER SlotNumber;
  ULONG DataSize;
  ULONG DeviceNumber;
  ULONG FunctionNumber;
  CHAR VendorIdString[8];
  CHAR DeviceIdString[8];
  ULONG i;
  ULONG RangeLength;

  DPRINT ("SpiGetPciConfiguration() called\n");

  if (NextSlotNumber->u.bits.FunctionNumber >= PCI_MAX_FUNCTION)
    {
      NextSlotNumber->u.bits.FunctionNumber = 0;
      NextSlotNumber->u.bits.DeviceNumber++;
    }

  if (NextSlotNumber->u.bits.DeviceNumber >= PCI_MAX_DEVICES)
    {
      NextSlotNumber->u.bits.DeviceNumber = 0;
      return FALSE;
    }

  for (DeviceNumber = NextSlotNumber->u.bits.DeviceNumber; DeviceNumber < PCI_MAX_DEVICES; DeviceNumber++)
    {
      SlotNumber.u.bits.DeviceNumber = DeviceNumber;

      for (FunctionNumber = NextSlotNumber->u.bits.FunctionNumber; FunctionNumber < PCI_MAX_FUNCTION; FunctionNumber++)
	{
	  SlotNumber.u.bits.FunctionNumber = FunctionNumber;

	  DataSize = HalGetBusData (PCIConfiguration,
				    BusNumber,
				    SlotNumber.u.AsULONG,
				    &PciConfig,
				    PCI_COMMON_HDR_LENGTH);
	  if (DataSize != PCI_COMMON_HDR_LENGTH)
	    {
	      if (FunctionNumber == 0)
		{
		  break;
		}
	      else
		{
		  continue;
		}
	    }

	  sprintf (VendorIdString, "%04hx", PciConfig.VendorID);
	  sprintf (DeviceIdString, "%04hx", PciConfig.DeviceID);

	  if (!_strnicmp(VendorIdString, HwInitializationData->VendorId, HwInitializationData->VendorIdLength) &&
	      !_strnicmp(DeviceIdString, HwInitializationData->DeviceId, HwInitializationData->DeviceIdLength))
	    {
	      DPRINT ("Found device 0x%04hx 0x%04hx at %1lu %2lu %1lu\n",
		      PciConfig.VendorID,
		      PciConfig.DeviceID,
		      BusNumber,
		      SlotNumber.u.bits.DeviceNumber,
		      SlotNumber.u.bits.FunctionNumber);

	      PortConfig->BusInterruptLevel =
	      PortConfig->BusInterruptVector = PciConfig.u.type0.InterruptLine;
	      PortConfig->SlotNumber = SlotNumber.u.AsULONG;

	      /* Initialize access ranges */
	      if (PortConfig->NumberOfAccessRanges > 0)
		{
		  if (PortConfig->NumberOfAccessRanges > PCI_TYPE0_ADDRESSES)
		    PortConfig->NumberOfAccessRanges = PCI_TYPE0_ADDRESSES;

		  for (i = 0; i < PortConfig->NumberOfAccessRanges; i++)
		    {
		      (*PortConfig->AccessRanges)[i].RangeStart.QuadPart =
			PciConfig.u.type0.BaseAddresses[i] & PCI_ADDRESS_IO_ADDRESS_MASK;
		      if ((*PortConfig->AccessRanges)[i].RangeStart.QuadPart != 0)
			{
			  RangeLength = (ULONG)-1;
			  HalSetBusDataByOffset (PCIConfiguration,
						 BusNumber,
						 SlotNumber.u.AsULONG,
						 (PVOID)&RangeLength,
						 0x10 + (i * sizeof(ULONG)),
						 sizeof(ULONG));

			  HalGetBusDataByOffset (PCIConfiguration,
						 BusNumber,
						 SlotNumber.u.AsULONG,
						 (PVOID)&RangeLength,
						 0x10 + (i * sizeof(ULONG)),
						 sizeof(ULONG));

			  HalSetBusDataByOffset (PCIConfiguration,
						 BusNumber,
						 SlotNumber.u.AsULONG,
						 (PVOID)&PciConfig.u.type0.BaseAddresses[i],
						 0x10 + (i * sizeof(ULONG)),
						 sizeof(ULONG));
			  if (RangeLength != 0)
			    {
			      (*PortConfig->AccessRanges)[i].RangeLength =
			        -(RangeLength & PCI_ADDRESS_IO_ADDRESS_MASK);
			      (*PortConfig->AccessRanges)[i].RangeInMemory =
				!(PciConfig.u.type0.BaseAddresses[i] & PCI_ADDRESS_IO_SPACE);

			      DPRINT("RangeStart 0x%lX  RangeLength 0x%lX  RangeInMemory %s\n",
				     PciConfig.u.type0.BaseAddresses[i] & PCI_ADDRESS_IO_ADDRESS_MASK,
				     -(RangeLength & PCI_ADDRESS_IO_ADDRESS_MASK),
				     (PciConfig.u.type0.BaseAddresses[i] & PCI_ADDRESS_IO_SPACE)?"FALSE":"TRUE");
			    }
			}
		    }
		}

	      NextSlotNumber->u.bits.DeviceNumber = DeviceNumber;
	      NextSlotNumber->u.bits.FunctionNumber = FunctionNumber + 1;

	      return TRUE;
	    }


	  if (FunctionNumber == 0 && !(PciConfig.HeaderType & PCI_MULTIFUNCTION))
	    {
	      break;
	    }
	}
       NextSlotNumber->u.bits.FunctionNumber = 0;
    }

  DPRINT ("No device found\n");

  return FALSE;
}



/**********************************************************************
 * NAME							INTERNAL
 *	ScsiPortCreateClose
 *
 * DESCRIPTION
 *	Answer requests for Create/Close calls: a null operation.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DeviceObject
 *		Pointer to a device object.
 *
 *	Irp
 *		Pointer to an IRP.
 *
 * RETURN VALUE
 * 	Status.
 */

static NTSTATUS STDCALL
ScsiPortCreateClose(IN PDEVICE_OBJECT DeviceObject,
		    IN PIRP Irp)
{
  DPRINT("ScsiPortCreateClose()\n");

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = FILE_OPENED;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							INTERNAL
 *	ScsiPortDispatchScsi
 *
 * DESCRIPTION
 *	Answer requests for SCSI calls
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	Standard dispatch arguments
 *
 * RETURNS
 *	NTSTATUS
 */

static NTSTATUS STDCALL
ScsiPortDispatchScsi(IN PDEVICE_OBJECT DeviceObject,
		     IN PIRP Irp)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    PIO_STACK_LOCATION Stack;
    PSCSI_REQUEST_BLOCK Srb;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("ScsiPortDispatchScsi(DeviceObject %p  Irp %p)\n",
        DeviceObject, Irp);

    DeviceExtension = DeviceObject->DeviceExtension;
    Stack = IoGetCurrentIrpStackLocation(Irp);

    Srb = Stack->Parameters.Scsi.Srb;
    if (Srb == NULL)
    {
        DPRINT1("ScsiPortDispatchScsi() called with Srb = NULL!\n");
        Status = STATUS_UNSUCCESSFUL;

        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return(Status);
    }

    DPRINT("Srb: %p\n", Srb);
    DPRINT("Srb->Function: %lu\n", Srb->Function);
    DPRINT("PathId: %lu  TargetId: %lu  Lun: %lu\n", Srb->PathId, Srb->TargetId, Srb->Lun);

    LunExtension = SpiGetLunExtension(DeviceExtension,
        Srb->PathId,
        Srb->TargetId,
        Srb->Lun);
    if (LunExtension == NULL)
    {
        DPRINT("ScsiPortDispatchScsi() called with an invalid LUN\n");
        Status = STATUS_NO_SUCH_DEVICE;

        Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return(Status);
    }

    switch (Srb->Function)
    {
    case SRB_FUNCTION_SHUTDOWN:
    case SRB_FUNCTION_FLUSH:
        DPRINT ("  SRB_FUNCTION_SHUTDOWN or FLUSH\n");
        if (DeviceExtension->PortConfig->CachesData == FALSE)
        {
            /* All success here */
            Srb->SrbStatus = SRB_STATUS_SUCCESS;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }
        /* Fall through to a usual execute operation */

    case SRB_FUNCTION_EXECUTE_SCSI:
    case SRB_FUNCTION_IO_CONTROL:
        /* Mark IRP as pending in all cases */
        IoMarkIrpPending(Irp);

        if (Srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE)
        {
            /* Start IO directly */
            IoStartPacket(DeviceObject, Irp, NULL, NULL);
        }
        else
        {
            KIRQL oldIrql;

            /* We need to be at DISPATCH_LEVEL */
            KeRaiseIrql (DISPATCH_LEVEL, &oldIrql);

            /* Insert IRP into the queue */
            if (!KeInsertByKeyDeviceQueue (&LunExtension->DeviceQueue,
                &Irp->Tail.Overlay.DeviceQueueEntry,
                Srb->QueueSortKey))
            {
                /* It means queue is empty, and we just start this request */
                IoStartPacket(DeviceObject, Irp, NULL, NULL);
            }

            /* Back to the old IRQL */
            KeLowerIrql (oldIrql);
        }
        return STATUS_PENDING;

    case SRB_FUNCTION_CLAIM_DEVICE:
    case SRB_FUNCTION_ATTACH_DEVICE:
        DPRINT ("  SRB_FUNCTION_CLAIM_DEVICE or ATTACH\n");

        /* Reference device object and keep the device object */
        /* TODO: Check if it's OK */
        ObReferenceObject(DeviceObject);
        LunExtension->DeviceObject = DeviceObject;
        LunExtension->DeviceClaimed = TRUE;
        Srb->DataBuffer = DeviceObject;
        break;

    case SRB_FUNCTION_RELEASE_DEVICE:
        DPRINT ("  SRB_FUNCTION_RELEASE_DEVICE\n");
        /* TODO: Check if it's OK */
        DPRINT ("PathId: %lu  TargetId: %lu  Lun: %lu\n",
            Srb->PathId, Srb->TargetId, Srb->Lun);

        /* Dereference device object and clear the device object */
        ObDereferenceObject(LunExtension->DeviceObject);
        LunExtension->DeviceObject = NULL;
        LunExtension->DeviceClaimed = FALSE;
        break;

    default:
        DPRINT1("SRB function not implemented (Function %lu)\n", Srb->Function);
        Status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    Irp->IoStatus.Status = Status;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return(Status);
}


/**********************************************************************
 * NAME							INTERNAL
 *	ScsiPortDeviceControl
 *
 * DESCRIPTION
 *	Answer requests for device control calls
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	Standard dispatch arguments
 *
 * RETURNS
 *	NTSTATUS
 */

static NTSTATUS STDCALL
ScsiPortDeviceControl(IN PDEVICE_OBJECT DeviceObject,
		      IN PIRP Irp)
{
  PIO_STACK_LOCATION Stack;
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;

  DPRINT("ScsiPortDeviceControl()\n");

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;


  Stack = IoGetCurrentIrpStackLocation(Irp);
  DeviceExtension = DeviceObject->DeviceExtension;

  switch (Stack->Parameters.DeviceIoControl.IoControlCode)
    {
      case IOCTL_SCSI_GET_DUMP_POINTERS:
	{
	  PDUMP_POINTERS DumpPointers;
	  DPRINT("  IOCTL_SCSI_GET_DUMP_POINTERS\n");
	  DumpPointers = (PDUMP_POINTERS)Irp->AssociatedIrp.SystemBuffer;
	  DumpPointers->DeviceObject = DeviceObject;
	  
	  Irp->IoStatus.Information = sizeof(DUMP_POINTERS);
	}
	break;

      case IOCTL_SCSI_GET_CAPABILITIES:
	{
	  DPRINT("  IOCTL_SCSI_GET_CAPABILITIES\n");

	  *((PIO_SCSI_CAPABILITIES *)Irp->AssociatedIrp.SystemBuffer) =
	    DeviceExtension->PortCapabilities;

	  Irp->IoStatus.Information = sizeof(PIO_SCSI_CAPABILITIES);
	}
	break;

      case IOCTL_SCSI_GET_INQUIRY_DATA:
	{
	  DPRINT("  IOCTL_SCSI_GET_INQUIRY_DATA\n");

	  /* Copy inquiry data to the port device extension */
	  Irp->IoStatus.Status = SpiGetInquiryData(DeviceExtension, Irp);
	}
	break;

      default:
	DPRINT1("  unknown ioctl code: 0x%lX\n",
	       Stack->Parameters.DeviceIoControl.IoControlCode);
	break;
    }

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


static VOID STDCALL
ScsiPortStartIo(IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    PIO_STACK_LOCATION IrpStack;
    PSCSI_REQUEST_BLOCK Srb;
    PSCSI_REQUEST_BLOCK_INFO SrbInfo;
    LONG CounterResult;

    DPRINT("ScsiPortStartIo() called!\n");

    DeviceExtension = DeviceObject->DeviceExtension;
    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("DeviceExtension %p\n", DeviceExtension);

    Srb = IrpStack->Parameters.Scsi.Srb;

    /* FIXME: Apply standard flags to Srb->SrbFlags ? */

    /* Get LUN extension */
    LunExtension = SpiGetLunExtension(DeviceExtension,
                                      Srb->PathId,
                                      Srb->TargetId,
                                      Srb->Lun);

    if (DeviceExtension->NeedSrbDataAlloc ||
        DeviceExtension->NeedSrbExtensionAlloc)
    {
        /* TODO: Implement */
        ASSERT(FALSE);
        SrbInfo = NULL;
    }
    else
    {
        /* No allocations are needed */
        SrbInfo = &LunExtension->SrbInfo;
        Srb->SrbExtension = NULL;
        Srb->QueueTag = SP_UNTAGGED;
    }

    /* FIXME: Increase sequence number here of SRB, if it's ever needed */

    /* Check some special SRBs */
    if (Srb->Function == SRB_FUNCTION_ABORT_COMMAND)
    {
        /* Some special handling */
        DPRINT1("Abort command! Unimplemented now\n");
    }
    else
    {
        SrbInfo->Srb = Srb;
    }

    if (Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION)
    {
        // Store the MDL virtual address in SrbInfo structure
        SrbInfo->DataOffset = MmGetMdlVirtualAddress(Irp->MdlAddress);

        if (DeviceExtension->MapBuffers && Irp->MdlAddress)
        {
            /* Calculate offset within DataBuffer */
            SrbInfo->DataOffset = MmGetSystemAddressForMdl(Irp->MdlAddress);
            Srb->DataBuffer = SrbInfo->DataOffset +
                (ULONG)((PUCHAR)Srb->DataBuffer -
                (PUCHAR)MmGetMdlVirtualAddress(Irp->MdlAddress));
        }

        if (DeviceExtension->AdapterObject)
        {
            /* Flush buffers */
            KeFlushIoBuffers(Irp->MdlAddress,
                             Srb->SrbFlags & SRB_FLAGS_DATA_IN ? TRUE : FALSE,
                             TRUE);
        }

        if (DeviceExtension->MapRegisters)
        {
#if 0
            /* Calculate number of needed map registers */
            SrbInfo->NumberOfMapRegisters = ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                    Srb->DataBuffer,
                    Srb->DataTransferLength);

            /* Allocate adapter channel */
            Status = IoAllocateAdapterChannel(DeviceExtension->AdapterObject,
                                              DeviceExtension->DeviceObject,
                                              SrbInfo->NumberOfMapRegisters,
                                              SpiAdapterControl,
                                              SrbInfo);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("IoAllocateAdapterChannel() failed!\n");

                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                ScsiPortNotification(RequestComplete,
                                     DeviceExtension + 1,
                                     Srb);

                ScsiPortNotification(NextRequest,
                                     DeviceExtension + 1);

                /* Request DPC for that work */
                IoRequestDpc(DeviceExtension->DeviceObject, NULL, NULL);
            }

            /* Control goes to SpiAdapterControl */
            return;
#else
            ASSERT(FALSE);
#endif
        }
    }

    /* Increase active request counter */
    CounterResult = InterlockedIncrement(&DeviceExtension->ActiveRequestCounter);

    if (CounterResult == 0 &&
        DeviceExtension->AdapterObject != NULL &&
        !DeviceExtension->MapRegisters)
    {
#if 0
        IoAllocateAdapterChannel(
            DeviceExtension->AdapterObject,
            DeviceObject,
            DeviceExtension->PortCapabilities.MaximumPhysicalPages,
            ScsiPortAllocationRoutine,
            LunExtension
            );

        return;
#else
        /* TODO: DMA is not implemented yet */
        ASSERT(FALSE);
#endif
    }


    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->IrpLock);

    if (!KeSynchronizeExecution(DeviceExtension->Interrupt,
                                ScsiPortStartPacket,
                                DeviceObject))
    {
        DPRINT("Synchronization failed!\n");

        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        Irp->IoStatus.Information = 0;
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->IrpLock);

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->IrpLock);

    DPRINT("ScsiPortStartIo() done\n");
}


static BOOLEAN STDCALL
ScsiPortStartPacket(IN OUT PVOID Context)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IrpStack;
    PSCSI_REQUEST_BLOCK Srb;
    PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)Context;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    BOOLEAN Result;
    BOOLEAN StartTimer;

    DPRINT("ScsiPortStartPacket() called\n");

    DeviceExtension = (PSCSI_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    IrpStack = IoGetCurrentIrpStackLocation(DeviceObject->CurrentIrp);
    Srb = IrpStack->Parameters.Scsi.Srb;

    /* Get LUN extension */
    LunExtension = SpiGetLunExtension(DeviceExtension,
                                      Srb->PathId,
                                      Srb->TargetId,
                                      Srb->Lun);

    /* Check if we are in a reset state */
    if (DeviceExtension->InterruptData.Flags & SCSI_PORT_RESET)
    {
        /* Mark the we've got requests while being in the reset state */
        DeviceExtension->InterruptData.Flags |= SCSI_PORT_RESET_REQUEST;
        return TRUE;
    }

    /* Set the time out value */
    DeviceExtension->TimerCount = Srb->TimeOutValue;

    /* We are busy */
    DeviceExtension->Flags |= SCSI_PORT_DEVICE_BUSY;

    if (LunExtension->RequestTimeout != -1)
    {
        /* Timer already active */
        StartTimer = FALSE;
    }
    else
    {
        /* It hasn't been initialized yet */
        LunExtension->RequestTimeout = Srb->TimeOutValue;
        StartTimer = TRUE;
    }

    if (Srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE)
    {
        /* TODO: Handle bypass-requests */
        ASSERT(FALSE);
    }
    else
    {
        if (Srb->SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT)
        {
            /* It's a disconnect, so no more requests can go */
            DeviceExtension->Flags &= ~SCSI_PORT_DISCONNECT_ALLOWED;
        }

        LunExtension->Flags |= SCSI_PORT_LU_ACTIVE;

        /* Increment queue count */
        LunExtension->QueueCount++;

        /* If it's tagged - special thing */
        if (Srb->QueueTag != SP_UNTAGGED)
        {
            /* TODO: Support tagged requests */
            ASSERT(FALSE);
        }
    }

    /* Mark this Srb active */
    Srb->SrbFlags |= SRB_FLAGS_IS_ACTIVE;

    /* Call HwStartIo routine */
    Result = DeviceExtension->HwStartIo(&DeviceExtension->MiniPortDeviceExtension,
                                             Srb);

    /* If notification is needed, then request a DPC */
    if (DeviceExtension->InterruptData.Flags & SCSI_PORT_NOTIFICATION_NEEDED)
        IoRequestDpc(DeviceExtension->DeviceObject, NULL, NULL);

    return Result;
}

static PSCSI_PORT_LUN_EXTENSION
SpiAllocateLunExtension (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension)
{
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    ULONG LunExtensionSize;

    DPRINT("SpiAllocateLunExtension (%p)\n",
        DeviceExtension);

    /* Round LunExtensionSize first to the sizeof LONGLONG */
    LunExtensionSize = (DeviceExtension->LunExtensionSize +
        sizeof(LONGLONG) - 1) & ~(sizeof(LONGLONG) - 1);

    LunExtensionSize += sizeof(SCSI_PORT_LUN_EXTENSION);
    DPRINT("LunExtensionSize %lu\n", LunExtensionSize);

    LunExtension = ExAllocatePool(NonPagedPool, LunExtensionSize);
    if (LunExtension == NULL)
    {
        DPRINT1("Out of resources!\n");
        return NULL;
    }

    /* Zero everything */
    RtlZeroMemory(LunExtension, LunExtensionSize);

    /* Initialize a list of requests */
    InitializeListHead(&LunExtension->SrbInfo.Requests);

    /* Initialize timeout counter */
    LunExtension->RequestTimeout = -1;

    /* TODO: Initialize other fields */

    /* Initialize request queue */
    KeInitializeDeviceQueue (&LunExtension->DeviceQueue);

    return LunExtension;
}

static PSCSI_PORT_LUN_EXTENSION
SpiGetLunExtension (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		    IN UCHAR PathId,
		    IN UCHAR TargetId,
		    IN UCHAR Lun)
{
    PSCSI_PORT_LUN_EXTENSION LunExtension;

    DPRINT("SpiGetLunExtension(%p %u %u %u) called\n",
        DeviceExtension, PathId, TargetId, Lun);

    /* Get appropriate list */
    LunExtension = DeviceExtension->LunExtensionList[(TargetId + Lun) % LUS_NUMBER];

    /* Iterate it until we find what we need */
    while (LunExtension)
    {
        if (LunExtension->TargetId == TargetId &&
            LunExtension->Lun == Lun &&
            LunExtension->PathId == PathId)
        {
            /* All matches, return */
            return LunExtension;
        }

        /* Advance to the next item */
        LunExtension = LunExtension->Next;
    }

    /* We did not find anything */
    DPRINT("Nothing found\n");
    return NULL;
}


static NTSTATUS
SpiSendInquiry (IN PDEVICE_OBJECT DeviceObject,
		IN PSCSI_LUN_INFO LunInfo)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION IrpStack;
    KEVENT Event;
    KIRQL Irql;
    PIRP Irp;
    NTSTATUS Status;
    PINQUIRYDATA InquiryBuffer;
    PSENSE_DATA SenseBuffer;
    BOOLEAN KeepTrying = TRUE;
    ULONG RetryCount = 0;
    SCSI_REQUEST_BLOCK Srb;
    PCDB Cdb;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;

    DPRINT ("SpiSendInquiry() called\n");

    DeviceExtension = (PSCSI_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    InquiryBuffer = ExAllocatePool (NonPagedPool, INQUIRYDATABUFFERSIZE);
    if (InquiryBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    SenseBuffer = ExAllocatePool (NonPagedPool, SENSE_BUFFER_SIZE);
    if (SenseBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    while (KeepTrying)
    {
        /* Initialize event for waiting */
        KeInitializeEvent(&Event,
                          NotificationEvent,
                          FALSE);

        /* Create an IRP */
        Irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_EXECUTE_IN,
            DeviceObject,
            NULL,
            0,
            InquiryBuffer,
            INQUIRYDATABUFFERSIZE,
            TRUE,
            &Event,
            &IoStatusBlock);
        if (Irp == NULL)
        {
            DPRINT("IoBuildDeviceIoControlRequest() failed\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Prepare SRB */
        RtlZeroMemory(&Srb, sizeof(SCSI_REQUEST_BLOCK));

        Srb.Length = sizeof(SCSI_REQUEST_BLOCK);
        Srb.OriginalRequest = Irp;
        Srb.PathId = LunInfo->PathId;
        Srb.TargetId = LunInfo->TargetId;
        Srb.Lun = LunInfo->Lun;
        Srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
        Srb.SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
        Srb.TimeOutValue = 4;
        Srb.CdbLength = 6;

        Srb.SenseInfoBuffer = SenseBuffer;
        Srb.SenseInfoBufferLength = SENSE_BUFFER_SIZE;

        Srb.DataBuffer = InquiryBuffer;
        Srb.DataTransferLength = INQUIRYDATABUFFERSIZE;

        /* Attach Srb to the Irp */
        IrpStack = IoGetNextIrpStackLocation (Irp);
        IrpStack->Parameters.Scsi.Srb = &Srb;

        /* Fill in CDB */
        Cdb = (PCDB)Srb.Cdb;
        Cdb->CDB6INQUIRY.LogicalUnitNumber = LunInfo->Lun;
        Cdb->CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;

        /* Call the driver */
        Status = IoCallDriver(DeviceObject, Irp);

        /* Wait for it to complete */
        if (Status == STATUS_PENDING)
        {
            DPRINT("SpiSendInquiry(): Waiting for the driver to process request...\n");
            KeWaitForSingleObject(&Event,
                Executive,
                KernelMode,
                FALSE,
                NULL);
            Status = IoStatusBlock.Status;
        }

        DPRINT("SpiSendInquiry(): Request processed by driver, status = 0x%08X\n", Status);

        if (SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_SUCCESS)
        {
            /* All fine, copy data over */
            RtlCopyMemory(LunInfo->InquiryData,
                          InquiryBuffer,
                          INQUIRYDATABUFFERSIZE);

            Status = STATUS_SUCCESS;
            KeepTrying = FALSE;
        }
        else
        {
            /* Check if the queue is frozen */
            if (Srb.SrbStatus & SRB_STATUS_QUEUE_FROZEN)
            {
                /* Something weird happened, deal with it (unfreeze the queue) */
                KeepTrying = FALSE;

                DPRINT("SpiSendInquiry(): the queue is frozen at TargetId %d\n", Srb.TargetId);

                LunExtension = SpiGetLunExtension(DeviceExtension,
                                                  LunInfo->PathId,
                                                  LunInfo->TargetId,
                                                  LunInfo->Lun);

                /* Clear frozen flag */
                LunExtension->Flags &= ~LUNEX_FROZEN_QUEUE;

                /* Acquire the spinlock */
                KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

                /* Process the request */
                SpiGetNextRequestFromLun(DeviceObject->DeviceExtension, LunExtension);

                /* Lower irql back */
                KeLowerIrql(Irql);
            }

            /* Check if data overrun happened */
            if (SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN)
            {
                /* Nothing dramatic, just copy data, but limiting the size */
                RtlCopyMemory(LunInfo->InquiryData,
                              InquiryBuffer,
                              (Srb.DataTransferLength > INQUIRYDATABUFFERSIZE) ?
                              INQUIRYDATABUFFERSIZE : Srb.DataTransferLength);

                Status = STATUS_SUCCESS;
                KeepTrying = FALSE;
            }
            else if ((Srb.SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&
                SenseBuffer->SenseKey == SCSI_SENSE_ILLEGAL_REQUEST)
            {
                /* LUN is not valid, but some device responds there.
                   Mark it as invalid anyway */

                Status = STATUS_INVALID_DEVICE_REQUEST;
                KeepTrying = FALSE;
            }
            else
            {
                /* Retry a couple of times if no timeout happened */
                if ((RetryCount < 2) &&
                    (SRB_STATUS(Srb.SrbStatus) != SRB_STATUS_NO_DEVICE) &&
                    (SRB_STATUS(Srb.SrbStatus) != SRB_STATUS_SELECTION_TIMEOUT))
                {
                    RetryCount++;
                    KeepTrying = TRUE;
                }
                else
                {
                    /* That's all, go to exit */
                    KeepTrying = FALSE;

                    /* Set status according to SRB status */
                    if (SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_BAD_FUNCTION ||
                        SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_BAD_SRB_BLOCK_LENGTH)
                    {
                        Status = STATUS_INVALID_DEVICE_REQUEST;
                    }
                    else
                    {
                        Status = STATUS_IO_DEVICE_ERROR;
                    }
                }
            }
        }
    }

    /* Free buffers */
    ExFreePool(InquiryBuffer);
    ExFreePool(SenseBuffer);

    DPRINT("SpiSendInquiry() done\n");

    return Status;
}


/* Scans all SCSI buses */
static VOID
SpiScanAdapter(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension)
{
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    ULONG Bus;
    ULONG Target;
    ULONG Lun;
    PSCSI_BUS_SCAN_INFO BusScanInfo;
    PSCSI_LUN_INFO LastLunInfo, LunInfo, LunInfoExists;
    BOOLEAN DeviceExists;
    ULONG Hint;
    NTSTATUS Status;
    ULONG DevicesFound;

    DPRINT ("SpiScanAdapter() called\n");

    /* Scan all buses */
    for (Bus = 0; Bus < DeviceExtension->PortConfig->NumberOfBuses; Bus++)
    {
        DevicesFound = 0;

        /* Get pointer to the scan information */
        BusScanInfo = DeviceExtension->BusesConfig->BusScanInfo[Bus];

        if (BusScanInfo)
        {
            /* Find the last LUN info in the list */
            LunInfo = DeviceExtension->BusesConfig->BusScanInfo[Bus]->LunInfo;
            LastLunInfo = LunInfo;

            while (LunInfo != NULL)
            {
                LastLunInfo = LunInfo;
                LunInfo = LunInfo->Next;
            }
        }
        else
        {
            /* We need to allocate this buffer */
            BusScanInfo = ExAllocatePool(NonPagedPool, sizeof(SCSI_BUS_SCAN_INFO));

            if (!BusScanInfo)
            {
                DPRINT1("Out of resources!\n");
                return;
            }

            /* Fill this struct (length and bus ids for now) */
            BusScanInfo->Length = sizeof(SCSI_BUS_SCAN_INFO);
            BusScanInfo->LogicalUnitsCount = 0;
            BusScanInfo->BusIdentifier = DeviceExtension->PortConfig->InitiatorBusId[Bus];
            BusScanInfo->LunInfo = NULL;

            /* Set pointer to the last LUN info to NULL */
            LastLunInfo = NULL;
        }

        /* Create LUN information structure */
        LunInfo = ExAllocatePool(PagedPool, sizeof(SCSI_LUN_INFO));

        if (LunInfo == NULL)
        {
            DPRINT1("Out of resources!\n");
            return;
        }

        RtlZeroMemory(LunInfo, sizeof(SCSI_LUN_INFO));

        /* Create LunExtension */
        LunExtension = SpiAllocateLunExtension (DeviceExtension);

        /* And send INQUIRY to every target */
        for (Target = 0; Target < DeviceExtension->PortConfig->MaximumNumberOfTargets; Target++)
        {
            /* TODO: Support scan bottom-up */

            /* Skip if it's the same address */
            if (Target == BusScanInfo->BusIdentifier)
                continue;

            /* Try to find an existing device here */
            DeviceExists = FALSE;
            LunInfoExists = BusScanInfo->LunInfo;

            /* Find matching address on this bus */
            while (LunInfoExists)
            {
                if (LunInfoExists->TargetId == Target)
                {
                    DeviceExists = TRUE;
                    break;
                }

                /* Advance to the next one */
                LunInfoExists = LunInfoExists->Next;
            }

            /* No need to bother rescanning, since we already did that before */
            if (DeviceExists)
                continue;

            /* Scan all logical units */
            for (Lun = 0; Lun < SCSI_MAXIMUM_LOGICAL_UNITS; Lun++)
            {
                if (!LunExtension)
                    break;

                /* Add extension to the list */
                Hint = (Target + Lun) % LUS_NUMBER;
                LunExtension->Next = DeviceExtension->LunExtensionList[Hint];
                DeviceExtension->LunExtensionList[Hint] = LunExtension;

                /* Fill Path, Target, Lun fields */
                LunExtension->PathId = LunInfo->PathId = Bus;
                LunExtension->TargetId = LunInfo->TargetId = Target;
                LunExtension->Lun = LunInfo->Lun = Lun;

                /* Set flag to prevent race conditions */
                LunExtension->Flags |= SCSI_PORT_SCAN_IN_PROGRESS;

                /* Zero LU extension contents */
                if (DeviceExtension->LunExtensionSize)
                {
                    RtlZeroMemory(LunExtension + 1,
                                  DeviceExtension->LunExtensionSize);
                }

                /* Finally send the inquiry command */
                Status = SpiSendInquiry(DeviceExtension->DeviceObject, LunInfo);

                if (NT_SUCCESS(Status))
                {
                    /* Let's see if we really found a device */
                    PINQUIRYDATA InquiryData = (PINQUIRYDATA)LunInfo->InquiryData;

                    /* Check if this device is unsupported */
                    if (InquiryData->DeviceTypeQualifier == DEVICE_QUALIFIER_NOT_SUPPORTED)
                    {
                        DeviceExtension->LunExtensionList[Hint] = 
                            DeviceExtension->LunExtensionList[Hint]->Next;

                        continue;
                    }

                    /* Clear the "in scan" flag */
                    LunExtension->Flags &= ~SCSI_PORT_SCAN_IN_PROGRESS;

                    DPRINT("SpiScanAdapter(): Found device of type %d at bus %d tid %d lun %d\n",
                        InquiryData->DeviceType, Bus, Target, Lun);

                    /* Add this info to the linked list */
                    LunInfo->Next = NULL;
                    if (LastLunInfo)
                        LastLunInfo->Next = LunInfo;
                    else
                        BusScanInfo->LunInfo = LunInfo;

                    /* Store the last LUN info */
                    LastLunInfo = LunInfo;

                    /* Store DeviceObject */
                    LunInfo->DeviceObject = DeviceExtension->DeviceObject;

                    /* Allocate another buffer */
                    LunInfo = ExAllocatePool(PagedPool, sizeof(SCSI_LUN_INFO));

                    if (!LunInfo)
                    {
                        DPRINT1("Out of resources!\n");
                        break;
                    }

                    RtlZeroMemory(LunInfo, sizeof(SCSI_LUN_INFO));

                    /* Create a new LU extension */
                    LunExtension = SpiAllocateLunExtension(DeviceExtension);

                    DevicesFound++;
                }
                else
                {
                    /* Remove this LUN from the list */
                    DeviceExtension->LunExtensionList[Hint] = 
                        DeviceExtension->LunExtensionList[Hint]->Next;

                    /* Decide whether we are continuing or not */
                    if (Status == STATUS_INVALID_DEVICE_REQUEST)
                        continue;
                    else
                        break;
                }
            }
        }

        /* Free allocated buffers */
        if (LunExtension)
            ExFreePool(LunExtension);

        if (LunInfo)
            ExFreePool(LunInfo);

        /* Sum what we found */
        BusScanInfo->LogicalUnitsCount += DevicesFound;
    }

    DPRINT ("SpiScanAdapter() done\n");
}


static NTSTATUS
SpiGetInquiryData(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		  PIRP Irp)
{
    ULONG InquiryDataSize;
    PSCSI_LUN_INFO LunInfo;
    ULONG BusCount, LunCount, Length;
    PIO_STACK_LOCATION IrpStack;
    PSCSI_ADAPTER_BUS_INFO AdapterBusInfo;
    PSCSI_INQUIRY_DATA InquiryData;
    PSCSI_BUS_DATA BusData;
    ULONG Bus;
    PUCHAR Buffer;

    DPRINT("SpiGetInquiryData() called\n");

    /* Get pointer to the buffer */
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    /* Initialize bus and LUN counters */
    BusCount = DeviceExtension->BusesConfig->NumberOfBuses;
    LunCount = 0;

    /* Calculate total number of LUNs */
    for (Bus = 0; Bus < BusCount; Bus++)
        LunCount += DeviceExtension->BusesConfig->BusScanInfo[Bus]->LogicalUnitsCount;

    /* Calculate size of inquiry data, rounding up to sizeof(ULONG) */
    InquiryDataSize =
        ((sizeof(SCSI_INQUIRY_DATA) - 1 + INQUIRYDATABUFFERSIZE +
        sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1));

    /* Calculate data size */
    Length = sizeof(SCSI_ADAPTER_BUS_INFO) + (BusCount - 1) *
        sizeof(SCSI_BUS_DATA);
    
    Length += InquiryDataSize * LunCount;

    /* Check, if all data is going to fit into provided buffer */
    if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength < Length)
    {
        Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Store data size in the IRP */
    Irp->IoStatus.Information = Length;

    DPRINT("Data size: %lu\n", Length);

    AdapterBusInfo = (PSCSI_ADAPTER_BUS_INFO)Buffer;

    AdapterBusInfo->NumberOfBuses = (UCHAR)BusCount;

    /* Point InquiryData to the corresponding place inside Buffer */
    InquiryData = (PSCSI_INQUIRY_DATA)(Buffer + sizeof(SCSI_ADAPTER_BUS_INFO) +
        (BusCount - 1) * sizeof(SCSI_BUS_DATA));

    /* Loop each bus */
    for (Bus = 0; Bus < BusCount; Bus++)
    {
        BusData = &AdapterBusInfo->BusData[Bus];

        /* Calculate and save an offset of the inquiry data */
        BusData->InquiryDataOffset = (PUCHAR)InquiryData - Buffer;

        /* Get a pointer to the LUN information structure */
        LunInfo = DeviceExtension->BusesConfig->BusScanInfo[Bus]->LunInfo;

        /* Store Initiator Bus Id */
        BusData->InitiatorBusId =
            DeviceExtension->BusesConfig->BusScanInfo[Bus]->BusIdentifier;

        /* Store LUN count */
        BusData->NumberOfLogicalUnits =
            DeviceExtension->BusesConfig->BusScanInfo[Bus]->LogicalUnitsCount;

        /* Loop all LUNs */
        while (LunInfo != NULL)
        {
            DPRINT("(Bus %lu Target %lu Lun %lu)\n",
                Bus, LunInfo->TargetId, LunInfo->Lun);

            /* Fill InquiryData with values */
            InquiryData->PathId = LunInfo->PathId;
            InquiryData->TargetId = LunInfo->TargetId;
            InquiryData->Lun = LunInfo->Lun;
            InquiryData->InquiryDataLength = INQUIRYDATABUFFERSIZE;
            InquiryData->DeviceClaimed = LunInfo->DeviceClaimed;
            InquiryData->NextInquiryDataOffset =
                (PUCHAR)InquiryData + InquiryDataSize - Buffer;

            /* Copy data in it */
            RtlCopyMemory(InquiryData->InquiryData,
                          LunInfo->InquiryData,
                          INQUIRYDATABUFFERSIZE);

            /* Move to the next LUN */
            LunInfo = LunInfo->Next;
            InquiryData = (PSCSI_INQUIRY_DATA) ((PCHAR)InquiryData + InquiryDataSize);
        }

        /* Either mark the end, or set offset to 0 */
        if (BusData->NumberOfLogicalUnits != 0)
            ((PSCSI_INQUIRY_DATA) ((PCHAR) InquiryData - InquiryDataSize))->NextInquiryDataOffset = 0;
        else
            BusData->InquiryDataOffset = 0;
    }

    /* Finish with success */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

static PSCSI_REQUEST_BLOCK_INFO
SpiGetSrbData(IN PVOID DeviceExtension,
              IN UCHAR PathId,
              IN UCHAR TargetId,
              IN UCHAR Lun,
              IN UCHAR QueueTag)
{
    PSCSI_PORT_LUN_EXTENSION LunExtension;

    if (QueueTag == SP_UNTAGGED)
    {
        /* Untagged request, get LU and return pointer to SrbInfo */
        LunExtension = SpiGetLunExtension(DeviceExtension,
                                          PathId,
                                          TargetId,
                                          Lun);

        /* Return NULL in case of error */
        if (!LunExtension)
            return(NULL);

        /* Return the pointer to SrbInfo */
        return &LunExtension->SrbInfo;
    }
    else
    {
        /* TODO: Implement when we have it */
        ASSERT(FALSE);
        return NULL;
    }
}

static VOID
SpiSendRequestSense(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                    IN PSCSI_REQUEST_BLOCK InitialSrb)
{
    PSCSI_REQUEST_BLOCK Srb;
    PCDB Cdb;
    PIRP Irp;
    PIO_STACK_LOCATION IrpStack;
    LARGE_INTEGER LargeInt;
    PVOID *Ptr;

    DPRINT("SpiSendRequestSense() entered\n");

    /* Allocate Srb */
    Srb = ExAllocatePool(NonPagedPool, sizeof(SCSI_REQUEST_BLOCK) + sizeof(PVOID));
    RtlZeroMemory(Srb, sizeof(SCSI_REQUEST_BLOCK));

    /* Allocate IRP */
    LargeInt.QuadPart = (LONGLONG) 1;
    Irp = IoBuildAsynchronousFsdRequest(IRP_MJ_READ,
                                        DeviceExtension->DeviceObject,
                                        InitialSrb->SenseInfoBuffer,
                                        InitialSrb->SenseInfoBufferLength,
                                        &LargeInt,
                                        NULL);

    IoSetCompletionRoutine(Irp,
                           (PIO_COMPLETION_ROUTINE)SpiCompletionRoutine,
                           Srb,
                           TRUE,
                           TRUE,
                           TRUE);

    IrpStack = IoGetNextIrpStackLocation(Irp);
    IrpStack->MajorFunction = IRP_MJ_SCSI;

    /* Put Srb address into Irp... */
    IrpStack->Parameters.Others.Argument1 = (PVOID)Srb;

    /* ...and vice versa */
    Srb->OriginalRequest = Irp;

    /* Save Srb */
    Ptr = (PVOID *)(Srb+1);
    *Ptr = Srb;

    /* Build CDB for REQUEST SENSE */
    Srb->CdbLength = 6;
    Cdb = (PCDB)Srb->Cdb;

    Cdb->CDB6INQUIRY.OperationCode = SCSIOP_REQUEST_SENSE;
    Cdb->CDB6INQUIRY.LogicalUnitNumber = 0;
    Cdb->CDB6INQUIRY.Reserved1 = 0;
    Cdb->CDB6INQUIRY.PageCode = 0;
    Cdb->CDB6INQUIRY.IReserved = 0;
    Cdb->CDB6INQUIRY.AllocationLength = (UCHAR)InitialSrb->SenseInfoBufferLength;
    Cdb->CDB6INQUIRY.Control = 0;

    /* Set address */
    Srb->TargetId = InitialSrb->TargetId;
    Srb->Lun = InitialSrb->Lun;
    Srb->PathId = InitialSrb->PathId;

    Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    Srb->Length = sizeof(SCSI_REQUEST_BLOCK);

    /* Timeout will be 2 seconds */
    Srb->TimeOutValue = 2;

    /* No auto request sense */
    Srb->SenseInfoBufferLength = 0;
    Srb->SenseInfoBuffer = NULL;

    /* Set necessary flags */
    Srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_BYPASS_FROZEN_QUEUE |
                    SRB_FLAGS_DISABLE_DISCONNECT;

    /* Transfer disable synch transfer flag */
    if (InitialSrb->SrbFlags & SRB_FLAGS_DISABLE_SYNCH_TRANSFER)
        Srb->SrbFlags |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    Srb->DataBuffer = InitialSrb->SenseInfoBuffer;

    /* Fill the transfer length */
    Srb->DataTransferLength = InitialSrb->SenseInfoBufferLength;

    /* Clear statuses */
    Srb->ScsiStatus = Srb->SrbStatus = 0;
    Srb->NextSrb = 0;

    /* Call the driver */
    (VOID)IoCallDriver(DeviceExtension->DeviceObject, Irp);

    DPRINT("SpiSendRequestSense() done\n");
}


static
VOID
STDCALL
SpiProcessCompletedRequest(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                           IN PSCSI_REQUEST_BLOCK_INFO SrbInfo,
                           OUT PBOOLEAN NeedToCallStartIo)
{
    PSCSI_REQUEST_BLOCK Srb;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    LONG Result;
    PIRP Irp;
    ULONG SequenceNumber;

    Srb = SrbInfo->Srb;
    Irp = Srb->OriginalRequest;

    /* Get Lun extension */
    LunExtension = SpiGetLunExtension(DeviceExtension,
                                     Srb->PathId,
                                     Srb->TargetId,
                                     Srb->Lun);

    if (Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION &&
        DeviceExtension->MapBuffers &&
        Irp->MdlAddress)
    {
        /* MDL is shared if transfer is broken into smaller parts */
        Srb->DataBuffer = (PCCHAR)MmGetMdlVirtualAddress(Irp->MdlAddress) +
            ((PCCHAR)Srb->DataBuffer - SrbInfo->DataOffset);

        /* In case of data going in, flush the buffers */
        if (Srb->SrbFlags & SRB_FLAGS_DATA_IN)
        {
            KeFlushIoBuffers(Irp->MdlAddress,
                             TRUE,
                             FALSE);
        }
    }


    /* Flush adapter if needed */
    if (SrbInfo->BaseOfMapRegister)
    {
        /* TODO: Implement */
        ASSERT(FALSE);
    }

    /* Clear the request */
    SrbInfo->Srb = NULL;

    /* If disconnect is disabled... */
    if (Srb->SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT)
    {
        /* Acquire the spinlock since we mess with flags */
        KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

        /* Set corresponding flag */
        DeviceExtension->Flags |= SCSI_PORT_DISCONNECT_ALLOWED;

        /* Clear the timer if needed */
        if (!(DeviceExtension->InterruptData.Flags & SCSI_PORT_RESET))
            DeviceExtension->TimerCount = -1;

        /* Spinlock is not needed anymore */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        if (!(DeviceExtension->Flags & SCSI_PORT_REQUEST_PENDING) &&
            !(DeviceExtension->Flags & SCSI_PORT_DEVICE_BUSY) &&
            !(*NeedToCallStartIo))
        {
            /* We're not busy, but we have a request pending */
            IoStartNextPacket(DeviceExtension->DeviceObject, FALSE);
        }
    }

    /* Scatter/gather */
    if (Srb->SrbFlags & SRB_FLAGS_SGLIST_FROM_POOL)
    {
        /* TODO: Implement */
        ASSERT(FALSE);
    }

    /* Acquire spinlock (we're freeing SrbExtension) */
    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

    /* Free it (if needed) */
    if (Srb->SrbExtension)
    {
        if (Srb->SenseInfoBuffer != NULL && DeviceExtension->SupportsAutoSense)
        {
            ASSERT(Srb->SenseInfoBuffer == NULL || SrbInfo->SaveSenseRequest != NULL);

            if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID)
            {
                /* Copy sense data to the buffer */
                RtlCopyMemory(SrbInfo->SaveSenseRequest,
                              Srb->SenseInfoBuffer,
                              Srb->SenseInfoBufferLength);
            }

            /* And restore the pointer */
            Srb->SenseInfoBuffer = SrbInfo->SaveSenseRequest;
        }

        /* Put it into the free srb extensions list */
        *((PVOID *)Srb->SrbExtension) = DeviceExtension->FreeSrbExtensions;
        DeviceExtension->FreeSrbExtensions = Srb->SrbExtension;
    }

    /* Save transfer length in the IRP */
    Irp->IoStatus.Information = Srb->DataTransferLength;

    SequenceNumber = SrbInfo->SequenceNumber;
    SrbInfo->SequenceNumber = 0;

    /* Decrement the queue count */
    LunExtension->QueueCount--;

    /* Free Srb, if needed*/
    if (Srb->QueueTag != SP_UNTAGGED)
    {
        /* Put it into the free list */
        SrbInfo->Requests.Blink = NULL;
        SrbInfo->Requests.Flink = (PLIST_ENTRY)DeviceExtension->FreeSrbInfo;
        DeviceExtension->FreeSrbInfo = SrbInfo;
    }

    /* SrbInfo is not used anymore */
    SrbInfo = NULL;

    if (DeviceExtension->Flags & SCSI_PORT_REQUEST_PENDING)
    {
        /* Clear the flag */
        DeviceExtension->Flags &= ~SCSI_PORT_REQUEST_PENDING;

        /* Note the caller about StartIo */
        *NeedToCallStartIo = TRUE;
    }

    if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_SUCCESS)
    {
        /* Start the packet */
        Irp->IoStatus.Status = STATUS_SUCCESS;

        if (!(Srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE) &&
            LunExtension->RequestTimeout == -1)
        {
            /* Start the next packet */
            SpiGetNextRequestFromLun(DeviceExtension, LunExtension);
        }
        else
        {
            /* Release the spinlock */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
        }

        DPRINT("IoCompleting request IRP 0x%08X", Irp);

        IoCompleteRequest(Irp, IO_DISK_INCREMENT);

        /* Decrement number of active requests, and analyze the result */
        Result = InterlockedDecrement(&DeviceExtension->ActiveRequestCounter);

        if (Result < 0 &&
            !DeviceExtension->MapRegisters &&
            DeviceExtension->AdapterObject != NULL)
        {
            /* Nullify map registers */
            DeviceExtension->MapRegisterBase = NULL;
            IoFreeAdapterChannel(DeviceExtension->AdapterObject);
        }

         /* Exit, we're done */
        return;
    }

    /* Decrement number of active requests, and analyze the result */
    Result = InterlockedDecrement(&DeviceExtension->ActiveRequestCounter);

    if (Result < 0 &&
        !DeviceExtension->MapRegisters &&
        DeviceExtension->AdapterObject != NULL)
    {
        /* Result is negative, so this is a slave, free map registers */
        DeviceExtension->MapRegisterBase = NULL;
        IoFreeAdapterChannel(DeviceExtension->AdapterObject);
    }

    /* Convert status */
    Irp->IoStatus.Status = SpiStatusSrbToNt(Srb->SrbStatus);

    /* It's not a bypass, it's busy or the queue is full? */
    if ((Srb->ScsiStatus == SCSISTAT_BUSY ||
         Srb->SrbStatus == SRB_STATUS_BUSY ||
         Srb->ScsiStatus == SCSISTAT_QUEUE_FULL) &&
         !(Srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE))
    {

        DPRINT("Busy SRB status %x\n", Srb->SrbStatus);

        /* Requeu, if needed */
        if (LunExtension->Flags & (LUNEX_FROZEN_QUEUE | LUNEX_BUSY))
        {
            DPRINT("it's being requeued\n");

            Srb->SrbStatus = SRB_STATUS_PENDING;
            Srb->ScsiStatus = 0;

            if (!KeInsertByKeyDeviceQueue(&LunExtension->DeviceQueue,
                                          &Irp->Tail.Overlay.DeviceQueueEntry,
                                          Srb->QueueSortKey))
            {
                /* It's a big f.ck up if we got here */
                Srb->SrbStatus = SRB_STATUS_ERROR;
                Srb->ScsiStatus = SCSISTAT_BUSY;

                ASSERT(FALSE);
                goto Error;
            }

            /* Release the spinlock */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        }
        else if (LunExtension->AttemptCount++ < 20)
        {
            /* LUN is still busy */
            Srb->ScsiStatus = 0;
            Srb->SrbStatus = SRB_STATUS_PENDING;

            LunExtension->BusyRequest = Irp;
            LunExtension->Flags |= LUNEX_BUSY;

            /* Release the spinlock */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
        }
        else
        {
Error:
            /* Freeze the queue*/
            Srb->SrbStatus |= SRB_STATUS_QUEUE_FROZEN;
            LunExtension->Flags |= LUNEX_FROZEN_QUEUE;

            /* "Unfull" the queue */
            LunExtension->Flags &= ~LUNEX_FULL_QUEUE;

            /* Release the spinlock */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            /* Return status that the device is not ready */
            Irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        }

        return;
    }

    /* Start the next request, if LUN is idle, and this is sense request */
    if (((Srb->ScsiStatus != SCSISTAT_CHECK_CONDITION) ||
        (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) ||
        !Srb->SenseInfoBuffer || !Srb->SenseInfoBufferLength)
        && (Srb->SrbFlags & SRB_FLAGS_NO_QUEUE_FREEZE))
    {
        if (LunExtension->RequestTimeout == -1)
            SpiGetNextRequestFromLun(DeviceExtension, LunExtension);
        else
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }
    else
    {
        /* Freeze the queue */
        Srb->SrbStatus |= SRB_STATUS_QUEUE_FROZEN;
        LunExtension->Flags |= LUNEX_FROZEN_QUEUE;

        /* Do we need a request sense? */
        if (Srb->ScsiStatus == SCSISTAT_CHECK_CONDITION &&
            !(Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&
            Srb->SenseInfoBuffer && Srb->SenseInfoBufferLength)
        {
            /* If LUN is busy, we have to requeue it in order to allow request sense */
            if (LunExtension->Flags & LUNEX_BUSY)
            {
                DPRINT("Requeueing busy request to allow request sense\n");

                if (!KeInsertByKeyDeviceQueue(&LunExtension->DeviceQueue,
                    &LunExtension->BusyRequest->Tail.Overlay.DeviceQueueEntry,
                    Srb->QueueSortKey))
                {
                    /* We should never get here */
                    ASSERT(FALSE);

                    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
                    IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                    return;

                }

                /* Clear busy flags */
                LunExtension->Flags &= ~(LUNEX_FULL_QUEUE | LUNEX_BUSY);
            }

            /* Release the spinlock */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            /* Send RequestSense */
            SpiSendRequestSense(DeviceExtension, Srb);

            /* Exit */
            return;
        }

        /* Release the spinlock */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }

    /* Complete the request */
    IoCompleteRequest(Irp, IO_DISK_INCREMENT);
}

NTSTATUS
STDCALL
SpiCompletionRoutine(PDEVICE_OBJECT DeviceObject,
                     PIRP Irp,
                     PVOID Context)
{
    PSCSI_REQUEST_BLOCK Srb = (PSCSI_REQUEST_BLOCK)Context;
    PSCSI_REQUEST_BLOCK InitialSrb;
    PIRP InitialIrp;

    DPRINT("SpiCompletionRoutine() entered, IRP %p \n", Irp);

    if ((Srb->Function == SRB_FUNCTION_RESET_BUS) ||
        (Srb->Function == SRB_FUNCTION_ABORT_COMMAND))
    {
        /* Deallocate SRB and IRP and exit */
        ExFreePool(Srb);
        IoFreeIrp(Irp);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Get a pointer to the SRB and IRP which were initially sent */
    InitialSrb = *((PVOID *)(Srb+1));
    InitialIrp = InitialSrb->OriginalRequest;

    if ((SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_SUCCESS) ||
        (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN))
    {
        /* Sense data is OK */
        InitialSrb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;

        /* Set length to be the same */
        InitialSrb->SenseInfoBufferLength = (UCHAR)Srb->DataTransferLength;
    }

    /* Make sure initial SRB's queue is frozen */
    ASSERT(InitialSrb->SrbStatus & SRB_STATUS_QUEUE_FROZEN);

    /* Complete this request */
    IoCompleteRequest(InitialIrp, IO_DISK_INCREMENT);

    /* Deallocate everything (internal) */
    ExFreePool(Srb);

    if (Irp->MdlAddress != NULL)
    {
        MmUnlockPages(Irp->MdlAddress);
        IoFreeMdl(Irp->MdlAddress);
        Irp->MdlAddress = NULL;
    }

    IoFreeIrp(Irp);
    return STATUS_MORE_PROCESSING_REQUIRED;
}



static BOOLEAN STDCALL
ScsiPortIsr(IN PKINTERRUPT Interrupt,
            IN PVOID ServiceContext)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    BOOLEAN Result;

    DPRINT("ScsiPortIsr() called!\n");

    DeviceExtension = (PSCSI_PORT_DEVICE_EXTENSION)ServiceContext;

    /* If interrupts are disabled - we don't expect any */
    if (DeviceExtension->InterruptData.Flags & SCSI_PORT_DISABLE_INTERRUPTS)
        return FALSE;

    /* Call miniport's HwInterrupt routine */
    Result = DeviceExtension->HwInterrupt(&DeviceExtension->MiniPortDeviceExtension);

    /* If flag of notification is set - queue a DPC */
    if (DeviceExtension->InterruptData.Flags & SCSI_PORT_NOTIFICATION_NEEDED)
    {
        IoRequestDpc(DeviceExtension->DeviceObject,
                     DeviceExtension->CurrentIrp,
                     DeviceExtension);
    }

    return TRUE;
}

BOOLEAN
STDCALL
SpiSaveInterruptData(IN PVOID Context)
{
    PSCSI_PORT_SAVE_INTERRUPT InterruptContext = Context;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    PSCSI_REQUEST_BLOCK Srb;
    PSCSI_REQUEST_BLOCK_INFO SrbInfo, NextSrbInfo;
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    BOOLEAN IsTimed;

    /* Get pointer to the device extension */
    DeviceExtension = InterruptContext->DeviceExtension;

    /* If we don't have anything pending - return */
    if (!(DeviceExtension->InterruptData.Flags & SCSI_PORT_NOTIFICATION_NEEDED))
        return FALSE;

    /* Actually save the interrupt data */
    *InterruptContext->InterruptData = DeviceExtension->InterruptData;

    /* Clear the data stored in the device extension */
    DeviceExtension->InterruptData.Flags &=
        (SCSI_PORT_RESET | SCSI_PORT_RESET_REQUEST | SCSI_PORT_DISABLE_INTERRUPTS);
    DeviceExtension->InterruptData.CompletedAbort = NULL;
    DeviceExtension->InterruptData.ReadyLun = NULL;
    DeviceExtension->InterruptData.CompletedRequests = NULL;

    /* Loop through the list of completed requests */
    SrbInfo = InterruptContext->InterruptData->CompletedRequests;

    while (SrbInfo)
    {
        /* Make sure we have SRV */
        ASSERT(SrbInfo->Srb);

        /* Get SRB and LunExtension */
        Srb = SrbInfo->Srb;

        LunExtension = SpiGetLunExtension(DeviceExtension,
                                          Srb->PathId,
                                          Srb->TargetId,
                                          Srb->Lun);

        /* We have to check special cases if request is unsuccessfull*/
        if (Srb->SrbStatus != SRB_STATUS_SUCCESS)
        {
            /* Check if we need request sense by a few conditions */
            if (Srb->SenseInfoBuffer && Srb->SenseInfoBufferLength &&
                Srb->ScsiStatus == SCSISTAT_CHECK_CONDITION &&
                !(Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID))
            {
                if (LunExtension->Flags & LUNEX_NEED_REQUEST_SENSE)
                {
                    /* It means: we tried to send REQUEST SENSE, but failed */

                    Srb->ScsiStatus = 0;
                    Srb->SrbStatus = SRB_STATUS_REQUEST_SENSE_FAILED;
                }
                else
                {
                    /* Set the corresponding flag, so that REQUEST SENSE
                       will be sent */
                    LunExtension->Flags |= LUNEX_NEED_REQUEST_SENSE;
                }

            }

            /* Check for a full queue */
            if (Srb->ScsiStatus == SCSISTAT_QUEUE_FULL)
            {
                /* TODO: Implement when it's encountered */
                ASSERT(FALSE);
            }
        }

        /* Let's decide if we need to watch timeout or not */
        if (Srb->QueueTag == SP_UNTAGGED)
        {
            IsTimed = TRUE;
        }
        else
        {
            if (LunExtension->SrbInfo.Requests.Flink == &SrbInfo->Requests)
                IsTimed = TRUE;
            else
                IsTimed = FALSE;

            /* Remove it from the queue */
            RemoveEntryList(&SrbInfo->Requests);
        }

        if (IsTimed)
        {
            /* We have to maintain timeout counter */
            if (IsListEmpty(&LunExtension->SrbInfo.Requests))
            {
                LunExtension->RequestTimeout = -1;
            }
            else
            {
                NextSrbInfo = CONTAINING_RECORD(LunExtension->SrbInfo.Requests.Flink,
                                                SCSI_REQUEST_BLOCK_INFO,
                                                Requests);

                Srb = NextSrbInfo->Srb;

                /* Update timeout counter */
                LunExtension->RequestTimeout = Srb->TimeOutValue;
            }
        }

        SrbInfo = SrbInfo->CompletedRequests;
    }

    return TRUE;
}

VOID
STDCALL
SpiGetNextRequestFromLun(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
                         IN PSCSI_PORT_LUN_EXTENSION LunExtension)
{
    PIO_STACK_LOCATION IrpStack;
    PIRP NextIrp;
    PKDEVICE_QUEUE_ENTRY Entry;
    PSCSI_REQUEST_BLOCK Srb;


    /* If LUN is not active or queue is more than maximum allowed  */
    if (LunExtension->QueueCount >= LunExtension->MaxQueueCount ||
        !(LunExtension->Flags & SCSI_PORT_LU_ACTIVE))
    {
        /* Release the spinlock and exit */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
        return;
    }

    /* Check if we can get a next request */
    if (LunExtension->Flags &
        (LUNEX_NEED_REQUEST_SENSE | LUNEX_BUSY |
         LUNEX_FULL_QUEUE | LUNEX_FROZEN_QUEUE | LUNEX_REQUEST_PENDING))
    {
        /* Pending requests can only be started if the queue is empty */
        if (IsListEmpty(&LunExtension->SrbInfo.Requests) &&
            !(LunExtension->Flags &
              (LUNEX_BUSY | LUNEX_FROZEN_QUEUE | LUNEX_FULL_QUEUE | LUNEX_NEED_REQUEST_SENSE)))
        {
            /* Make sure we have SRB */
            ASSERT(LunExtension->SrbInfo.Srb == NULL);

            /* Clear active and pending flags */
            LunExtension->Flags &= ~(LUNEX_REQUEST_PENDING | SCSI_PORT_LU_ACTIVE);

            /* Get next Irp, and clear pending requests list */
            NextIrp = LunExtension->PendingRequest;
            LunExtension->PendingRequest = NULL;

            /* Set attempt counter to zero */
            LunExtension->AttemptCount = 0;

            /* Release the spinlock */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            /* Start the next pending request */
            IoStartPacket(DeviceExtension->DeviceObject, NextIrp, (PULONG)NULL, NULL);

            return;
        }
        else
        {
            /* Release the spinlock, without clearing any flags and exit */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            return;
        }
    }

    /* Reset active flag */
    LunExtension->Flags &= ~SCSI_PORT_LU_ACTIVE;

    /* Set attempt counter to zero */
    LunExtension->AttemptCount = 0;

    /* Remove packet from the device queue */
    Entry = KeRemoveByKeyDeviceQueue(&LunExtension->DeviceQueue, LunExtension->SortKey);

    if (Entry != NULL)
    {
        /* Get pointer to the next irp */
        NextIrp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.DeviceQueueEntry);

        /* Get point to the SRB */
        IrpStack = IoGetCurrentIrpStackLocation(NextIrp);
        Srb = (PSCSI_REQUEST_BLOCK)IrpStack->Parameters.Others.Argument1;

        /* Set new key*/
        LunExtension->SortKey = Srb->QueueSortKey;
        LunExtension->SortKey++;

        /* Release the spinlock */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        /* Start the next pending request */
        IoStartPacket(DeviceExtension->DeviceObject, NextIrp, (PULONG)NULL, NULL);
    }
    else
    {
        /* Release the spinlock */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }
}



//    ScsiPortDpcForIsr
//  DESCRIPTION:
//
//  RUN LEVEL:
//
//  ARGUMENTS:
//    IN PKDPC          Dpc
//    IN PDEVICE_OBJECT DpcDeviceObject
//    IN PIRP           DpcIrp
//    IN PVOID          DpcContext
//
static VOID STDCALL
ScsiPortDpcForIsr(IN PKDPC Dpc,
		  IN PDEVICE_OBJECT DpcDeviceObject,
		  IN PIRP DpcIrp,
		  IN PVOID DpcContext)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension = DpcDeviceObject->DeviceExtension;
    SCSI_PORT_INTERRUPT_DATA InterruptData;
    SCSI_PORT_SAVE_INTERRUPT Context;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    BOOLEAN NeedToStartIo;
    PSCSI_REQUEST_BLOCK_INFO SrbInfo;

    DPRINT("ScsiPortDpcForIsr(Dpc %p  DpcDeviceObject %p  DpcIrp %p  DpcContext %p)\n",
           Dpc, DpcDeviceObject, DpcIrp, DpcContext);

    /* We need to acquire spinlock */
    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

TryAgain:

    /* Interrupt structure must be snapshotted, and only then analyzed */
    Context.InterruptData = &InterruptData;
    Context.DeviceExtension = DeviceExtension;

    if (!KeSynchronizeExecution(DeviceExtension->Interrupt,
                                SpiSaveInterruptData,
                                &Context))
    {
        /* Nothing - just return (don't forget to release the spinlock */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
        DPRINT("ScsiPortDpcForIsr() done\n");
        return;
    }

    /* If flush of adapters is needed - do it */
    if (InterruptData.Flags & SCSI_PORT_FLUSH_ADAPTERS)
    {
        /* TODO: Implement */
        ASSERT(FALSE);
    }

    /* Check for IoMapTransfer */
    if (InterruptData.Flags & SCSI_PORT_MAP_TRANSFER)
    {
        /* TODO: Implement */
        ASSERT(FALSE);
    }

    /* Check if timer is needed */
    if (InterruptData.Flags & SCIS_PORT_TIMER_NEEDED)
    {
        /* TODO: Implement */
        ASSERT(FALSE);
    }

    /* If it's ready for the next request */
    if (InterruptData.Flags & SCSI_PORT_NEXT_REQUEST_READY)
    {
        /* Check for a duplicate request (NextRequest+NextLuRequest) */
        if ((DeviceExtension->Flags &
            (SCSI_PORT_DEVICE_BUSY | SCSI_PORT_DISCONNECT_ALLOWED)) ==
            (SCSI_PORT_DEVICE_BUSY | SCSI_PORT_DISCONNECT_ALLOWED))
        {
            /* Clear busy flag set by ScsiPortStartPacket() */
            DeviceExtension->Flags &= ~SCSI_PORT_DEVICE_BUSY;

            if (!(InterruptData.Flags & SCSI_PORT_RESET))
            {
                /* Ready for next, and no reset is happening */
                DeviceExtension->TimerCount = -1;
            }
        }
        else
        {
            /* Not busy, but not ready for the next request */
            DeviceExtension->Flags &= ~SCSI_PORT_DEVICE_BUSY;
            InterruptData.Flags &= ~SCSI_PORT_NEXT_REQUEST_READY;
        }
    }

    /* Any resets? */
    if (InterruptData.Flags & SCSI_PORT_RESET_REPORTED)
    {
        /* Hold for a bit */
        DeviceExtension->TimerCount = 4;
    }

    /* Any ready LUN? */
    if (InterruptData.ReadyLun != NULL)
    {

        /* Process all LUNs from the list*/
        while (TRUE)
        {
            /* Remove it from the list first (as processed) */
            LunExtension = InterruptData.ReadyLun;
            InterruptData.ReadyLun = LunExtension->ReadyLun;
            LunExtension->ReadyLun = NULL;

            /* Get next request for this LUN */
            SpiGetNextRequestFromLun(DeviceExtension, LunExtension);

            /* Still ready requests exist?
               If yes - get spinlock, if no - stop here */
            if (InterruptData.ReadyLun != NULL)
                KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);
            else
                break;
        }
    }
    else
    {
        /* Release the spinlock */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }

    /* If we ready for next packet, start it */
    if (InterruptData.Flags & SCSI_PORT_NEXT_REQUEST_READY)
        IoStartNextPacket(DeviceExtension->DeviceObject, FALSE);

    NeedToStartIo = FALSE;

    /* Loop the completed request list */
    while (InterruptData.CompletedRequests)
    {
        /* Remove the request */
        SrbInfo = InterruptData.CompletedRequests;
        InterruptData.CompletedRequests = SrbInfo->CompletedRequests;
        SrbInfo->CompletedRequests = NULL;

        /* Process it */
        SpiProcessCompletedRequest(DeviceExtension,
                                  SrbInfo,
                                  &NeedToStartIo);
    }

    /* Loop abort request list */
    while (InterruptData.CompletedAbort)
    {
        LunExtension = InterruptData.CompletedAbort;

        /* Remove the request */
        InterruptData.CompletedAbort = LunExtension->CompletedAbortRequests;

        /* Get spinlock since we're going to change flags */
        KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

        /* TODO: Put SrbExtension to the list of free extensions */
        ASSERT(FALSE);
    }

    /* If we need - call StartIo routine */
    if (NeedToStartIo)
    {
        /* Make sure CurrentIrp is not null! */
        ASSERT(DpcDeviceObject->CurrentIrp != NULL);
        ScsiPortStartIo(DpcDeviceObject, DpcDeviceObject->CurrentIrp);
    }

    /* Everything has been done, check */
    if (InterruptData.Flags & SCSI_PORT_ENABLE_INT_REQUEST)
    {
        /* Synchronize using spinlock */
        KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

        /* Request an interrupt */
        DeviceExtension->HwInterrupt(DeviceExtension->MiniPortDeviceExtension);

        ASSERT(DeviceExtension->Flags & SCSI_PORT_DISABLE_INT_REQUESET);

        /* Should interrupts be enabled again? */
        if (DeviceExtension->Flags & SCSI_PORT_DISABLE_INT_REQUESET)
        {
            /* Clear this flag */
            DeviceExtension->Flags &= ~SCSI_PORT_DISABLE_INT_REQUESET;

            /* Call a special routine to do this */
            ASSERT(FALSE);
#if 0
            KeSynchronizeExecution(DeviceExtension->Interrupt,
                                   SpiEnableInterrupts,
                                   DeviceExtension);
#endif
        }

        /* If we need a notification again - loop */
        if (DeviceExtension->InterruptData.Flags & SCSI_PORT_NOTIFICATION_NEEDED)
            goto TryAgain;

        /* Release the spinlock */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }

    DPRINT("ScsiPortDpcForIsr() done\n");
}


//    ScsiPortIoTimer
//  DESCRIPTION:
//    This function handles timeouts and other time delayed processing
//
//  RUN LEVEL:
//
//  ARGUMENTS:
//    IN  PDEVICE_OBJECT  DeviceObject  Device object registered with timer
//    IN  PVOID           Context       the Controller extension for the
//                                      controller the device is on
//
static VOID STDCALL
ScsiPortIoTimer(PDEVICE_OBJECT DeviceObject,
		PVOID Context)
{
  DPRINT1("ScsiPortIoTimer()\n");
}

#if 0
static PSCSI_REQUEST_BLOCK
ScsiPortInitSenseRequestSrb(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
			    PSCSI_REQUEST_BLOCK OriginalSrb)
{
  PSCSI_REQUEST_BLOCK Srb;
  PCDB Cdb;

  Srb = &DeviceExtension->InternalSrb;

  RtlZeroMemory(Srb,
		sizeof(SCSI_REQUEST_BLOCK));

  Srb->PathId = OriginalSrb->PathId;
  Srb->TargetId = OriginalSrb->TargetId;
  Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
  Srb->Length = sizeof(SCSI_REQUEST_BLOCK);
  Srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

  Srb->TimeOutValue = 4;

  Srb->CdbLength = 6;
  Srb->DataBuffer = &DeviceExtension->InternalSenseData;
  Srb->DataTransferLength = sizeof(SENSE_DATA);

  Cdb = (PCDB)Srb->Cdb;
  Cdb->CDB6INQUIRY.OperationCode = SCSIOP_REQUEST_SENSE;
  Cdb->CDB6INQUIRY.AllocationLength = sizeof(SENSE_DATA);

  return(Srb);
}


static VOID
ScsiPortFreeSenseRequestSrb(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension)
{
  DeviceExtension->OriginalSrb = NULL;
}
#endif

/**********************************************************************
 * NAME							INTERNAL
 *	SpiBuildDeviceMap
 *
 * DESCRIPTION
 *	Builds the registry device map of all device which are attached
 *	to the given SCSI HBA port. The device map is located at:
 *	  \Registry\Machine\DeviceMap\Scsi
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DeviceExtension
 *		...
 *
 *	RegistryPath
 *		Name of registry driver service key.
 *
 * RETURNS
 *	NTSTATUS
 */

static NTSTATUS
SpiBuildDeviceMap (PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		   PUNICODE_STRING RegistryPath)
{
  PSCSI_PORT_LUN_EXTENSION LunExtension;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  UNICODE_STRING ValueName;
  WCHAR NameBuffer[64];
  ULONG Disposition;
  HANDLE ScsiKey;
  HANDLE ScsiPortKey = NULL;
  HANDLE ScsiBusKey = NULL;
  HANDLE ScsiInitiatorKey = NULL;
  HANDLE ScsiTargetKey = NULL;
  HANDLE ScsiLunKey = NULL;
  ULONG BusNumber;
  ULONG Target;
  ULONG CurrentTarget;
  ULONG Lun;
  PWCHAR DriverName;
  ULONG UlongData;
  PWCHAR TypeName;
  NTSTATUS Status;

  DPRINT("SpiBuildDeviceMap() called\n");

  if (DeviceExtension == NULL || RegistryPath == NULL)
    {
      DPRINT1("Invalid parameter\n");
      return(STATUS_INVALID_PARAMETER);
    }

  /* Open or create the 'Scsi' subkey */
  RtlInitUnicodeString(&KeyName,
			  L"\\Registry\\Machine\\Hardware\\DeviceMap\\Scsi");
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     0,
			     NULL);
  Status = ZwCreateKey(&ScsiKey,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_VOLATILE,
		       &Disposition);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Create new 'Scsi Port X' subkey */
  DPRINT("Scsi Port %lu\n",
	 DeviceExtension->PortNumber);

  swprintf(NameBuffer,
	   L"Scsi Port %lu",
	   DeviceExtension->PortNumber);
  RtlInitUnicodeString(&KeyName,
		       NameBuffer);
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     0,
			     ScsiKey,
			     NULL);
  Status = ZwCreateKey(&ScsiPortKey,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_VOLATILE,
		       &Disposition);
  ZwClose(ScsiKey);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
      return(Status);
    }

  /*
   * Create port-specific values
   */

  /* Set 'DMA Enabled' (REG_DWORD) value */
  UlongData = (ULONG)!DeviceExtension->PortCapabilities->AdapterUsesPio;
  DPRINT("  DMA Enabled = %s\n", (UlongData) ? "TRUE" : "FALSE");
  RtlInitUnicodeString(&ValueName,
		       L"DMA Enabled");
  Status = ZwSetValueKey(ScsiPortKey,
			 &ValueName,
			 0,
			 REG_DWORD,
			 &UlongData,
			 sizeof(ULONG));
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ZwSetValueKey('DMA Enabled') failed (Status %lx)\n", Status);
      ZwClose(ScsiPortKey);
      return(Status);
    }

  /* Set 'Driver' (REG_SZ) value */
  DriverName = wcsrchr(RegistryPath->Buffer, L'\\') + 1;
  RtlInitUnicodeString(&ValueName,
		       L"Driver");
  Status = ZwSetValueKey(ScsiPortKey,
			 &ValueName,
			 0,
			 REG_SZ,
			 DriverName,
			 (wcslen(DriverName) + 1) * sizeof(WCHAR));
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ZwSetValueKey('Driver') failed (Status %lx)\n", Status);
      ZwClose(ScsiPortKey);
      return(Status);
    }

  /* Set 'Interrupt' (REG_DWORD) value (NT4 only) */
  UlongData = (ULONG)DeviceExtension->PortConfig->BusInterruptLevel;
  DPRINT("  Interrupt = %lu\n", UlongData);
  RtlInitUnicodeString(&ValueName,
		       L"Interrupt");
  Status = ZwSetValueKey(ScsiPortKey,
			 &ValueName,
			 0,
			 REG_DWORD,
			 &UlongData,
			 sizeof(ULONG));
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ZwSetValueKey('Interrupt') failed (Status %lx)\n", Status);
      ZwClose(ScsiPortKey);
      return(Status);
    }

  /* Set 'IOAddress' (REG_DWORD) value (NT4 only) */
  UlongData = ScsiPortConvertPhysicalAddressToUlong((*DeviceExtension->PortConfig->AccessRanges)[0].RangeStart);
  DPRINT("  IOAddress = %lx\n", UlongData);
  RtlInitUnicodeString(&ValueName,
		       L"IOAddress");
  Status = ZwSetValueKey(ScsiPortKey,
			 &ValueName,
			 0,
			 REG_DWORD,
			 &UlongData,
			 sizeof(ULONG));
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ZwSetValueKey('IOAddress') failed (Status %lx)\n", Status);
      ZwClose(ScsiPortKey);
      return(Status);
    }

  /* Enumerate buses */
  for (BusNumber = 0; BusNumber < DeviceExtension->PortConfig->NumberOfBuses; BusNumber++)
    {
      /* Create 'Scsi Bus X' key */
      DPRINT("    Scsi Bus %lu\n", BusNumber);
      swprintf(NameBuffer,
	       L"Scsi Bus %lu",
	       BusNumber);
      RtlInitUnicodeString(&KeyName,
			   NameBuffer);
      InitializeObjectAttributes(&ObjectAttributes,
				 &KeyName,
				 0,
				 ScsiPortKey,
				 NULL);
      Status = ZwCreateKey(&ScsiBusKey,
			   KEY_ALL_ACCESS,
			   &ObjectAttributes,
			   0,
			   NULL,
			   REG_OPTION_VOLATILE,
			   &Disposition);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
	  goto ByeBye;
	}

      /* Create 'Initiator Id X' key */
      DPRINT("      Initiator Id %u\n",
	      DeviceExtension->PortConfig->InitiatorBusId[BusNumber]);
      swprintf(NameBuffer,
	       L"Initiator Id %u",
	       (unsigned int)(UCHAR)DeviceExtension->PortConfig->InitiatorBusId[BusNumber]);
      RtlInitUnicodeString(&KeyName,
			   NameBuffer);
      InitializeObjectAttributes(&ObjectAttributes,
				 &KeyName,
				 0,
				 ScsiBusKey,
				 NULL);
      Status = ZwCreateKey(&ScsiInitiatorKey,
			   KEY_ALL_ACCESS,
			   &ObjectAttributes,
			   0,
			   NULL,
			   REG_OPTION_VOLATILE,
			   &Disposition);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
	  goto ByeBye;
	}

      /* FIXME: Are there any initiator values (??) */

      ZwClose(ScsiInitiatorKey);
      ScsiInitiatorKey = NULL;


      /* Enumerate targets */
      CurrentTarget = (ULONG)-1;
      ScsiTargetKey = NULL;
      for (Target = 0; Target < DeviceExtension->PortConfig->MaximumNumberOfTargets; Target++)
	{
	  for (Lun = 0; Lun < SCSI_MAXIMUM_LOGICAL_UNITS; Lun++)
	    {
	      LunExtension = SpiGetLunExtension(DeviceExtension,
						BusNumber,
						Target,
						Lun);
	      if (LunExtension != NULL)
		{
		  if (Target != CurrentTarget)
		    {
		      /* Close old target key */
		      if (ScsiTargetKey != NULL)
			{
			  ZwClose(ScsiTargetKey);
			  ScsiTargetKey = NULL;
			}

		      /* Create 'Target Id X' key */
		      DPRINT("      Target Id %lu\n", Target);
		      swprintf(NameBuffer,
			       L"Target Id %lu",
			       Target);
		      RtlInitUnicodeString(&KeyName,
					   NameBuffer);
		      InitializeObjectAttributes(&ObjectAttributes,
						 &KeyName,
						 0,
						 ScsiBusKey,
						 NULL);
		      Status = ZwCreateKey(&ScsiTargetKey,
					   KEY_ALL_ACCESS,
					   &ObjectAttributes,
					   0,
					   NULL,
					   REG_OPTION_VOLATILE,
					   &Disposition);
		      if (!NT_SUCCESS(Status))
			{
			  DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
			  goto ByeBye;
			}

		      CurrentTarget = Target;
		    }

		  /* Create 'Logical Unit Id X' key */
		  DPRINT("        Logical Unit Id %lu\n", Lun);
		  swprintf(NameBuffer,
			   L"Logical Unit Id %lu",
			   Lun);
		  RtlInitUnicodeString(&KeyName,
				       NameBuffer);
		  InitializeObjectAttributes(&ObjectAttributes,
					     &KeyName,
					     0,
					     ScsiTargetKey,
					     NULL);
		  Status = ZwCreateKey(&ScsiLunKey,
				       KEY_ALL_ACCESS,
				       &ObjectAttributes,
				       0,
				       NULL,
				       REG_OPTION_VOLATILE,
				       &Disposition);
		  if (!NT_SUCCESS(Status))
		    {
		      DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
		      goto ByeBye;
		    }

		  /* Set 'Identifier' (REG_SZ) value */
		  swprintf(NameBuffer,
			   L"%.8S%.16S%.4S",
			   LunExtension->InquiryData.VendorId,
			   LunExtension->InquiryData.ProductId,
			   LunExtension->InquiryData.ProductRevisionLevel);
		  DPRINT("          Identifier = '%S'\n", NameBuffer);
		  RtlInitUnicodeString(&ValueName,
				       L"Identifier");
		  Status = ZwSetValueKey(ScsiLunKey,
					 &ValueName,
					 0,
					 REG_SZ,
					 NameBuffer,
					 (wcslen(NameBuffer) + 1) * sizeof(WCHAR));
		  if (!NT_SUCCESS(Status))
		    {
		      DPRINT("ZwSetValueKey('Identifier') failed (Status %lx)\n", Status);
		      goto ByeBye;
		    }

		  /* Set 'Type' (REG_SZ) value */
		  switch (LunExtension->InquiryData.DeviceType)
		    {
		      case 0:
			TypeName = L"DiskPeripheral";
			break;
		      case 1:
			TypeName = L"TapePeripheral";
			break;
		      case 2:
			TypeName = L"PrinterPeripheral";
			break;
		      case 4:
			TypeName = L"WormPeripheral";
			break;
		      case 5:
			TypeName = L"CdRomPeripheral";
			break;
		      case 6:
			TypeName = L"ScannerPeripheral";
			break;
		      case 7:
			TypeName = L"OpticalDiskPeripheral";
			break;
		      case 8:
			TypeName = L"MediumChangerPeripheral";
			break;
		      case 9:
			TypeName = L"CommunicationPeripheral";
			break;
		      default:
			TypeName = L"OtherPeripheral";
			break;
		    }
		  DPRINT("          Type = '%S'\n", TypeName);
		  RtlInitUnicodeString(&ValueName,
				       L"Type");
		  Status = ZwSetValueKey(ScsiLunKey,
					 &ValueName,
					 0,
					 REG_SZ,
					 TypeName,
					 (wcslen(TypeName) + 1) * sizeof(WCHAR));
		  if (!NT_SUCCESS(Status))
		    {
		      DPRINT("ZwSetValueKey('Type') failed (Status %lx)\n", Status);
		      goto ByeBye;
		    }

		  ZwClose(ScsiLunKey);
		  ScsiLunKey = NULL;
		}
	    }

	  /* Close old target key */
	  if (ScsiTargetKey != NULL)
	    {
	      ZwClose(ScsiTargetKey);
	      ScsiTargetKey = NULL;
	    }
	}

      ZwClose(ScsiBusKey);
      ScsiBusKey = NULL;
    }

ByeBye:
  if (ScsiLunKey != NULL)
    ZwClose (ScsiLunKey);

  if (ScsiInitiatorKey != NULL)
    ZwClose (ScsiInitiatorKey);

  if (ScsiTargetKey != NULL)
    ZwClose (ScsiTargetKey);

  if (ScsiBusKey != NULL)
    ZwClose (ScsiBusKey);

  if (ScsiPortKey != NULL)
    ZwClose (ScsiPortKey);

  DPRINT("SpiBuildDeviceMap() done\n");

  return Status;
}

static
NTSTATUS
SpiStatusSrbToNt(UCHAR SrbStatus)
{
    switch (SRB_STATUS(SrbStatus))
    {
    case SRB_STATUS_TIMEOUT:
    case SRB_STATUS_COMMAND_TIMEOUT:
        return STATUS_IO_TIMEOUT;
    
    case SRB_STATUS_BAD_SRB_BLOCK_LENGTH:
    case SRB_STATUS_BAD_FUNCTION:
        return STATUS_INVALID_DEVICE_REQUEST;

    case SRB_STATUS_NO_DEVICE:
    case SRB_STATUS_INVALID_LUN:
    case SRB_STATUS_INVALID_TARGET_ID:
    case SRB_STATUS_NO_HBA:
        return STATUS_DEVICE_DOES_NOT_EXIST;

    case SRB_STATUS_DATA_OVERRUN:
        return STATUS_BUFFER_OVERFLOW;

    case SRB_STATUS_SELECTION_TIMEOUT:
        return STATUS_DEVICE_NOT_CONNECTED;

    default:
        return STATUS_IO_DEVICE_ERROR;
    }

    return STATUS_IO_DEVICE_ERROR;
}


#undef ScsiPortConvertPhysicalAddressToUlong
/*
 * @implemented
 */
ULONG STDCALL
ScsiPortConvertPhysicalAddressToUlong(IN SCSI_PHYSICAL_ADDRESS Address)
{
  DPRINT("ScsiPortConvertPhysicalAddressToUlong()\n");
  return(Address.u.LowPart);
}


/* EOF */
