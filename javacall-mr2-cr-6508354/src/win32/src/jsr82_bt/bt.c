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

/**
 * @file bt.c
 * @ingroup JSR82Bluetooth
 * @brief Javacall reference implementation (emulation over TCP/IP)
 */

#include "javacall_bt.h" 

#include <stdarg.h>
#include <stdio.h>

#include <winsock2.h>

static SOCKET g_sock;
static javacall_bt_address g_addr;
static DWORD idAcceptThread, idConnectThread, idReceiveThread, idSendThread;
static javacall_bool g_enabled = JAVACALL_FALSE;
static int g_inquiry_canceled = 0;

int get_last_error()
{
	return WSAGetLastError();
}

void print_last_error(const char *msg)
{
   static char buffer[512];
   FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, get_last_error(), 0, buffer, sizeof(buffer) - 1, NULL);
   printf("%s: %s\n", msg, buffer);
}

void wait_thread(SOCKET sock, int read, int write, javacall_bt_callback_type callback)
{
	int num;
	fd_set readfds, writefds;
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	if (read) {
		FD_SET(sock, &readfds);
	}
	if (write) {
		FD_SET(sock, &writefds);
	}
	num = select(0, &readfds, &writefds, NULL, NULL);
	if (num > 0) {
		javanotify_bt_protocol_event(callback, (javacall_handle)sock, JAVACALL_OK);
	}
	if (num < 0) {
		javanotify_bt_protocol_event(callback, (javacall_handle)sock, JAVACALL_FAIL);
		print_last_error("select");
	}
}

DWORD WINAPI AcceptThread(LPVOID lpParam)
{
	wait_thread((SOCKET)lpParam, 1, 0, JAVACALL_EVENT_BT_ACCEPT_COMPLETE);
	return 0;
}

DWORD WINAPI ConnectThread(LPVOID lpParam)
{
	wait_thread((SOCKET)lpParam, 0, 1, JAVACALL_EVENT_BT_CONNECT_COMPLETE);
	return 0;
}

DWORD WINAPI ReceiveThread(LPVOID lpParam)
{
	wait_thread((SOCKET)lpParam, 1, 0, JAVACALL_EVENT_BT_RECEIVE_COMPLETE);
	return 0;
}

DWORD WINAPI SendThread(LPVOID lpParam)
{
	wait_thread((SOCKET)lpParam, 0, 1, JAVACALL_EVENT_BT_SEND_COMPLETE);
	return 0;
}

static void send_command(const char *cmd, ...)
{
	static char buffer[256];
	va_list args;
	va_start(args, cmd);
	vsprintf(buffer, cmd, args);
	send(g_sock, buffer, strlen(buffer) + 1, 0);
	va_end(args);
}

static void read_data(void *data, int len)
{
	if (recv(g_sock, data, len, 0) == SOCKET_ERROR) {
		print_last_error("recv");
	}
}

static char *read_string(char *buf)
{
	unsigned char length;
	static char buffer[256];
	if (buf == NULL) {
		buf = buffer;
	}
	read_data(&length, 1);
	read_data(buf, length);
	*(buf + length) = '\0';
	return buf;
}

static void read_addr(javacall_bt_address addr)
{
	int a0, a1, a2, a3, a4, a5;
	sscanf(read_string(NULL), "%02X:%02X:%02X:%02X:%02X:%02X", &a5, &a4, &a3, &a2, &a1, &a0);
	addr[0] = a0;
	addr[1] = a1;
	addr[2] = a2;
	addr[3] = a3;
	addr[4] = a4;
	addr[5] = a5;
}

static const char *get_addr_string(const javacall_bt_address addr)
{
	static char buffer[18];
	sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
	return buffer;
}

javacall_result javacall_bt_bcc_initialize(void)
{
	return JAVACALL_OK;
}

javacall_result javacall_bt_bcc_finalize(void)
{
	return JAVACALL_OK;
}

javacall_result javacall_bt_bcc_is_connectable(/*OUT*/javacall_bool *pBool)
{
	*pBool = JAVACALL_TRUE;
	return JAVACALL_OK;
}

javacall_result javacall_bt_bcc_is_connected(
		const javacall_bt_address addr, 
		int *pBool)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_bcc_is_paired(
        const javacall_bt_address addr, 
        /*OUT*/javacall_bool *pBool)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_bcc_is_authenticated(
        const javacall_bt_address addr, 
        /*OUT*/javacall_bool *pBool)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_bcc_is_trusted(
        const javacall_bt_address addr, 
        /*OUT*/javacall_bool *pBool)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_bcc_is_encrypted(
        const javacall_bt_address addr, 
        /*OUT*/javacall_bool *pBool)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_bcc_bond(
        const javacall_bt_address addr, 
        const char *pin,
		javacall_bool *pBool)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_bcc_get_preknown_devices(
        /*OUT*/javacall_bt_address devices[JAVACALL_BT_MAX_PREKNOWN_DEVICES],
        /*OUT*/int *pCount)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_bcc_set_encryption(
		const javacall_bt_address addr, 
		javacall_bool enable,
        javacall_bool *pBool)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_stack_initialize(void)
{
	const char *ip;
	struct sockaddr_in dest_addr;
	WSADATA data;
	WSAStartup(0x202, &data);
	g_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (g_sock == INVALID_SOCKET) {
		print_last_error("socket");
		return JAVACALL_FAIL;
	}
	dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(5992);
    ip = getenv("BTSERV");
	dest_addr.sin_addr.s_addr = inet_addr(ip != NULL ? ip : "127.0.0.1");
    memset(&(dest_addr.sin_zero), '\0', 8);  // zero the rest of the struct
	if (connect(g_sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr))) {
		print_last_error("connect");
		return JAVACALL_FAIL;
	}
	send_command("register");
	read_addr(g_addr);
	return JAVACALL_OK;
}

javacall_result javacall_bt_stack_finalize(void)
{
	send_command("unregister");
	shutdown(g_sock, 2);
	WSACleanup();
	return JAVACALL_OK;
}

javacall_result javacall_bt_stack_is_enabled(
        /*OUT*/javacall_bool *pBool)
{
	*pBool = g_enabled;
	return JAVACALL_OK;
}

javacall_result javacall_bt_stack_enable(void)
{
	g_enabled = JAVACALL_TRUE;
	return JAVACALL_OK;
}

javacall_result javacall_bt_stack_get_local_address(
        /*OUT*/javacall_bt_address *pAddr)
{
	memcpy(pAddr, &g_addr, sizeof(g_addr));
	return JAVACALL_OK;
}

javacall_result javacall_bt_stack_get_local_name(char *pName)
{
	send_command("name");
	read_string(pName);
	return JAVACALL_OK;
}

javacall_result javacall_bt_stack_set_service_classes(int classes)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_stack_get_device_class(
        /*OUT*/int *pValue)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_stack_get_access_code(
        /*OUT*/int *pValue)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_stack_set_access_code(int accessCode)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_stack_start_inquiry(int accessCode)
{
	unsigned char count, i;
	javacall_bt_address addr;
	send_command("inquiry");
	read_data(&count, 1);
	g_inquiry_canceled = 0;
	for (i = 0; i < count; i++) {
		read_addr(addr);
		javanotify_bt_device_discovered(addr, 0);
		if (g_inquiry_canceled) {
			break;
		}
	}
	javanotify_bt_inquiry_complete(g_inquiry_canceled ? JAVACALL_FALSE : JAVACALL_TRUE);
	return JAVACALL_OK;
}

javacall_result javacall_bt_stack_cancel_inquiry(void)
{
	g_inquiry_canceled = 1;
	return JAVACALL_OK;
}

javacall_result javacall_bt_stack_ask_friendly_name(
        const javacall_bt_address addr)
{
	char buffer[JAVACALL_BT_MAX_USER_FRIENDLY_NAME + 1];
	send_command("name %02X:%02X:%02X:%02X:%02X:%02X", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
	read_string(buffer);
	javanotify_bt_remote_name_complete(addr, buffer);
	return JAVACALL_OK;
}

javacall_result javacall_bt_stack_authenticate(const javacall_bt_address addr)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_stack_encrypt(
        const javacall_bt_address addr,
        javacall_bool enable)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_stack_check_events(javacall_bool *pBool)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_stack_read_data(void *data, int len, int *pBytes)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_sddb_initialize(void)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_sddb_finalize(void)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_sddb_update_record(
        /*OUT*/unsigned long *pId,
        unsigned long classes,
        void *data,
        unsigned long size)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_sddb_update_psm(
        unsigned long id,
        unsigned short psm)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_sddb_update_channel(
        unsigned long id,
        unsigned char cn)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_bool javacall_bt_sddb_exists_record(unsigned long id)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_sddb_read_record(unsigned long id,
        /*OUT*/unsigned long *pClasses, void *data,
        /*OUT*/unsigned long *pSize)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_sddb_remove_record(unsigned long id)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

unsigned long javacall_bt_sddb_get_records(
        /*OUT*/unsigned long *array,
        unsigned long count)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

unsigned long javacall_bt_sddb_get_service_classes(unsigned long id)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_l2cap_close(javacall_handle handle)
{
	struct sockaddr_in iaddr;
	int len = sizeof(struct sockaddr);
	SOCKET sock = (SOCKET)handle;
	if (getsockname(sock, (struct sockaddr *)&iaddr, &len)) {
		print_last_error("getsockname");
		shutdown(sock, 2);
		return JAVACALL_FAIL;
	}
	send_command("close l2cap %d", ntohs(iaddr.sin_port));
	shutdown(sock, 2);
	return JAVACALL_OK;
}

/*OPTIONAL*/ javacall_result javacall_bt_l2cap_get_error(
        javacall_handle handle,
        /*OUT*/char **pErrStr)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_l2cap_create_server(
        int receiveMTU,
        int transmitMTU,
        javacall_bool authenticate,
        javacall_bool authorize,
        javacall_bool encrypt,
        javacall_bool master,
        /*OUT*/javacall_handle *pHandle,
        /*OUT*/int *pPsm)
{
	struct sockaddr_in iaddr;
	int len = sizeof(struct sockaddr);
	SOCKET sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		print_last_error("socket");
		return JAVACALL_FAIL;
	}
	iaddr.sin_family = AF_INET;
    iaddr.sin_port = 0;
    iaddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sock, (struct sockaddr *)&iaddr, sizeof(struct sockaddr))) {
		print_last_error("bind");
		shutdown(sock, 2);
		return JAVACALL_FAIL;
	}
	if (getsockname(sock, (struct sockaddr *)&iaddr, &len)) {
		print_last_error("getsockname");
		shutdown(sock, 2);
		return JAVACALL_FAIL;
	}
	send_command("server l2cap %d", ntohs(iaddr.sin_port));
	sscanf(read_string(NULL), "%d", pPsm);
	*pHandle = (javacall_handle)sock;
	return JAVACALL_OK;
}

javacall_result javacall_bt_l2cap_listen(javacall_handle handle)
{
	unsigned long arg = 1;
	SOCKET sock = (SOCKET)handle;
	if (listen(sock, 5)) {
		print_last_error("listen");
		return JAVACALL_FAIL;
	}
	if (ioctlsocket(sock, FIONBIO, &arg)) {
		print_last_error("ioctlsocket");
		shutdown(sock, 2);
		return JAVACALL_FAIL;
	}
	return JAVACALL_OK;
}

javacall_result javacall_bt_l2cap_accept(
        javacall_handle handle, 
        /*OUT*/javacall_handle *pPeerHandle,
        /*OUT*/javacall_bt_address *pPeerAddr,
        /*OUT*/int *pReceiveMTU,
        /*OUT*/int *pTransmitMTU)
{
	struct sockaddr_in iaddr;
	int len = sizeof(iaddr);
	unsigned long arg = 1;
	SOCKET ssock = (SOCKET)handle;
	SOCKET csock = accept(ssock, (struct sockaddr *)&iaddr, &len);
	if (csock != INVALID_SOCKET) {
		if (ioctlsocket(csock, FIONBIO, &arg)) {
			print_last_error("ioctlsocket");
			shutdown(csock, 2);
			return JAVACALL_FAIL;
		}
		*pPeerHandle = (javacall_handle)csock;
		send_command("accept l2cap %s %d", inet_ntoa(iaddr.sin_addr), ntohs(iaddr.sin_port));
		read_addr(*pPeerAddr);
		return JAVACALL_OK;
	}
	if (get_last_error() == WSAEWOULDBLOCK) {
		CreateThread(NULL, 0, AcceptThread, (void *)ssock, 0, &idAcceptThread);
		return JAVACALL_WOULD_BLOCK;
	}
	print_last_error("accept");
	return JAVACALL_FAIL;
}

javacall_result javacall_bt_l2cap_create_client(
        int receiveMTU,
        int transmitMTU,
        javacall_bool authenticate,
        javacall_bool encrypt,
        javacall_bool master,
        /*OUT*/javacall_handle *pHandle)
{
	unsigned long arg = 1;
	struct sockaddr_in iaddr;
	int len = sizeof(struct sockaddr);
	SOCKET sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		print_last_error("socket");
		return JAVACALL_FAIL;
	}
	iaddr.sin_family = AF_INET;
    iaddr.sin_port = 0;
    iaddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sock, (struct sockaddr *)&iaddr, sizeof(struct sockaddr))) {
		print_last_error("bind");
		shutdown(sock, 2);
		return JAVACALL_FAIL;
	}
	if (getsockname(sock, (struct sockaddr *)&iaddr, &len)) {
		print_last_error("getsockname");
		shutdown(sock, 2);
		return JAVACALL_FAIL;
	}
	if (ioctlsocket(sock, FIONBIO, &arg)) {
		print_last_error("ioctlsocket");
		shutdown(sock, 2);
		return JAVACALL_FAIL;
	}
	send_command("client l2cap %d", ntohs(iaddr.sin_port));
	*pHandle = (javacall_handle)sock;
	return JAVACALL_OK;
}

javacall_result javacall_bt_l2cap_connect(
        javacall_handle handle,
        const javacall_bt_address addr,
        int psm,
        /*OUT*/int *pReceiveMTU,
        /*OUT*/int *pTransmitMTU)
{
	struct sockaddr_in iaddr;
	SOCKET sock = (SOCKET)handle;
	send_command("connect l2cap %s %d", get_addr_string(addr), psm);
	iaddr.sin_family = AF_INET;
    iaddr.sin_addr.s_addr = inet_addr(read_string(NULL));
    iaddr.sin_port = htons(atoi(read_string(NULL)));
	if (connect(sock, (struct sockaddr *)&iaddr, sizeof(iaddr)) == 0 || get_last_error() == WSAEISCONN) {
		*pReceiveMTU = 0;
		*pTransmitMTU = 0;
		return JAVACALL_OK;
	}
	if (get_last_error() == WSAEWOULDBLOCK) {
		CreateThread(NULL, 0, ConnectThread, (void *)sock, 0, &idConnectThread);
		return JAVACALL_WOULD_BLOCK;
	}
	print_last_error("connect");
	return JAVACALL_FAIL;
}

javacall_result javacall_bt_l2cap_send(
        javacall_handle handle,
        const char *pData,
        int len,
        /*OUT*/int *pBytesSent)
{
	SOCKET sock = (SOCKET)handle;
	int retval = send(sock, pData, len, 0);
	if (retval != SOCKET_ERROR) {
		*pBytesSent = retval;
		return JAVACALL_OK;
	}
	if (get_last_error() == WSAEWOULDBLOCK) {
		CreateThread(NULL, 0, SendThread, (void *)sock, 0, &idSendThread);
		return JAVACALL_WOULD_BLOCK;
	}
	return JAVACALL_FAIL;
}

javacall_result javacall_bt_l2cap_receive(
        javacall_handle handle,
        /*OUT*/char *pData,
        int len,
        /*OUT*/int *pBytesReceived)
{
	SOCKET sock = (SOCKET)handle;
	int retval = recv(sock, pData, len, 0);
	if (retval != SOCKET_ERROR) {
		*pBytesReceived = retval;
		return JAVACALL_OK;
	}
	if (get_last_error() == WSAEWOULDBLOCK) {
		CreateThread(NULL, 0, ReceiveThread, (void *)sock, 0, &idReceiveThread);
		return JAVACALL_WOULD_BLOCK;
	}
	return JAVACALL_FAIL;
}

javacall_result javacall_bt_l2cap_get_ready(
        javacall_handle handle,
        /*OUT*/javacall_bool *pReady)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_rfcomm_close(javacall_handle handle)
{
	struct sockaddr_in iaddr;
	int len = sizeof(struct sockaddr);
	SOCKET sock = (SOCKET)handle;
	if (getsockname(sock, (struct sockaddr *)&iaddr, &len)) {
		print_last_error("getsockname");
		shutdown(sock, 2);
		return JAVACALL_FAIL;
	}
	send_command("close rfcomm %d", ntohs(iaddr.sin_port));
	shutdown(sock, 2);
	return JAVACALL_OK;
}

javacall_result javacall_bt_rfcomm_get_error(javacall_handle handle,
    /*OUT*/char **pErrStr)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_bt_rfcomm_create_server(
        javacall_bool authenticate,
        javacall_bool authorize,
        javacall_bool encrypt,
        javacall_bool master,
        /*OUT*/javacall_handle *pHandle,
        /*OUT*/int *pCn)
{
	struct sockaddr_in iaddr;
	int len = sizeof(struct sockaddr);
	SOCKET sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		print_last_error("socket");
		return JAVACALL_FAIL;
	}
	iaddr.sin_family = AF_INET;
    iaddr.sin_port = 0;
    iaddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sock, (struct sockaddr *)&iaddr, sizeof(struct sockaddr))) {
		print_last_error("bind");
		shutdown(sock, 2);
		return JAVACALL_FAIL;
	}
	if (getsockname(sock, (struct sockaddr *)&iaddr, &len)) {
		print_last_error("getsockname");
		shutdown(sock, 2);
		return JAVACALL_FAIL;
	}
	send_command("server rfcomm %d", ntohs(iaddr.sin_port));
	sscanf(read_string(NULL), "%d", pCn);
	*pHandle = (javacall_handle)sock;
	return JAVACALL_OK;
}

javacall_result javacall_bt_rfcomm_listen(javacall_handle handle)
{
	unsigned long arg = 1;
	SOCKET sock = (SOCKET)handle;
	if (listen(sock, 5)) {
		print_last_error("listen");
		return JAVACALL_FAIL;
	}
	if (ioctlsocket(sock, FIONBIO, &arg)) {
		print_last_error("ioctlsocket");
		shutdown(sock, 2);
		return JAVACALL_FAIL;
	}
	return JAVACALL_OK;
}

javacall_result javacall_bt_rfcomm_accept(
        javacall_handle handle, 
        /*OUT*/javacall_handle *pPeerHandle,
        /*OUT*/javacall_bt_address *pPeerAddr)
{
	struct sockaddr_in iaddr;
	int len = sizeof(iaddr);
	unsigned long arg = 1;
	SOCKET ssock = (SOCKET)handle;
	SOCKET csock = accept(ssock, (struct sockaddr *)&iaddr, &len);
	if (csock != INVALID_SOCKET) {
		if (ioctlsocket(csock, FIONBIO, &arg)) {
			print_last_error("ioctlsocket");
			shutdown(csock, 2);
			return JAVACALL_FAIL;
		}
		*pPeerHandle = (javacall_handle)csock;
		send_command("accept rfcomm %s %d", inet_ntoa(iaddr.sin_addr), ntohs(iaddr.sin_port));
		read_addr(*pPeerAddr);
		return JAVACALL_OK;
	}
	if (get_last_error() == WSAEWOULDBLOCK) {
		CreateThread(NULL, 0, AcceptThread, (void *)ssock, 0, &idAcceptThread);
		return JAVACALL_WOULD_BLOCK;
	}
	print_last_error("accept");
	return JAVACALL_FAIL;
}

javacall_result javacall_bt_rfcomm_create_client(
        javacall_bool authenticate,
        javacall_bool encrypt,
        javacall_bool master,
        /*OUT*/javacall_handle *pHandle)
{
	unsigned long arg = 1;
	struct sockaddr_in iaddr;
	int len = sizeof(struct sockaddr);
	SOCKET sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		print_last_error("socket");
		return JAVACALL_FAIL;
	}
	iaddr.sin_family = AF_INET;
    iaddr.sin_port = 0;
    iaddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sock, (struct sockaddr *)&iaddr, sizeof(struct sockaddr))) {
		print_last_error("bind");
		shutdown(sock, 2);
		return JAVACALL_FAIL;
	}
	if (getsockname(sock, (struct sockaddr *)&iaddr, &len)) {
		print_last_error("getsockname");
		shutdown(sock, 2);
		return JAVACALL_FAIL;
	}
	if (ioctlsocket(sock, FIONBIO, &arg)) {
		print_last_error("ioctlsocket");
		shutdown(sock, 2);
		return JAVACALL_FAIL;
	}
	send_command("client rfcomm %d", ntohs(iaddr.sin_port));
	*pHandle = (javacall_handle)sock;
	return JAVACALL_OK;
}

javacall_result javacall_bt_rfcomm_connect(
        javacall_handle handle,
        const javacall_bt_address addr, 
        int cn)
{
	struct sockaddr_in iaddr;
	SOCKET sock = (SOCKET)handle;
	send_command("connect rfcomm %s %d", get_addr_string(addr), cn);
	iaddr.sin_family = AF_INET;
    iaddr.sin_addr.s_addr = inet_addr(read_string(NULL));
    iaddr.sin_port = htons(atoi(read_string(NULL)));
	if (connect(sock, (struct sockaddr *)&iaddr, sizeof(iaddr)) == 0 || get_last_error() == WSAEISCONN) {
		return JAVACALL_OK;
	}
	if (get_last_error() == WSAEWOULDBLOCK) {
		CreateThread(NULL, 0, ConnectThread, (void *)sock, 0, &idConnectThread);
		return JAVACALL_WOULD_BLOCK;
	}
	print_last_error("connect");
	return JAVACALL_FAIL;
}

javacall_result javacall_bt_rfcomm_send(
        javacall_handle handle,
        const char *pData, 
        int len, 
        /*OUT*/int *pBytesSent)
{
	SOCKET sock = (SOCKET)handle;
	int retval = send(sock, pData, len, 0);
	if (retval != SOCKET_ERROR) {
		*pBytesSent = retval;
		return JAVACALL_OK;
	}
	if (get_last_error() == WSAEWOULDBLOCK) {
		CreateThread(NULL, 0, SendThread, (void *)sock, 0, &idSendThread);
		return JAVACALL_WOULD_BLOCK;
	}
	return JAVACALL_FAIL;
}

javacall_result javacall_bt_rfcomm_receive(
        javacall_handle handle,
        char *pData, 
        int len, 
        /*OUT*/int *pBytesReceived)
{
	SOCKET sock = (SOCKET)handle;
	int retval = recv(sock, pData, len, 0);
	if (retval != SOCKET_ERROR) {
		*pBytesReceived = retval;
		return JAVACALL_OK;
	}
	if (get_last_error() == WSAEWOULDBLOCK) {
		CreateThread(NULL, 0, ReceiveThread, (void *)sock, 0, &idReceiveThread);
		return JAVACALL_WOULD_BLOCK;
	}
	return JAVACALL_FAIL;
}

javacall_result javacall_bt_rfcomm_get_available(
        javacall_handle handle,
        /*OUT*/int *pCount)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

/*OPTIONAL*/ javacall_result javacall_bt_bcc_confirm_enable(
        /*OUT*/javacall_bool *pBool)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

/*OPTIONAL*/ javacall_result javacall_bt_bcc_put_passkey(
        const javacall_bt_address addr, 
        char *pin,
		javacall_bool ask)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

/*OPTIONAL*/ javacall_result javacall_bt_bcc_authorize(
            const javacall_bt_address addr, 
            javacall_handle record,
            javacall_bool *pBool)
{
	return JAVACALL_NOT_IMPLEMENTED;
}

/*OPTIONAL*/ javacall_result javacall_bt_stack_get_acl_handle(
		const javacall_bt_address addr, 
		int *pHandle)
{
	return JAVACALL_NOT_IMPLEMENTED;
}
