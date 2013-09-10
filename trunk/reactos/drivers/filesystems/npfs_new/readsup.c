#include "npfs.h"

IO_STATUS_BLOCK
NTAPI
NpReadDataQueue(IN PNP_DATA_QUEUE DataQueue, 
                IN BOOLEAN Peek,
                IN BOOLEAN ReadOverflowOperation,
                IN PVOID Buffer,
                IN ULONG BufferSize, 
                IN ULONG Mode, 
                IN PNP_CCB Ccb,
                IN PLIST_ENTRY List)
{
    PNP_DATA_QUEUE_ENTRY DataEntry, TempDataEntry;
    PVOID DataBuffer;
    ULONG DataSize, DataLength, TotalBytesCopied, RemainingSize, Offset;
    PIRP Irp;
    IO_STATUS_BLOCK Status;
    BOOLEAN CompleteWrites = FALSE;
    PAGED_CODE();

    if (ReadOverflowOperation) Peek = TRUE;

    RemainingSize = BufferSize;
    Status.Status = STATUS_SUCCESS;
    TotalBytesCopied = 0;

    if (Peek)
    {
        DataEntry = CONTAINING_RECORD(DataQueue->Queue.Flink,
                                      NP_DATA_QUEUE_ENTRY,
                                      QueueEntry);
    }
    else
    {
        DataEntry = NpGetNextRealDataQueueEntry(DataQueue, List);
    }

    while ((&DataEntry->QueueEntry != &DataQueue->Queue) && (RemainingSize))
    {
        if (!Peek ||
            DataEntry->DataEntryType == Buffered ||
            DataEntry->DataEntryType == Unbuffered)
        {
            if (DataEntry->DataEntryType == Unbuffered)
            {
                DataBuffer = DataEntry->Irp->AssociatedIrp.SystemBuffer;
            }
            else
            {
                DataBuffer = &DataEntry[1];
            }

            DataSize = DataEntry->DataSize;
            Offset = DataSize;

            if (&DataEntry->QueueEntry == DataQueue->Queue.Flink)
            {
                Offset -= DataQueue->ByteOffset;
            }

            DataLength = Offset;
            if (Offset >= RemainingSize) DataLength = RemainingSize;

            _SEH2_TRY
            {
                RtlCopyMemory((PVOID)((ULONG_PTR)Buffer + BufferSize - RemainingSize),
                              (PVOID)((ULONG_PTR)DataBuffer + DataSize - Offset),
                              DataLength);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                ASSERT(FALSE);
            }
            _SEH2_END;


            RemainingSize -= DataLength;
            Offset -= DataLength;
            TotalBytesCopied += DataLength;

            if (!Peek)
            {
                DataEntry->QuotaInEntry -= DataLength;
                DataQueue->QuotaUsed -= DataLength;
                DataQueue->ByteOffset += DataLength;
                CompleteWrites = TRUE;
            }

            NpCopyClientContext(Ccb, DataEntry);

            if ((Offset) || (ReadOverflowOperation && !TotalBytesCopied))
            {
                if (Mode == FILE_PIPE_MESSAGE_MODE)
                {
                    Status.Status = STATUS_BUFFER_OVERFLOW;
                    break;
                }
            }
            else
            {
                if (!Peek || ReadOverflowOperation)
                {
                    if (ReadOverflowOperation)
                    {
                        TempDataEntry = NpGetNextRealDataQueueEntry(DataQueue, List);
                        ASSERT(TempDataEntry == DataEntry);
                    }

                    Irp = NpRemoveDataQueueEntry(DataQueue, TRUE, List);
                    if (Irp)
                    {
                        Irp->IoStatus.Information = DataSize;
                        Irp->IoStatus.Status = STATUS_SUCCESS;
                        InsertTailList(List, &Irp->Tail.Overlay.ListEntry);
                    }
                }
                if (Mode == FILE_PIPE_MESSAGE_MODE)
                {
                    Status.Status = STATUS_SUCCESS;
                    break;
                }
                ASSERT(!ReadOverflowOperation);
            }
        }
        if (Peek)
        {
            DataEntry = CONTAINING_RECORD(DataEntry->QueueEntry.Flink,
                                          NP_DATA_QUEUE_ENTRY,
                                          QueueEntry);
        }
        else
        {
            DataEntry = NpGetNextRealDataQueueEntry(DataQueue, List);
        }
    }

    Status.Information = TotalBytesCopied;
    if (CompleteWrites) NpCompleteStalledWrites(DataQueue, List);
    return Status;
}

