/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/include/ps.h
* PURPOSE:         Internal header for the Process Manager
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

//
// Define this if you want debugging support
//
#define _PS_DEBUG_                                      0x00

//
// These define the Debug Masks Supported
//
#define PS_THREAD_DEBUG                                 0x01
#define PS_PROCESS_DEBUG                                0x02
#define PS_SECURITY_DEBUG                               0x04
#define PS_JOB_DEBUG                                    0x08
#define PS_NOTIFICATIONS_DEBUG                          0x10
#define PS_WIN32K_DEBUG                                 0x20
#define PS_STATE_DEBUG                                  0x40
#define PS_QUOTA_DEBUG                                  0x80

//
// Debug/Tracing support
//
#if _PS_DEBUG_
#ifdef NEW_DEBUG_SYSTEM_IMPLEMENTED // enable when Debug Filters are implemented
#define PSTRACE DbgPrintEx
#else
#define PSTRACE(x, ...)                                 \
    if (x & PspTraceLevel) DbgPrint(__VA_ARGS__)
#endif
#else
#define PSTRACE(x, ...) DPRINT(__VA_ARGS__)
#endif

//
// Maximum Count of Notification Routines
//
#define PSP_MAX_CREATE_THREAD_NOTIFY            8
#define PSP_MAX_LOAD_IMAGE_NOTIFY               8
#define PSP_MAX_CREATE_PROCESS_NOTIFY           8

//
// Maximum Job Scheduling Classes
//
#define PSP_JOB_SCHEDULING_CLASSES              10

//
// Initialization Functions
//
VOID
NTAPI
PspShutdownProcessManager(
    VOID
);

VOID
INIT_FUNCTION
NTAPI
PsInitThreadManagment(
    VOID
);

VOID
INIT_FUNCTION
NTAPI
PiInitProcessManager(
    VOID
);

VOID
INIT_FUNCTION
NTAPI
PsInitProcessManagment(
    VOID
);

VOID
INIT_FUNCTION
NTAPI
PsInitIdleThread(
    VOID
);

NTSTATUS
NTAPI
PsInitializeIdleOrFirstThread(
    IN PEPROCESS Process,
    OUT PETHREAD* ThreadPtr,
    IN PKSTART_ROUTINE StartRoutine,
    IN KPROCESSOR_MODE AccessMode,
    IN BOOLEAN First
);

VOID
NTAPI
INIT_FUNCTION
PsInitJobManagment(
    VOID
);

//
// Utility Routines
//
PETHREAD
NTAPI
PsGetNextProcessThread(
    IN PEPROCESS Process,
    IN PETHREAD Thread OPTIONAL
);

PEPROCESS
NTAPI
PsGetNextProcess(
    IN PEPROCESS OldProcess OPTIONAL
);

NTSTATUS
NTAPI
PspMapSystemDll(
    IN PEPROCESS Process,
    OUT PVOID *DllBase
);

NTSTATUS
NTAPI
PsLocateSystemDll(
    VOID
);

NTSTATUS
NTAPI
PspGetSystemDllEntryPoints(
    VOID
);

//
// Security Routines
//
PACCESS_TOKEN
NTAPI
PsReferenceEffectiveToken(
    IN PETHREAD Thread,
    OUT PTOKEN_TYPE TokenType,
    OUT PUCHAR b,
    OUT PSECURITY_IMPERSONATION_LEVEL Level
);

NTSTATUS
NTAPI
PsOpenTokenOfProcess(
    IN HANDLE ProcessHandle,
    OUT PACCESS_TOKEN* Token
);

NTSTATUS
NTAPI
PspSetPrimaryToken(
    IN PEPROCESS Process,
    IN HANDLE TokenHandle OPTIONAL,
    IN PTOKEN Token OPTIONAL
);

NTSTATUS
NTAPI
PspInitializeProcessSecurity(
    IN PEPROCESS Process,
    IN PEPROCESS Parent OPTIONAL
);

VOID
NTAPI
PspDeleteProcessSecurity(
    IN PEPROCESS Process
);

VOID
NTAPI
PspDeleteThreadSecurity(
    IN PETHREAD Thread
);

//
// Reaping and Deletion
//
VOID
NTAPI
PsExitSpecialApc(
    PKAPC Apc,
    PKNORMAL_ROUTINE *NormalRoutine,
    PVOID *NormalContext,
    PVOID *SystemArgument1,
    PVOID *SystemArgument2
);

VOID
NTAPI
PspReapRoutine(
    IN PVOID Context
);

VOID
NTAPI
PspExitThread(
    IN NTSTATUS ExitStatus
);

NTSTATUS
NTAPI
PspTerminateThreadByPointer(
    IN PETHREAD Thread,
    IN NTSTATUS ExitStatus,
    IN BOOLEAN bSelf
);

VOID
NTAPI
PspExitProcess(
    IN BOOLEAN LastThread,
    IN PEPROCESS Process
);

VOID
NTAPI
PspDeleteProcess(
    IN PVOID ObjectBody
);

VOID
NTAPI
PspDeleteThread(
    IN PVOID ObjectBody
);

//
// Thread/Process Startup
//
VOID
NTAPI
PspSystemThreadStartup(
    PKSTART_ROUTINE StartRoutine,
    PVOID StartContext
);

VOID
NTAPI
PsIdleThreadMain(
    IN PVOID Context
);

//
// Quota Support
//
VOID
NTAPI
PspInheritQuota(
    IN PEPROCESS Process,
    IN PEPROCESS ParentProcess
);

VOID
NTAPI
PspDestroyQuotaBlock(
    IN PEPROCESS Process
);

//
// VDM Support
//
NTSTATUS
NTAPI
PspDeleteLdt(
    IN PEPROCESS Process
);

NTSTATUS
NTAPI
PspDeleteVdmObjects(
    IN PEPROCESS Process
);

//
// Job Routines
//
VOID
NTAPI
PspExitProcessFromJob(
    IN PEJOB Job,
    IN PEPROCESS Process
);

VOID
NTAPI
PspRemoveProcessFromJob(
    IN PEPROCESS Process,
    IN PEJOB Job
);

//
// Global data inside the Process Manager
//
extern ULONG PspTraceLevel;
extern LCID PsDefaultThreadLocaleId;
extern LCID PsDefaultSystemLocaleId;
extern LIST_ENTRY PspReaperListHead;
extern WORK_QUEUE_ITEM PspReaperWorkItem;
extern BOOLEAN PspReaping;
extern PEPROCESS PsInitialSystemProcess;
extern PEPROCESS PsIdleProcess;
extern LIST_ENTRY PsActiveProcessHead;
extern KGUARDED_MUTEX PspActiveProcessMutex;
extern LARGE_INTEGER ShortPsLockDelay;
extern EPROCESS_QUOTA_BLOCK PspDefaultQuotaBlock;
extern PHANDLE_TABLE PspCidTable;
extern PCREATE_THREAD_NOTIFY_ROUTINE
PspThreadNotifyRoutine[PSP_MAX_CREATE_THREAD_NOTIFY];
extern PCREATE_PROCESS_NOTIFY_ROUTINE
PspProcessNotifyRoutine[PSP_MAX_CREATE_PROCESS_NOTIFY];
extern PLOAD_IMAGE_NOTIFY_ROUTINE
PspLoadImageNotifyRoutine[PSP_MAX_LOAD_IMAGE_NOTIFY];
extern PLEGO_NOTIFY_ROUTINE PspLegoNotifyRoutine;
extern ULONG PspThreadNotifyRoutineCount;
extern PKWIN32_PROCESS_CALLOUT PspW32ProcessCallout;
extern PKWIN32_THREAD_CALLOUT PspW32ThreadCallout;
extern PVOID PspSystemDllEntryPoint;
extern PVOID PspSystemDllBase;
extern BOOLEAN PspUseJobSchedulingClasses;
extern CHAR PspJobSchedulingClasses[PSP_JOB_SCHEDULING_CLASSES];

//
// Inlined Functions
//
#include "ps_x.h"
