#ifndef INCLUDE_REACTOS_CAPTURE_H
#define INCLUDE_REACTOS_CAPTURE_H

#if ! defined(_NTOSKRNL_) && ! defined(_WIN32K_)
#error Header intended for use by NTOSKRNL/WIN32K only!
#endif

static const UNICODE_STRING __emptyUnicodeString = {0};
static const LARGE_INTEGER __emptyLargeInteger = {{0, 0}};
static const ULARGE_INTEGER __emptyULargeInteger = {{0, 0}};

#if defined(_WIN32K_)
/*
 * NOTE: NTOSKRNL unfortunately doesn't export RtlRaiseStatus!
 */
VOID NTAPI W32kRaiseStatus(NTSTATUS Status);
#define RtlRaiseStatus W32kRaiseStatus
#endif

/*
 * NOTE: Alignment of the pointers is not verified!
 */
#define ProbeForWriteGenericType(Ptr, Type)                                    \
    do {                                                                       \
        if ((ULONG_PTR)(Ptr) + sizeof(Type) - 1 < (ULONG_PTR)(Ptr) ||          \
            (ULONG_PTR)(Ptr) + sizeof(Type) - 1 >= (ULONG_PTR)MmUserProbeAddress) { \
            RtlRaiseStatus (STATUS_ACCESS_VIOLATION);                          \
        }                                                                      \
        *(volatile Type *)(Ptr) = *(volatile Type *)(Ptr);                     \
    } while (0)

#define ProbeForWriteBoolean(Ptr) ProbeForWriteGenericType(Ptr, BOOLEAN)
#define ProbeForWriteUchar(Ptr) ProbeForWriteGenericType(Ptr, UCHAR)
#define ProbeForWriteChar(Ptr) ProbeForWriteGenericType(Ptr, CHAR)
#define ProbeForWriteUshort(Ptr) ProbeForWriteGenericType(Ptr, USHORT)
#define ProbeForWriteShort(Ptr) ProbeForWriteGenericType(Ptr, SHORT)
#define ProbeForWriteUlong(Ptr) ProbeForWriteGenericType(Ptr, ULONG)
#define ProbeForWriteLong(Ptr) ProbeForWriteGenericType(Ptr, LONG)
#define ProbeForWriteUint(Ptr) ProbeForWriteGenericType(Ptr, UINT)
#define ProbeForWriteInt(Ptr) ProbeForWriteGenericType(Ptr, INT)
#define ProbeForWriteUlonglong(Ptr) ProbeForWriteGenericType(Ptr, ULONGLONG)
#define ProbeForWriteLonglong(Ptr) ProbeForWriteGenericType(Ptr, LONGLONG)
#define ProbeForWriteLonglong(Ptr) ProbeForWriteGenericType(Ptr, LONGLONG)
#define ProbeForWritePointer(Ptr) ProbeForWriteGenericType(Ptr, PVOID)
#define ProbeForWriteHandle(Ptr) ProbeForWriteGenericType(Ptr, HANDLE)
#define ProbeForWriteLangid(Ptr) ProbeForWriteGenericType(Ptr, LANGID)
#define ProbeForWriteSize_t(Ptr) ProbeForWriteGenericType(Ptr, SIZE_T)
#define ProbeForWriteLargeInteger(Ptr) ProbeForWriteGenericType(&(Ptr)->QuadPart, LONGLONG)
#define ProbeForWriteUlargeInteger(Ptr) ProbeForWriteGenericType(&(Ptr)->QuadPart, ULONGLONG)
#define ProbeForWriteUnicodeString(Ptr) ProbeForWriteGenericType(Ptr, UNICODE_STRING)

#define ProbeForReadGenericType(Ptr, Type, Default)                            \
    (((ULONG_PTR)(Ptr) + sizeof(Type) - 1 < (ULONG_PTR)(Ptr) ||                \
	 (ULONG_PTR)(Ptr) + sizeof(Type) - 1 >= (ULONG_PTR)MmUserProbeAddress) ?   \
	     ExRaiseStatus (STATUS_ACCESS_VIOLATION), Default :                    \
	     *(Type *)(Ptr))

#define ProbeForReadBoolean(Ptr) ProbeForReadGenericType(Ptr, BOOLEAN, FALSE)
#define ProbeForReadUchar(Ptr) ProbeForReadGenericType(Ptr, UCHAR, 0)
#define ProbeForReadChar(Ptr) ProbeForReadGenericType(Ptr, CHAR, 0)
#define ProbeForReadUshort(Ptr) ProbeForReadGenericType(Ptr, USHORT, 0)
#define ProbeForReadShort(Ptr) ProbeForReadGenericType(Ptr, SHORT, 0)
#define ProbeForReadUlong(Ptr) ProbeForReadGenericType(Ptr, ULONG, 0)
#define ProbeForReadLong(Ptr) ProbeForReadGenericType(Ptr, LONG, 0)
#define ProbeForReadUint(Ptr) ProbeForReadGenericType(Ptr, UINT, 0)
#define ProbeForReadInt(Ptr) ProbeForReadGenericType(Ptr, INT, 0)
#define ProbeForReadUlonglong(Ptr) ProbeForReadGenericType(Ptr, ULONGLONG, 0)
#define ProbeForReadLonglong(Ptr) ProbeForReadGenericType(Ptr, LONGLONG, 0)
#define ProbeForReadPointer(Ptr) ProbeForReadGenericType(Ptr, PVOID, NULL)
#define ProbeForReadHandle(Ptr) ProbeForReadGenericType(Ptr, HANDLE, NULL)
#define ProbeForReadLangid(Ptr) ProbeForReadGenericType(Ptr, LANGID, 0)
#define ProbeForReadSize_t(Ptr) ProbeForReadGenericType(Ptr, SIZE_T, 0)
#define ProbeForReadLargeInteger(Ptr) ProbeForReadGenericType(Ptr, LARGE_INTEGER, __emptyLargeInteger)
#define ProbeForReadUlargeInteger(Ptr) ProbeForReadGenericType(Ptr, ULARGE_INTEGER, __emptyULargeInteger)
#define ProbeForReadUnicodeString(Ptr) ProbeForReadGenericType(Ptr, UNICODE_STRING, __emptyUnicodeString)

/*
 * Inlined Probing Macros
 */
static __inline
NTSTATUS
NTAPI
ProbeAndCaptureUnicodeString(OUT PUNICODE_STRING Dest,
                             IN KPROCESSOR_MODE CurrentMode,
                             IN PUNICODE_STRING UnsafeSrc)
{
    NTSTATUS Status = STATUS_SUCCESS;
    WCHAR *Buffer = NULL;
    ASSERT(Dest != NULL);

    /* Probe the structure and buffer*/
    if(CurrentMode != KernelMode)
    {
        _SEH_TRY
        {
            *Dest = ProbeForReadUnicodeString(UnsafeSrc);
            if(Dest->Buffer != NULL)
            {
                if (Dest->Length != 0)
                {
                    ProbeForRead(Dest->Buffer,
                                 Dest->Length,
                                 sizeof(WCHAR));

                    /* Allocate space for the buffer */
                    Buffer = ExAllocatePoolWithTag(PagedPool,
                                                   Dest->Length + sizeof(WCHAR),
                                                   TAG('U', 'S', 'T', 'R'));
                    if (Buffer == NULL)
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        _SEH_LEAVE;
                    }

                    /* Copy it */
                    RtlCopyMemory(Buffer, Dest->Buffer, Dest->Length);
                    Buffer[Dest->Length / sizeof(WCHAR)] = UNICODE_NULL;

                    /* Set it as the buffer */
                    Dest->Buffer = Buffer;
                    Dest->MaximumLength = Dest->Length + sizeof(WCHAR);
                }
                else
                {
                    /* sanitize structure */
                    Dest->MaximumLength = 0;
                    Dest->Buffer = NULL;
                }
            }
            else
            {
                /* sanitize structure */
                Dest->Length = 0;
                Dest->MaximumLength = 0;
            }
        }
        _SEH_HANDLE
        {
            /* Free allocated resources and zero the destination string */
            if (Buffer != NULL)
            {
                ExFreePool(Buffer);
            }
            Dest->Length = 0;
            Dest->MaximumLength = 0;
            Dest->Buffer = NULL;

            /* Return the error code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }
    else
    {
        /* Just copy the UNICODE_STRING structure, don't allocate new memory!
           We trust the caller to supply valid pointers and data. */
        *Dest = *UnsafeSrc;
    }

    /* Return */
    return Status;
}

static __inline
VOID
NTAPI
ReleaseCapturedUnicodeString(IN PUNICODE_STRING CapturedString,
                             IN KPROCESSOR_MODE CurrentMode)
{
    if(CurrentMode != KernelMode && CapturedString->Buffer != NULL)
    {
        ExFreePool(CapturedString->Buffer);
    }
}

#endif /* INCLUDE_REACTOS_CAPTURE_H */
