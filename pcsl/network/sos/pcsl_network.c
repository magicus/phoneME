/*
 * 
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

/** @file
 *
 * This file implements all the necessary PCSL interfaces for SoS client.
 * Please note that SoS Proxy server must be started before SoS client. SoS
 * client and proxy server communication is based on RPC mechanism
 */

#include <stdio.h>
#include <pcsl_network.h>
#include <netinet/in.h>

#define FUNC_OPEN_HOST 100
#define FUNC_OPEN_IPN 101
#define FUNC_READ 102
#define FUNC_AVAILABLE 103
#define FUNC_WRITE 104
#define FUNC_CLOSE 105
#define FUNC_SHUTDOWN 106
#define FUNC_GET_IPNUMBER 107

#define BUFFERSIZE 32
#define READ_BUFFER_SIZE 256

#define UNKNOWNHOST_EXCEPTION_ERROR -1

int portfd = -1;
int open_connections;

/** 'C' string for javax.microedition.io.ConnectionNotFoundException */
const char* pcslConnectionNotFoundException = "javax/microedition/io/ConnectionNotFoundException";

/** 'C' string for java.io.IOException */
const char* pcslIOException = "java/io/IOException";

/** Receive bytes from a serial port */
static char* serial_receive(int); 

/** Receive four bytes from a serial port for an integer */
static char* serial_receive_int();

/** Write an integer to serial port */
static void serial_send_int(int);

static void pcsl_strncpy(char*, char*, int); 

/** Parse an array of characters at serial port into an integer */
static int pcsl_serial_parse_INT(unsigned char buffer[]); 

/** Encode an integer into a character array */
static void pcsl_serial_encode_INT(int nBuf, unsigned char* buffer); 

/**
 * See pcsl_network.h for definition.
 */ 
int pcsl_socket_open_start(
    unsigned char *ipBytes, 
    int port, 
    void **pHandle,
    void **pContext) 
{
    char* proxy_char_data;
    int result;
    int ipn;

    if (pcsl_network_init_start(NULL) == PCSL_NET_IOERROR) { 
        return PCSL_NET_IOERROR;
    }

    printf("Serial port fd = %d\n", portfd);

    /* Send open request to sos server */
    serial_send_int(FUNC_OPEN_IPN);

    /* Send the host IP-address bytes to server */
    pcsl_serial_writechar(portfd, ipBytes, sizeof(ipBytes));

    /* Send the port-id to server */
    serial_send_int(port);
 
    /* Receive handle from proxy */
    proxy_char_data = (char *)serial_receive_int();

    /* Parse the char array into an integer */
    result = pcsl_serial_parse_INT(proxy_char_data);
    if (result == PCSL_NET_IOERROR) {
        return PCSL_NET_IOERROR;
    } else if (result == UNKNOWNHOST_EXCEPTION_ERROR) {
        return PCSL_NET_CONNECTION_NOTFOUND;
    } else {
        *pHandle = (void *)result;

        // Increment the count for open connections
        open_connections++;
    }

    printf("socketid = %d\n", *pHandle);
    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_open_finish(
    void *handle,
    void *context)
{   
    //Dummy stub
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 */ 
int pcsl_socket_read_start(
    void *handle, 
    unsigned char* buf, 
    int numbytes,
    int *pBytesRead, 
    void **pContext) 
{
    char* proxy_char_result;
    int bytesRead;
    char *output;
    int i;

    if (portfd == -1) {
        printf("Error in pcsl_socket_read0 : Serial port not initialised\n");
        return PCSL_NET_IOERROR;
    }
    
    // send read request to sos server
    serial_send_int(FUNC_READ);

    // send handle request to sos server
    serial_send_int((int)handle);

    // send numbytes to sos server
    serial_send_int(numbytes);

    // read the number of bytes to be read from proxy thr serial port
    proxy_char_result = (char *)serial_receive_int();
    bytesRead = pcsl_serial_parse_INT(proxy_char_result);
    if ((bytesRead == PCSL_NET_IOERROR)||(bytesRead == PCSL_NET_INTERRUPTED)) { 
        printf("ALERT in pcsl_socket_read0 : Socket exception occurred at proxy\n");
        return bytesRead;
    }

    if (bytesRead == 0) {
        //printf("Nothing to read from server!\n");
        buf = NULL;
        //pcsl_serial_flush(portfd);
        *pBytesRead = 0;
        return PCSL_NET_SUCCESS;
    }
    
    output = (char *)serial_receive(bytesRead);
    
    pcsl_strncpy(buf, output, bytesRead);
    //strncpy(buf, output, bytes_read);
    *pBytesRead = bytesRead;

    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_read_finish(
    void *handle,
    unsigned char *pData,
    int len,
    int *pBytesRead,
    void *context)
{
    //Dummy stub
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 */ 
int pcsl_socket_write_start(
    void *handle, 
    char* buf, 
    int numbytes,
    int *pBytesWritten, 
    void **pContext) 
{
    int bytesWritten = -1;
    int length;
    char* proxy_char_data;

    if (portfd == -1) {
        printf("Error in write_data : Serial port not initialised\n");
        return PCSL_NET_IOERROR;
    }
    
    // send write request to sos server
    serial_send_int(FUNC_WRITE);

    // send handle request to sos server
    serial_send_int((int)handle);
   
    // send length of the character array
    serial_send_int(numbytes);

    //write data to the server
    pcsl_serial_writechar(portfd, buf, numbytes);

    // wait for acknowledgement from server for
    // write operation 
    proxy_char_data = (char *)serial_receive_int();
    bytesWritten = pcsl_serial_parse_INT(proxy_char_data);

    if ((bytesWritten == PCSL_NET_IOERROR)||
        (bytesWritten == PCSL_NET_INTERRUPTED)) { 
        return bytesWritten;
    }
        
    printf("No of bytes written : %d\n", bytesWritten);
    *pBytesWritten = bytesWritten;
    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_write_finish(
    void *handle,
    char *pData,
    int len,
    int *pBytesWritten,
    void *context)
{
    //Dummy stub
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 */ 
int pcsl_socket_available(void *handle, int *pBytesAvailable)
{
    char* proxy_char_result;
    int result;

    if (portfd == -1) {
        printf("Error in pcsl_socket_available0 : Serial port not initialised\n");
        return PCSL_NET_IOERROR;
    }
    
    // send read request to sos server
    serial_send_int(FUNC_AVAILABLE);

    // send handle request to sos server
    serial_send_int((int)handle);

    // read the number of bytes to be read from proxy thr serial port
    proxy_char_result = (char *)serial_receive_int();
    result = pcsl_serial_parse_INT(proxy_char_result);
    if (result == PCSL_NET_IOERROR) {
        printf("ALERT in pcsl_socket_available0 : Socket exception occurred at proxy\n");
        return PCSL_NET_IOERROR;
    } else {
        *pBytesAvailable = result;
    }

    printf("Bytes available to read from proxy : %d\n", *pBytesAvailable);

    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 */ 
int pcsl_socket_close_start(void *handle, void **pContext) {
    char* proxy_char_data;
    int result;

    //send close request to sos server
    serial_send_int(FUNC_CLOSE);

    //send the handle to the server
    serial_send_int((int)handle);

    // wait for response from server. 
    proxy_char_data = (char *)serial_receive_int();
    
    result = pcsl_serial_parse_INT(proxy_char_data);
    if (result == PCSL_NET_IOERROR) {
        return PCSL_NET_IOERROR;
    } else {
        printf("Result of close operation: %d\n", result);

        //Decrement the count for open connections
        open_connections--;
    }

    // Close the serial port if there are no more open connections 
    if (open_connections == 0) {
        pcsl_serial_close(portfd);
    }

    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 *
 * Since the start function never returns PCSL_NET_WOULDBLOCK, this
 * function should never be called.
 */ 
int pcsl_socket_close_finish(
    void *handle,
    void *context)
{
    return PCSL_NET_INVALID;
}

/**
 * See pcsl_network.h for definition.
 */ 
int pcsl_socket_shutdown_output(void *handle) {
    char* proxy_char_data;
    int result;

    //send close request to sos server
    serial_send_int(FUNC_SHUTDOWN);

    //send the handle to the server
    serial_send_int((int)handle);

    // wait for response from server. 
    proxy_char_data = (char *)serial_receive_int();
    
    result = pcsl_serial_parse_INT(proxy_char_data);
    if (result == PCSL_NET_IOERROR) {
        printf("ALERT in read_data : Socket exception occurred at proxy\n");
        return PCSL_NET_IOERROR;
    } else {
        printf("Result of shutdown operation: %d\n", result);

        //Decrement the count for open connections
        open_connections--;
    }

    // Close the serial port if there are no open connections at 
    // client and server
    if (open_connections == 0) {
        pcsl_serial_close(portfd);
    }

    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 */ 
int pcsl_network_gethostbyname_start(
    char *hostname,
    unsigned char *pAddress,
    int maxLen,
    int *pLen,
    void **pHandle,
    void **pContext)
{
    char* proxy_char_data;
    int length;
    int result;

    /* Initially open the linux serial port */
    if (pcsl_network_init_start(NULL) == PCSL_NET_IOERROR) { 
        return PCSL_NET_IOERROR;
    }

    printf("Serial port fd = %d\n", portfd);

    /* send open request to sos server */
    serial_send_int(FUNC_GET_IPNUMBER);

    /* Send the host address to server */
    length = strlen(hostname);
    serial_send_int(length);
    pcsl_serial_writechar(portfd, hostname, length);

    /* 
     * Receive data from proxy, Byte 0 represents the highest
     * order byte of the IPaddress
     */
    proxy_char_data = (char *)serial_receive_int();

    /* Parse the char array into an integer */
    result = pcsl_serial_parse_INT(proxy_char_data);
    if (result == UNKNOWNHOST_EXCEPTION_ERROR) {
        printf("ALERT in pcsl_socket_getIpNumber : " 
               "UnknownHost exception occurred at proxy\n");
        return PCSL_NET_IOERROR;
    } 

    pcsl_strncpy(pAddress, proxy_char_data, MAX_ADDR_LENGTH);
    *pLen = MAX_ADDR_LENGTH;

    printf("ipn = %d\n", result);
    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 *
 * Since the start function never returns PCSL_NET_WOULDBLOCK, this
 * function would never be called.
 */
int pcsl_network_gethostbyname_finish(
    unsigned char *pAddress,
    int maxLen,
    int *pLen,
    void *handle,
    void *context)
{
    (void)handle;
    (void)context;
    return PCSL_NET_INVALID;
}


/**
 * See pcsl_network.h for definition.
 */
int pcsl_socket_getsockopt(
    void *handle,
    int flag,
    int *pOptval)
{ 
    //Dummy stub
    return PCSL_NET_IOERROR;
}   

/**
 * See pcsl_network.h for definition.
 */ 
int pcsl_socket_setsockopt(
    void *handle,
    int flag,
    int optval)
{
    //Dummy stub
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 */ 
int pcsl_socket_getlocaladdr(
    void *handle,
    char *pAddress)
{
    //Dummy stub
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 */ 
int pcsl_socket_getremoteaddr(
    void *handle,
    char *pAddress)
{
    //Dummy stub
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 */ 
int pcsl_socket_getlocalport(
    void *handle,
    int *pPortNumber)
{
    //Dummy stub
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 */ 
int pcsl_socket_getremoteport(
    void *handle,
    int *pPortNumber)
{
    //Dummy stub
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 */ 
int pcsl_network_init(void){
	return pcsl_network_init_start(NULL);
}

/**
 * See pcsl_network.h for definition.
 */ 
int pcsl_network_init_start(PCSL_NET_CALLBACK pcsl_network_callback) {
    int result;

    (void)pcsl_network_callback;

    /* Initially open the linux serial port */
    if (portfd == -1) {
        result = pcsl_serial_open(0);
        if (result == -1 ) {
            return PCSL_NET_IOERROR;
        } else {
            portfd = result;
        }
    }

    printf("Serial port fd = %d\n", portfd);

    return PCSL_NET_SUCCESS;
} 

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_init_finish(void) {
    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_finalize_start(PCSL_NET_CALLBACK pcsl_network_callback) {
    (void)pcsl_network_callback;
    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_finalize_finish(void) {
    return PCSL_NET_SUCCESS;
}


/**
 * See pcsl_network.h for definition.
 */ 
int pcsl_network_error(void *handle) {
    //Value of portfd is > 0 if it is opened successfully, hence it can
    //be used to represent an error
    return ((portfd < 0)? portfd : 0);
}

/**
 * See pcsl_network.h for definition.
 */ 
int pcsl_network_getLocalHostName(char *pLocalHost) {
    /* Initialize the network */
    if (pcsl_network_init_start() != PCSL_NET_SUCCESS) {
        return PCSL_NET_IOERROR;
    }

    if (gethostname(pLocalHost, MAX_HOST_LENGTH) == 0) {
        return PCSL_NET_IOERROR;
    }

    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 */ 
int pcsl_network_getLocalIPAddressAsString(
    char *pLocalIPAddress)
{
    struct in_addr addr;
    char hostname[MAX_HOST_LENGTH];
    int len;
    int status;
    unsigned char IPAddressBytes[16];
    void *dummy;
    /*
    hostname = pcsl_socket_getLocalHostName();
    addr.s_addr = pcsl_socket_getIpNumber(hostname);
    return inet_ntoa(addr);
    */

    status = pcsl_network_getLocalHostName(hostname);
    if (status == PCSL_NET_IOERROR) {
        return PCSL_NET_IOERROR;
    }

    status = pcsl_network_gethostbyname_start(
                hostname, IPAddressBytes, sizeof(IPAddressBytes),
                &len, &dummy, &dummy);

    if (status == PCSL_NET_IOERROR) {
        return PCSL_NET_IOERROR;
    }

    memcpy(&addr.s_addr, IPAddressBytes, sizeof(addr.s_addr));

    strcpy(pLocalIPAddress, (char *)inet_ntoa(addr));

    return PCSL_NET_SUCCESS;
}

/**
 * Receive bytes from the serial port
 *
 * @param numbytes Number of characters to be read from the serial port
 *
 * @return Array of bytes(characters) of size equal to "numbytes"
 *
 */
static char* serial_receive(int numbytes) {
    int bytes_read = 0;
    int count;
    char read_buffer[numbytes];
    char *p; 

    memset(read_buffer, 0, numbytes);

    while (bytes_read < numbytes) {
        count = 0;
        count = pcsl_serial_readchar(portfd, (read_buffer + bytes_read), 
                                (numbytes - bytes_read));
        if (count > 0) {
            bytes_read += count;
        }
    }

    p = (char *)&read_buffer;
    return p;
} 
     
/**
 * Receive four bytes from the serial port
 *
 */
static char* serial_receive_int() {
    return serial_receive(4);
} 

/**
 * Write to serial port
 *
 * @param nBuf Integer to be written at the serial port
 */
static void serial_send_int(int nBuf) {
    char buffer[4];

    pcsl_serial_encode_INT(nBuf, buffer);

    pcsl_serial_writechar(portfd, buffer, sizeof(buffer));
   
}

/**
 * Copy array of characters in another string. This function is
 * similar to "strncpy" but it does not depend on null terminated strings
 *
 * @param dest Destination string
 * @param src Source string
 * @param numbytes No of characters to be copied
 *
 */
static void pcsl_strncpy(char* dest, char* src, int numbytes) {
    int i;
    char *p = dest;
    for (i=0; i < numbytes; i++) {
        *p++ = *src++;
    }
}

/**
 * Parse an array of characters at serial port into an integer
 *
 * This function may need to be ported according to the 
 * endianness of the host
 *
 * @param buffer : An array of characters
 * @return Integer value of parsed characters
 *
 */
static int pcsl_serial_parse_INT(unsigned char buffer[]) {
    int n = 0;
    int i;

    for(i = 0; i < 4; i++){
        n += ((0xFF & buffer[i]) << (8*(3 - i)));
    }
    return n;
}   

/**
 * Encode an integer into a character array. The first
 * element in the array represents the most significant byte
 * 
 * \verbatim
 
   31      24         16          8         0 
  [ -------- | --------  | --------| -------- ] 
     buf[0]     buf[1]      buf[2]    buf[3]
 
  \endverbatim
 *
 * @param nBuf : integer number
 * @param buf : An array of output characters
 *
 */
static void pcsl_serial_encode_INT(int nBuf, unsigned char* buffer) {
    int i;
   
    for(i = 0; i < 4; i++) {
        buffer[3 - i] = (unsigned char)((nBuf >> 8*i) & 0xFF);
    }
}

/**
 * Opens a platform specific TCP socket. This function is not a part of
 * PCSL interface
 *
 * @param host Host name of the server
 * @param port number of the port to open
 *
 * @return a non-negative platform-specific handle if the function completes
 * successfully; IO_WOULDBLOCK if the operation would be blocked; otherwise,
 * any other negative value if there was an error (If there was an error, 
 * exception will also set.)
 */
static int pcsl_socket_open_by_host(char* host, int port) {
    int handle = -1;
    char* proxy_char_data;
    int length;
    int result;

    /* Initially open the linux serial port */
    if (portfd == -1) {
        portfd = pcsl_serial_open(0);
    }

    printf("Serial port fd = %d\n", portfd);

    if (portfd == -1) {
       return -1;
    }
 
    /* send open request to sos server */
    serial_send_int(FUNC_OPEN_HOST);

    /* Send the host address to server */
    length = strlen(host);
    serial_send_int(length);
    pcsl_serial_writechar(portfd, host, length);

    /* Send the port-id to server */
    serial_send_int(port);
 
    /* Receive handle from proxy */
    proxy_char_data = (char *)serial_receive_int();

    /* Parse the char array into an integer */
    result = pcsl_serial_parse_INT(proxy_char_data);
    if (result == PCSL_NET_IOERROR) {
        printf("ALERT in pcsl_socket_open_by_host0 : IO exception occurred at proxy\n");
    } else if (result == UNKNOWNHOST_EXCEPTION_ERROR) {
        printf("ALERT in pcsl_socket_open_by_host0 : "
                " UnknownHost exception occurred at proxy\n");
    } else {
        handle = result;

        // Increment the count for open connections
        open_connections++;
    }

    printf("socketid = %d\n", handle);

    return handle;
}

#ifdef ENABLE_SERVER_SOCKET

/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_serversocket_open(
    int port,
    void **pHandle)
{
    /* need revisit */
    return PCSL_NET_IOERROR;
}
    
/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_serversocket_accept_start(
    void *handle,
    void **pConnectionHandle,
    void **pContext)
{
    /* need revisit */
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_serversocket_accept_finish(
    void *handle,
    void **pConnectionHandle,
    void **pContext)
{
    /* need revisit */
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_sereversocket_close_start(
    void *handle,
    void **pContext)
{
    return pcsl_socket_close_start(handle, pContext);
}


/**
 * See pcsl_serversocket.h for definition.
 */
int pcsl_serversocket_close_finish(
	void *handle,
	void *context)
{
    return pcsl_socket_close_finish(handle, context);
}

#endif /* ifdef ENABLE_SERVER_SOCKET */

/**
 * See pcsl_network.h for definition.
 */
void pcsl_add_network_notifier(
    void *handle,
    int event) {
    /* need revisit */
}

/**
 * See pcsl_network.h for definition.
 */
void pcsl_remove_network_notifier(
    void *handle,
    int event) {
    /* need revisit */
}

/**
 * See pcsl_network.h for definition.
 */
unsigned int pcsl_network_htonl(
    unsigned int value) {
    return htonl(value);
}

/**
 * See pcsl_network.h for definition.
 */
unsigned int pcsl_network_ntohl(
    unsigned int value) {
    return ntohl(value);
}
    
/**
 * See pcsl_network.h for definition.
 */
unsigned short pcsl_network_htons(
    unsigned short value) {
    return htons(value);
}

/**
 * See pcsl_network.h for definition.
 */
unsigned short pcsl_network_ntohs(
    unsigned short value) {
    return ntohs(value);
}

/**
 * See pcsl_network.h for definition.
 */
char * pcsl_inet_ntoa (void *ipBytes) {
    static char buffer[] = {'\0'};

    return buffer;
}
