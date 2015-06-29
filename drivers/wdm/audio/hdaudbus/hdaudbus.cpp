/*
* COPYRIGHT:       See COPYING in the top level directory
* PROJECT:         ReactOS Kernel Streaming
* FILE:            drivers/wdm/audio/hdaudbus/hdaudbus.cpp
* PURPOSE:         HDA Driver Entry
* PROGRAMMER:      Johannes Anderwald
*/


#include "hdaudbus.h"


PVOID
AllocateItem(
IN POOL_TYPE PoolType,
IN SIZE_T NumberOfBytes)
{
    PVOID Item = ExAllocatePoolWithTag(PoolType, NumberOfBytes, TAG_HDA);
    if (!Item)
        return Item;

    RtlZeroMemory(Item, NumberOfBytes);
    return Item;
}

VOID
FreeItem(
    IN PVOID Item)
{
    ExFreePool(Item);
}

BOOLEAN
NTAPI
HDA_InterruptService(
IN PKINTERRUPT  Interrupt,
IN PVOID  ServiceContext)
{
    PHDA_FDO_DEVICE_EXTENSION DeviceExtension;
    ULONG InterruptStatus, Response, ResponseFlags, Cad;
    UCHAR RirbStatus, CorbStatus;
    USHORT WritePos;
    PHDA_CODEC_ENTRY Codec;

    /* get device extension */
    DeviceExtension = (PHDA_FDO_DEVICE_EXTENSION)ServiceContext;
    
    // Check if this interrupt is ours
    InterruptStatus = READ_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_INTR_STATUS));

    DPRINT1("HDA_InterruptService %lx\n", InterruptStatus);
    if ((InterruptStatus & INTR_STATUS_GLOBAL) == 0)
        return FALSE;

    // Controller or stream related?
    if (InterruptStatus & INTR_STATUS_CONTROLLER) {
        RirbStatus = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_STATUS);
        CorbStatus = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_STATUS);

        // Check for incoming responses
        if (RirbStatus) {
            WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_STATUS, RirbStatus);

            if ((RirbStatus & RIRB_STATUS_RESPONSE) != 0) {
                WritePos = (READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_RIRB_WRITE_POS)) + 1) % DeviceExtension->RirbLength;

                for (; DeviceExtension->RirbReadPos != WritePos; DeviceExtension->RirbReadPos = (DeviceExtension->RirbReadPos + 1) % DeviceExtension->RirbLength)
                {

                    Response = DeviceExtension->RirbBase[DeviceExtension->RirbReadPos].response;
                    ResponseFlags = DeviceExtension->RirbBase[DeviceExtension->RirbReadPos].flags;
                    Cad = ResponseFlags & RESPONSE_FLAGS_CODEC_MASK;
                    DPRINT1("Response %lx ResponseFlags %lx Cad %lx\n", Response, ResponseFlags, Cad);
                    
                    /* get codec */
                    Codec = DeviceExtension->Codecs[Cad];
                    if (Codec == NULL)
                    {
                        DPRINT1("hda: response for unknown codec %x Response %x ResponseFlags %x\n", Cad, Response, ResponseFlags);
                        continue;
                    }

                    /* check response count */
                    if (Codec->ResponseCount >= MAX_CODEC_RESPONSES)
                    {
                        DPRINT1("too many responses for codec %x Response %x ResponseFlags %x\n", Cad, Response, ResponseFlags);
                        continue;
                    }

                    // FIXME handle unsolicited responses
                    ASSERT((ResponseFlags & RESPONSE_FLAGS_UNSOLICITED) == 0);

                    /* store response */
                    Codec->Responses[Codec->ResponseCount] = Response;
                    Codec->ResponseCount++;
                }
            }

            if ((RirbStatus & RIRB_STATUS_OVERRUN) != 0)
                DPRINT1("hda: RIRB Overflow\n");
        }

        // Check for sending errors
        if (CorbStatus) {
            WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_STATUS, CorbStatus);

            if ((CorbStatus & CORB_STATUS_MEMORY_ERROR) != 0)
                DPRINT1("hda: CORB Memory Error!\n");
        }
    }
#if 0
    if ((intrStatus & INTR_STATUS_STREAM_MASK) != 0) {
        for (uint32 index = 0; index < HDA_MAX_STREAMS; index++) {
            if ((intrStatus & (1 << index)) != 0) {
                if (controller->streams[index]) {
                    if (stream_handle_interrupt(controller,
                        controller->streams[index], index)) {
                        handled = B_INVOKE_SCHEDULER;
                    }
                }
                else {
                    dprintf("hda: Stream interrupt for unconfigured stream "
                        "%ld!\n", index);
                }
            }
        }
    }
#endif
    return TRUE;
}

VOID
HDA_SendVerbs(
    IN PDEVICE_OBJECT DeviceObject,
    IN PHDA_CODEC_ENTRY Codec,
    IN PULONG Verbs, 
    OUT PULONG Responses, 
    IN ULONG Count)
{
    PHDA_FDO_DEVICE_EXTENSION DeviceExtension;
    ULONG Sent = 0, ReadPosition, WritePosition, Queued;

    /* get device extension */
    DeviceExtension = (PHDA_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* reset response count */
    Codec->ResponseCount = 0;

    while (Sent < Count) {
        ReadPosition = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_READ_POS));

        Queued = 0;

        while (Sent < Count) {
            WritePosition = (DeviceExtension->CorbWritePos + 1) % DeviceExtension->CorbLength;

            if (WritePosition == ReadPosition) {
                // There is no space left in the ring buffer; execute the
                // queued commands and wait until
                break;
            }

            DeviceExtension->CorbBase[WritePosition] = Verbs[Sent++];
            DeviceExtension->CorbWritePos = WritePosition;
    
            // FIXME HACK
            // do proper synchronization
            WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_WRITE_POS), DeviceExtension->CorbWritePos);
            KeStallExecutionProcessor(30);
            Queued++;
        }

        WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_WRITE_POS), DeviceExtension->CorbWritePos);
    }

    if (Responses != NULL) {
        memcpy(Responses, Codec->Responses, Codec->ResponseCount * sizeof(ULONG));
    }
}

NTSTATUS
HDA_InitCodec(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG codecAddress)
{
    PHDA_CODEC_ENTRY Entry;
    ULONG verbs[3];
    PHDA_FDO_DEVICE_EXTENSION DeviceExtension;
    CODEC_RESPONSE Response;
    ULONG NodeId, GroupType;
    NTSTATUS Status;
    PHDA_CODEC_AUDIO_GROUP AudioGroup;
    PHDA_PDO_DEVICE_EXTENSION ChildDeviceExtension;

    /* lets allocate the entry */
    Entry = (PHDA_CODEC_ENTRY)AllocateItem(NonPagedPool, sizeof(HDA_CODEC_ENTRY));
    if (!Entry)
    {
        DPRINT1("hda: failed to allocate memory");
        return STATUS_UNSUCCESSFUL;
    }

    /* init codec */
    Entry->Addr = codecAddress;

    /* get device extension */
    DeviceExtension = (PHDA_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    
    /* store codec */
    DeviceExtension->Codecs[codecAddress] = Entry;

    verbs[0] = MAKE_VERB(codecAddress, 0, VID_GET_PARAMETER, PID_VENDOR_ID);
    verbs[1] = MAKE_VERB(codecAddress, 0, VID_GET_PARAMETER, PID_REVISION_ID);
    verbs[2] = MAKE_VERB(codecAddress, 0, VID_GET_PARAMETER, PID_SUB_NODE_COUNT);

    /* get basic info */
    HDA_SendVerbs(DeviceObject, Entry, verbs, (PULONG)&Response, 3);

    /* store codec details */
    Entry->Major = Response.major;
    Entry->Minor = Response.minor;
    Entry->ProductId = Response.device;
    Entry->Revision = Response.revision;
    Entry->Stepping = Response.stepping;
    Entry->VendorId = Response.vendor;

    DPRINT1("hda Codec %ld Vendor: %04lx Product: %04lx, Revision: %lu.%lu.%lu.%lu NodeStart %u NodeCount %u \n", codecAddress, Response.vendor,
        Response.device, Response.major, Response.minor, Response.revision, Response.stepping, Response.start, Response.count);

    for (NodeId = Response.start; NodeId < Response.start + Response.count; NodeId++) {

        /* get function type */
        verbs[0] = MAKE_VERB(codecAddress, NodeId, VID_GET_PARAMETER, PID_FUNCTION_GROUP_TYPE);

        HDA_SendVerbs(DeviceObject, Entry, verbs, &GroupType, 1);
        DPRINT1("NodeId %u GroupType %x\n", NodeId, GroupType);

        if ((GroupType & FUNCTION_GROUP_NODETYPE_MASK) == FUNCTION_GROUP_NODETYPE_AUDIO) {
            
            AudioGroup = (PHDA_CODEC_AUDIO_GROUP)AllocateItem(NonPagedPool, sizeof(HDA_CODEC_AUDIO_GROUP));
            if (!AudioGroup)
            {
                DPRINT1("hda: insufficient memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            /* init audio group */
            AudioGroup->NodeId = NodeId;
            AudioGroup->FunctionGroup = FUNCTION_GROUP_NODETYPE_AUDIO;

            // Found an Audio Function Group!
            DPRINT1("NodeId %x found an audio function group!\n");

            Status = IoCreateDevice(DeviceObject->DriverObject, sizeof(HDA_PDO_DEVICE_EXTENSION), NULL, FILE_DEVICE_SOUND, 0, FALSE, &AudioGroup->ChildPDO);
            if (!NT_SUCCESS(Status))
            {
                FreeItem(AudioGroup);
                DPRINT1("hda failed to create device object %x\n", Status);
                return Status;
            }

            /* init child pdo*/
            ChildDeviceExtension = (PHDA_PDO_DEVICE_EXTENSION)AudioGroup->ChildPDO->DeviceExtension;
            ChildDeviceExtension->IsFDO = FALSE;
            ChildDeviceExtension->Codec = Entry;
            ChildDeviceExtension->AudioGroup = AudioGroup;

            /* setup flags */
            AudioGroup->ChildPDO->Flags |= DO_POWER_PAGABLE;
            AudioGroup->ChildPDO->Flags &= ~DO_DEVICE_INITIALIZING;

            /* add audio group*/
            Entry->AudioGroups[Entry->AudioGroupCount] = AudioGroup;
            Entry->AudioGroupCount++;
        }
    }
    return STATUS_SUCCESS;

}

NTSTATUS
NTAPI
HDA_InitCorbRirbPos(
    IN PDEVICE_OBJECT DeviceObject)
{
    PHDA_FDO_DEVICE_EXTENSION DeviceExtension;
    UCHAR corbSize, value, rirbSize;
    PHYSICAL_ADDRESS HighestPhysicalAddress, CorbPhysicalAddress;
    ULONG Index;
    USHORT corbReadPointer, rirbWritePointer, interruptValue, corbControl, rirbControl;

    /* get device extension */
    DeviceExtension = (PHDA_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    // Determine and set size of CORB
    corbSize = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_SIZE);
    if ((corbSize & CORB_SIZE_CAP_256_ENTRIES) != 0) {
        DeviceExtension->CorbLength = 256;

        value = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_SIZE) & HDAC_CORB_SIZE_MASK;
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_SIZE, value | CORB_SIZE_256_ENTRIES);
    }
    else if (corbSize & CORB_SIZE_CAP_16_ENTRIES) {
        DeviceExtension->CorbLength = 16;

        value = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_SIZE) & HDAC_CORB_SIZE_MASK;
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_SIZE, value | CORB_SIZE_16_ENTRIES);
    }
    else if (corbSize & CORB_SIZE_CAP_2_ENTRIES) {
        DeviceExtension->CorbLength = 2;

        value = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_SIZE) & HDAC_CORB_SIZE_MASK;
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_SIZE, value | CORB_SIZE_2_ENTRIES);
    }

    // Determine and set size of RIRB
    rirbSize = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_SIZE);
    if (rirbSize & RIRB_SIZE_CAP_256_ENTRIES) {
        DeviceExtension->RirbLength = 256;

        value = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_SIZE) & HDAC_RIRB_SIZE_MASK;
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_SIZE, value | RIRB_SIZE_256_ENTRIES);
    }
    else if (rirbSize & RIRB_SIZE_CAP_16_ENTRIES) {
        DeviceExtension->RirbLength = 16;

        value = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_SIZE) & HDAC_RIRB_SIZE_MASK;
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_SIZE, value | RIRB_SIZE_16_ENTRIES);
    }
    else if (rirbSize & RIRB_SIZE_CAP_2_ENTRIES) {
        DeviceExtension->RirbLength = 2;

        value = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_SIZE) & HDAC_RIRB_SIZE_MASK;
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_SIZE, value | RIRB_SIZE_2_ENTRIES);
    }
    
    /* init corb */
    HighestPhysicalAddress.QuadPart = 0x00000000FFFFFFFF;
    DeviceExtension->CorbBase = (PULONG)MmAllocateContiguousMemory(PAGE_SIZE * 3, HighestPhysicalAddress);

    // FIXME align rirb 128bytes
    ASSERT(DeviceExtension->CorbLength == 256);
    ASSERT(DeviceExtension->RirbLength == 256);

    CorbPhysicalAddress = MmGetPhysicalAddress(DeviceExtension->CorbBase);

    // Program CORB/RIRB for these locations
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_CORB_BASE_LOWER), CorbPhysicalAddress.LowPart);
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_CORB_BASE_UPPER), CorbPhysicalAddress.HighPart);

    DeviceExtension->RirbBase = (PRIRB_RESPONSE)((ULONG_PTR)DeviceExtension->CorbBase + PAGE_SIZE);	
    CorbPhysicalAddress.QuadPart += PAGE_SIZE;
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_RIRB_BASE_LOWER), CorbPhysicalAddress.LowPart);
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_RIRB_BASE_UPPER), CorbPhysicalAddress.HighPart);
    
    // Program DMA position update
    DeviceExtension->StreamPositions = (PVOID)((ULONG_PTR)DeviceExtension->RirbBase + PAGE_SIZE);
    CorbPhysicalAddress.QuadPart += PAGE_SIZE;
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_DMA_POSITION_BASE_LOWER), CorbPhysicalAddress.LowPart);
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_DMA_POSITION_BASE_UPPER), CorbPhysicalAddress.HighPart);

    value = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_WRITE_POS)) & HDAC_CORB_WRITE_POS_MASK;
    WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_WRITE_POS), value);

    // Reset CORB read pointer. Preseve bits marked as RsvdP.
    // After setting the reset bit, we must wait for the hardware
    // to acknowledge it, then manually unset it and wait for that
    // to be acknowledged as well.
    corbReadPointer = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_READ_POS));

    corbReadPointer |= CORB_READ_POS_RESET;
    WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_READ_POS), corbReadPointer);

    for (Index = 0; Index < 100; Index++) {
        KeStallExecutionProcessor(10);
        corbReadPointer = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_READ_POS));
        if ((corbReadPointer & CORB_READ_POS_RESET) != 0)
            break;
    }
    if ((corbReadPointer & CORB_READ_POS_RESET) == 0) {
        DPRINT1("hda: CORB read pointer reset not acknowledged\n");

        // According to HDA spec v1.0a ch3.3.21, software must read the
        // bit as 1 to verify that the reset completed. However, at least
        // some nVidia HDA controllers do not update the bit after reset.
        // Thus don't fail here on nVidia controllers.
        //if (controller->pci_info.vendor_id != PCI_VENDOR_NVIDIA)
        //	return B_BUSY;
    }

    corbReadPointer &= ~CORB_READ_POS_RESET;
    WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_READ_POS), corbReadPointer);
    for (Index = 0; Index < 10; Index++) {
        KeStallExecutionProcessor(10);
        corbReadPointer = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_READ_POS));
        if ((corbReadPointer & CORB_READ_POS_RESET) == 0)
            break;
    }
    if ((corbReadPointer & CORB_READ_POS_RESET) != 0) {
        DPRINT1("hda: CORB read pointer reset failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    // Reset RIRB write pointer
    rirbWritePointer = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_RIRB_WRITE_POS)) & RIRB_WRITE_POS_RESET;
    WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_RIRB_WRITE_POS), rirbWritePointer | RIRB_WRITE_POS_RESET);

    // Generate interrupt for every response
    interruptValue = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_RESPONSE_INTR_COUNT)) & HDAC_RESPONSE_INTR_COUNT_MASK;
    WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_RESPONSE_INTR_COUNT), interruptValue | 1);

    // Setup cached read/write indices
    DeviceExtension->RirbReadPos = 1;
    DeviceExtension->CorbWritePos = 0;

    // Gentlemen, start your engines...
    corbControl = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_CONTROL)) &HDAC_CORB_CONTROL_MASK;
    WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_CONTROL), corbControl | CORB_CONTROL_RUN | CORB_CONTROL_MEMORY_ERROR_INTR);

    rirbControl = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_RIRB_CONTROL)) & HDAC_RIRB_CONTROL_MASK;
    WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_RIRB_CONTROL), rirbControl | RIRB_CONTROL_DMA_ENABLE | RIRB_CONTROL_OVERRUN_INTR | RIRB_CONTROL_RESPONSE_INTR);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HDA_ResetController(
    IN PDEVICE_OBJECT DeviceObject)
{
    USHORT ValCapabilities;
    ULONG Index;
    PHDA_FDO_DEVICE_EXTENSION DeviceExtension;
    ULONG InputStreams, OutputStreams, BiDirStreams, Control;
    UCHAR corbControl, rirbControl;

    /* get device extension */
    DeviceExtension = (PHDA_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* read caps */
    ValCapabilities = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_GLOBAL_CAP));

    InputStreams = GLOBAL_CAP_INPUT_STREAMS(ValCapabilities);
    OutputStreams = GLOBAL_CAP_OUTPUT_STREAMS(ValCapabilities);
    BiDirStreams = GLOBAL_CAP_BIDIR_STREAMS(ValCapabilities);

    DPRINT1("NumInputStreams %u\n", InputStreams);
    DPRINT1("NumOutputStreams %u\n", OutputStreams);
    DPRINT1("NumBiDirStreams %u\n", BiDirStreams);

    /* stop all streams */
    for (Index = 0; Index < InputStreams; Index++)
    {
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_STREAM_CONTROL0 + HDAC_STREAM_BASE + HDAC_INPUT_STREAM_OFFSET(Index), 0);
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_STREAM_STATUS + HDAC_STREAM_BASE + HDAC_INPUT_STREAM_OFFSET(Index), 0);
    }

    for (Index = 0; Index < OutputStreams; Index++) {
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_STREAM_CONTROL0 + HDAC_STREAM_BASE + HDAC_OUTPUT_STREAM_OFFSET(InputStreams, Index), 0);
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_STREAM_STATUS + HDAC_STREAM_BASE + HDAC_OUTPUT_STREAM_OFFSET(InputStreams, Index), 0);
    }

    for (Index = 0; Index < BiDirStreams; Index++) {
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_STREAM_CONTROL0 + HDAC_STREAM_BASE + HDAC_BIDIR_STREAM_OFFSET(InputStreams, OutputStreams, Index), 0);
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_STREAM_STATUS + HDAC_STREAM_BASE + HDAC_BIDIR_STREAM_OFFSET(InputStreams, OutputStreams, Index), 0);
    }

    // stop DMA
    Control = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_CONTROL) & HDAC_CORB_CONTROL_MASK;
    WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_CONTROL, Control);

    Control = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_CONTROL) & HDAC_RIRB_CONTROL_MASK;
    WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_CONTROL, Control);

    for (int timeout = 0; timeout < 10; timeout++) {
        KeStallExecutionProcessor(10);

        corbControl = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_CONTROL);
        rirbControl = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_CONTROL);
        if (corbControl == 0 && rirbControl == 0)
            break;
    }
    if (corbControl != 0 || rirbControl != 0) {
        DPRINT1("hda: unable to stop dma\n");
        return STATUS_UNSUCCESSFUL;
    }

    // reset DMA position buffer
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_DMA_POSITION_BASE_LOWER), 0);
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_DMA_POSITION_BASE_UPPER), 0);

    // Set reset bit - it must be asserted for at least 100us
    Control = READ_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_GLOBAL_CONTROL));
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_GLOBAL_CONTROL), Control & ~GLOBAL_CONTROL_RESET);

    for (int timeout = 0; timeout < 10; timeout++) {
        KeStallExecutionProcessor(10);

        Control = READ_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_GLOBAL_CONTROL));
        if ((Control & GLOBAL_CONTROL_RESET) == 0)
            break;
    }
    if ((Control & GLOBAL_CONTROL_RESET) != 0)
    {
        DPRINT1("hda: unable to reset controller\n");
        return STATUS_UNSUCCESSFUL;
    }

    // Unset reset bit
    Control = READ_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_GLOBAL_CONTROL));
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_GLOBAL_CONTROL), Control | GLOBAL_CONTROL_RESET);

    for (int timeout = 0; timeout < 10; timeout++) {
        KeStallExecutionProcessor(10);

        Control = READ_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_GLOBAL_CONTROL));
        if ((Control & GLOBAL_CONTROL_RESET) != 0)
            break;
    }
    if ((Control & GLOBAL_CONTROL_RESET) == 0) {
        DPRINT1("hda: unable to exit reset\n");
        return STATUS_UNSUCCESSFUL;
    }

    // Wait for codecs to finish their own reset (apparently needs more
    // time than documented in the specs)
    KeStallExecutionProcessor(100);

    // Enable unsolicited responses
    Control = READ_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_GLOBAL_CONTROL));
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_GLOBAL_CONTROL), Control | GLOBAL_CONTROL_UNSOLICITED);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HDA_QueryId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    WCHAR DeviceName[200];
    PHDA_PDO_DEVICE_EXTENSION DeviceExtension;
    ULONG Length;
    LPWSTR Device;

    /* get device extension */
    DeviceExtension = (PHDA_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(DeviceExtension->IsFDO == FALSE);

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.QueryId.IdType == BusQueryInstanceID)
    {
        UNIMPLEMENTED;

        // FIXME
        swprintf(DeviceName, L"%08x", 1);
        Length = wcslen(DeviceName) + 20;

        /* allocate result buffer*/
        Device = (LPWSTR)AllocateItem(PagedPool, Length * sizeof(WCHAR));
        if (!Device)
            return STATUS_INSUFFICIENT_RESOURCES;

        swprintf(Device, L"%08x", 1);

        DPRINT1("ID: %S\n", Device);
        /* store result */
        Irp->IoStatus.Information = (ULONG_PTR)Device;
        return STATUS_SUCCESS;
    }
    else if (IoStack->Parameters.QueryId.IdType == BusQueryDeviceID ||
        IoStack->Parameters.QueryId.IdType == BusQueryHardwareIDs)
    {

        /* calculate size */
        swprintf(DeviceName, L"HDAUDIO\\FUNC_%02X&VEN_%04X&DEV_%04X&SUBSYS_%08X", DeviceExtension->AudioGroup->FunctionGroup, DeviceExtension->Codec->VendorId, DeviceExtension->Codec->ProductId, DeviceExtension->Codec->VendorId << 16 | DeviceExtension->Codec->ProductId);
        Length = wcslen(DeviceName) + 20;

        /* allocate result buffer*/
        Device = (LPWSTR)AllocateItem(PagedPool, Length * sizeof(WCHAR));
        if (!Device)
            return STATUS_INSUFFICIENT_RESOURCES;

        swprintf(Device, L"HDAUDIO\\FUNC_%02X&VEN_%04X&DEV_%04X&SUBSYS_%08X", DeviceExtension->AudioGroup->FunctionGroup, DeviceExtension->Codec->VendorId, DeviceExtension->Codec->ProductId, DeviceExtension->Codec->VendorId << 16 | DeviceExtension->Codec->ProductId);

        DPRINT1("ID: %S\n", Device);
        /* store result */
        Irp->IoStatus.Information = (ULONG_PTR)Device;
        return STATUS_SUCCESS;
    }
    else
    {
        DPRINT1("QueryID Type %x not implemented\n", IoStack->Parameters.QueryId.IdType);
        return Irp->IoStatus.Status;
    }
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HDA_StartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status = STATUS_SUCCESS;
    PHDA_FDO_DEVICE_EXTENSION DeviceExtension;
    PCM_RESOURCE_LIST Resources;
    ULONG Index;
    USHORT Value;

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get device extension */
    DeviceExtension = (PHDA_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(DeviceExtension->IsFDO == TRUE);

    Resources = IoStack->Parameters.StartDevice.AllocatedResourcesTranslated;
    for (Index = 0; Index < Resources->List[0].PartialResourceList.Count; Index++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor = &Resources->List[0].PartialResourceList.PartialDescriptors[Index];

        if (Descriptor->Type == CmResourceTypeMemory)
        {
            DeviceExtension->RegBase = (PUCHAR)MmMapIoSpace(Descriptor->u.Memory.Start, Descriptor->u.Memory.Length, MmNonCached);
            if (DeviceExtension->RegBase == NULL)
            {
                DPRINT1("[HDAB] Failed to map registers\n");
                Status = STATUS_UNSUCCESSFUL;
                break;
            }
        }
        else if (Descriptor->Type == CmResourceTypeInterrupt)
        {
            Status = IoConnectInterrupt(&DeviceExtension->Interrupt, 
                               HDA_InterruptService, 
                               (PVOID)DeviceExtension, 
                               NULL, 
                               Descriptor->u.Interrupt.Vector, 
                               Descriptor->u.Interrupt.Level, 
                               Descriptor->u.Interrupt.Level, 
                               (KINTERRUPT_MODE)(Descriptor->Flags & CM_RESOURCE_INTERRUPT_LATCHED),
                               (Descriptor->ShareDisposition != CmResourceShareDeviceExclusive),
                               Descriptor->u.Interrupt.Affinity,
                               FALSE);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("[HDAB] Failed to connect interrupt\n");
                break;
            }

        }
    }

    if (NT_SUCCESS(Status))
    {
        // Get controller into valid state
        Status = HDA_ResetController(DeviceObject);
        if (!NT_SUCCESS(Status)) return Status;

        // Setup CORB/RIRB/DMA POS
        Status = HDA_InitCorbRirbPos(DeviceObject);
        if (!NT_SUCCESS(Status)) return Status;


        // Don't enable codec state change interrupts. We don't handle
        // them, as we want to use the STATE_STATUS register to identify
        // available codecs. We'd have to clear that register in the interrupt
        // handler to 'ack' the codec change.
        Value = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_WAKE_ENABLE)) & HDAC_WAKE_ENABLE_MASK;
        WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_WAKE_ENABLE), Value);
        
        // Enable controller interrupts
        WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_INTR_CONTROL), INTR_CONTROL_GLOBAL_ENABLE | INTR_CONTROL_CONTROLLER_ENABLE);

        KeStallExecutionProcessor(100);

        Value = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_STATE_STATUS));
        if (!Value) {
            DPRINT1("hda: bad codec status\n");
            return STATUS_UNSUCCESSFUL;
        }
        WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_STATE_STATUS), Value);

        // Create codecs
        DPRINT1("Codecs %lx\n", Value);
        for (Index = 0; Index < HDA_MAX_CODECS; Index++) {
            if ((Value & (1 << Index)) != 0) {
                HDA_InitCodec(DeviceObject, Index);
            }
        }
    }

    return Status;
}

NTSTATUS
NTAPI
HDA_QueryBusRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    ULONG DeviceCount, CodecIndex, AFGIndex;
    PHDA_FDO_DEVICE_EXTENSION DeviceExtension;
    PHDA_CODEC_ENTRY Codec;
    PDEVICE_RELATIONS DeviceRelations;

    /* get device extension */
    DeviceExtension = (PHDA_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(DeviceExtension->IsFDO == TRUE);

    DeviceCount = 0;
    for (CodecIndex = 0; CodecIndex < HDA_MAX_CODECS; CodecIndex++)
    {
        if (DeviceExtension->Codecs[CodecIndex] == NULL)
            continue;

        Codec = DeviceExtension->Codecs[CodecIndex];
        DeviceCount += Codec->AudioGroupCount;
    }

    if (DeviceCount == 0)
        return STATUS_UNSUCCESSFUL;

    DeviceRelations = (PDEVICE_RELATIONS)AllocateItem(NonPagedPool, sizeof(DEVICE_RELATIONS) + (DeviceCount > 1 ? sizeof(PDEVICE_OBJECT) * (DeviceCount - 1) : 0));
    if (!DeviceRelations)
        return STATUS_INSUFFICIENT_RESOURCES;

    DeviceCount = 0;
    for (CodecIndex = 0; CodecIndex < HDA_MAX_CODECS; CodecIndex++)
    {
        if (DeviceExtension->Codecs[CodecIndex] == NULL)
            continue;

        Codec = DeviceExtension->Codecs[CodecIndex];
        for (AFGIndex = 0; AFGIndex < Codec->AudioGroupCount; AFGIndex++)
        {
            DeviceRelations->Objects[DeviceRelations->Count] = Codec->AudioGroups[AFGIndex]->ChildPDO;
            ObReferenceObject(Codec->AudioGroups[AFGIndex]->ChildPDO);
            DeviceRelations->Count++;
        }
    }

    /* FIXME handle existing device relations */
    ASSERT(Irp->IoStatus.Information == 0);

    /* store device relations */
    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;

    /* done */
    return STATUS_SUCCESS;
}

NTSTATUS
HDA_QueryBusInformation(
    IN PIRP Irp)
{
    PPNP_BUS_INFORMATION BusInformation;

    /* allocate bus information */
    BusInformation = (PPNP_BUS_INFORMATION)AllocateItem(PagedPool, sizeof(PNP_BUS_INFORMATION));

    if (!BusInformation)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* return info */
    BusInformation->BusNumber = 0;
    BusInformation->LegacyBusType = PCIBus;
    RtlMoveMemory(&BusInformation->BusTypeGuid, &GUID_HDAUDIO_BUS_INTERFACE, sizeof(GUID));

    /* store result */
    Irp->IoStatus.Information = (ULONG_PTR)BusInformation;

    /* done */
    return STATUS_SUCCESS;
}

NTSTATUS
HDA_QueryBusDeviceCapabilities(
    IN PIRP Irp)
{
    PDEVICE_CAPABILITIES Capabilities;
    PIO_STACK_LOCATION IoStack;

    /* get stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get capabilities */
    Capabilities = IoStack->Parameters.DeviceCapabilities.Capabilities;

    RtlZeroMemory(Capabilities, sizeof(DEVICE_CAPABILITIES));

    /* setup capabilities */
    Capabilities->UniqueID = TRUE;
    Capabilities->SilentInstall = TRUE;
    Capabilities->SurpriseRemovalOK = TRUE;
    Capabilities->Address = 0;
    Capabilities->UINumber = 0;
    Capabilities->SystemWake = PowerSystemWorking; /* FIXME common device extension */
    Capabilities->DeviceWake = PowerDeviceD0;

    /* done */
    return STATUS_SUCCESS;
}

NTSTATUS
HDA_QueryBusDevicePnpState(
    IN PIRP Irp)
{
    /* set device flags */
    Irp->IoStatus.Information = PNP_DEVICE_DONT_DISPLAY_IN_UI | PNP_DEVICE_NOT_DISABLEABLE;

    /* done */
    return STATUS_SUCCESS;
}

NTSTATUS
HDA_PdoHandleQueryDeviceText(
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    LPWSTR Buffer;
    static WCHAR DeviceText[] = L"Audio Device on High Definition Audio Bus";

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    if (IoStack->Parameters.QueryDeviceText.DeviceTextType == DeviceTextDescription)
    {
        DPRINT("HDA_PdoHandleQueryDeviceText DeviceTextDescription\n");

        Buffer = (LPWSTR)AllocateItem(PagedPool, sizeof(DeviceText));
        if (!Buffer)
        {
            Irp->IoStatus.Information = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        wcscpy(Buffer, DeviceText);

        Irp->IoStatus.Information = (ULONG_PTR)Buffer;
        return STATUS_SUCCESS;
    }
    else
    {
        DPRINT("HDA_PdoHandleQueryDeviceText DeviceTextLocationInformation\n");

        Buffer = (LPWSTR)AllocateItem(PagedPool, sizeof(DeviceText));
        if (!Buffer)
        {
            Irp->IoStatus.Information = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        wcscpy(Buffer, DeviceText);

        /* save result */
        Irp->IoStatus.Information = (ULONG_PTR)Buffer;
        return STATUS_SUCCESS;
    }

}

NTSTATUS
NTAPI
HDA_Pnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;
    PIO_STACK_LOCATION IoStack;
    PDEVICE_RELATIONS DeviceRelation;
    PHDA_FDO_DEVICE_EXTENSION FDODeviceExtension;
    //PHDA_PDO_DEVICE_EXTENSION ChildDeviceExtension;

    FDODeviceExtension = (PHDA_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    //ChildDeviceExtension = (PHDA_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    DPRINT1("HDA_Pnp Minor: %u IsFDO%d\n", IoStack->MinorFunction, FDODeviceExtension->IsFDO);

    if (FDODeviceExtension->IsFDO)
    {
        if (IoStack->MinorFunction == IRP_MN_START_DEVICE)
        {
            DPRINT1("IRP_MN_START_DEVICE\n");
            Status = HDA_StartDevice(DeviceObject, Irp);
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_DEVICE_RELATIONS)
        {
            DPRINT1("IRP_MN_QUERY_DEVICE_RELATIONS\n");
            /* handle bus device relations */
            if (IoStack->Parameters.QueryDeviceRelations.Type == BusRelations)
            {
                Status = HDA_QueryBusRelations(DeviceObject, Irp);
            }
            else
            {
                Status = Irp->IoStatus.Status;
            }
        } 
        else
        {
            /* get default status */
            Status = Irp->IoStatus.Status;
        }
    }
    else
    {
        if (IoStack->MinorFunction == IRP_MN_START_DEVICE)
        {
            DPRINT1("IRP_MN_START_DEVICE\n");
            /* no op for pdo */
            Status = STATUS_SUCCESS;
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_BUS_INFORMATION)
        {
            DPRINT1("IRP_MN_QUERY_BUS_INFORMATION\n");
            /* query bus information */
            Status = HDA_QueryBusInformation(Irp);
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_PNP_DEVICE_STATE)
        {
            DPRINT1("IRP_MN_QUERY_PNP_DEVICE_STATE\n");
            /* query pnp state */
            Status = HDA_QueryBusDevicePnpState(Irp);
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_DEVICE_RELATIONS)
        {
            DPRINT1("IRP_MN_QUERY_DEVICE_RELATIONS\n");
            if (IoStack->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation)
            {
                /* handle target device relations */
                ASSERT(IoStack->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation);
                ASSERT(Irp->IoStatus.Information == 0);

                /* allocate device relation */
                DeviceRelation = (PDEVICE_RELATIONS)AllocateItem(PagedPool, sizeof(DEVICE_RELATIONS));
                if (DeviceRelation)
                {
                    DeviceRelation->Count = 1;
                    DeviceRelation->Objects[0] = DeviceObject;

                    /* reference self */
                    ObReferenceObject(DeviceObject);

                    /* store result */
                    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelation;

                    /* done */
                    Status = STATUS_SUCCESS;
                }
                else
                {
                    /* no memory */
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_CAPABILITIES)
        {
            DPRINT1("IRP_MN_QUERY_CAPABILITIES\n");
            /* query capabilities */
            Status = HDA_QueryBusDeviceCapabilities(Irp);
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_RESOURCE_REQUIREMENTS)
        {
            DPRINT1("IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
            /* no op */
            Status = STATUS_SUCCESS;
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_ID)
        {
            DPRINT1("IRP_MN_QUERY_ID\n");
            Status = HDA_QueryId(DeviceObject, Irp);
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_DEVICE_TEXT)
        {
            DPRINT1("IRP_MN_QUERY_DEVICE_TEXT\n");
            Status = HDA_PdoHandleQueryDeviceText(Irp);
        }
        else
        {
            /* get default status */
            Status = Irp->IoStatus.Status;
        }
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);


    return Status;
}


//PDRIVER_ADD_DEVICE HDA_AddDevice;

NTSTATUS
NTAPI
HDA_AddDevice(
IN PDRIVER_OBJECT DriverObject,
IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDEVICE_OBJECT DeviceObject;
    PHDA_FDO_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;

    /* create device object */
    Status = IoCreateDevice(DriverObject, sizeof(HDA_FDO_DEVICE_EXTENSION), NULL, FILE_DEVICE_BUS_EXTENDER, 0, FALSE, &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        return Status;
    }

    /* get device extension*/
    DeviceExtension = (PHDA_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* init device extension*/
    DeviceExtension->IsFDO = TRUE;
    DeviceExtension->LowerDevice = IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);
    RtlZeroMemory(DeviceExtension->Codecs, sizeof(PHDA_CODEC_ENTRY) * (HDA_MAX_CODECS + 1));


    /* set device flags */
    DeviceObject->Flags |= DO_POWER_PAGABLE;

    return Status;
}
extern "C"
{
NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName)
{
    DriverObject->DriverExtension->AddDevice = HDA_AddDevice;
    DriverObject->MajorFunction[IRP_MJ_PNP] = HDA_Pnp;

    return STATUS_SUCCESS;
}

}