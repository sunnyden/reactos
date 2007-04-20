@ stdcall BindMoniker(ptr long ptr ptr)
@ stdcall CLIPFORMAT_UserFree(ptr ptr)
@ stdcall CLIPFORMAT_UserMarshal(ptr ptr ptr)
@ stdcall CLIPFORMAT_UserSize(ptr long ptr)
@ stdcall CLIPFORMAT_UserUnmarshal(ptr ptr ptr)
@ stdcall CLSIDFromProgID(wstr ptr)
@ stdcall CLSIDFromString(wstr ptr)
@ stdcall CoAddRefServerProcess()
@ stdcall CoAllowSetForegroundWindow(ptr ptr)
@ stdcall CoBuildVersion()
@ stdcall CoCopyProxy(ptr ptr)
@ stdcall CoCreateFreeThreadedMarshaler(ptr ptr)
@ stdcall CoCreateGuid(ptr)
@ stdcall CoCreateInstance(ptr ptr long ptr ptr)
@ stdcall CoCreateInstanceEx(ptr ptr long ptr long ptr)
@ stdcall CoDisconnectObject(ptr long)
@ stdcall CoDosDateTimeToFileTime(long long ptr) kernel32.DosDateTimeToFileTime
@ stdcall CoFileTimeNow(ptr)
@ stdcall CoFileTimeToDosDateTime(ptr ptr ptr) kernel32.FileTimeToDosDateTime
@ stdcall CoFreeAllLibraries()
@ stdcall CoFreeLibrary(long)
@ stdcall CoFreeUnusedLibraries()
@ stdcall CoGetCallContext(ptr ptr)
@ stub CoGetCallerTID
@ stdcall CoGetClassObject(ptr long ptr ptr ptr)
@ stub CoGetCurrentLogicalThreadId
@ stdcall CoGetCurrentProcess()
@ stub CoGetInstanceFromFile      #@ stdcall (ptr ptr ptr long wstr long ptr) return 0,ERR_NOTIMPLEMENTED
@ stub CoGetInstanceFromIStorage  #@ stdcall (ptr ptr ptr long ptr long ptr) return 0,ERR_NOTIMPLEMENTED
@ stdcall CoGetInterfaceAndReleaseStream(ptr ptr ptr)
@ stdcall CoGetMalloc(long ptr)
@ stdcall CoGetMarshalSizeMax(ptr ptr ptr long ptr long)
@ stdcall CoGetObject(wstr ptr ptr ptr)
@ stdcall CoGetPSClsid(ptr ptr)
@ stdcall CoGetStandardMarshal(ptr ptr long ptr long ptr)
@ stdcall CoGetState(ptr)
@ stub CoGetTIDFromIPID
@ stdcall CoGetTreatAsClass(ptr ptr)
@ stdcall CoImpersonateClient()
@ stdcall CoInitialize(ptr)
@ stdcall CoInitializeEx(ptr long)
@ stdcall CoInitializeSecurity(ptr long ptr ptr long long ptr long ptr)
@ stdcall CoInitializeWOW(long long)
@ stdcall CoIsHandlerConnected(ptr)
@ stdcall CoIsOle1Class (ptr)
@ stdcall CoLoadLibrary(wstr long)
@ stdcall CoLockObjectExternal(ptr long long)
@ stdcall CoMarshalHresult(ptr long)
@ stdcall CoMarshalInterThreadInterfaceInStream(ptr ptr ptr)
@ stdcall CoMarshalInterface(ptr ptr ptr long ptr long)
@ stub CoQueryAuthenticationServices
@ stdcall CoQueryClientBlanket(ptr ptr ptr ptr ptr ptr ptr)
@ stdcall CoQueryProxyBlanket(ptr ptr ptr ptr ptr ptr ptr ptr)
@ stub CoQueryReleaseObject
@ stdcall CoRegisterChannelHook(ptr ptr)
@ stdcall CoRegisterClassObject(ptr ptr long long ptr)
@ stdcall CoRegisterMallocSpy (ptr)
@ stdcall CoRegisterMessageFilter(ptr ptr)
@ stdcall CoRegisterPSClsid(ptr ptr)
@ stub CoRegisterSurrogate
@ stdcall CoReleaseMarshalData(ptr)
@ stdcall CoReleaseServerProcess()
@ stdcall CoResumeClassObjects()
@ stdcall CoRevertToSelf()
@ stdcall CoRevokeClassObject(long)
@ stdcall CoRevokeMallocSpy()
@ stdcall CoSetProxyBlanket(ptr long long wstr long long ptr long)
@ stdcall CoSetState(ptr)
@ stdcall CoSuspendClassObjects()
@ stub CoSwitchCallContext
@ stdcall CoTaskMemAlloc(long)
@ stdcall CoTaskMemFree(ptr)
@ stdcall CoTaskMemRealloc(ptr long)
@ stdcall CoTreatAsClass(ptr ptr)
@ stdcall CoUninitialize()
@ stub CoUnloadingWOW
@ stdcall CoUnmarshalHresult(ptr ptr)
@ stdcall CoUnmarshalInterface(ptr ptr ptr)
@ stdcall CoWaitForMultipleHandles(long long long ptr ptr)
@ stdcall CreateAntiMoniker(ptr)
@ stdcall CreateBindCtx(long ptr)
@ stdcall CreateClassMoniker(ptr ptr)
@ stdcall CreateDataAdviseHolder(ptr)
@ stdcall CreateDataCache(ptr ptr ptr ptr)
@ stdcall CreateErrorInfo(ptr)
@ stdcall CreateFileMoniker(wstr ptr)
@ stdcall CreateGenericComposite(ptr ptr ptr)
@ stdcall CreateILockBytesOnHGlobal(ptr long ptr)
@ stdcall CreateItemMoniker(wstr wstr ptr)
@ stub CreateObjrefMoniker
@ stdcall CreateOleAdviseHolder(ptr)
@ stub CreatePointerMoniker       #@ stdcall (ptr ptr) return 0,ERR_NOTIMPLEMENTED
@ stdcall CreateStreamOnHGlobal(ptr long ptr)
@ stdcall DllDebugObjectRPCHook(long ptr)
@ stdcall -private DllGetClassObject (ptr ptr ptr)
@ stub DllGetClassObjectWOW
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stdcall DoDragDrop(ptr ptr long ptr)
@ stub EnableHookObject
@ stdcall FmtIdToPropStgName(ptr wstr)
@ stdcall FreePropVariantArray(long ptr)
@ stdcall GetClassFile(wstr ptr)
@ stdcall GetConvertStg(ptr)
@ stub GetDocumentBitStg
@ stdcall GetErrorInfo(long ptr)
@ stdcall GetHGlobalFromILockBytes(ptr ptr)
@ stdcall GetHGlobalFromStream(ptr ptr)
@ stub GetHookInterface
@ stdcall GetRunningObjectTable(long ptr)
@ stdcall HACCEL_UserFree(ptr ptr)
@ stdcall HACCEL_UserMarshal(ptr ptr ptr)
@ stdcall HACCEL_UserSize(ptr long ptr)
@ stdcall HACCEL_UserUnmarshal(ptr ptr ptr)
@ stdcall HBITMAP_UserFree(ptr ptr)
@ stdcall HBITMAP_UserMarshal(ptr ptr ptr)
@ stdcall HBITMAP_UserSize(ptr long ptr)
@ stdcall HBITMAP_UserUnmarshal(ptr ptr ptr)
@ stub HBRUSH_UserFree
@ stub HBRUSH_UserMarshal
@ stub HBRUSH_UserSize
@ stub HBRUSH_UserUnmarshal
@ stdcall HDC_UserFree(ptr ptr)
@ stdcall HDC_UserMarshal(ptr ptr ptr)
@ stdcall HDC_UserSize(ptr long ptr)
@ stdcall HDC_UserUnmarshal(ptr ptr ptr)
@ stdcall HENHMETAFILE_UserFree(ptr ptr)
@ stdcall HENHMETAFILE_UserMarshal(ptr ptr ptr)
@ stdcall HENHMETAFILE_UserSize(ptr long ptr)
@ stdcall HENHMETAFILE_UserUnmarshal(ptr ptr ptr)
@ stdcall HGLOBAL_UserFree(ptr ptr)
@ stdcall HGLOBAL_UserMarshal(ptr ptr ptr)
@ stdcall HGLOBAL_UserSize(ptr long ptr)
@ stdcall HGLOBAL_UserUnmarshal(ptr ptr ptr)
@ stdcall HMENU_UserFree(ptr ptr)
@ stdcall HMENU_UserMarshal(ptr ptr ptr)
@ stdcall HMENU_UserSize(ptr long ptr)
@ stdcall HMENU_UserUnmarshal(ptr ptr ptr)
@ stdcall HMETAFILEPICT_UserFree(ptr ptr)
@ stdcall HMETAFILEPICT_UserMarshal(ptr ptr ptr)
@ stdcall HMETAFILEPICT_UserSize(ptr long ptr)
@ stdcall HMETAFILEPICT_UserUnmarshal(ptr ptr ptr)
@ stdcall HMETAFILE_UserFree(ptr ptr)
@ stdcall HMETAFILE_UserMarshal(ptr ptr ptr)
@ stdcall HMETAFILE_UserSize(ptr long ptr)
@ stdcall HMETAFILE_UserUnmarshal(ptr ptr ptr)
@ stdcall HPALETTE_UserFree(ptr ptr)
@ stdcall HPALETTE_UserMarshal(ptr ptr ptr)
@ stdcall HPALETTE_UserSize(ptr long ptr)
@ stdcall HPALETTE_UserUnmarshal(ptr ptr ptr)
@ stdcall HWND_UserFree(ptr ptr)
@ stdcall HWND_UserMarshal(ptr ptr ptr)
@ stdcall HWND_UserSize(ptr long ptr)
@ stdcall HWND_UserUnmarshal(ptr ptr ptr)
@ stdcall IIDFromString(wstr ptr) CLSIDFromString
@ stub I_RemoteMain
@ stdcall IsAccelerator(long long ptr long)
@ stdcall IsEqualGUID(ptr ptr)
@ stub IsValidIid
@ stdcall IsValidInterface(ptr)
@ stub IsValidPtrIn
@ stub IsValidPtrOut
@ stdcall MkParseDisplayName(ptr ptr ptr ptr)
@ stdcall MonikerCommonPrefixWith(ptr ptr ptr)
@ stub MonikerRelativePathTo
@ stdcall OleBuildVersion()
@ stdcall OleConvertIStorageToOLESTREAM(ptr ptr)
@ stub OleConvertIStorageToOLESTREAMEx
@ stdcall OleConvertOLESTREAMToIStorage(ptr ptr ptr)
@ stub OleConvertOLESTREAMToIStorageEx
@ stdcall OleCreate(ptr ptr long ptr ptr ptr ptr)
@ stdcall OleCreateDefaultHandler(ptr ptr ptr ptr)
@ stub OleCreateEmbeddingHelper
@ stub OleCreateEx
@ stdcall OleCreateFromData(ptr ptr long ptr ptr ptr ptr)
@ stub OleCreateFromDataEx
@ stdcall OleCreateFromFile(ptr ptr ptr long ptr ptr ptr ptr)
@ stub OleCreateFromFileEx
@ stdcall OleCreateLink(ptr ptr long ptr ptr ptr ptr)
@ stub OleCreateLinkEx
@ stdcall OleCreateLinkFromData(ptr ptr long ptr ptr ptr ptr)
@ stub OleCreateLinkFromDataEx
@ stdcall OleCreateLinkToFile(ptr ptr long ptr ptr ptr ptr)
@ stub OleCreateLinkToFileEx
@ stdcall OleCreateMenuDescriptor(long ptr)
@ stdcall OleCreateStaticFromData(ptr ptr long ptr ptr ptr ptr)
@ stdcall OleDestroyMenuDescriptor(long)
@ stdcall OleDoAutoConvert(ptr ptr)
@ stdcall OleDraw(ptr long long ptr)
@ stdcall OleDuplicateData(long long long)
@ stdcall OleFlushClipboard()
@ stdcall OleGetAutoConvert(ptr ptr)
@ stdcall OleGetClipboard(ptr)
@ stdcall OleGetIconOfClass(ptr ptr long)
@ stub OleGetIconOfFile
@ stdcall OleInitialize(ptr)
@ stdcall OleInitializeWOW(long long)
@ stdcall OleIsCurrentClipboard(ptr)
@ stdcall OleIsRunning(ptr)
@ stdcall OleLoad(ptr ptr ptr ptr)
@ stdcall OleLoadFromStream(ptr ptr ptr)
@ stdcall OleLockRunning(ptr long long)
@ stdcall OleMetafilePictFromIconAndLabel(long ptr ptr long)
@ stdcall OleNoteObjectVisible(ptr long)
@ stdcall OleQueryCreateFromData(ptr)
@ stdcall OleQueryLinkFromData(ptr)
@ stdcall OleRegEnumFormatEtc(ptr long ptr)
@ stdcall OleRegEnumVerbs(long ptr)
@ stdcall OleRegGetMiscStatus(ptr long ptr)
@ stdcall OleRegGetUserType(long long ptr)
@ stdcall OleRun(ptr)
@ stdcall OleSave(ptr ptr long)
@ stdcall OleSaveToStream(ptr ptr)
@ stdcall OleSetAutoConvert(ptr ptr)
@ stdcall OleSetClipboard(ptr)
@ stdcall OleSetContainedObject(ptr long)
@ stdcall OleSetMenuDescriptor(long long long ptr ptr)
@ stdcall OleTranslateAccelerator(ptr ptr ptr)
@ stdcall OleUninitialize()
@ stub OpenOrCreateStream
@ stdcall ProgIDFromCLSID(ptr ptr)
@ stdcall PropStgNameToFmtId(wstr ptr)
@ stdcall PropSysAllocString(wstr)
@ stdcall PropSysFreeString(wstr)
@ stdcall PropVariantClear(ptr)
@ stdcall PropVariantCopy(ptr ptr)
@ stdcall ReadClassStg(ptr ptr)
@ stdcall ReadClassStm(ptr ptr)
@ stdcall ReadFmtUserTypeStg(ptr ptr ptr)
@ stub ReadOleStg
@ stub ReadStringStream
@ stdcall RegisterDragDrop(long ptr)
@ stdcall ReleaseStgMedium(ptr)
@ stdcall RevokeDragDrop(long)
@ stdcall SNB_UserFree(ptr ptr)
@ stdcall SNB_UserMarshal(ptr ptr ptr)
@ stdcall SNB_UserSize(ptr long ptr)
@ stdcall SNB_UserUnmarshal(ptr ptr ptr)
@ stdcall STGMEDIUM_UserFree(ptr ptr)
@ stdcall STGMEDIUM_UserMarshal(ptr ptr ptr)
@ stdcall STGMEDIUM_UserSize(ptr long ptr)
@ stdcall STGMEDIUM_UserUnmarshal(ptr ptr ptr)
@ stdcall SetConvertStg(ptr long)
@ stub SetDocumentBitStg
@ stdcall SetErrorInfo(long ptr)
@ stdcall StgCreateDocfile(wstr long long ptr)
@ stdcall StgCreateDocfileOnILockBytes(ptr long long ptr)
@ stdcall StgCreatePropSetStg(ptr long ptr)
@ stdcall StgCreateStorageEx(wstr long long long ptr ptr ptr ptr)
@ stub StgGetIFillLockBytesOnFile
@ stub StgGetIFillLockBytesOnILockBytes
@ stdcall StgIsStorageFile(wstr)
@ stdcall StgIsStorageILockBytes(ptr)
@ stub StgOpenAsyncDocfileOnIFillLockBytes
@ stdcall StgOpenStorage(wstr ptr long ptr long ptr)
@ stdcall StgOpenStorageEx(wstr long long long ptr ptr ptr ptr)
@ stdcall StgOpenStorageOnILockBytes(ptr ptr long long long ptr)
@ stdcall StgSetTimes(wstr ptr ptr ptr )
@ stdcall StringFromCLSID(ptr ptr)
@ stdcall StringFromGUID2(ptr ptr long)
@ stdcall StringFromIID(ptr ptr) StringFromCLSID
@ stub UpdateDCOMSettings
@ stub UtConvertDvtd16toDvtd32
@ stub UtConvertDvtd32toDvtd16
@ stub UtGetDvtd16Info
@ stub UtGetDvtd32Info
@ stub WdtpInterfacePointer_UserFree
@ stub WdtpInterfacePointer_UserMarshal
@ stub WdtpInterfacePointer_UserSize
@ stub WdtpInterfacePointer_UserUnmarshal
@ stdcall WriteClassStg(ptr ptr)
@ stdcall WriteClassStm(ptr ptr)
@ stdcall WriteFmtUserTypeStg(ptr long ptr)
@ stub WriteOleStg
@ stub WriteStringStream
