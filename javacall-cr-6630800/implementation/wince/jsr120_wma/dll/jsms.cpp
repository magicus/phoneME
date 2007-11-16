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

#define INITGUID
#include "windows.h"
#include "cemapi.h"

// {FAE79405-C31A-4064-9623-5A4A5B34A729}
DEFINE_GUID(CLSID_JSMS, 
0xfae79405, 0xc31a, 0x4064, 0x96, 0x23, 0x5a, 0x4a, 0x5b, 0x34, 0xa7, 0x29);

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

    if (ppvObject == NULL) {
        return E_INVALIDARG;
    }

    if ((rif == IID_IUnknown) || (rif == IID_IClassFactory)) {
        *ppvObject = (LPVOID) this;
        ((LPUNKNOWN) *ppvObject)->AddRef();
	return S_OK;
    } else {
        *ppvObject = NULL;
	return E_NOINTERFACE;
    }
}

HRESULT MailRuleClientImpl::QueryInterface(REFIID rif, LPVOID *ppvObject) {

    if (ppvObject == NULL) {
        return E_INVALIDARG;
    }
    
    if ((rif == IID_IUnknown) || (rif == IID_IMailRuleClient)) {
        *ppvObject = (LPVOID) this;
        ((LPUNKNOWN) *ppvObject)->AddRef();
	return S_OK;
    } else {
        *ppvObject = NULL;
	return E_NOINTERFACE;
    }
}

ULONG ClassFactoryImpl::AddRef() {

    return countRef++;
}

ULONG ClassFactoryImpl::Release() {

    countRef--;

    if (countRef == 0) {
        delete this;
        return 0;
    } else {
        return countRef;
    }
}

ULONG MailRuleClientImpl::AddRef() {

    return countRef++;
}

ULONG MailRuleClientImpl::Release() {

    countRef--;
    
    if (countRef == 0) {
        delete this;
        return 0;
    } else {
        return countRef;
    }
} 

/**
 *  IClassFactory methods impl:
 *    CreateInstance
 *    LockServer
 */

HRESULT ClassFactoryImpl::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObject) {

    MailRuleClientImpl *pClient = NULL;

    if (pUnkOuter != NULL) {
        return CLASS_E_NOAGGREGATION;
    }

    pClient = new MailRuleClientImpl();
    if (pClient == NULL) {
        return E_FAIL;
    }
    
    return pClient->QueryInterface(riid, ppvObject);
}

int lockServerNum = 0;

HRESULT ClassFactoryImpl::LockServer(BOOL fLock) {

    if (fLock) {
        lockServerNum++;
    } else {
        lockServerNum--;
    }

    return S_OK;
}

/**
 *  IMailRuleClient methods impl:
 *    Initialize
 *    ProcessMessage
 */

HRESULT MailRuleClientImpl::Initialize(IMsgStore *pMsgStore, MRCACCESS *pmaDesired) {

    (void)pMsgStore;
    *pmaDesired = MRC_ACCESS_WRITE;
    return S_OK;
}

HRESULT DeleteMsg(IMsgStore *pMsgStore, ULONG cbMsg, LPENTRYID lpMsg, ULONG cbDestFolder, 
                  LPENTRYID lpDestFolder, ULONG *pulEventType, MRCHANDLED *pHandled) {

    HRESULT result = S_OK;
    IMAPIFolder *pFolder = NULL;
    
    result = pMsgStore->OpenEntry(cbDestFolder, lpDestFolder, NULL, 0, NULL, (LPUNKNOWN*)&pFolder);
    if (result == S_OK) {
        SBinary   binary;
        ENTRYLIST entrylist;

        binary.cb = cbMsg;
        binary.lpb = (LPBYTE) lpMsg;
        entrylist.cValues = 1;
        entrylist.lpbin = &binary;

        result = pFolder->DeleteMessages(&entrylist, NULL, NULL, 0); 
        if (result == S_OK) {
            *pulEventType = fnevObjectDeleted;
            *pHandled = MRC_HANDLED_DONTCONTINUE;
        }
    }

    if (pFolder) {
        pFolder->Release();
    }

    return result;
}

HRESULT MailRuleClientImpl::ProcessMessage(
        IMsgStore *pMsgStore, ULONG cbMsg, LPENTRYID lpMsg, ULONG cbDestFolder, 
        LPENTRYID lpDestFolder, ULONG *pulEventType, MRCHANDLED *pHandled)
{
    HRESULT result = S_OK;
    IMessage *pMsg = NULL;

    SizedSPropTagArray(1, propTagSubject) = {1, PR_SUBJECT}; 
    SizedSPropTagArray(1, propTagTime)    = {1, PR_MESSAGE_DELIVERY_TIME}; 
    SizedSPropTagArray(1, propTagEmail)   = {1, PR_SENDER_EMAIL_ADDRESS}; 
    
    SPropValue *propSubject = NULL;
    SPropValue *propTime = NULL;
    SPropValue *propEmail = NULL;

    ULONG dummy = 0;

    //MessageBox(NULL, L"ProcessMessage", L"ProcessMessage", MB_OK);

    result = pMsgStore->OpenEntry(cbMsg, lpMsg, NULL, 0, NULL, (LPUNKNOWN *) &pMsg);
    if (result == S_OK) {
    
        pMsg->GetProps((SPropTagArray *) &propTagSubject, NULL, &dummy, &propSubject);
        pMsg->GetProps((SPropTagArray *) &propTagEmail,   NULL, &dummy, &propEmail);
        pMsg->GetProps((SPropTagArray *) &propTagTime,    NULL, &dummy, &propTime);
        
        if (propSubject && propTime && propEmail) {
            if (strncmp(propSubject->Value.lpszA, "//WMA", 5) == 0) {
                //MessageBox(NULL, L"Message accepted", L"jsms.dll", MB_OK);

                //size of datagram is not accessible by MAPI interface,
                //the only way to get the info is to extract them from the package
                unsigned char datagramSize = (propSubject->Value.lpszA[5+4+4] == 0x20) ? 
                    propSubject->Value.lpszA[5+4+4+1+1] : propSubject->Value.lpszA[5+4+4+2+2+2+1+1];

                WriteMapFile((char*)propSubject->Value.lpszW, datagramSize, //propContentLength->Value.l,
                   &propTime->Value.ft, (wchar_t*)propEmail->Value.lpszW);

                result = DeleteMsg(pMsgStore, cbMsg, lpMsg, cbDestFolder, lpDestFolder, pulEventType, pHandled);
            }
            else {
                //MessageBox(NULL, L"Message rejected", L"jsms.dll", MB_OK);
                *pHandled = MRC_NOT_HANDLED;    
            }
        } else {
            result = S_FALSE;
        }

        if (propEmail) {
            MAPIFreeBuffer(propEmail);
        }
        if (propSubject){
            MAPIFreeBuffer(propSubject);
        }
        if (propTime){
            MAPIFreeBuffer(propTime);
        }
    }

    if (pMsg) {
        pMsg->Release();
    }

    return result;
}

/**
 * Dll functions impl:
 *   DllCanUnloadNow
 *   DllGetClassObject
 *   DllMain
 */

STDAPI DllCanUnloadNow() {

    return lockServerNum ? S_FALSE : S_OK;
}

STDAPI DllGetClassObject(const CLSID& clsid, REFIID riid, LPVOID *ppv) {

    HRESULT result;

    if (clsid != CLSID_JSMS) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    ClassFactoryImpl *pFactory = new ClassFactoryImpl;
    if (pFactory == NULL) {
        return E_OUTOFMEMORY;
    }

    result = pFactory->QueryInterface(riid, ppv);
    pFactory->Release();

    return result;
}

BOOL WINAPI DllMain(HANDLE hinst, DWORD dwReason, LPVOID lpv) {

    (void)hinst;
    (void)dwReason;
    (void)lpv;
    return TRUE;
}

BOOL WINAPI _DllMainCRTStartup(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {

    return DllMain(hinstDLL, fdwReason, lpReserved);
}
