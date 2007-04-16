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

static int status;
static void *pContext;

//void NotifyStatusChanged(void *handle, int waitingFor) { }

void testGetHostByName(char* hostname, unsigned char* ipBytes) {
    int len; 
    void *handle;
    
    status = pcsl_network_gethostbyname_start(hostname, ipBytes, 
                     MAX_ADDR_LENGTH, &len, &handle, &pContext );
    assertTrue("Host name not converted to byte array", status == PCSL_NET_SUCCESS); 
    printf("Length = %d\n", len);
}

void* testOpen(unsigned char* ipBytes, int port) {
    char* exception;
    void *handle = NULL;
   
    do { 
        status = pcsl_socket_open_start(ipBytes, port, &handle, 
                    &pContext);
        assertTrue("Error in opening socket connection", 
                    ((status == PCSL_NET_SUCCESS)||(status == PCSL_NET_WOULDBLOCK)));
    } while (status == PCSL_NET_WOULDBLOCK);
    
    return handle;
}

void testWrite(void *handle, char* request) {
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
   
int testAvailable(void *handle) {
    int numBytes;
    usleep(200);
 
    status = pcsl_socket_available(handle, &numBytes);
    assertTrue("Error in available() at socket ", status == PCSL_NET_SUCCESS);
    
    return numBytes;
}

void testRead(void *handle, char* data, int numBytes) {
    int count;
    int bytesRead = 0;
 
    do {
        count = 0;
        status = pcsl_socket_read_start(handle, (data + bytesRead),
                                   (numBytes - bytesRead), &count, &pContext);
        if (status == PCSL_NET_INTERRUPTED) {
            assertTrue("Interrupted IO Exception in testRead", 0);
            break;
        }

        if (count > 0) {
            bytesRead += count;
        }
    } while (bytesRead < numBytes);
}

void testClose(void *handle) {
    status = pcsl_socket_close_start(handle, &pContext);
    assertTrue("Error in closing socket connection", status == PCSL_NET_SUCCESS);
}    
    
/**
 * Unit test framework entry point for this set of unit tests.
 *  
 */
void testSimpleNetwork_runTests() {
    unsigned char ipBytes[MAX_ADDR_LENGTH]; //can be set to INET6_ADDRSTRLEN;
    void *handle;
    int numBytes;
    char *data;
    char *hostname = "host.domain";
    char* request = "GET /midlets/midlet.jad HTTP/1.0\n\n";

    testGetHostByName(hostname, ipBytes);

    handle = testOpen(ipBytes, 80);
    printf("open handle = %d\n", (int)handle);

    testWrite(handle, request);

    numBytes = testAvailable(handle);
    printf("numBytes = %d\n", numBytes);

    if (numBytes > 0) {
        data = (char *) malloc((numBytes + 1)*sizeof(char));
        testRead(handle, data, numBytes);
        printf("\n Data from server :\n");
        printf("%s\n", data);
    }

    testClose(handle);
}


