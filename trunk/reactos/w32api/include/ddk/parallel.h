/*
 * parallel.h
 *
 * ParPort driver interface
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __PARALLEL_H
#define __PARALLEL_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push,4)

#include "ntddk.h"
#include "ntddpar.h"


#define DD_PARALLEL_PORT_BASE_NAME        "ParallelPort"
#define DD_PARALLEL_PORT_BASE_NAME_U      L"ParallelPort"

#define IOCTL_INTERNAL_DESELECT_DEVICE \
  CTL_CODE (FILE_DEVICE_PARALLEL_PORT, 24, METHOD_BUFFERED, FILE_ANY_ACCESS) 
#define IOCTL_INTERNAL_GET_MORE_PARALLEL_PORT_INFO \
  CTL_CODE (FILE_DEVICE_PARALLEL_PORT, 17, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_GET_PARALLEL_PNP_INFO \
  CTL_CODE (FILE_DEVICE_PARALLEL_PORT, 21, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_GET_PARALLEL_PORT_INFO \
  CTL_CODE (FILE_DEVICE_PARALLEL_PORT, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_INIT_1284_3_BUS \
  CTL_CODE (FILE_DEVICE_PARALLEL_PORT, 22, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_PARALLEL_CLEAR_CHIP_MODE \
  CTL_CODE (FILE_DEVICE_PARALLEL_PORT, 20, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_PARALLEL_CONNECT_INTERRUPT \
  CTL_CODE (FILE_DEVICE_PARALLEL_PORT, 13, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_PARALLEL_DISCONNECT_INTERRUPT \
  CTL_CODE (FILE_DEVICE_PARALLEL_PORT, 14, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE \
  CTL_CODE (FILE_DEVICE_PARALLEL_PORT, 11, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_PARALLEL_PORT_FREE \
  CTL_CODE (FILE_DEVICE_PARALLEL_PORT, 40, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_PARALLEL_SET_CHIP_MODE \
  CTL_CODE (FILE_DEVICE_PARALLEL_PORT, 19, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_RELEASE_PARALLEL_PORT_INFO \
  CTL_CODE (FILE_DEVICE_PARALLEL_PORT, 15, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_SELECT_DEVICE \
  CTL_CODE (FILE_DEVICE_PARALLEL_PORT, 23, METHOD_BUFFERED, FILE_ANY_ACCESS)


typedef struct _PARALLEL_1284_COMMAND {
  UCHAR  ID;
  UCHAR  Port;
  ULONG  CommandFlags;
} PARALLEL_1284_COMMAND, *PPARALLEL_1284_COMMAND;

/* PARALLEL_1284_COMMAND.CommandFlags */
#define PAR_END_OF_CHAIN_DEVICE           0x00000001
#define PAR_HAVE_PORT_KEEP_PORT           0x00000002

typedef struct _MORE_PARALLEL_PORT_INFORMATION {
	INTERFACE_TYPE  InterfaceType;
	ULONG  BusNumber;
	ULONG  InterruptLevel;
	ULONG  InterruptVector;
	KAFFINITY  InterruptAffinity;
	KINTERRUPT_MODE  InterruptMode;
} MORE_PARALLEL_PORT_INFORMATION, *PMORE_PARALLEL_PORT_INFORMATION;

typedef NTSTATUS DDKAPI
(*PPARALLEL_SET_CHIP_MODE)(
	IN  PVOID  SetChipContext,
	IN  UCHAR  ChipMode);

typedef NTSTATUS DDKAPI
(*PPARALLEL_CLEAR_CHIP_MODE)(
	IN  PVOID  ClearChipContext,
	IN  UCHAR  ChipMode);

typedef NTSTATUS DDKAPI
(*PPARCHIP_CLEAR_CHIP_MODE)(
	IN  PVOID  ClearChipContext,
	IN  UCHAR  ChipMode);

typedef NTSTATUS DDKAPI
(*PPARALLEL_TRY_SELECT_ROUTINE)(
	IN  PVOID  TrySelectContext,
	IN  PVOID  TrySelectCommand);

typedef NTSTATUS DDKAPI
(*PPARALLEL_DESELECT_ROUTINE)(
	IN PVOID  DeselectContext,
	IN PVOID  DeselectCommand);

/* PARALLEL_PNP_INFORMATION.HardwareCapabilities */
#define PPT_NO_HARDWARE_PRESENT           0x00000000
#define PPT_ECP_PRESENT                   0x00000001
#define PPT_EPP_PRESENT                   0x00000002
#define PPT_EPP_32_PRESENT                0x00000004
#define PPT_BYTE_PRESENT                  0x00000008
#define PPT_BIDI_PRESENT                  0x00000008
#define PPT_1284_3_PRESENT                0x00000010

typedef struct _PARALLEL_PNP_INFORMATION {
  PHYSICAL_ADDRESS  OriginalEcpController;
  PUCHAR  EcpController;
  ULONG  SpanOfEcpController;
  ULONG  PortNumber;
  ULONG  HardwareCapabilities;
  PPARALLEL_SET_CHIP_MODE  TrySetChipMode;
  PPARALLEL_CLEAR_CHIP_MODE  ClearChipMode;
  ULONG  FifoDepth;
  ULONG  FifoWidth;
  PHYSICAL_ADDRESS  EppControllerPhysicalAddress;
  ULONG  SpanOfEppController;
  ULONG  Ieee1284_3DeviceCount;
  PPARALLEL_TRY_SELECT_ROUTINE  TrySelectDevice;
  PPARALLEL_DESELECT_ROUTINE  DeselectDevice;
  PVOID  Context;
  ULONG  CurrentMode;
  PWSTR  PortName;
} PARALLEL_PNP_INFORMATION, *PPARALLEL_PNP_INFORMATION;

typedef BOOLEAN DDKAPI
(*PPARALLEL_TRY_ALLOCATE_ROUTINE)(
  IN  PVOID  TryAllocateContext);

typedef VOID DDKAPI
(*PPARALLEL_FREE_ROUTINE)(
  IN  PVOID  FreeContext);

typedef ULONG DDKAPI
(*PPARALLEL_QUERY_WAITERS_ROUTINE)(
	IN  PVOID  QueryAllocsContext);

typedef struct _PARALLEL_PORT_INFORMATION {
  PHYSICAL_ADDRESS  OriginalController;
  PUCHAR  Controller;
  ULONG  SpanOfController;
  PPARALLEL_TRY_ALLOCATE_ROUTINE  TryAllocatePort;
  PPARALLEL_FREE_ROUTINE  FreePort;
  PPARALLEL_QUERY_WAITERS_ROUTINE  QueryNumWaiters;
  PVOID  Context;
} PARALLEL_PORT_INFORMATION, *PPARALLEL_PORT_INFORMATION;

/* PARALLEL_CHIP_MODE.ModeFlags */
#define INITIAL_MODE                      0x00
#define PARCHIP_ECR_ARBITRATOR            0x01

typedef struct _PARALLEL_CHIP_MODE {
  UCHAR  ModeFlags;
  BOOLEAN  success;
} PARALLEL_CHIP_MODE, *PPARALLEL_CHIP_MODE;

typedef VOID DDKAPI
(*PPARALLEL_DEFERRED_ROUTINE)(
	IN  PVOID  DeferredContext);

typedef struct _PARALLEL_INTERRUPT_SERVICE_ROUTINE {
  PKSERVICE_ROUTINE  InterruptServiceRoutine;
  PVOID  InterruptServiceContext;
  PPARALLEL_DEFERRED_ROUTINE  DeferredPortCheckRoutine;
  PVOID  DeferredPortCheckContext;
} PARALLEL_INTERRUPT_SERVICE_ROUTINE, *PPARALLEL_INTERRUPT_SERVICE_ROUTINE;


#define IOCTL_INTERNAL_DISCONNECT_IDLE \
  CTL_CODE (FILE_DEVICE_PARALLEL_PORT, 32, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_LOCK_PORT \
  CTL_CODE (FILE_DEVICE_PARALLEL_PORT, 37, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_LOCK_PORT_NO_SELECT \
  CTL_CODE (FILE_DEVICE_PARALLEL_PORT, 52, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_PARCLASS_CONNECT \
  CTL_CODE (FILE_DEVICE_PARALLEL_PORT, 30, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_PARCLASS_DISCONNECT \
  CTL_CODE (FILE_DEVICE_PARALLEL_PORT, 31, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_UNLOCK_PORT \
  CTL_CODE (FILE_DEVICE_PARALLEL_PORT, 38, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_UNLOCK_PORT_NO_DESELECT \
  CTL_CODE (FILE_DEVICE_PARALLEL_PORT, 53, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef USHORT DDKAPI
(*PDETERMINE_IEEE_MODES)(
  IN PVOID  Context);

typedef enum _PARALLEL_SAFETY {
	SAFE_MODE,
	UNSAFE_MODE
} PARALLEL_SAFETY;

typedef NTSTATUS DDKAPI
(*PNEGOTIATE_IEEE_MODE)(
  IN PVOID  Context,
  IN USHORT  ModeMaskFwd,
  IN USHORT  ModeMaskRev,
  IN PARALLEL_SAFETY  ModeSafety,
  IN BOOLEAN  IsForward);

typedef NTSTATUS DDKAPI
(*PTERMINATE_IEEE_MODE)(
	IN  PVOID  Context);
	
typedef NTSTATUS DDKAPI
(*PPARALLEL_IEEE_FWD_TO_REV)(
  IN  PVOID  Context);

typedef NTSTATUS DDKAPI
(*PPARALLEL_IEEE_REV_TO_FWD)(
  IN  PVOID  Context);

typedef NTSTATUS DDKAPI
(*PPARALLEL_READ)(
	IN  PVOID  Context,
	OUT PVOID  Buffer,
	IN  ULONG  NumBytesToRead,
	OUT PULONG  NumBytesRead,
	IN  UCHAR  Channel);
	
typedef NTSTATUS DDKAPI
(*PPARALLEL_WRITE)(
	IN  PVOID  Context,
	OUT PVOID  Buffer,
	IN  ULONG  NumBytesToWrite,
	OUT PULONG  NumBytesWritten,
	IN  UCHAR   Channel);

typedef NTSTATUS DDKAPI
(*PPARALLEL_TRYSELECT_DEVICE)(
  IN  PVOID  Context,
  IN  PARALLEL_1284_COMMAND  Command);

typedef NTSTATUS DDKAPI
(*PPARALLEL_DESELECT_DEVICE)(
  IN  PVOID  Context,
  IN  PARALLEL_1284_COMMAND  Command);

typedef struct _PARCLASS_INFORMATION {
  PUCHAR  Controller;
  PUCHAR  EcrController;
  ULONG  SpanOfController;
  PDETERMINE_IEEE_MODES  DetermineIeeeModes;
  PNEGOTIATE_IEEE_MODE  NegotiateIeeeMode;
  PTERMINATE_IEEE_MODE  TerminateIeeeMode;
  PPARALLEL_IEEE_FWD_TO_REV  IeeeFwdToRevMode;
  PPARALLEL_IEEE_REV_TO_FWD  IeeeRevToFwdMode;
  PPARALLEL_READ  ParallelRead;
  PPARALLEL_WRITE  ParallelWrite;
  PVOID  ParclassContext;
  ULONG  HardwareCapabilities;
  ULONG  FifoDepth;
  ULONG  FifoWidth;
  PPARALLEL_TRYSELECT_DEVICE  ParallelTryselect;
  PPARALLEL_DESELECT_DEVICE  ParallelDeSelect;
} PARCLASS_INFORMATION, *PPARCLASS_INFORMATION;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* __PARALLEL_H */
