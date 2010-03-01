/******************************************************************************
 *
 * Name: acinterp.h - Interpreter subcomponent prototypes and defines
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2009, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code.  No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#ifndef __ACINTERP_H__
#define __ACINTERP_H__


#define ACPI_WALK_OPERANDS          (&(WalkState->Operands [WalkState->NumOperands -1]))

/* Macros for tables used for debug output */

#define ACPI_EXD_OFFSET(f)          (UINT8) ACPI_OFFSET (ACPI_OPERAND_OBJECT,f)
#define ACPI_EXD_NSOFFSET(f)        (UINT8) ACPI_OFFSET (ACPI_NAMESPACE_NODE,f)
#define ACPI_EXD_TABLE_SIZE(name)   (sizeof(name) / sizeof (ACPI_EXDUMP_INFO))

/*
 * If possible, pack the following structures to byte alignment, since we
 * don't care about performance for debug output. Two cases where we cannot
 * pack the structures:
 *
 * 1) Hardware does not support misaligned memory transfers
 * 2) Compiler does not support pointers within packed structures
 */
#if (!defined(ACPI_MISALIGNMENT_NOT_SUPPORTED) && !defined(ACPI_PACKED_POINTERS_NOT_SUPPORTED))
#pragma pack(1)
#endif

typedef const struct acpi_exdump_info
{
    UINT8                   Opcode;
    UINT8                   Offset;
    char                    *Name;

} ACPI_EXDUMP_INFO;

/* Values for the Opcode field above */

#define ACPI_EXD_INIT                   0
#define ACPI_EXD_TYPE                   1
#define ACPI_EXD_UINT8                  2
#define ACPI_EXD_UINT16                 3
#define ACPI_EXD_UINT32                 4
#define ACPI_EXD_UINT64                 5
#define ACPI_EXD_LITERAL                6
#define ACPI_EXD_POINTER                7
#define ACPI_EXD_ADDRESS                8
#define ACPI_EXD_STRING                 9
#define ACPI_EXD_BUFFER                 10
#define ACPI_EXD_PACKAGE                11
#define ACPI_EXD_FIELD                  12
#define ACPI_EXD_REFERENCE              13

/* restore default alignment */

#pragma pack()


/*
 * exconvrt - object conversion
 */
ACPI_STATUS
AcpiExConvertToInteger (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **ResultDesc,
    UINT32                  Flags);

ACPI_STATUS
AcpiExConvertToBuffer (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **ResultDesc);

ACPI_STATUS
AcpiExConvertToString (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **ResultDesc,
    UINT32                  Type);

/* Types for ->String conversion */

#define ACPI_EXPLICIT_BYTE_COPY         0x00000000
#define ACPI_EXPLICIT_CONVERT_HEX       0x00000001
#define ACPI_IMPLICIT_CONVERT_HEX       0x00000002
#define ACPI_EXPLICIT_CONVERT_DECIMAL   0x00000003

ACPI_STATUS
AcpiExConvertToTargetType (
    ACPI_OBJECT_TYPE        DestinationType,
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     **ResultDesc,
    ACPI_WALK_STATE         *WalkState);


/*
 * exfield - ACPI AML (p-code) execution - field manipulation
 */
ACPI_STATUS
AcpiExCommonBufferSetup (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    UINT32                  BufferLength,
    UINT32                  *DatumCount);

ACPI_STATUS
AcpiExWriteWithUpdateRule (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_INTEGER            Mask,
    ACPI_INTEGER            FieldValue,
    UINT32                  FieldDatumByteOffset);

void
AcpiExGetBufferDatum(
    ACPI_INTEGER            *Datum,
    void                    *Buffer,
    UINT32                  BufferLength,
    UINT32                  ByteGranularity,
    UINT32                  BufferOffset);

void
AcpiExSetBufferDatum (
    ACPI_INTEGER            MergedDatum,
    void                    *Buffer,
    UINT32                  BufferLength,
    UINT32                  ByteGranularity,
    UINT32                  BufferOffset);

ACPI_STATUS
AcpiExReadDataFromField (
    ACPI_WALK_STATE         *WalkState,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **RetBufferDesc);

ACPI_STATUS
AcpiExWriteDataToField (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **ResultDesc);


/*
 * exfldio - low level field I/O
 */
ACPI_STATUS
AcpiExExtractFromField (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    void                    *Buffer,
    UINT32                  BufferLength);

ACPI_STATUS
AcpiExInsertIntoField (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    void                    *Buffer,
    UINT32                  BufferLength);

ACPI_STATUS
AcpiExAccessRegion (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    UINT32                  FieldDatumByteOffset,
    ACPI_INTEGER            *Value,
    UINT32                  ReadWrite);


/*
 * exmisc - misc support routines
 */
ACPI_STATUS
AcpiExGetObjectReference (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **ReturnDesc,
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExConcatTemplate (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     *ObjDesc2,
    ACPI_OPERAND_OBJECT     **ActualReturnDesc,
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExDoConcatenate (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     *ObjDesc2,
    ACPI_OPERAND_OBJECT     **ActualReturnDesc,
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExDoLogicalNumericOp (
    UINT16                  Opcode,
    ACPI_INTEGER            Integer0,
    ACPI_INTEGER            Integer1,
    BOOLEAN                 *LogicalResult);

ACPI_STATUS
AcpiExDoLogicalOp (
    UINT16                  Opcode,
    ACPI_OPERAND_OBJECT     *Operand0,
    ACPI_OPERAND_OBJECT     *Operand1,
    BOOLEAN                 *LogicalResult);

ACPI_INTEGER
AcpiExDoMathOp (
    UINT16                  Opcode,
    ACPI_INTEGER            Operand0,
    ACPI_INTEGER            Operand1);

ACPI_STATUS
AcpiExCreateMutex (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExCreateProcessor (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExCreatePowerResource (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExCreateRegion (
    UINT8                   *AmlStart,
    UINT32                  AmlLength,
    UINT8                   RegionSpace,
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExCreateEvent (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExCreateAlias (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExCreateMethod (
    UINT8                   *AmlStart,
    UINT32                  AmlLength,
    ACPI_WALK_STATE         *WalkState);


/*
 * exconfig - dynamic table load/unload
 */
ACPI_STATUS
AcpiExLoadOp (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     *Target,
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExLoadTableOp (
    ACPI_WALK_STATE         *WalkState,
    ACPI_OPERAND_OBJECT     **ReturnDesc);

ACPI_STATUS
AcpiExUnloadTable (
    ACPI_OPERAND_OBJECT     *DdbHandle);


/*
 * exmutex - mutex support
 */
ACPI_STATUS
AcpiExAcquireMutex (
    ACPI_OPERAND_OBJECT     *TimeDesc,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExAcquireMutexObject (
    UINT16                  Timeout,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_THREAD_ID          ThreadId);

ACPI_STATUS
AcpiExReleaseMutex (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExReleaseMutexObject (
    ACPI_OPERAND_OBJECT     *ObjDesc);

void
AcpiExReleaseAllMutexes (
    ACPI_THREAD_STATE       *Thread);

void
AcpiExUnlinkMutex (
    ACPI_OPERAND_OBJECT     *ObjDesc);


/*
 * exprep - ACPI AML execution - prep utilities
 */
ACPI_STATUS
AcpiExPrepCommonFieldObject (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    UINT8                   FieldFlags,
    UINT8                   FieldAttribute,
    UINT32                  FieldBitPosition,
    UINT32                  FieldBitLength);

ACPI_STATUS
AcpiExPrepFieldValue (
    ACPI_CREATE_FIELD_INFO  *Info);


/*
 * exsystem - Interface to OS services
 */
ACPI_STATUS
AcpiExSystemDoNotifyOp (
    ACPI_OPERAND_OBJECT     *Value,
    ACPI_OPERAND_OBJECT     *ObjDesc);

ACPI_STATUS
AcpiExSystemDoSuspend(
    ACPI_INTEGER            Time);

ACPI_STATUS
AcpiExSystemDoStall (
    UINT32                  Time);

ACPI_STATUS
AcpiExSystemSignalEvent(
    ACPI_OPERAND_OBJECT     *ObjDesc);

ACPI_STATUS
AcpiExSystemWaitEvent(
    ACPI_OPERAND_OBJECT     *Time,
    ACPI_OPERAND_OBJECT     *ObjDesc);

ACPI_STATUS
AcpiExSystemResetEvent(
    ACPI_OPERAND_OBJECT     *ObjDesc);

ACPI_STATUS
AcpiExSystemWaitSemaphore (
    ACPI_SEMAPHORE          Semaphore,
    UINT16                  Timeout);

ACPI_STATUS
AcpiExSystemWaitMutex (
    ACPI_MUTEX              Mutex,
    UINT16                  Timeout);

/*
 * exoparg1 - ACPI AML execution, 1 operand
 */
ACPI_STATUS
AcpiExOpcode_0A_0T_1R (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExOpcode_1A_0T_0R (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExOpcode_1A_0T_1R (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExOpcode_1A_1T_1R (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExOpcode_1A_1T_0R (
    ACPI_WALK_STATE         *WalkState);

/*
 * exoparg2 - ACPI AML execution, 2 operands
 */
ACPI_STATUS
AcpiExOpcode_2A_0T_0R (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExOpcode_2A_0T_1R (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExOpcode_2A_1T_1R (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExOpcode_2A_2T_1R (
    ACPI_WALK_STATE         *WalkState);


/*
 * exoparg3 - ACPI AML execution, 3 operands
 */
ACPI_STATUS
AcpiExOpcode_3A_0T_0R (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExOpcode_3A_1T_1R (
    ACPI_WALK_STATE         *WalkState);


/*
 * exoparg6 - ACPI AML execution, 6 operands
 */
ACPI_STATUS
AcpiExOpcode_6A_0T_1R (
    ACPI_WALK_STATE         *WalkState);


/*
 * exresolv - Object resolution and get value functions
 */
ACPI_STATUS
AcpiExResolveToValue (
    ACPI_OPERAND_OBJECT     **StackPtr,
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExResolveMultiple (
    ACPI_WALK_STATE         *WalkState,
    ACPI_OPERAND_OBJECT     *Operand,
    ACPI_OBJECT_TYPE        *ReturnType,
    ACPI_OPERAND_OBJECT     **ReturnDesc);


/*
 * exresnte - resolve namespace node
 */
ACPI_STATUS
AcpiExResolveNodeToValue (
    ACPI_NAMESPACE_NODE     **StackPtr,
    ACPI_WALK_STATE         *WalkState);


/*
 * exresop - resolve operand to value
 */
ACPI_STATUS
AcpiExResolveOperands (
    UINT16                  Opcode,
    ACPI_OPERAND_OBJECT     **StackPtr,
    ACPI_WALK_STATE         *WalkState);


/*
 * exdump - Interpreter debug output routines
 */
void
AcpiExDumpOperand (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    UINT32                  Depth);

void
AcpiExDumpOperands (
    ACPI_OPERAND_OBJECT     **Operands,
    const char              *OpcodeName,
    UINT32                  NumOpcodes);

void
AcpiExDumpObjectDescriptor (
    ACPI_OPERAND_OBJECT     *Object,
    UINT32                  Flags);

void
AcpiExDumpNamespaceNode (
    ACPI_NAMESPACE_NODE     *Node,
    UINT32                  Flags);


/*
 * exnames - AML namestring support
 */
ACPI_STATUS
AcpiExGetNameString (
    ACPI_OBJECT_TYPE        DataType,
    UINT8                   *InAmlAddress,
    char                    **OutNameString,
    UINT32                  *OutNameLength);


/*
 * exstore - Object store support
 */
ACPI_STATUS
AcpiExStore (
    ACPI_OPERAND_OBJECT     *ValDesc,
    ACPI_OPERAND_OBJECT     *DestDesc,
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExStoreObjectToNode (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_WALK_STATE         *WalkState,
    UINT8                   ImplicitConversion);

#define ACPI_IMPLICIT_CONVERSION        TRUE
#define ACPI_NO_IMPLICIT_CONVERSION     FALSE


/*
 * exstoren - resolve/store object
 */
ACPI_STATUS
AcpiExResolveObject (
    ACPI_OPERAND_OBJECT     **SourceDescPtr,
    ACPI_OBJECT_TYPE        TargetType,
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExStoreObjectToObject (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     *DestDesc,
    ACPI_OPERAND_OBJECT     **NewDesc,
    ACPI_WALK_STATE         *WalkState);


/*
 * exstorob - store object - buffer/string
 */
ACPI_STATUS
AcpiExStoreBufferToBuffer (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     *TargetDesc);

ACPI_STATUS
AcpiExStoreStringToString (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     *TargetDesc);


/*
 * excopy - object copy
 */
ACPI_STATUS
AcpiExCopyIntegerToIndexField (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     *TargetDesc);

ACPI_STATUS
AcpiExCopyIntegerToBankField (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     *TargetDesc);

ACPI_STATUS
AcpiExCopyDataToNamedField (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_NAMESPACE_NODE     *Node);

ACPI_STATUS
AcpiExCopyIntegerToBufferField (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     *TargetDesc);


/*
 * exutils - interpreter/scanner utilities
 */
void
AcpiExEnterInterpreter (
    void);

void
AcpiExExitInterpreter (
    void);

void
AcpiExReacquireInterpreter (
    void);

void
AcpiExRelinquishInterpreter (
    void);

void
AcpiExTruncateFor32bitTable (
    ACPI_OPERAND_OBJECT     *ObjDesc);

void
AcpiExAcquireGlobalLock (
    UINT32                  Rule);

void
AcpiExReleaseGlobalLock (
    UINT32                  Rule);

void
AcpiExEisaIdToString (
    char                    *Dest,
    ACPI_INTEGER            CompressedId);

void
AcpiExIntegerToString (
    char                    *Dest,
    ACPI_INTEGER            Value);


/*
 * exregion - default OpRegion handlers
 */
ACPI_STATUS
AcpiExSystemMemorySpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    ACPI_INTEGER            *Value,
    void                    *HandlerContext,
    void                    *RegionContext);

ACPI_STATUS
AcpiExSystemIoSpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    ACPI_INTEGER            *Value,
    void                    *HandlerContext,
    void                    *RegionContext);

ACPI_STATUS
AcpiExPciConfigSpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    ACPI_INTEGER            *Value,
    void                    *HandlerContext,
    void                    *RegionContext);

ACPI_STATUS
AcpiExCmosSpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    ACPI_INTEGER            *Value,
    void                    *HandlerContext,
    void                    *RegionContext);

ACPI_STATUS
AcpiExPciBarSpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    ACPI_INTEGER            *Value,
    void                    *HandlerContext,
    void                    *RegionContext);

ACPI_STATUS
AcpiExEmbeddedControllerSpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    ACPI_INTEGER            *Value,
    void                    *HandlerContext,
    void                    *RegionContext);

ACPI_STATUS
AcpiExSmBusSpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    ACPI_INTEGER            *Value,
    void                    *HandlerContext,
    void                    *RegionContext);


ACPI_STATUS
AcpiExDataTableSpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    ACPI_INTEGER            *Value,
    void                    *HandlerContext,
    void                    *RegionContext);

#endif /* __INTERP_H__ */
