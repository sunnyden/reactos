/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/event.c
 * PURPOSE:         Named event support
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>

#include <internal/debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE ExEventType = NULL;

/* FUNCTIONS *****************************************************************/

VOID NtInitializeEventImplementation(VOID)
{
   ANSI_STRING AnsiName;
   
   ExEventType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   RtlInitAnsiString(&AnsiName,"Event");
   RtlAnsiStringToUnicodeString(&ExEventType->TypeName,&AnsiName,TRUE);
   
   ExEventType->MaxObjects = ULONG_MAX;
   ExEventType->MaxHandles = ULONG_MAX;
   ExEventType->TotalObjects = 0;
   ExEventType->TotalHandles = 0;
   ExEventType->PagedPoolCharge = 0;
   ExEventType->NonpagedPoolCharge = sizeof(KEVENT);
   ExEventType->Dump = NULL;
   ExEventType->Open = NULL;
   ExEventType->Close = NULL;
   ExEventType->Delete = NULL;
   ExEventType->Parse = NULL;
   ExEventType->Security = NULL;
   ExEventType->QueryName = NULL;
   ExEventType->OkayToClose = NULL;
}

NTSTATUS STDCALL NtClearEvent(IN HANDLE EventHandle)
{
   return(ZwClearEvent(EventHandle));
}

NTSTATUS STDCALL ZwClearEvent(IN HANDLE EventHandle)
{
   PKEVENT Event;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(EventHandle,
				      EVENT_MODIFY_STATE,
				      ExEventType,
				      UserMode,
				      (PVOID*)&Event,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   KeClearEvent(Event);
   ObDereferenceObject(Event);
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL NtCreateEvent(OUT PHANDLE EventHandle,
			       IN ACCESS_MASK DesiredAccess,
			       IN POBJECT_ATTRIBUTES ObjectAttributes,
			       IN BOOLEAN ManualReset,
			       IN BOOLEAN InitialState)
{
   return(ZwCreateEvent(EventHandle,
			DesiredAccess,
			ObjectAttributes,
			ManualReset,
			InitialState));
}

NTSTATUS STDCALL ZwCreateEvent(OUT PHANDLE EventHandle,
			       IN ACCESS_MASK DesiredAccess,
			       IN POBJECT_ATTRIBUTES ObjectAttributes,
			       IN BOOLEAN ManualReset,
			       IN BOOLEAN InitialState)
{
   PKEVENT Event;
   
   Event = ObGenericCreateObject(EventHandle,
				 DesiredAccess,
				 ObjectAttributes,
				 ExEventType);
   if (ManualReset == TRUE)
     {
	KeInitializeEvent(Event,NotificationEvent,InitialState);
     }
   else
     {
	KeInitializeEvent(Event,SynchronizationEvent,InitialState);
     }
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtOpenEvent(OUT PHANDLE EventHandle,
			     IN ACCESS_MASK DesiredAccess,
			     IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   return(ZwOpenEvent(EventHandle,DesiredAccess,ObjectAttributes));
}

NTSTATUS STDCALL ZwOpenEvent(OUT PHANDLE EventHandle,
			     IN ACCESS_MASK DesiredAccess,
			     IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   NTSTATUS Status;
   PKEVENT Event;   
   PWSTR Ignored;
   
   Status = ObOpenObjectByName(ObjectAttributes,(PVOID*)Event,&Ignored);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   *EventHandle = ObInsertHandle(KeGetCurrentProcess(),
				 Event,
				 DesiredAccess,
				 FALSE);
   
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL NtPulseEvent(IN HANDLE EventHandle,
			      IN PULONG PulseCount OPTIONAL)
{
   return(ZwPulseEvent(EventHandle,PulseCount));
}

NTSTATUS STDCALL ZwPulseEvent(IN HANDLE EventHandle, 
			      IN PULONG PulseCount OPTIONAL)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtQueryEvent(IN HANDLE EventHandle,
			      IN CINT EventInformationClass,
			      OUT PVOID EventInformation,
			      IN ULONG EventInformationLength,
			      OUT PULONG ReturnLength)
{
   return(ZwQueryEvent(EventHandle,
		       EventInformationClass,
		       EventInformation,
		       EventInformationLength,
		       ReturnLength));
}

NTSTATUS STDCALL ZwQueryEvent(IN HANDLE EventHandle,
			      IN CINT EventInformationClass,
			      OUT PVOID EventInformation,
			      IN ULONG EventInformationLength,
			      OUT PULONG ReturnLength)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtResetEvent(HANDLE EventHandle,
			      PULONG NumberOfWaitingThreads OPTIONAL)
{
   return(ZwResetEvent(EventHandle,NumberOfWaitingThreads));
}

NTSTATUS STDCALL ZwResetEvent(HANDLE EventHandle,
			      PULONG NumberOfWaitingThreads OPTIONAL)
{
   PKEVENT Event;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(EventHandle,
				      EVENT_MODIFY_STATE,
				      ExEventType,
				      UserMode,
				      (PVOID*)&Event,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   KeResetEvent(Event);
   ObDereferenceObject(Event);
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL NtSetEvent(IN HANDLE EventHandle,
			    PULONG NumberOfThreadsReleased)
{
   return(ZwSetEvent(EventHandle,NumberOfThreadsReleased));
}

NTSTATUS STDCALL ZwSetEvent(IN HANDLE EventHandle,
			    PULONG NumberOfThreadsReleased)
{
   PKEVENT Event;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(EventHandle,
				      EVENT_MODIFY_STATE,
				      ExEventType,
				      UserMode,
				      (PVOID*)&Event,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   KeSetEvent(Event,IO_NO_INCREMENT,FALSE);
   ObDereferenceObject(Event);
   return(STATUS_SUCCESS);
}
