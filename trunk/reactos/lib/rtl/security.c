/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * FILE:              lib/rtl/security.c
 * PURPOSE:           Security related functions and Security Objects
 * PROGRAMMER:        Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
NTSTATUS STDCALL
RtlImpersonateSelf(IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
   HANDLE ProcessToken;
   HANDLE ImpersonationToken;
   NTSTATUS Status;
   OBJECT_ATTRIBUTES ObjAttr;
   SECURITY_QUALITY_OF_SERVICE Sqos;

   PAGED_CODE_RTL();

   Status = NtOpenProcessToken(NtCurrentProcess(),
                               TOKEN_DUPLICATE,
                               &ProcessToken);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("NtOpenProcessToken() failed (Status %lx)\n", Status);
      return(Status);
   }

   Sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
   Sqos.ImpersonationLevel = ImpersonationLevel;
   Sqos.ContextTrackingMode = 0;
   Sqos.EffectiveOnly = FALSE;

   InitializeObjectAttributes(
      &ObjAttr,
      NULL,
      0,
      NULL,
      NULL
      );

   ObjAttr.SecurityQualityOfService = &Sqos;

   Status = NtDuplicateToken(ProcessToken,
                             TOKEN_IMPERSONATE,
                             &ObjAttr,
                             Sqos.EffectiveOnly, /* why both here _and_ in Sqos? */
                             TokenImpersonation,
                             &ImpersonationToken);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("NtDuplicateToken() failed (Status %lx)\n", Status);
      NtClose(ProcessToken);
      return(Status);
   }

   Status = NtSetInformationThread(NtCurrentThread(),
                                   ThreadImpersonationToken,
                                   &ImpersonationToken,
                                   sizeof(HANDLE));
   if (!NT_SUCCESS(Status))
   {
     DPRINT1("NtSetInformationThread() failed (Status %lx)\n", Status);
   }

   NtClose(ImpersonationToken);
   NtClose(ProcessToken);

   return(Status);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlAdjustPrivilege(IN ULONG Privilege,
                   IN BOOLEAN Enable,
                   IN BOOLEAN CurrentThread,
                   OUT PBOOLEAN Enabled)
{
   TOKEN_PRIVILEGES NewState;
   TOKEN_PRIVILEGES OldState;
   ULONG ReturnLength;
   HANDLE TokenHandle;
   NTSTATUS Status;

   PAGED_CODE_RTL();

   DPRINT ("RtlAdjustPrivilege() called\n");

   if (CurrentThread)
   {
      Status = NtOpenThreadToken (NtCurrentThread (),
                                  TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                  FALSE,
                                  &TokenHandle);
   }
   else
   {
      Status = NtOpenProcessToken (NtCurrentProcess (),
                                   TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                   &TokenHandle);
   }

   if (!NT_SUCCESS (Status))
   {
      DPRINT1 ("Retrieving token handle failed (Status %lx)\n", Status);
      return Status;
   }

   OldState.PrivilegeCount = 1;

   NewState.PrivilegeCount = 1;
   NewState.Privileges[0].Luid.LowPart = Privilege;
   NewState.Privileges[0].Luid.HighPart = 0;
   NewState.Privileges[0].Attributes = (Enable) ? SE_PRIVILEGE_ENABLED : 0;

   Status = NtAdjustPrivilegesToken (TokenHandle,
                                     FALSE,
                                     &NewState,
                                     sizeof(TOKEN_PRIVILEGES),
                                     &OldState,
                                     &ReturnLength);
   NtClose (TokenHandle);
   if (Status == STATUS_NOT_ALL_ASSIGNED)
   {
      DPRINT1 ("Failed to assign all privileges\n");
      return STATUS_PRIVILEGE_NOT_HELD;
   }
   if (!NT_SUCCESS(Status))
   {
      DPRINT1 ("NtAdjustPrivilegesToken() failed (Status %lx)\n", Status);
      return Status;
   }

   if (OldState.PrivilegeCount == 0)
   {
      *Enabled = Enable;
   }
   else
   {
      *Enabled = (OldState.Privileges[0].Attributes & SE_PRIVILEGE_ENABLED);
   }

   DPRINT ("RtlAdjustPrivilege() done\n");

   return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlDeleteSecurityObject(IN PSECURITY_DESCRIPTOR *ObjectDescriptor)
{
    DPRINT("RtlDeleteSecurityObject(%p)\n", ObjectDescriptor);

    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                *ObjectDescriptor);

    return STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlNewSecurityObject(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                     IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                     OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                     IN BOOLEAN IsDirectoryObject,
                     IN HANDLE Token,
                     IN PGENERIC_MAPPING GenericMapping)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlQuerySecurityObject(IN PSECURITY_DESCRIPTOR ObjectDescriptor,
                       IN SECURITY_INFORMATION SecurityInformation,
                       OUT PSECURITY_DESCRIPTOR ResultantDescriptor,
                       IN ULONG DescriptorLength,
                       OUT PULONG ReturnLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlSetSecurityObject(IN SECURITY_INFORMATION SecurityInformation,
                     IN PSECURITY_DESCRIPTOR ModificationDescriptor,
                     OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
                     IN PGENERIC_MAPPING GenericMapping,
                     IN HANDLE Token)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
/* EOF */
