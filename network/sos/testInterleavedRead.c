/*
 * E
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

#include <stdio.h>
#include <string.h>

#include <pcsl_network.h>
#include <donuts.h>

#define READ_BUFFER_SIZE 256

static int status;
static void *pContext;

void NotifyStatusChanged(void *handle, int waitingFor) {}

void testGetHostByName2(char* hostname, unsigned char* ipBytes) {
    int len; 
    void *handle;

    status = pcsl_network_gethostbyname_start(hostname, ipBytes,
                     MAX_ADDR_LENGTH, &len, &handle, &pContext );
    assertTrue("Host name not converted to byte array", 
                status == PCSL_NET_SUCCESS); 
    printf("Length = %d\n", len);
}

void* testOpen2(unsigned char* ipBytes, int port) {
    char* exception;
    void *handle = NULL;
   
    do { 
        status = pcsl_socket_open_start(ipBytes, port, &handle, &pContext);
        assertTrue("Error in opening socket connection", 
            ((status == PCSL_NET_SUCCESS)||(status == PCSL_NET_WOULDBLOCK)));
    } while (status == PCSL_NET_WOULDBLOCK);
    
    return handle;
}

void testWrite2(void *handle, char* request) {
    int length;
    int count;
    int bytesWritten = 0;
 
    length = strlen(request);
 
    do {
        count = 0;
        status = pcsl_socket_write_start(handle, (request + bytesWritten),
                                   (length - bytesWritten), &count, &pContext);
        if (status == PCSL_NET_INTERRUPTED) {
            assertTrue("Interrupted IO Exception in testWrite", 0);
            break;
        }

        if (count > 0) {
            bytesWritten += count;
        }
    } while (bytesWritten < length);
}
   
int testAvailable2(void *handle) {
    int numBytes;
    usleep(300);
 
    status = pcsl_socket_available(handle, &numBytes);
    assertTrue("Error in available() at socket ", status == PCSL_NET_SUCCESS);
    
    return numBytes;
}

void testInterleavedRead(void *handle1, void *handle2, 
            char* data1, char* data2, int numBytes1, int numBytes2) {
    int count;
    char data[READ_BUFFER_SIZE+1];
    int bytesRead1 = 0;
    int bytesRead2 = 0;

    do {
        //Read for the first handle
        count = 0;
        memset(data, 0, READ_BUFFER_SIZE+1);
        if (bytesRead1 < numBytes1) {
            status = pcsl_socket_read_start(handle1, data, READ_BUFFER_SIZE, 
                                     &count, &pContext);
        }
        data[READ_BUFFER_SIZE] = 0;
        if ((status == PCSL_NET_SUCCESS)&&(count > 0)) {
            bytesRead1 += count;
            strcat(data1, data);
        }

        //Read for the second handle
        count = 0;
        memset(data, 0, READ_BUFFER_SIZE+1);
        if (bytesRead2 < numBytes2) {
            status = pcsl_socket_read_start(handle2, data, READ_BUFFER_SIZE, 
                                     &count, &pContext);
        }
        data[READ_BUFFER_SIZE] = 0;
        if ((status == PCSL_NET_SUCCESS)&&(count > 0)) {
            bytesRead2 += count;
            strcat(data2, data);
        }
    } while ((bytesRead1 < numBytes1)||(bytesRead2 < numBytes2));
}

void testClose2(void *handle) {
    status = pcsl_socket_close_start(handle, &pContext);
    assertTrue("Error in closing socket connection", status == PCSL_NET_SUCCESS);
}    
    
/**
 * Unit test framework entry point for this set of unit tests.
 *  
 */
void testInterleavedRead_runTests() {
    unsigned char ipBytes[MAX_ADDR_LENGTH]; //can be set to INET6_ADDRSTRLEN;
    void *handle1, *handle2;
    int numBytes1, numBytes2;
    char data1[1024*8];
    char data2[1024*8];
    char *hostname = "host.domain";
    char* request1 = "GET /midlets/midlet.jad HTTP/1.0\n\n";
    char* request2 = "GET /midlets/midlet.jad HTTP/1.0\n\n";

    testGetHostByName2(hostname, ipBytes);

    handle1 = testOpen2(ipBytes, 80);
    handle2 = testOpen2(ipBytes, 80);
    printf("Open handles : handle1 = %d, handle2 = %d\n", handle1, handle2);

    testWrite2(handle1, request1);
    testWrite2(handle2, request2);

    numBytes1 = testAvailable2(handle1);
    numBytes2 = testAvailable2(handle2);
    printf("numByte1 = %d numBytes=%d\n", numBytes1, numBytes2);

    testInterleavedRead(handle1, handle2, data1, data2, numBytes1, numBytes2);

    printf("\n Data from server-1 :\n");
    printf("%s\n", data1);
    printf("\n Data from server-2 :\n");
    printf("%s\n", data2);

    testClose2(handle1);
    testClose2(handle2);
}


