/*** Autogenerated by WIDL 0.3.0 from include\psdk\austream.idl - Do not edit ***/
#include <rpc.h>
#include <rpcndr.h>

#ifndef __WIDL_INCLUDE_PSDK_AUSTREAM_H
#define __WIDL_INCLUDE_PSDK_AUSTREAM_H
#ifdef __cplusplus
extern "C" {
#endif
#include <unknwn.h>
#include <mmstream.h>
#if 0
typedef struct tWAVEFORMATEX WAVEFORMATEX;
#endif
#ifndef __IAudioMediaStream_FWD_DEFINED__
#define __IAudioMediaStream_FWD_DEFINED__
typedef interface IAudioMediaStream IAudioMediaStream;
#endif

#ifndef __IAudioStreamSample_FWD_DEFINED__
#define __IAudioStreamSample_FWD_DEFINED__
typedef interface IAudioStreamSample IAudioStreamSample;
#endif

#ifndef __IMemoryData_FWD_DEFINED__
#define __IMemoryData_FWD_DEFINED__
typedef interface IMemoryData IMemoryData;
#endif

#ifndef __IAudioData_FWD_DEFINED__
#define __IAudioData_FWD_DEFINED__
typedef interface IAudioData IAudioData;
#endif

/*****************************************************************************
 * IAudioMediaStream interface
 */
#ifndef __IAudioMediaStream_INTERFACE_DEFINED__
#define __IAudioMediaStream_INTERFACE_DEFINED__

DEFINE_GUID(IID_IAudioMediaStream, 0xf7537560, 0xa3be, 0x11d0, 0x82,0x12, 0x00,0xc0,0x4f,0xc3,0x2c,0x45);
#if defined(__cplusplus) && !defined(CINTERFACE)
interface IAudioMediaStream : public IMediaStream
{
    virtual HRESULT STDMETHODCALLTYPE GetFormat(
        WAVEFORMATEX* pWaveFormatCurrent) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetFormat(
        const WAVEFORMATEX* lpWaveFormat) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateSample(
        IAudioData* pAudioData,
        DWORD dwFlags,
        IAudioStreamSample** ppSample) = 0;

};
#else
typedef struct IAudioMediaStreamVtbl {
    BEGIN_INTERFACE

    /*** IUnknown methods ***/
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IAudioMediaStream* This,
        REFIID riid,
        void** ppvObject);

    ULONG (STDMETHODCALLTYPE *AddRef)(
        IAudioMediaStream* This);

    ULONG (STDMETHODCALLTYPE *Release)(
        IAudioMediaStream* This);

    /*** IMediaStream methods ***/
    HRESULT (STDMETHODCALLTYPE *GetMultiMediaStream)(
        IAudioMediaStream* This,
        IMultiMediaStream** ppMultiMediaStream);

    HRESULT (STDMETHODCALLTYPE *GetInformation)(
        IAudioMediaStream* This,
        MSPID* pPurposeId,
        STREAM_TYPE* pType);

    HRESULT (STDMETHODCALLTYPE *SetSameFormat)(
        IAudioMediaStream* This,
        IMediaStream* pStreamThatHasDesiredFormat,
        DWORD dwFlags);

    HRESULT (STDMETHODCALLTYPE *AllocateSample)(
        IAudioMediaStream* This,
        DWORD dwFlags,
        IStreamSample** ppSample);

    HRESULT (STDMETHODCALLTYPE *CreateSharedSample)(
        IAudioMediaStream* This,
        IStreamSample* pExistingSample,
        DWORD dwFlags,
        IStreamSample** ppNewSample);

    HRESULT (STDMETHODCALLTYPE *SendEndOfStream)(
        IAudioMediaStream* This,
        DWORD dwFlags);

    /*** IAudioMediaStream methods ***/
    HRESULT (STDMETHODCALLTYPE *GetFormat)(
        IAudioMediaStream* This,
        WAVEFORMATEX* pWaveFormatCurrent);

    HRESULT (STDMETHODCALLTYPE *SetFormat)(
        IAudioMediaStream* This,
        const WAVEFORMATEX* lpWaveFormat);

    HRESULT (STDMETHODCALLTYPE *CreateSample)(
        IAudioMediaStream* This,
        IAudioData* pAudioData,
        DWORD dwFlags,
        IAudioStreamSample** ppSample);

    END_INTERFACE
} IAudioMediaStreamVtbl;
interface IAudioMediaStream {
    const IAudioMediaStreamVtbl* lpVtbl;
};

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IAudioMediaStream_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IAudioMediaStream_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IAudioMediaStream_Release(p) (p)->lpVtbl->Release(p)
/*** IMediaStream methods ***/
#define IAudioMediaStream_GetMultiMediaStream(p,a) (p)->lpVtbl->GetMultiMediaStream(p,a)
#define IAudioMediaStream_GetInformation(p,a,b) (p)->lpVtbl->GetInformation(p,a,b)
#define IAudioMediaStream_SetSameFormat(p,a,b) (p)->lpVtbl->SetSameFormat(p,a,b)
#define IAudioMediaStream_AllocateSample(p,a,b) (p)->lpVtbl->AllocateSample(p,a,b)
#define IAudioMediaStream_CreateSharedSample(p,a,b,c) (p)->lpVtbl->CreateSharedSample(p,a,b,c)
#define IAudioMediaStream_SendEndOfStream(p,a) (p)->lpVtbl->SendEndOfStream(p,a)
/*** IAudioMediaStream methods ***/
#define IAudioMediaStream_GetFormat(p,a) (p)->lpVtbl->GetFormat(p,a)
#define IAudioMediaStream_SetFormat(p,a) (p)->lpVtbl->SetFormat(p,a)
#define IAudioMediaStream_CreateSample(p,a,b,c) (p)->lpVtbl->CreateSample(p,a,b,c)
#endif

#endif

HRESULT CALLBACK IAudioMediaStream_GetFormat_Proxy(
    IAudioMediaStream* This,
    WAVEFORMATEX* pWaveFormatCurrent);
void __RPC_STUB IAudioMediaStream_GetFormat_Stub(
    IRpcStubBuffer* This,
    IRpcChannelBuffer* pRpcChannelBuffer,
    PRPC_MESSAGE pRpcMessage,
    DWORD* pdwStubPhase);
HRESULT CALLBACK IAudioMediaStream_SetFormat_Proxy(
    IAudioMediaStream* This,
    const WAVEFORMATEX* lpWaveFormat);
void __RPC_STUB IAudioMediaStream_SetFormat_Stub(
    IRpcStubBuffer* This,
    IRpcChannelBuffer* pRpcChannelBuffer,
    PRPC_MESSAGE pRpcMessage,
    DWORD* pdwStubPhase);
HRESULT CALLBACK IAudioMediaStream_CreateSample_Proxy(
    IAudioMediaStream* This,
    IAudioData* pAudioData,
    DWORD dwFlags,
    IAudioStreamSample** ppSample);
void __RPC_STUB IAudioMediaStream_CreateSample_Stub(
    IRpcStubBuffer* This,
    IRpcChannelBuffer* pRpcChannelBuffer,
    PRPC_MESSAGE pRpcMessage,
    DWORD* pdwStubPhase);

#endif  /* __IAudioMediaStream_INTERFACE_DEFINED__ */

/*****************************************************************************
 * IAudioStreamSample interface
 */
#ifndef __IAudioStreamSample_INTERFACE_DEFINED__
#define __IAudioStreamSample_INTERFACE_DEFINED__

DEFINE_GUID(IID_IAudioStreamSample, 0x345fee00, 0xaba5, 0x11d0, 0x82,0x12, 0x00,0xc0,0x4f,0xc3,0x2c,0x45);
#if defined(__cplusplus) && !defined(CINTERFACE)
interface IAudioStreamSample : public IStreamSample
{
    virtual HRESULT STDMETHODCALLTYPE GetAudioData(
        IAudioData** ppAudio) = 0;

};
#else
typedef struct IAudioStreamSampleVtbl {
    BEGIN_INTERFACE

    /*** IUnknown methods ***/
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IAudioStreamSample* This,
        REFIID riid,
        void** ppvObject);

    ULONG (STDMETHODCALLTYPE *AddRef)(
        IAudioStreamSample* This);

    ULONG (STDMETHODCALLTYPE *Release)(
        IAudioStreamSample* This);

    /*** IStreamSample methods ***/
    HRESULT (STDMETHODCALLTYPE *GetMediaStream)(
        IAudioStreamSample* This,
        IMediaStream** ppMediaStream);

    HRESULT (STDMETHODCALLTYPE *GetSampleTimes)(
        IAudioStreamSample* This,
        STREAM_TIME* pStartTime,
        STREAM_TIME* pEndTime,
        STREAM_TIME* pCurrentTime);

    HRESULT (STDMETHODCALLTYPE *SetSampleTimes)(
        IAudioStreamSample* This,
        const STREAM_TIME* pStartTime,
        const STREAM_TIME* pEndTime);

    HRESULT (STDMETHODCALLTYPE *Update)(
        IAudioStreamSample* This,
        DWORD dwFlags,
        HANDLE hEvent,
        PAPCFUNC pfnAPC,
        DWORD dwAPCData);

    HRESULT (STDMETHODCALLTYPE *CompletionStatus)(
        IAudioStreamSample* This,
        DWORD dwFlags,
        DWORD dwMilliseconds);

    /*** IAudioStreamSample methods ***/
    HRESULT (STDMETHODCALLTYPE *GetAudioData)(
        IAudioStreamSample* This,
        IAudioData** ppAudio);

    END_INTERFACE
} IAudioStreamSampleVtbl;
interface IAudioStreamSample {
    const IAudioStreamSampleVtbl* lpVtbl;
};

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IAudioStreamSample_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IAudioStreamSample_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IAudioStreamSample_Release(p) (p)->lpVtbl->Release(p)
/*** IStreamSample methods ***/
#define IAudioStreamSample_GetMediaStream(p,a) (p)->lpVtbl->GetMediaStream(p,a)
#define IAudioStreamSample_GetSampleTimes(p,a,b,c) (p)->lpVtbl->GetSampleTimes(p,a,b,c)
#define IAudioStreamSample_SetSampleTimes(p,a,b) (p)->lpVtbl->SetSampleTimes(p,a,b)
#define IAudioStreamSample_Update(p,a,b,c,d) (p)->lpVtbl->Update(p,a,b,c,d)
#define IAudioStreamSample_CompletionStatus(p,a,b) (p)->lpVtbl->CompletionStatus(p,a,b)
/*** IAudioStreamSample methods ***/
#define IAudioStreamSample_GetAudioData(p,a) (p)->lpVtbl->GetAudioData(p,a)
#endif

#endif

HRESULT CALLBACK IAudioStreamSample_GetAudioData_Proxy(
    IAudioStreamSample* This,
    IAudioData** ppAudio);
void __RPC_STUB IAudioStreamSample_GetAudioData_Stub(
    IRpcStubBuffer* This,
    IRpcChannelBuffer* pRpcChannelBuffer,
    PRPC_MESSAGE pRpcMessage,
    DWORD* pdwStubPhase);

#endif  /* __IAudioStreamSample_INTERFACE_DEFINED__ */

/*****************************************************************************
 * IMemoryData interface
 */
#ifndef __IMemoryData_INTERFACE_DEFINED__
#define __IMemoryData_INTERFACE_DEFINED__

DEFINE_GUID(IID_IMemoryData, 0x327fc560, 0xaf60, 0x11d0, 0x82,0x12, 0x00,0xc0,0x4f,0xc3,0x2c,0x45);
#if defined(__cplusplus) && !defined(CINTERFACE)
interface IMemoryData : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE SetBuffer(
        DWORD cbSize,
        BYTE* pbData,
        DWORD dwFlags) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetInfo(
        DWORD* pdwLength,
        BYTE** ppbData,
        DWORD* pcbActualData) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetActual(
        DWORD cbDataValid) = 0;

};
#else
typedef struct IMemoryDataVtbl {
    BEGIN_INTERFACE

    /*** IUnknown methods ***/
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IMemoryData* This,
        REFIID riid,
        void** ppvObject);

    ULONG (STDMETHODCALLTYPE *AddRef)(
        IMemoryData* This);

    ULONG (STDMETHODCALLTYPE *Release)(
        IMemoryData* This);

    /*** IMemoryData methods ***/
    HRESULT (STDMETHODCALLTYPE *SetBuffer)(
        IMemoryData* This,
        DWORD cbSize,
        BYTE* pbData,
        DWORD dwFlags);

    HRESULT (STDMETHODCALLTYPE *GetInfo)(
        IMemoryData* This,
        DWORD* pdwLength,
        BYTE** ppbData,
        DWORD* pcbActualData);

    HRESULT (STDMETHODCALLTYPE *SetActual)(
        IMemoryData* This,
        DWORD cbDataValid);

    END_INTERFACE
} IMemoryDataVtbl;
interface IMemoryData {
    const IMemoryDataVtbl* lpVtbl;
};

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IMemoryData_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IMemoryData_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IMemoryData_Release(p) (p)->lpVtbl->Release(p)
/*** IMemoryData methods ***/
#define IMemoryData_SetBuffer(p,a,b,c) (p)->lpVtbl->SetBuffer(p,a,b,c)
#define IMemoryData_GetInfo(p,a,b,c) (p)->lpVtbl->GetInfo(p,a,b,c)
#define IMemoryData_SetActual(p,a) (p)->lpVtbl->SetActual(p,a)
#endif

#endif

HRESULT CALLBACK IMemoryData_SetBuffer_Proxy(
    IMemoryData* This,
    DWORD cbSize,
    BYTE* pbData,
    DWORD dwFlags);
void __RPC_STUB IMemoryData_SetBuffer_Stub(
    IRpcStubBuffer* This,
    IRpcChannelBuffer* pRpcChannelBuffer,
    PRPC_MESSAGE pRpcMessage,
    DWORD* pdwStubPhase);
HRESULT CALLBACK IMemoryData_GetInfo_Proxy(
    IMemoryData* This,
    DWORD* pdwLength,
    BYTE** ppbData,
    DWORD* pcbActualData);
void __RPC_STUB IMemoryData_GetInfo_Stub(
    IRpcStubBuffer* This,
    IRpcChannelBuffer* pRpcChannelBuffer,
    PRPC_MESSAGE pRpcMessage,
    DWORD* pdwStubPhase);
HRESULT CALLBACK IMemoryData_SetActual_Proxy(
    IMemoryData* This,
    DWORD cbDataValid);
void __RPC_STUB IMemoryData_SetActual_Stub(
    IRpcStubBuffer* This,
    IRpcChannelBuffer* pRpcChannelBuffer,
    PRPC_MESSAGE pRpcMessage,
    DWORD* pdwStubPhase);

#endif  /* __IMemoryData_INTERFACE_DEFINED__ */

/*****************************************************************************
 * IAudioData interface
 */
#ifndef __IAudioData_INTERFACE_DEFINED__
#define __IAudioData_INTERFACE_DEFINED__

DEFINE_GUID(IID_IAudioData, 0x54c719c0, 0xaf60, 0x11d0, 0x82,0x12, 0x00,0xc0,0x4f,0xc3,0x2c,0x45);
#if defined(__cplusplus) && !defined(CINTERFACE)
interface IAudioData : public IMemoryData
{
    virtual HRESULT STDMETHODCALLTYPE GetFormat(
        WAVEFORMATEX* pWaveFormatCurrent) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetFormat(
        const WAVEFORMATEX* lpWaveFormat) = 0;

};
#else
typedef struct IAudioDataVtbl {
    BEGIN_INTERFACE

    /*** IUnknown methods ***/
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IAudioData* This,
        REFIID riid,
        void** ppvObject);

    ULONG (STDMETHODCALLTYPE *AddRef)(
        IAudioData* This);

    ULONG (STDMETHODCALLTYPE *Release)(
        IAudioData* This);

    /*** IMemoryData methods ***/
    HRESULT (STDMETHODCALLTYPE *SetBuffer)(
        IAudioData* This,
        DWORD cbSize,
        BYTE* pbData,
        DWORD dwFlags);

    HRESULT (STDMETHODCALLTYPE *GetInfo)(
        IAudioData* This,
        DWORD* pdwLength,
        BYTE** ppbData,
        DWORD* pcbActualData);

    HRESULT (STDMETHODCALLTYPE *SetActual)(
        IAudioData* This,
        DWORD cbDataValid);

    /*** IAudioData methods ***/
    HRESULT (STDMETHODCALLTYPE *GetFormat)(
        IAudioData* This,
        WAVEFORMATEX* pWaveFormatCurrent);

    HRESULT (STDMETHODCALLTYPE *SetFormat)(
        IAudioData* This,
        const WAVEFORMATEX* lpWaveFormat);

    END_INTERFACE
} IAudioDataVtbl;
interface IAudioData {
    const IAudioDataVtbl* lpVtbl;
};

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IAudioData_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IAudioData_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IAudioData_Release(p) (p)->lpVtbl->Release(p)
/*** IMemoryData methods ***/
#define IAudioData_SetBuffer(p,a,b,c) (p)->lpVtbl->SetBuffer(p,a,b,c)
#define IAudioData_GetInfo(p,a,b,c) (p)->lpVtbl->GetInfo(p,a,b,c)
#define IAudioData_SetActual(p,a) (p)->lpVtbl->SetActual(p,a)
/*** IAudioData methods ***/
#define IAudioData_GetFormat(p,a) (p)->lpVtbl->GetFormat(p,a)
#define IAudioData_SetFormat(p,a) (p)->lpVtbl->SetFormat(p,a)
#endif

#endif

HRESULT CALLBACK IAudioData_GetFormat_Proxy(
    IAudioData* This,
    WAVEFORMATEX* pWaveFormatCurrent);
void __RPC_STUB IAudioData_GetFormat_Stub(
    IRpcStubBuffer* This,
    IRpcChannelBuffer* pRpcChannelBuffer,
    PRPC_MESSAGE pRpcMessage,
    DWORD* pdwStubPhase);
HRESULT CALLBACK IAudioData_SetFormat_Proxy(
    IAudioData* This,
    const WAVEFORMATEX* lpWaveFormat);
void __RPC_STUB IAudioData_SetFormat_Stub(
    IRpcStubBuffer* This,
    IRpcChannelBuffer* pRpcChannelBuffer,
    PRPC_MESSAGE pRpcMessage,
    DWORD* pdwStubPhase);

#endif  /* __IAudioData_INTERFACE_DEFINED__ */

/* Begin additional prototypes for all interfaces */


/* End additional prototypes */

#ifdef __cplusplus
}
#endif
#endif /* __WIDL_INCLUDE_PSDK_AUSTREAM_H */
