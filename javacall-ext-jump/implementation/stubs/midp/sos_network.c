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

/** @file
*
* This file implements all the necessary PCSL interfaces for SoS client.
* Please note that SoS Proxy server must be started before SoS client. SoS
* client and proxy server communication is based on RPC mechanism.
* 
* This version supports multiple sockets.
* 
*
* for SoS network, replace javacall_impl\<your platform>\src\network.c by this file.
*/

#include <stdio.h>
#include <string.h>
#include "javacall_network.h"
#include "javacall_socket.h"
#include "javacall_logging.h"

#define REPORT(msg) javacall_print(msg)

#define UNKNOWNHOST_EXCEPTION_ERROR -1

#define PROTOCOL_VERSION 100

static unsigned char pid = 0;  
typedef struct _SOS_HEADER {
    unsigned char lenl;
    unsigned char lenh;
    unsigned char type;
    unsigned char xid;
    unsigned char handle;
}SOS_HEADER;

/*MTU: The largest packet length in bytes, including HEADER_LENGTH & CRC_LENGTH*/
#define MTU 1032
/*HEADER_LENGTH: Length of packet header*/
#define HEADER_LENGTH 5
/*CRC_LENGTH: Length of CRC value*/
#define CRC_LENGTH 2

#define TYPE_CHALLENGE 0x03
#define TYPE_RESULT 0x02
#define TYPE_INVOKE 0x01

//#define FUNC_OPEN 0x03
#define FUNC_OPEN 0x10
#define FUNC_SEND 0x04
#define FUNC_RECV 0x05
#define FUNC_SHUTDOWN 0x06
#define FUNC_GETIPNUMBER 0x07
#define FUNC_CLOSE 0x08
#define FUNC_PUTSTRING 0x01
#define FUNC_RESYNC 0x00

#define SYNC_BYTE ((unsigned char)0x55)
#define ESCAPE_CHAR ((unsigned char)0xdd)

#define SOS_SD_RECV 0x01
#define SOS_SD_SEND 0x02

#define ERROR_OPEN_IO (-3)
#define ERROR_OPEN_CONNECTION (-4)

static void init_crc_table(void);
static unsigned short int calc_crc(unsigned short crc, unsigned char* buf, int len);
static char* get_content(char* packet);
static unsigned char get_cid(char* packet);
static unsigned char get_pid(char* packet);
static unsigned char get_klass(char* packet);
static int invoke_remote(int func, unsigned char handle, void* in, int in_len, void* out, int out_len);
static unsigned char maketype(unsigned char klass, unsigned char func);
static void pid_dec();
static void pid_inc();
static int recv_char();
static int remote_close(unsigned char handle);
static int remote_getipnumber(char* host);
static int remote_open(int ipn, int port);
static int remote_recv(unsigned char handle, char* buf, int len);
static void remote_resync();
static int remote_send(unsigned char handle, char* buf, int len);
static int remote_shutdown(unsigned char handle, unsigned char flag);
static void send_buf(char* buf, int len);
static void send_char(unsigned char ch);
static void send_ESCAPE();
static void send_SYNCBYTE();
static int send_packet(int func, unsigned char cid, unsigned char pid, unsigned char handle, char* content, int clen);
static char* wait_packet();
int serial_DebugPutString(const char* buf);

static int parse_INT(char* p);
static void encode_INT(int i, char* p);

static int netinit = 0;
int open_connections;

/** 'C' string for javax.microedition.io.ConnectionNotFoundException */
const char* pcslConnectionNotFoundException = "javax/microedition/io/ConnectionNotFoundException";

/** 'C' string for java.io.IOException */
const char* pcslIOException = "java/io/IOException";

/** Parse an array of characters at serial port into an integer */
static int javacall_serial_parse_INT(unsigned char buffer[]); 

/** Encode an integer into a character array */
static void javacall_serial_encode_INT(int nBuf, unsigned char* buffer); 

/**
* See javacall_network.h for definition.
*/ 
javacall_result javacall_socket_open_start(
    unsigned char *ipBytes, 
    int port, 
    void **pHandle,
    void **/*pContext*/) 
{
    int fd = -1;
    int ip = 0;
    char buff[256] = {0};

    sprintf(buff,"javacall_socket_open_start() >> port=%d , ipBytes: %d.%d.%d.%d\n",port,ipBytes[0], ipBytes[1], ipBytes[2], ipBytes[3]);
    REPORT(buff);

    ip = javacall_serial_parse_INT(ipBytes);
    fd = remote_open(ip, port);

    if (fd != -1 ) {
        *pHandle = (void *)fd;
        return JAVACALL_OK;
    } else

    REPORT("ERROR: javacall_socket_open_start: \n");
    return JAVACALL_FAIL;
}

/**
* See javacall_network.h for definition.
*/
javacall_result javacall_socket_open_finish(
    void */*handle*/,
    void */*context*/)
{   
    //Dummy stub
    return JAVACALL_FAIL;
}

/**
* See javacall_network.h for definition.
*/ 
javacall_result javacall_socket_read_start(
    void *handle, 
    unsigned char* buf, 
    int numbytes,
    int *pBytesRead, 
    void **/*pContext*/) 
{
    int bytesRead;
    int maxlen;

    maxlen = MTU - HEADER_LENGTH - CRC_LENGTH - 4;
    if (numbytes > maxlen)
        numbytes = maxlen;
    bytesRead = remote_recv((int)handle,(char *)buf, numbytes);
    if (bytesRead == 0) {
        buf = NULL;
        *pBytesRead = 0;
        /*
           In case we receive a 'No data' packet from the 
           SoS server, we will send back a JAVACALL_NO_DATA_AVAILABLE
           So that java will know to yield and then re-try.
        */
        return JAVACALL_NO_DATA_AVAILABLE;
    } else if (bytesRead < 0) {
        REPORT("ERROR: javacall_socket_read_start returning JAVACALL_FAIL\n");
        return JAVACALL_FAIL;
    }
    *pBytesRead = bytesRead;
    return JAVACALL_OK;
}

/**
* See javacall_network.h for definition.
*/
javacall_result javacall_socket_read_finish(
    void */*handle*/,
    unsigned char */*pData*/,
    int /*len*/,
    int */*pBytesRead*/,
    void */*context*/)
{
    //Dummy stub
    return JAVACALL_FAIL;
}

/**
* See javacall_network.h for definition.
*/ 
javacall_result javacall_socket_write_start(
    void *handle, 
    char* buf, 
    int numbytes,
    int *pBytesWritten, 
    void **/*pContext*/) 
{
    int bytesWritten = 0;
    int maxlen;

    //REPORT("javacall_socket_write_start\n");
    //Since we can't send any packet larger than MTU, cut the length to fit it
    maxlen = MTU - HEADER_LENGTH - CRC_LENGTH;
    if (numbytes > maxlen)
        numbytes = maxlen;

    bytesWritten = remote_send((int)handle,buf, numbytes);

    if (bytesWritten < 0) {
        REPORT("ERROR: javacall_socket_write_start remote_send returned error\n");
        return JAVACALL_FAIL;
    }
    *pBytesWritten = bytesWritten;
    return JAVACALL_OK;
}

/**
* See javacall_network.h for definition.
*/
javacall_result javacall_socket_write_finish(
    void */*handle*/,
    char */*pData*/,
    int /*len*/,
    int */*pBytesWritten*/,
    void */*context*/)
{
    //Dummy stub
    return JAVACALL_FAIL;
}

/**
* See javacall_network.h for definition.
*/ 
javacall_result javacall_socket_available(void */*handle*/, int */*pBytesAvailable*/)
{
    return JAVACALL_FAIL;
}

/**
* See javacall_network.h for definition.
*/ 
javacall_result javacall_socket_close_start(void *handle, void **/*pContext*/) {
    int status;
    //REPORT1("javacall_socket_close_start: handle=%d\n", (int)handle);

    status = remote_close((int)handle);
    if (status != 0) {
        REPORT("javacall_socket_close_start failed <<<<< \n");
        return JAVACALL_FAIL;
    } 
    //REPORT("javacall_socket_close_start succeeded <<<\n");
    return JAVACALL_OK;
}

/**
* See javacall_network.h for definition.
*
* Since the start function doesn't block, this
* function should never be called.
*/ 
javacall_result javacall_socket_close_finish(
    void */*handle*/,
    void */*context*/)
{
    return JAVACALL_INVALID_ARGUMENT;
}

/**
* See javacall_network.h for definition.
*/ 
javacall_result javacall_socket_shutdown_output(void *handle) {

    int status;

    //REPORT("javacall_socket_shutdown_output >>>\n");
    status = remote_shutdown((int)handle,SOS_SD_SEND);

    if (status != 0 ) {
        REPORT("ERROR javacall_socket_shutdown_output\n");
        return JAVACALL_FAIL;
    } 

    //REPORT("javacall_socket_shutdown_output <<<\n");
    return JAVACALL_OK;
}

/**
* See javacall_network.h for definition.
*/ 
javacall_result javacall_network_gethostbyname_start(
    char *hostname,
    unsigned char *pAddress,
    int /*maxLen*/,
    int *pLen,
    void **/*pHandle*/,
    void **/*pContext*/) {

    int result = -1;

    //REPORT("javacall_network_gethostbyname_start:\n");
    if (javacall_network_init() != JAVACALL_OK) {
        REPORT("ERROR: javacall_network_gethostbyname_start: init failed, retuning error\n");   
        return JAVACALL_FAIL;
    }

    result = remote_getipnumber((char *)hostname);

    if (result == -1 ) {
        REPORT("ALERT in javacall_socket_getIpNumber : " 
            "UnknownHost exception occurred at proxy\n");
        return JAVACALL_FAIL; 
    }

    pAddress[0] = (((int)result >> 24) & 0xFF) ;
    pAddress[1] = (((int)result >> 16) & 0xFF) ;
    pAddress[2] = (((int)result >> 8) & 0xFF) ;
    pAddress[3] = (int)result & 0xFF ;

    *pLen = 4;

return JAVACALL_OK;
}

/**
* See javacall_network.h for definition.
*
* Since the start function never returns PCSL_NET_WOULDBLOCK, this
* function would never be called.
*/
javacall_result javacall_network_gethostbyname_finish(
    unsigned char */*pAddress*/,
    int /*maxLen*/,
    int */*pLen*/,
    void */*handle*/,
    void */*context*/)
{
    return JAVACALL_FAIL;
}


/**
* See javacall_network.h for definition.
*/
javacall_result javacall_socket_getsockopt(
    void */*handle*/,
    int /*flag*/,
    int */*pOptval*/)
{ 
    return JAVACALL_FAIL;
}   

/**
* See javacall_network.h for definition.
*/ 
javacall_result javacall_socket_setsockopt(
    void */*handle*/,
    int /*flag*/,
    int /*optval*/)
{
    
    return JAVACALL_FAIL;
}

/**
* See javacall_network.h for definition.
*/ 
javacall_result javacall_socket_getlocaladdr(
    void */*handle*/,
    char */*pAddress*/)
{
    return JAVACALL_FAIL;
}

/**
* See javacall_network.h for definition.
*/ 
javacall_result javacall_socket_getremoteaddr(
    void */*handle*/,
    char */*pAddress*/)
{
    return JAVACALL_FAIL;
}

/**
* See javacall_network.h for definition.
*/ 
javacall_result javacall_socket_getlocalport(
    void */*handle*/,
    int */*pPortNumber*/)
{
    return JAVACALL_FAIL;
}

/**
* See javacall_network.h for definition.
*/ 
javacall_result javacall_socket_getremoteport(
    void */*handle*/,
    int */*pPortNumber*/)
{
    return JAVACALL_FAIL;
}

/**
* See javacall_network.h for definition.
*/ 
javacall_result javacall_network_init(void) {
    javacall_result status = JAVACALL_OK;
    //REPORT("javacall_network_init\n");
    if (!netinit) {
        //REPORT("javacall_network_init: >>\n");
        status = javacall_serial_init();
        if (status != JAVACALL_OK) {
            REPORT("networkInit: fatal error \n");
            return JAVACALL_FAIL;
        }
        init_crc_table();
        remote_resync();
        netinit = 1;
        serial_DebugPutString("connected");
        //REPORT("javacall_network_init: <<\n");
    }
    return status;
} 

/**
* See javacall_network.h for definition.
*/ 
int javacall_network_error(void */*handle*/) {
    return 0;
}

/**
* See javacall_network.h for definition.
*/ 
javacall_result javacall_network_getLocalHostName(char */*pLocalHost*/) {
    return JAVACALL_FAIL;   
}

/**
* See javacall_network.h for definition.
*/ 
javacall_result javacall_network_getLocalIPAddressAsString(
    char */*pLocalIPAddress*/)
{
    return JAVACALL_FAIL;   
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
static void javacall_strncpy(char* dest, char* src, int numbytes) {
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
static int javacall_serial_parse_INT(unsigned char buffer[]) {
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
static void javacall_serial_encode_INT(int nBuf, unsigned char* buffer) {
    int i;

    for(i = 0; i < 4; i++) {
        buffer[3 - i] = (unsigned char)((nBuf >> 8*i) & 0xFF);
    }
}


/**
* See javacall_network.h for definition.
*/
javacall_result javacall_network_getlocalport(
    void */*handle*/,
    int */*pPortNumber*/)
{
    return JAVACALL_FAIL;
}


javacall_result javacall_network_close(void) {
    if (netinit) {
        //REPORT("WARNING: javacall_network_close() doing nothing<<\n");
        //need revisit - after disconnecting from sos server, "crc error" occurs when re-connecting
        //javacall_serial_finalize();
        //netinit = 0;
    }
    return JAVACALL_OK;
}

javacall_result javacall_network_getremoteport(
    void */*handle*/,
    int */*pPortNumber*/)
{

    return JAVACALL_FAIL;
}

javacall_result javacall_network_getsockopt(
    void */*handle*/,
    int /*flag*/,
    int */*pOptval*/)
    //int pOptval)
{

    return JAVACALL_FAIL;
}

javacall_result javacall_network_setsockopt(
    void */*handle*/,
    int /*flag*/,
    int /*optval*/)
{

    return JAVACALL_OK;
}

/**
* Porting for SoS (Socket over Serial):
*	To pass TCK without any hardware network support, we have to make an 
* alternative way to achieve it via serial port. The conception is: replace
* all machine-dependant socket implementation functions with SoS implementation,
* to package every socket operation into serial packet, transfer it to Proxy,
* perform by Proxy, then send the result back via serial port as same.
* 	This version of porting has no supporting for serversocket and 
* multi-connetions, but it's enough to pass TCK.
*
*/


/**************
SoS functions
***************/
static char* get_content(char* packet)
{
    return packet + HEADER_LENGTH;
}

static unsigned char get_cid(char* packet)
{
    SOS_HEADER* header = (SOS_HEADER*)packet;
    unsigned char cid = header->xid & 0x0f;
    return cid;
}

static unsigned char get_pid(char* packet)
{
    SOS_HEADER* header = (SOS_HEADER*)packet;
    unsigned char pid = (header->xid >> 4) & 0x07;
    return pid;
}

static unsigned char get_klass(char* packet)
{
    SOS_HEADER* header = (SOS_HEADER*)packet;
    return (header->type >> 5) & 0x07;
}

static unsigned char get_version(char* packet)
{
    SOS_HEADER* header = (SOS_HEADER*)packet;
    return (header->handle);
}


/*****************************************************
Send function invoking command to Proxy and wait reply
*****************************************************/
static int invoke_remote(int func, unsigned char handle, void* in, int in_len, void* out, int out_len)
{
    char *packet;
    int status = JAVACALL_OK;


    //To send any packet to Proxy, client must wait for the CHALLENGE first
wait:
    do {
        do {
            packet = wait_packet();
        } while (packet == NULL);
    } while (TYPE_CHALLENGE != get_klass(packet));

    if (get_version(packet)!= PROTOCOL_VERSION) {
        REPORT("Server is using a different version protocol version !!\r\n");
    }

    //Send the packet out
resend:
    switch (func) {
case FUNC_OPEN:
    //REPORT("invoke_remote: FUNC_OPEN\n");
    if ((out == NULL) || (out_len != 4))
        return JAVACALL_INVALID_ARGUMENT;
    if (send_packet(maketype(TYPE_INVOKE, FUNC_OPEN), get_cid(packet), pid, handle, (char *)in, in_len) < 0)
        return JAVACALL_FAIL;
    break;
case FUNC_SEND:
    //REPORT("invoke_remote: FUNC_SEND\n");
    if ((out == NULL) || (out_len != 4))
        return JAVACALL_INVALID_ARGUMENT;
    if ((status = send_packet(maketype(TYPE_INVOKE, FUNC_SEND), get_cid(packet), pid, handle, (char *)in, in_len)) != JAVACALL_OK)
        return status;
    break;
case FUNC_RECV:
    {
        char tmp[4];
        //REPORT("invoke_remote: FUNC_RECV\n");
        if ((out == NULL) || (out_len != 4))
            return JAVACALL_INVALID_ARGUMENT;
        encode_INT(in_len, &tmp[0]);
        if ((status = send_packet(maketype(TYPE_INVOKE, FUNC_RECV), get_cid(packet), pid, handle, /*(char*)&in_len*/(char *)tmp, 2) ) != JAVACALL_OK)
            return status;
    }
    break;
case FUNC_SHUTDOWN:
    //REPORT("invoke_remote: FUNC_SHUTDOWN\n");
    if (send_packet(maketype(TYPE_INVOKE, FUNC_SHUTDOWN), get_cid(packet), pid, handle, (char *)in, in_len) < 0)
        return JAVACALL_FAIL;
    break;
case FUNC_RESYNC:
    //REPORT("invoke_remote: FUNC_RESYNC\n");
    if (send_packet(maketype(TYPE_INVOKE, FUNC_RESYNC), get_cid(packet), 0/*pid*/, handle, NULL, 0) < 0) {
        REPORT("invoke_rmote: FUNC_RESYNC failed\n");
        return JAVACALL_FAIL;
    }
    //REPORT("invoke_remote: FUNC_RESYNC suceeded :)\n");
    break;
case FUNC_GETIPNUMBER:
    //REPORT("invoke_remote: FUNC_GETIPNUMBER\n");  
    if ((out == NULL) || (out_len != 4)) {
        REPORT("invoke_remote: FUNC_GETIPNUMBER: Arg error\n");
        return JAVACALL_INVALID_ARGUMENT;
    }

    if (send_packet(maketype(TYPE_INVOKE, FUNC_GETIPNUMBER), get_cid(packet), pid, handle, (char *)in, in_len) < 0){
        REPORT("FUNC_GETIPNUMBER failed\n");
        return JAVACALL_FAIL;
    }
      
    break;
case FUNC_CLOSE:
    //REPORT("invoke_remote: FUNC_CLOSE\n");
    if (send_packet(maketype(TYPE_INVOKE, FUNC_CLOSE), get_cid(packet), pid, handle,  NULL, 0) < 0)
        return JAVACALL_FAIL;
    break;
case FUNC_PUTSTRING:
    //REPORT("invoke_remote: FUNC_PUTSTRING\n");
    if (send_packet(maketype(TYPE_INVOKE, FUNC_PUTSTRING), get_cid(packet), pid, handle, (char *)in, in_len) < 0)
        return JAVACALL_FAIL;
    break;


default:
    REPORT("invoke_remote: unknown function!!!\n");
    return JAVACALL_FAIL;
    }

    //Now we waiting for answer, be patient :)
    do {
        packet = wait_packet();
    } while (packet == NULL);


    if (get_klass(packet) == TYPE_CHALLENGE){
        //REPORT("invoke_remote got TYPE_CHALLENGE, resending\n");
        goto resend;	//If receive CHALLENGE, client need re-send the request
    }

    //We received an answer, check CLASS & PID field first
    if ((get_klass(packet) == TYPE_RESULT) && (pid == get_pid(packet))) {

        //Yes, CLASS & PID are both match for the expected answer	
        char* p;
        //REPORT("invoke_remote got TYPE_RESULT\n");
        pid_inc(); //According to the protocol, since we received an answer, 
        //pid should be increased

        p = get_content(packet);
        switch (func) {
        case FUNC_SHUTDOWN:
        case FUNC_PUTSTRING:
            return JAVACALL_OK;
        case FUNC_OPEN:
        case FUNC_SEND:		
            *(int*)out = parse_INT((char *)p);			
            return JAVACALL_OK;
        case FUNC_RECV:
            {
                int count;
                count = parse_INT(p);
                if (count > 0) {
                    if (count > in_len) {

                        REPORT("WARNING!!! FUNC_RECV: Recv packet larger than request, resend!");
                        pid_dec();
                        goto wait;
                    } else {
                        *(int*)out = count;
                        memcpy(in, p+4, count);
                    }
                } else if (count == 0) {
                    *(int*)out = 0; //end of stream
                    //REPORT("FUNC_RECV end of stream\n");
                } else { //count < 0
                    REPORT("FUNC_RECV count < 0 JAVACALL_FAIL\n");
                    return JAVACALL_FAIL;
                }
                return JAVACALL_OK;
            }

        case FUNC_RESYNC:
            {
                pid = 1;
                return JAVACALL_OK;
            }

        case FUNC_GETIPNUMBER:
            {
                int ipn;
                ipn = parse_INT(p);			
                *(int*)out = ipn;
                return JAVACALL_OK;
            }

        case FUNC_CLOSE:
            return JAVACALL_OK;

        default:
            return JAVACALL_FAIL;
        }
    } else {
        REPORT("Abnormal result! Resend again\n");
        goto resend;
    }

    REPORT("Never reach here\n");
    return JAVACALL_FAIL;


}

static unsigned char maketype(unsigned char klass, unsigned char func)
{
    unsigned char type;
    type = klass & 0x07;
    type = type << 5;
    type |= (func & 0x1f);

    return type;
}

/*
static short parse_SHORT(char* p)
{
short i;
char* pi = (char*)&i;
pi[0] = *p++;
pi[1] = *p++;

return i;
}
*/
static void pid_dec()
{
    if (pid == 1)
        pid = 7;
    else
        pid --;
}

static void pid_inc()
{
    if (pid == 7)
        pid = 1;
    else 
        pid ++;
}

static int recv_char()
{
    unsigned char ch;

    if ( javacall_serial_read(&ch,1) < 1 ) 
        return -1;
    else 
        return (int)(ch & 0xff);

}

int serial_DebugPutString(const char* buf)
{
    int maxlen = MTU - HEADER_LENGTH - CRC_LENGTH;
    int len = strlen(buf);
    if (invoke_remote(FUNC_PUTSTRING, PROTOCOL_VERSION, (void *)buf, len>maxlen?maxlen:len, NULL, 0) == 0) {
        return 0;
    } else {
        return -1;
    }
}

/********************************************************
FUNC: 	int remote_close()
DESC:  	Send "socket close" command to Proxy
INPUT: 	No
OUTPUT:
0	Success
-1	Error occured while invoking remotly
NOTE:	Proxy never return fail to client
********************************************************/
static int remote_close(unsigned char handle)
{
    return invoke_remote(FUNC_CLOSE, handle, NULL, 0, NULL, 0);
}




/********************************************************
FUNC: 	int remote_getipnumber(char* host)
DESC:  	Get host's ip from Proxy
INPUT: 
host	Null-ended string of the host's name
OUTPUT:
-1	Unknown host name
Other value	IP number of the host
NOTE:
********************************************************/
static int remote_getipnumber(char* host)
{
    int ipn = -1;
    //REPORT("remote_getipnumber\n");
    if (invoke_remote(FUNC_GETIPNUMBER, PROTOCOL_VERSION, host, strlen((char *)host), &ipn, 4) == JAVACALL_OK) {
        return ipn;
    } else {
        return JAVACALL_FAIL;
    }
}

/********************************************************
FUNC: 	int remote_open(int ipn, int port)
DESC:  	Open a new connection to remote host
INPUT: 
ipn	IP number of the remote host to connect
port	Remote port number
OUTPUT:
-1	Error occured while invoking remotly
-3	Create socket on Proxy error
0	Create connection OK
NOTE:
********************************************************/
static int remote_open(int ipn, int port)
{
    int result;
    static char par[6];
    //sprintf(b,"remote_open: ipn=%d,port=%d\n",ipn,port);
    //REPORT(b);
    encode_INT(port, &par[0]);
    encode_INT(ipn,  &par[2]);

    if (invoke_remote(FUNC_OPEN, PROTOCOL_VERSION, par, 6, &result, 4) == JAVACALL_OK)
        return result;
    else
        return JAVACALL_FAIL;
}

/********************************************************
FUNC: 	int remote_recv(char* buf, int len)
DESC:  	Receive len chars from comm port, put it into buf
INPUT: 
buf	Buffer for receiving chars
len	Maxium number of chars to receive
OUTPUT:
-1	Error occured while invoking remotly
0	EOF received
>0	Actual number of chars received
NOTE:
********************************************************/
static int remote_recv(unsigned char handle, char* buf, int len)
{
    int result;

    if (invoke_remote(FUNC_RECV, handle, buf, len, &result, 4) == JAVACALL_OK)
        return result;	
    else
        return JAVACALL_FAIL;
}

/********************************************************
FUNC: 	void remote_resync()
DESC:  	Tell server that client has restarted, the caches should be discarded
INPUT: 
OUTPUT:	
NOTE:	If client restart, PID could be same as the packet cached by Proxy, so
we have to send a RESYNC packet to Proxy, to let it discard cache
********************************************************/
static void remote_resync()
{
    invoke_remote(FUNC_RESYNC, PROTOCOL_VERSION, NULL, 0, NULL, 0);
}

/********************************************************
FUNC: 	int remote_send(char* buf, int len)
DESC:  	Send len chars from buf
INPUT: 
buf	Buffer that contain the datas to send
len	Nunmber of datas to send
OUTPUT:
-1	Fail
>=0 	Actual number of datas that have been sent out
NOTE:	remote_send will block until the result come back from Proxy
********************************************************/
static int remote_send(unsigned char handle, char* buf, int len)
{
    int result = -1;

    if (invoke_remote(FUNC_SEND, handle, buf, len, &result, 4) == JAVACALL_OK) {
        if (result == 0)
            return len;
        else 
            return -1;
    } else {
        return -1;
    }
}

/********************************************************
FUNC: 	void remote_shutdown(unsigned char flag)
DESC:  	Shutdown the connection
INPUT: 
flag	Combined value of:
1	for shutdown input
2	for shutdown output
OUTPUT:
NOTE:
********************************************************/
int remote_shutdown(unsigned char handle, unsigned char flag)
{
    return invoke_remote(FUNC_SHUTDOWN, handle, &flag, 1, NULL, 0);
}

static void send_buf(char* buf, int len)
{
    while (len--) 
        send_char((unsigned char)*buf++);
}

static void send_char(unsigned char ch)
{

    if ((ch == SYNC_BYTE) || (ch == ESCAPE_CHAR)) {
        ch = ch ^ 0x80;
        send_ESCAPE();	
    }

    while (javacall_serial_write(&ch,1) != 1);
    //FlushFileBuffers((HANDLE)hPort);
}

static void send_ESCAPE()
{
    unsigned char ch = ESCAPE_CHAR;
    while (javacall_serial_write(&ch,1) != 1);
}

static void send_SYNCBYTE()
{
    unsigned char ch = SYNC_BYTE;

    while (javacall_serial_write(&ch,1) != 1);
}

/********************************************************
FUNC: int send_packet(int func, unsigned char cid, unsigned char pid, char* content, int clen)
DESC: Create a packet and send it out through comm port
INPUT:
func	Which function to send
cid	CID of the packet
pid	PID of the packet
content	Content of the packet
clen	Length of content
OUTPUT:
<0	Fail
0	Success
NOTE:
********************************************************/
static int send_packet(int func, unsigned char cid, unsigned char pid, unsigned char handle, char* content, int clen)
{
    unsigned char buf[MTU + 6]; //6 is guard size
    SOS_HEADER header;
    int plen = 0;
    unsigned short crc;
    //char buff[256] = {0};
    //sprintf(buff,"send_packet() >> func=%d ,cid=%d , pid=%d\n",func,cid, pid);
    //REPORT(buff);
    if (clen > MTU - HEADER_LENGTH - CRC_LENGTH) {
        REPORT("Packet too large to send\n");
        return JAVACALL_INVALID_ARGUMENT;
    }

    plen = clen + HEADER_LENGTH + CRC_LENGTH;
    header.lenl = plen & 0xff;
    header.lenh = (plen >> 8) & 0xff;
    header.type = (unsigned char)(func & 0xff);
    header.xid = ((pid & 0x07) << 4) | (cid & 0x0f);
    header.handle = handle;

    memcpy(buf, &header, HEADER_LENGTH);
    memcpy(buf + HEADER_LENGTH, content, clen);

    crc = calc_crc(0, buf, HEADER_LENGTH + clen);

    send_SYNCBYTE();
    send_buf((char *)buf, HEADER_LENGTH + clen);
    send_char((unsigned char)(crc & 0xff));
    send_char((unsigned char)((crc >> 8) & 0xff));

    //REPORT("send_packet() <<\n");
    return JAVACALL_OK;
}

/********************************************************
FUNC: char* wait_packet()
DESC: Blocking wait for a incoming packet
INPUT:
OUTPUT: 
NULL	No valid packet received
others	Point to received packet
NOTE:
********************************************************/
static char* wait_packet()
{
    static unsigned char buf[MTU + 6]; //6 is guard size
    static unsigned char* p = buf;
    static int escape = 0;
    static int sync = 0;

    SOS_HEADER* header = (SOS_HEADER*)buf;

    //REPORT("wait_packet() >>\n");
    while (1) {
        unsigned char ch;
        unsigned short packet_len = 0;

        int res = recv_char();
        if (res < 0) {
            //REPORT("wait_packet() << NULL\n");	
            return NULL;
        }

        ch = (unsigned char)(res & 0xff);
        if (ch == SYNC_BYTE) {		
            sync = 1;
            p = buf;
            escape = 0;
            packet_len = header->lenl = header->lenh = 0;
            header->type = (unsigned char)-1;
            header->xid = (unsigned char)-1;
            continue;
        }

        //Now, it's not SYNC_BYTE
        if (!sync)
            continue;

        if (ch == ESCAPE_CHAR) {
            escape = 1;
            continue;
        }

        if (escape) {
            ch = ch ^ 0x80;
            escape = 0;
        }

        *p++ = ch;	//Fill in the buf
        //sprintf(b, "%d,",ch);
        //REPORT(b);
        if (p - buf < HEADER_LENGTH)
            continue;

        packet_len = header->lenl + header->lenh * 256;

        if (packet_len > MTU) {
            sync = 0;
            continue;
        }


        if (p - buf >= packet_len) {
            p = buf;
            if (0 != calc_crc(0, buf, packet_len)) {
                //CRC error
                REPORT("\r\nCRC error!:");
                /*
                // Print the packet
                int i;

                sprintf(b,"[len:%d] ",packet_len);
                REPORT(b);
                for(i=0;i<packet_len;i++) {
                    sprintf(b,"%d,",buf[i]);
                    REPORT(b);
                } */
                REPORT("\r\n");
                
                sync = 0;
                continue;
            } else {
                //sprintf(b,"\nwait_packet() << klass:%d\n", get_klass((char*)buf));
                /*sprintf(b,".%d.", get_klass((char*)buf));
                REPORT(b);			*/
                return (char *)buf;
            }
        }
    }
    //REPORT("wait_packet() <<\n");
}

/**************
CRC
***************/
static unsigned short crc_table[256];

static unsigned short int calc_crc(unsigned short crc, unsigned char* buf, int len)
{
    unsigned char *p, *lim;

    p = buf;
    lim = p + len;

    while (p < lim) {
        crc = (crc >> 8 ) ^ crc_table[(crc & 0xFF) ^ *p++];
    }

    return crc;
}

static void init_crc_table(void)
{
    int i, j;
    unsigned short k;

    for (i = 0; i < 256; i++)
    {
        k = 0xC0C1;
        for (j = 1; j < 256; j <<= 1)
        {
            if (i & j) 
                crc_table[i] ^= k;					

            k = (k << 1) ^ 0x4003;
        }		
    }	  
}


/**
* Parse an array of characters into an integer
*
* This function may need to be ported according to the 
* endianness of the host
*
* @param buffer : An array of characters
* @return Integer value of parsed characters
*
*/
static int parse_INT(char* p)
{
    int i;
    char* pi = (char*)&i;
    pi[0] = *p++;
    pi[1] = *p++;
    pi[2] = *p++;
    pi[3] = *p++;
    /*pi[3] = *p++;
    pi[2] = *p++;
    pi[1] = *p++;
    pi[0] = *p++;*/

    return i;
}

static void encode_INT(int i, char* p)
{
    char* pi = (char*)&i;

    *p++ = pi[0];
    *p++ = pi[1];
    *p++ = pi[2];
    *p++ = pi[3];

    /*
    *p++ = pi[3];
    *p++ = pi[2];
    *p++ = pi[1];
    *p++ = pi[0];
    */
}
