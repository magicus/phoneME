/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 */

#include "windows.h"
#include "cemapi.h"

/**
 *  WriteMapFile function stores loaded data to Mapped file
 *  The file expected to be already opened by CVM.
 *  A Mutex and Event used to synchronize data i/o.
 */

#define lpFileMappingName L"jsms_temp_file"
#define lpFileMutex       L"jsms_mutex"
#define lpFileEvent       L"jsms_event"

#define SENDERPHONE_MAX_LENGTH  52
#define DATAGRAM_MAX_LENGTH     160

#define FILE_OFFSET_MSG_SIZE    0
#define FILE_OFFSET_SENDERPHONE 4
#define FILE_OFFSET_SENDTIME    56
#define FILE_OFFSET_DATAGRAM    64

#define SMS_MAPFILE_SIZE        224

void WriteMapFile(const char* messageBuffer, int bufferSize,
                  FILETIME* sendTime, wchar_t* senderPhone) {

    HANDLE hFileMap = CreateFileMapping(
        INVALID_HANDLE_VALUE, 
        NULL,
        PAGE_READWRITE,
        0, SMS_MAPFILE_SIZE,
        lpFileMappingName);

    HANDLE pMutex = CreateMutex(NULL, FALSE, lpFileMutex);
    //DWORD dd = GetLastError();
    //dd == 183 = ERROR_ALREADY_EXISTS ??

    if (pMutex) {
        int max_waiting_msec = 1000; //we could not handup windows thread!
        DWORD waitOk = WaitForSingleObject(pMutex, max_waiting_msec);
        if (waitOk != WAIT_OBJECT_0) {
            //printf("Error reseiving sms. Mutex locked\n");
            return;
        } //else MessageBox(NULL, L"MutexReleased", L"testDevice", MB_OK);
        //printf("Error reseiving sms. No mutex found. Is CVM not started?\n");
        //return;
    }

    if(hFileMap) {
        char* pFileMemory = (CHAR*)MapViewOfFile(
            hFileMap,
            FILE_MAP_WRITE,
            0,0,0);

        int bufferSize1 = bufferSize < DATAGRAM_MAX_LENGTH ? bufferSize : DATAGRAM_MAX_LENGTH; //else error

        *((int*)(pFileMemory + FILE_OFFSET_MSG_SIZE)) = bufferSize1; 

        int senderPhoneLength = wcslen(senderPhone) * sizeof(wchar_t);
        if (senderPhoneLength > SENDERPHONE_MAX_LENGTH) {
            memcpy(pFileMemory + FILE_OFFSET_SENDERPHONE, senderPhone, SENDERPHONE_MAX_LENGTH);
            *(pFileMemory + FILE_OFFSET_SENDERPHONE + SENDERPHONE_MAX_LENGTH - 1) = 0;
        } else {
            memcpy(pFileMemory + FILE_OFFSET_SENDERPHONE, senderPhone, senderPhoneLength + 1);
        }

        *(DWORD*)(pFileMemory + FILE_OFFSET_SENDTIME)     = sendTime->dwHighDateTime;
        *(DWORD*)(pFileMemory + FILE_OFFSET_SENDTIME + 4) = sendTime->dwLowDateTime;

        memcpy(pFileMemory + FILE_OFFSET_DATAGRAM, messageBuffer, bufferSize1);
    }

    if (pMutex) {
        ReleaseMutex(pMutex);
    }

    HANDLE evnt = OpenEvent(EVENT_ALL_ACCESS, FALSE, lpFileEvent); 
    if (evnt != NULL) { 
        SetEvent(evnt); 
    } else { 
        //printf("Error sending event. Is CVM not started?")
        //MessageBox(NULL, L"No event!", L"testDevice", MB_OK); 
    }
}

/**
 * 
 */

class ClassFactoryImpl : public IClassFactory {
public:
    ClassFactoryImpl() {countRef = 1;};
    ~ClassFactoryImpl() {};

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID rif, LPVOID *ppvObject);
    ULONG   STDMETHODCALLTYPE AddRef();
    ULONG   STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown *pUnkOuter, const IID& riid, LPVOID *ppvObject);
    HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock);
private:
    long countRef;
};

class MailRuleClientImpl : public IMailRuleClient {
public:
    MailRuleClientImpl() {countRef = 1;};
    ~MailRuleClientImpl() {};
    
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID rif, LPVOID *ppvObject);
    ULONG   STDMETHODCALLTYPE AddRef();
    ULONG   STDMETHODCALLTYPE Release();

    MAPIMETHOD(Initialize)(IMsgStore *pMsgStore, MRCACCESS *pmaDesired);
    MAPIMETHOD(ProcessMessage)(IMsgStore *pMsgStore, ULONG cbMsg, LPENTRYID lpMsg, ULONG cbDestFolder,
                           LPENTRYID lpDestFolder, ULONG *pulEventType, MRCHANDLED *pHandled);
private:
    long countRef;
};

/**
 *  IUnknown methods impl:
 *    QueryInterface
 *    AddRef
 *    Release
 */

HRESULT ClassFactoryImpl::QueryInterface(REFIID rif, LPVOID *ppvObject) {

    return NULL;
}

HRESULT MailRuleClientImpl::QueryInterface(REFIID rif, LPVOID *ppvObject) {

    return NULL;
}

ULONG ClassFactoryImpl::AddRef() {

    return NULL;
}

ULONG ClassFactoryImpl::Release() {

    return NULL;
}

ULONG MailRuleClientImpl::AddRef() {

    return NULL;
}

ULONG MailRuleClientImpl::Release() {

    return NULL;
} 

/**
 *  IClassFactory methods impl:
 *    CreateInstance
 *    LockServer
 */

HRESULT ClassFactoryImpl::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObject) {

    return NULL;
}

HRESULT ClassFactoryImpl::LockServer(BOOL fLock) {

    return NULL;
}

/**
 *  IMailRuleClient methods impl:
 *    Initialize
 *    ProcessMessage
 */

HRESULT MailRuleClientImpl::Initialize(IMsgStore *pMsgStore, MRCACCESS *pmaDesired) {

    return NULL;
}

HRESULT MailRuleClientImpl::ProcessMessage(
        IMsgStore *pMsgStore, ULONG cbMsg, LPENTRYID lpMsg, ULONG cbDestFolder, 
        LPENTRYID lpDestFolder, ULONG *pulEventType, MRCHANDLED *pHandled) {

    //WriteMapFile here
    return NULL;
}

/**
 * Dll functions impl:
 *   DllCanUnloadNow
 *   DllGetClassObject
 *   DllMain
 */

STDAPI DllCanUnloadNow() {

    return S_OK;
}

STDAPI DllGetClassObject(const CLSID& clsid, REFIID riid, LPVOID *ppv) {

    return S_FALSE;
}

BOOL WINAPI DllMain(HANDLE hinst, DWORD dwReason, LPVOID lpv) {

    return TRUE;
}

