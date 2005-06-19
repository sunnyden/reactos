/*
   ReactOS specific functions for UHCI module
   by Aleksey Bragin (aleksey@reactos.com)
   and Herv� Poussineau (hpoussin@reactos.com)
   Some parts of code are inspired (or even just copied) from ReactOS Videoport driver
*/
#define NDEBUG
#define INITGUID
#include "uhci.h"

/* declare basic init functions and structures */
void uhci_hcd_cleanup(void);
void STDCALL usb_exit(void);

extern struct pci_driver uhci_pci_driver;

static ULONG DeviceNumber = 0; /* FIXME: what is that? */

static NTSTATUS
CreateRootHubPdo(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Fdo,
	OUT PDEVICE_OBJECT* pPdo)
{
	PDEVICE_OBJECT Pdo;
	POHCI_DEVICE_EXTENSION DeviceExtension;
	NTSTATUS Status;
	
	DPRINT("UHCI: CreateRootHubPdo()\n");
	
	Status = IoCreateDevice(
		DriverObject,
		sizeof(OHCI_DEVICE_EXTENSION),
		NULL, /* DeviceName */
		FILE_DEVICE_BUS_EXTENDER,
		FILE_DEVICE_SECURE_OPEN | FILE_AUTOGENERATED_DEVICE_NAME,
		FALSE,
		&Pdo);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("UHCI: IoCreateDevice() call failed with status 0x%08x\n", Status);
		return Status;
	}
	
	Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;
	Pdo->Flags |= DO_POWER_PAGABLE;
	
	// zerofill device extension
	DeviceExtension = (POHCI_DEVICE_EXTENSION)Pdo->DeviceExtension;
	RtlZeroMemory(DeviceExtension, sizeof(OHCI_DEVICE_EXTENSION));
	
	DeviceExtension->IsFDO = false;
	DeviceExtension->FunctionalDeviceObject = Fdo;
	
	Pdo->Flags &= ~DO_DEVICE_INITIALIZING;
	
	*pPdo = Pdo;
	return STATUS_SUCCESS;
}

NTSTATUS STDCALL
AddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT pdo)
{
	PDEVICE_OBJECT fdo;
	NTSTATUS Status;
	WCHAR DeviceBuffer[20];
	WCHAR LinkDeviceBuffer[20];
	UNICODE_STRING DeviceName;
	UNICODE_STRING LinkDeviceName;
	POHCI_DRIVER_EXTENSION DriverExtension;
	POHCI_DEVICE_EXTENSION DeviceExtension;

	DPRINT("UHCI: AddDevice called\n");

	// Allocate driver extension now
	DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);
	if (DriverExtension == NULL)
	{
		Status = IoAllocateDriverObjectExtension(
					DriverObject,
					DriverObject,
					sizeof(OHCI_DRIVER_EXTENSION),
					(PVOID *)&DriverExtension);

		if (!NT_SUCCESS(Status))
		{
			DPRINT("UHCI: Allocating DriverObjectExtension failed.\n");
			return Status;
		}
	}

	// Create a unicode device name
//	DeviceNumber = 0; //TODO: Allocate new device number every time
	swprintf(DeviceBuffer, L"\\Device\\USBFDO-%lu", DeviceNumber);
	RtlInitUnicodeString(&DeviceName, DeviceBuffer);

	Status = IoCreateDevice(DriverObject,
				sizeof(OHCI_DEVICE_EXTENSION),
				&DeviceName,
				FILE_DEVICE_BUS_EXTENDER,
				0,
				FALSE,
				&fdo);

	if (!NT_SUCCESS(Status))
	{
		DPRINT("UHCI: IoCreateDevice call failed with status 0x%08lx\n", Status);
		return Status;
	}

	// zerofill device extension
	DeviceExtension = (POHCI_DEVICE_EXTENSION)fdo->DeviceExtension;
	RtlZeroMemory(DeviceExtension, sizeof(OHCI_DEVICE_EXTENSION));
	
	/* Create root hub Pdo */
	Status = CreateRootHubPdo(DriverObject, fdo, &DeviceExtension->RootHubPdo);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("UHCI: CreateRootHubPdo() failed with status 0x%08lx\n", Status);
		IoDeleteDevice(fdo);
		return Status;
	}

	/* Register device interface for controller */
	Status = IoRegisterDeviceInterface(
		pdo,
		&GUID_DEVINTERFACE_USB_HOST_CONTROLLER,
		NULL,
		&DeviceExtension->HcdInterfaceName);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("UHCI: IoRegisterDeviceInterface() failed with status 0x%08lx\n", Status);
		IoDeleteDevice(DeviceExtension->RootHubPdo);
		IoDeleteDevice(fdo);
		return Status;
	}

	DeviceExtension->NextDeviceObject = IoAttachDeviceToDeviceStack(fdo, pdo);

	fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	// Initialize device extension
	DeviceExtension->IsFDO = TRUE;
	DeviceExtension->DeviceNumber = DeviceNumber;
	DeviceExtension->PhysicalDeviceObject = pdo;
	DeviceExtension->FunctionalDeviceObject = fdo;
	DeviceExtension->DriverExtension = DriverExtension;

	/* FIXME: do a loop to find an available number */
	swprintf(LinkDeviceBuffer, L"\\??\\HCD%lu", 0);

	RtlInitUnicodeString(&LinkDeviceName, LinkDeviceBuffer);

	Status = IoCreateSymbolicLink(&LinkDeviceName, &DeviceName);

	if (!NT_SUCCESS(Status))
	{
		DPRINT("UHCI: IoCreateSymbolicLink call failed with status 0x%08x\n", Status);
		IoDeleteDevice(DeviceExtension->RootHubPdo);
		IoDeleteDevice(fdo);
		return Status;
	}

	return STATUS_SUCCESS;
}

VOID STDCALL 
DriverUnload(PDRIVER_OBJECT DriverObject)
{
	POHCI_DEVICE_EXTENSION DeviceExtension;
	PDEVICE_OBJECT DeviceObject;
	struct pci_dev *dev;

	DeviceObject = DriverObject->DeviceObject;
	DeviceExtension = (POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	dev = DeviceExtension->pdev;

	DPRINT1("UHCI: DriverUnload()\n");

	// Exit usb device
	usb_exit();

	// Remove device (ohci_pci_driver.remove)
	uhci_pci_driver.remove(dev);

	ExFreePool(dev->slot_name);
	ExFreePool(dev);

	// Perform some cleanup
	uhci_hcd_cleanup();
}

NTSTATUS STDCALL
IrpStub(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	NTSTATUS Status = STATUS_NOT_SUPPORTED;

	if (((POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO)
	{
		DPRINT1("UHCI: FDO stub for major function 0x%lx\n",
			IoGetCurrentIrpStackLocation(Irp)->MajorFunction);
#ifndef NDEBUG
		DbgBreakPoint();
#endif
		return ForwardIrpAndForget(DeviceObject, Irp);
	}
	else
	{
		/* We can't forward request to the lower driver, because
		 * we are a Pdo, so we don't have lower driver...
		 */
		DPRINT1("UHCI: PDO stub for major function 0x%lx\n",
			IoGetCurrentIrpStackLocation(Irp)->MajorFunction);
#ifndef NDEBUG
		DbgBreakPoint();
#endif
	}

	Status = Irp->IoStatus.Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

static NTSTATUS STDCALL
DispatchCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	if (((POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO)
		return UhciCreate(DeviceObject, Irp);
	else
		return IrpStub(DeviceObject, Irp);
}

static NTSTATUS STDCALL
DispatchClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	if (((POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO)
		return UhciClose(DeviceObject, Irp);
	else
		return IrpStub(DeviceObject, Irp);
}

static NTSTATUS STDCALL
DispatchCleanup(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	if (((POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO)
		return UhciCleanup(DeviceObject, Irp);
	else
		return IrpStub(DeviceObject, Irp);
}

static NTSTATUS STDCALL
DispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	if (((POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO)
		return UhciDeviceControlFdo(DeviceObject, Irp);
	else
		return UhciDeviceControlPdo(DeviceObject, Irp);
}

static NTSTATUS STDCALL
DispatchPnp(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	if (((POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO)
		return UhciPnpFdo(DeviceObject, Irp);
	else
		return UhciPnpPdo(DeviceObject, Irp);
}

static NTSTATUS STDCALL 
DispatchPower(PDEVICE_OBJECT fido, PIRP Irp)
{
	DPRINT1("UHCI: IRP_MJ_POWER unimplemented\n");
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

/*
 * Standard DriverEntry method.
 */
NTSTATUS STDCALL
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegPath)
{
	ULONG i;
	DPRINT("********* Cromwell UHCI *********\n");

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->DriverExtension->AddDevice = AddDevice;

	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		DriverObject->MajorFunction[i] = IrpStub;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = DispatchCleanup;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;
	DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
	DriverObject->MajorFunction[IRP_MJ_POWER] = DispatchPower;

	return STATUS_SUCCESS;
}
