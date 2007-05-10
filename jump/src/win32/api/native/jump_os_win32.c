/*
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
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "porting/JUMPMessageQueue.h"
#include "porting/JUMPProcess.h"
/*
 * The unique Mailslot names are in the following format:
 * BASIC_MAILSLOT_NAME<process ID>
 */
#define BASIC_MAILSLOT_NAME "\\\\.\\mailslot\\CVM"

typedef struct {
    JUMPPlatformCString key;
    HANDLE value;
} MailslotNode;


static int mailslotsBufferSize = 10;
static int numOfhSlots = 0;
// A handle to the Mailslot used by this messageQueue
static MailslotNode mailslots[10] ;
static int executiveProcessId;


static int JUMP_WIN_DEBUG = 0;

//==============Mailslot container functions ===================
static HANDLE mailslotGet(JUMPPlatformCString key, int * index){
    int i;
    if (JUMP_WIN_DEBUG) {
        printf("mailSlotGet by type %s \n", key);
    }
    for (i=0; i< numOfhSlots; i++){
        if (strcmp(key, mailslots[i].key) == 0){
            if (index != NULL) {
                *index = i;
            }
            if (JUMP_WIN_DEBUG) {
                printf("mailSlotGet: mailslot found\n");
            }
            
            return mailslots[i].value;
        }
    }
    return NULL;
}

static void mailslotResizeBuffer(){
}
static void mailslotAdd(JUMPPlatformCString key, HANDLE value) {
    if (numOfhSlots==mailslotsBufferSize) {
        mailslotResizeBuffer();
    }
    mailslots[numOfhSlots].key = strdup(key);
    mailslots[numOfhSlots].value = value;
    numOfhSlots++;
}
static void mailslotRemove(JUMPPlatformCString key) {
    int i = 0;
    for (; i<numOfhSlots; i++) {
        if (strcmp(mailslots[i].key, key)) {
            mailslots[i] = mailslots[--numOfhSlots];
            return;
        }
    }
}

//==============END OF  ===  Mailslot container functions ============


// place the name in the "name" buffer.
static void  generateMailslotName(JUMPPlatformCString  type, JUMPPlatformCString name, DWORD procID) {
    //    DWORD procID = GetCurrentProcessId();
    char buffer[20];
    char * basicName = BASIC_MAILSLOT_NAME;
    name[0] = 0;
    _itoa(procID, buffer, 10);
    strcat(name, basicName);
    strcat(name, buffer);
    strcat(name, type);
}

void jumpMessageQueueCreate(JUMPPlatformCString type, JUMPMessageQueueStatusCode* code){
    char mailslotName[100];
    HANDLE hSlot = mailslotGet(type, NULL);
    generateMailslotName(type, mailslotName, GetCurrentProcessId());
    if (JUMP_WIN_DEBUG) {
        printf("jumpMessageQueueCreate: Mailslot name=%s\n", mailslotName);
    }
    if (hSlot!= NULL) {
        printf("Warning: jumpMessageQueueCreate - MessageQueue already exists for process \n");
        *code = JUMP_MQ_FAILURE;
        return;
    }
    hSlot = CreateMailslot(mailslotName,   0,    MAILSLOT_WAIT_FOREVER,    NULL);
    if (hSlot == INVALID_HANDLE_VALUE) {
        printf("ERROR: jumpCreateMailslot failed with %d\n", GetLastError());
        *code = JUMP_MQ_FAILURE;
        return;
    }
    else {
        if (JUMP_WIN_DEBUG) {
            printf("jumpCreateMailslot: Mailsot created successfully.\n");
        }
    }
    mailslotAdd(type, hSlot);
    *code = JUMP_MQ_SUCCESS;
}
int jumpMessageQueueDestroy(JUMPPlatformCString type){
    HANDLE hSlot = mailslotGet(type, NULL);
    if (hSlot == NULL) {
        printf("Warning: jumpMessageQueueDestroy - MessageQueue does not exist for this process\n");
        return 1;
    }
    if (!CloseHandle(hSlot)) {
        printf("Error: jumpMessageQueueClose (%d)\n", GetLastError());
    } else {
        if (JUMP_WIN_DEBUG) {
            printf("jumpMessageQueueDestroy() MessageQueue was destroyed successfully\n");
        }
        return GetLastError();
    }
    hSlot = NULL;
    return 0;
}
JUMPMessageQueueHandle jumpMessageQueueOpen(int procId, JUMPPlatformCString type) {
    char mailslotName[100];
    HANDLE hMailslot;
    generateMailslotName(type, mailslotName, procId);
    if (JUMP_WIN_DEBUG) {
        printf("jumpMessageQueueOpen: trying to open Mailslot %s\n", mailslotName);
    }
    hMailslot = CreateFile(
    mailslotName,          // mailslot name
    GENERIC_WRITE ,         // mailslot write only
    FILE_SHARE_READ | FILE_SHARE_WRITE,       // required for mailslots
    NULL,                  // default security attributes
    OPEN_EXISTING,         // opens existing mailslot
    FILE_ATTRIBUTE_NORMAL, // normal attributes
    NULL);                 // no template file
    if (INVALID_HANDLE_VALUE == hMailslot) {
        DWORD errCode = GetLastError();
        if (errCode == 2 ) { // File not found
            printf("ERROR: jumpMessageQueueOpen - coudln't open MessageQueue (%d) - MesageQueue does not exists\n", errCode);
        } else {
            printf("ERROR: jumpMessageQueueOpen - coudln't open MessageQueue (%d)\n", errCode);
        }
        return NULL;  //Error
    }
    else {
        if (JUMP_WIN_DEBUG) {
            printf("jumpMessageQueueOpen: Mailslot created successfully for communication with process %d.\n", procId);
        }
    }
    return hMailslot;
}
void jumpMessageQueueClose(JUMPMessageQueueHandle handle) {
    if (!CloseHandle((HANDLE)handle)) {
        printf("Error: jumpMessageQueueClose (%d)\n", GetLastError());
    } else {
        if (JUMP_WIN_DEBUG) {
            printf("jumpMessageQueueClose() MessageQueue was closed successfully\n");
        }
    }
}


// No message header on windows
int jumpMessageQueueDataOffset(void) {
    return 0;
}

int jumpMessageQueueSend(JUMPMessageQueueHandle handle,
char *buffer, int messageDataSize){
    DWORD cbBytes;
    BOOL fResult;
    //Send the message to server
    fResult = WriteFile(
    handle,            // handle to mailslot
    buffer,             // buffer to write from
    messageDataSize,   // number of bytes to write, include the NULL
    &cbBytes,             // number of bytes written
    NULL);                // not overlapped I/O
    
    if ( (!fResult) || (messageDataSize != cbBytes)) {
        printf("ERROR: jumpMessageQueueSend - Error occurred while writing to the MessageQueue:(%d)", GetLastError());
        return 1;  //Error
    }
    else {
        if (JUMP_WIN_DEBUG) {
            printf("jumpMessageQueueSend: WriteFile(%s) was successful.\n", buffer);
        }
    }
    return 0; //Success
}

int jumpMessageQueueWaitForMessage(JUMPPlatformCString type, int32 timeout){
    DWORD cbBytes, cbMessage, cMessage = 0;
    BOOL fResult;
    char buffer;
    DWORD err, to;// error code, time out parameter
    HANDLE hSlot = mailslotGet(type, NULL);
    if (hSlot == NULL) {
        printf("ERROR: jumpMessageQueueWaitForMessage - MessageQueue does not exist\n");
        return 1;
    }
    fResult = GetMailslotInfo(hSlot,                  // mailslot handle
    (LPDWORD) NULL,         // no maximum message size
    &cbMessage,             // size of next message
    &cMessage,              // number of messages
    &to);                   // retrieves read time-out
    
    if (to != timeout){
        if (timeout < 0){
            to = MAILSLOT_WAIT_FOREVER;
        } else {
            to = timeout;
        }
        fResult = SetMailslotInfo(hSlot, to);
        if (JUMP_WIN_DEBUG) {
            printf("jumpMessageQueueWaitForMessage: read timeout set to %d\n", to);
        }
    }
    
    if (cMessage > 0) { // there are messages waiting in the MessageQueue
        return 1;
    }
    
    
    // Wait for message using blocking read
    fResult = ReadFile(hSlot,            // handle to mailslot
    &buffer,           // buffer to receive data
    0,                // size of buffer
    &cbBytes,         // number of bytes read
    NULL);            // not overlapped I/O
    if (fResult == 0) {
        err = GetLastError();
        if (err != ERROR_INSUFFICIENT_BUFFER && err != ERROR_SEM_TIMEOUT) {
            printf("ERROR: jumpMessageQueueWaitForMessage - error in reading file (%d)\n", err);
        }
    }
    if (JUMP_WIN_DEBUG) {
        GetMailslotInfo(hSlot, // mailslot handle
        (LPDWORD) NULL,               // no maximum message size
        &cbMessage,                   // size of next message
        &cMessage,                    // number of messages
        NULL);              // no read time-out
        printf("jumpMessageQueueWaitForMessage: # of messages in MessageQueue: %d\n", cMessage);
    }
    return 0;
}

int jumpMessageQueueReceive(JUMPPlatformCString messageType, char *buffer, int bufferLength) {
    DWORD cbBytes, cbMessage, cMessage, timeout;
    BOOL fResult;
    HANDLE hSlot = mailslotGet(messageType, NULL);
    if (hSlot == NULL) {
        printf("ERROR: jumpMessageQueueReceive - MessageQueue does not exists\n");
        return 0;
    }
    fResult = GetMailslotInfo(hSlot, // mailslot handle
    (LPDWORD) NULL,               // no maximum message size
    &cbMessage,                   // size of next message
    &cMessage,                    // number of messages
    (LPDWORD) &timeout);          // no read time-out
    
    
    if (JUMP_WIN_DEBUG) {
        printf("jumpMessageQueueReceive: %d new Messages\n", cMessage);
    }
    if (cbMessage > bufferLength) {
        // buffer smaller than the next message
        return -1;
    }
    //Read client message
    fResult = ReadFile(hSlot,             // handle to mailslot
    buffer,            // buffer to receive data
    bufferLength,      // size of buffer
    &cbBytes,          // number of bytes read
    NULL);             // not overlapped I/O
    
    if ( (!fResult) || (0 == cbBytes)) {
        printf("ERROR: jumpMessageQueueReceive - occurred while reading from the client: %d", GetLastError());
        return 0;  //Error
    }
    else {
        if (JUMP_WIN_DEBUG) {
            printf("jumpMessageQueueReceive: ReadFile() was successful.\n");
        }
    }
    if (JUMP_WIN_DEBUG) {
        printf("jumpMessageQueueReceive: Client sent the following message: %s\n", buffer);
    }
    return cbBytes; //Success
}

void jumpMessageQueueInterfaceDestroy(void){
    int i;
    for (i = 0; i< numOfhSlots; i++) {
        jumpMessageQueueDestroy(mailslots[i].key);
    }
}

int jumpProcessGetId(void){
    return GetCurrentProcessId();
}

int jumpProcessGetExecutiveId(void){
    return executiveProcessId;
}

void jumpProcessSetExecutiveId(int execPid){
    executiveProcessId = execPid;
}

int jumpThreadGetId(void){
    return GetCurrentThreadId();
}

/**
 * FIXME: Stub implementation
 */
int 
jumpProcessRunDriver(char *driverName, char *libName) {
    (void)driverName;
    (void)libName;
    return -1;
}

/**
 * FIXME: Stub implementation
 */
int 
jumpProcessNativeCreate(int argc, char** argv) {
    (void)argc;
    (void)argv;
    return -1;
}

int  jumpProcessCreate(int argc, char** argv){
    BOOL b;
    int argsBufferSize = 0;
    int i = 0;
    char*  argsBuffer;
    char* command = "vm-internal.exe com.sun.jumpimpl.isolate.jvmprocess.JUMPIsolateProcessImpl ";
    char * commandLine;
    char * path = ""; //FIXME add lime call to get the full path of the emulator bin directory
    char space = ' ';
    PROCESS_INFORMATION pi;
    DWORD creationFlags = 0;
    for (; i<argc; i++) {
        argsBufferSize += strlen(argv[i]);
    }
    argsBufferSize += argc;
    argsBuffer = (char *) malloc(sizeof (char) * argsBufferSize);
    commandLine = (char*) malloc(sizeof (char) * (argsBufferSize + strlen(command) + strlen(path)));
    commandLine[0] = 0;
    strcat(commandLine, path);
    strcat(commandLine, command);
    for (i=0; i<argc; i++) {
        strcat(commandLine, argv[i]);
        strcat(commandLine, &space);
    }
    //FIXME maybe not needed
    strcat(commandLine, "\n");
    
    b = CreateProcess(NULL, commandLine, NULL, NULL, TRUE, creationFlags, NULL, NULL, NULL, &pi);
    if (b == 0) {
        return -(GetLastError());
    }
    return pi.dwProcessId;
}

int jumpProcessIsAlive(int pid){
    HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    DWORD errCode;
    BOOL res;
    DWORD ExitCode;
    
    if (h == NULL) {
        return 0;
    }
    res =  GetExitCodeProcess(h, &ExitCode);
    errCode = GetLastError();
    CloseHandle(h);
    if ((res == 0) && (errCode == STILL_ACTIVE)) {
        return 1;
    }
    return 0;
}


