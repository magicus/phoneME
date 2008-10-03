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

/**
 * @file
 *
 * Stub Implementation for pcsl_network.h 
 *
 */

#include <stdio.h>

#include <pcsl_network.h>


/**
 * See pcsl_network.h for definition.
 */
int
pcsl_network_init(void) {
    javacall_result res;

	res = pcsl_network_init_start(NULL);

	return res;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_init_start(PCSL_NET_CALLBACK pcsl_network_callback) {
    /*
     * return SUCCESS, so that calling application can initialize successfully
     *
     */
    (void)pcsl_network_callback;
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
int pcsl_network_finalize_finish(void){
    return PCSL_NET_SUCCESS;
}

/**
 * See pcsl_network.h for definition.
 * 
 * Note that this function NEVER returns PCSL_NET_WOULDBLOCK. Therefore, the 
 * finish() function should never be called and does nothing.
 */
int pcsl_network_gethostbyname_start(
	char *hostname,
	unsigned char *pAddress, 
	int maxLen,
	int *pLen,
        void **pHandle,
	void **pContext)
{
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 *
 * Since the start function never returns PCSL_NET_WOULDBLOCK, this
 * function should never be called.
 */
int pcsl_network_gethostbyname_finish(
	unsigned char *pAddress,
	int maxLen,
	int *pLen,
        void *handle,
	void *context)
{
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_error(
	void *handle)
{
    return 0;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_getLocalHostName(
    char *pLocalHost)
{
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_getLocalIPAddressAsString(
	char *pLocalIPAddress)
{
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_getHostByAddr_start(
    int ipn,
    char *hostname,
    void **pHandle,
    void **pContext)
{
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_getHostByAddr_finish(
    int ipn,
    char *hostname,
    void **pHandle,
    void *context)
{
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_getlocalport(
	void *handle,
	int *pPortNumber)
{
    return PCSL_NET_IOERROR;
}
        

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_getremoteport(
	void *handle,
	int *pPortNumber)
{
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_getsockopt(
	void *handle,
	int flag,
	int *pOptval)
{
    return PCSL_NET_IOERROR;
}


/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_setsockopt(
	void *handle,
	int flag,
	int optval)
{
    return PCSL_NET_INVALID;
}

/**
 * See pcsl_network.h for definition.
 */
int pcsl_network_addrToString(unsigned char *ipBytes,
        unsigned short** pResult, int* pResultLen) {
    return PCSL_NET_IOERROR;
}

/**
 * See pcsl_network.h for definition.
 */
void pcsl_add_network_notifier(
    void *handle,
    int event) {
}

/**
 * See pcsl_network.h for definition.
 */
void pcsl_remove_network_notifier(
    void *handle,
    int event) {
}

/**
 * See pcsl_network.h for definition.
 */
unsigned int pcsl_network_htonl(
    unsigned int value) {
    return value;
}

/**
 * See pcsl_network.h for definition.
 */
unsigned int pcsl_network_ntohl(
    unsigned int value) {
    return value;
}
    
/**
 * See pcsl_network.h for definition.
 */
unsigned short pcsl_network_htons(
    unsigned short value) {
    return value;
}

/**
 * See pcsl_network.h for definition.
 */
unsigned short pcsl_network_ntohs(
    unsigned short value) {
    return value;
}

/**
 * See pcsl_network.h for definition.
 */
char * pcsl_inet_ntoa (void *ipBytes) {
    static char buffer[] = {'\0'};

    return buffer;
}
