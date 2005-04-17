/* 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/efi.c
 * PURPOSE:         EFI Unimplemented Function Calls
 * 
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
NtAddBootEntry(
	IN PUNICODE_STRING EntryName,
	IN PUNICODE_STRING EntryValue
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
                                 
NTSTATUS
STDCALL
NtDeleteBootEntry(
	IN PUNICODE_STRING EntryName,
	IN PUNICODE_STRING EntryValue
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
                                 
NTSTATUS
STDCALL
NtEnumerateBootEntries(
	IN ULONG Unknown1,
	IN ULONG Unknown2
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
                                   
NTSTATUS
STDCALL
NtQueryBootEntryOrder(
	IN ULONG Unknown1,
	IN ULONG Unknown2
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
              
NTSTATUS
STDCALL
NtQueryBootOptions(
	IN ULONG Unknown1,
	IN ULONG Unknown2
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
              
NTSTATUS
STDCALL
NtSetBootEntryOrder(
	IN ULONG Unknown1,
	IN ULONG Unknown2
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
            
NTSTATUS 
STDCALL 
NtSetBootOptions(
	ULONG Unknown1, 
	ULONG Unknown2
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}              
                  
NTSTATUS 
STDCALL 
NtTranslateFilePath(
	ULONG Unknown1, 
	ULONG Unknown2,
	ULONG Unknown3
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
