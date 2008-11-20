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

#include <stdio.h>
#include <string.h>

#include <pcsl_network.h>
#include <donuts.h>

#define PORT_ECHO      7
#define PORT_DISCARD   9
#define PORT_DAYTIME  13
#define PORT_CHARGEN  19
#define PORT_NEWS    144

static char msgbuf[100];

static char testhost[] = "host.domain";

static unsigned char testaddr[4] = { 10, 5, 32, 19 };
    /* address of host.domain */

static char testString[] =
	"When the comments and the code differ,\n"
	"both are probably wrong.\n"
	"-- Dennis Ritchie\n";

static int utilGetHostByName(char *, unsigned char *, int, int *);
static int utilOpen(void **, unsigned char *, unsigned int);
static int utilRead(void *, unsigned char *, int);
static int utilWrite(void *, unsigned char *, int);
static int utilClose(void *);


/**
 * Internal utility to lookup an address for a hostname, synchronously.
 *
 * @param hostname the hostname to look up
 * @param addr_return the location to which to write the address
 * @param len_return the address length
 *
 * @return 1 if successful, 0 if failed
 */
static int
utilGetHostByName(
    char *hostname,
    unsigned char *addr_return,
    int maxLen,
    int *len_return)
{
    int status;
    int len;
	void *handle;
    void *context;

    status = pcsl_network_gethostbyname_start(
	hostname, addr_return, maxLen, len_return, &handle, &context);

    if (status == PCSL_NET_SUCCESS) {
	return 1;
    } else {
	return 0;
    }

    /* IMPL NOTE: need to handle asynchronous gethostbyname */
}


/**
 * Internal utility to open a socket, synchronously.
 *
 * @param handle_return receives the open handle, if successful
 * @param addr pointer to base of ip address
 * @param port the port number to open
 *
 * @return 1 if successful, 0 if failed
 */
static int
utilOpen(
    void **handle_return,
    unsigned char *addr,
    unsigned int port)
{
    int status;
    char *exception;
    void *handle;
    void *context;

    status = pcsl_socket_open_start(
	addr, port, &handle, &context);

    if (status == PCSL_NET_SUCCESS) {
	*handle_return = handle;
	return 1;
    }

    if (status == PCSL_NET_WOULDBLOCK) {
	if (wait_on_handle(handle, 0)) {
	    status = pcsl_socket_open_finish(handle, context);
	    if (status == PCSL_NET_SUCCESS) {
		*handle_return = handle;
		return 1;
	    }
	}
	utilClose(handle);
    }

    return 0;
}


/**
 * Internal utility to read from a socket, synchronously.
 *
 * @param handle the open socket handle
 * @param buf the buffer into which to read
 * @param len the number of bytes to attempt to read
 * 
 * @return -1 if error, otherwise the number of bytes actually read
 */
static int
utilRead(
    void *handle,
    unsigned char *buf,
    int len)
{
    int status;
    int nread;
    void *context;

    status = pcsl_socket_read_start(
	handle, buf, len, &nread, &context);

    if (status == PCSL_NET_SUCCESS) {
	return nread;
    }

    if (status == PCSL_NET_WOULDBLOCK) {
	if (wait_on_handle(handle, 1)) {
	    status =
		pcsl_socket_read_finish(handle, buf, len, &nread, context);
	    if (status == PCSL_NET_SUCCESS) {
		return nread;
	    }
	}
    }

    return -1;
}


/**
 * Internal utility to write to a socket, synchronously.
 *
 * @param handle the open socket handle
 * @param buf the buffer from which to write
 * @param len the number of bytes to attempt to write
 * 
 * @return -1 if error, otherwise the number of bytes actually written
 */
static int
utilWrite(
    void *handle,
    unsigned char *buf,
    int len)
{
    int status;
    int nwritten;
    void *context;

    status = pcsl_socket_write_start(
	handle, buf, len, &nwritten, &context);

    if (status == PCSL_NET_SUCCESS) {
	return nwritten;
    }

    if (status == PCSL_NET_WOULDBLOCK) {
	if (wait_on_handle(handle, 0)) {
	    status =
		pcsl_socket_write_finish(handle, buf, len, &nwritten, context);
	    if (status == PCSL_NET_SUCCESS) {
		return nwritten;
	    }
	}
    }

    return -1;
}


/**
 * Internal utility to close a socket synchronously.
 *
 * @param handle the socket to close
 * 
 * @return 1 if successful, 0 if failure
 */
static int
utilClose(void *handle)
{
    int status;
    void *context;

    status = pcsl_socket_close_start(handle, &context);

    if (status == PCSL_NET_SUCCESS) {
	return 1;
    }

    if (status == PCSL_NET_WOULDBLOCK) {
	if (wait_on_handle(handle, 1)) {
	    status =
		pcsl_socket_close_finish(handle, context);
	    if (status == PCSL_NET_SUCCESS) {
		return 1;
	    }
	}
    }

    return 0;
}

    
/*
 * The tests.
 */


/**
 * Test of opening a socket.
 */
void
testSocketOpen() {
    int status;
    char *exception;
    void *handle;
    void *context;

    status = pcsl_socket_open_start(
	testaddr, PORT_DISCARD, &handle, &context);

    switch (status) {
	case PCSL_NET_SUCCESS:
	    assertTrue("open successful", 1);
	    break;
	case PCSL_NET_WOULDBLOCK:
	    if (wait_on_handle(handle, 0)) {
		status = pcsl_socket_open_finish(handle, context);
		if (status == PCSL_NET_SUCCESS) {
		    assertTrue("open successful after waiting", 1);
		} else {
		    sprintf(msgbuf, "open_finish => %d\n", status);
		    assertTrue(msgbuf, 0);
		}
	    } else {
		assertTrue("wait failed", 0);
	    }
	    break;
	default:
	    sprintf(msgbuf, "open_start => %d\n", status);
	    assertTrue(msgbuf, 0);
	    break;
    }

    (void)utilClose(handle);
}


/**
 * Test opening a socket to a port on which no one should be listening
 * (NeWS).
 */
void
testSocketOpenFail() {
    int status;
    void *handle;

    status = utilOpen(&handle, testaddr, PORT_NEWS);
    assertTrue("open succeeded unexpectedly", status == 0);
}


/**
 * Test of reading from a socket.
 */
void
testSocketRead() {
    int status;
    void *handle;
    void *context;
    char buf[100];
    int nread;

    status = utilOpen(&handle, testaddr, PORT_DAYTIME);
    assertTrue("open failed", status);

    while (1) {
	status = utilRead(handle, buf, sizeof(buf)-1);
	assertTrue("read failed", status != -1);
	if (status == 0) {
	    break;
	}
	buf[status] = '\0';
	printf("read => '%s'\n", buf);
    }

    puts("read => EOF");
    (void)utilClose(handle);
}



/**
 * Test of writing to a socket.
 */
void
testSocketWrite() {
    int status;
    void *handle;
    void *context;
    int ntowrite;
    int nwritten;
    char *buf = testString;

    status = utilOpen(&handle, testaddr, PORT_DISCARD);
    assertTrue("open failed", status);

    ntowrite = sizeof(*buf);

    do {
	status = utilWrite(handle, buf, ntowrite);
	assertTrue("write failed", status > 0);
	ntowrite -= status;
	buf += status;
    } while (ntowrite > 0);

    (void)utilClose(handle);
}


/**
 * Test writing to and reading from the same socket.
 */
void
testSocketReadWrite() {
    int status;
    void *handle;
    void *context;
    int ntowrite;
    int nwritten;
    char *buf = testString;
    char inbuf[100];

    status = utilOpen(&handle, testaddr, PORT_ECHO);
    assertTrue("open failed", status);

    ntowrite = strlen(buf);

    do {
	status = utilWrite(handle, buf, ntowrite);
	assertTrue("write failed", status > 0);
	ntowrite -= status;
	buf += status;
    } while (ntowrite > 0);

    status = pcsl_socket_shutdown_output(handle);
    assertTrue("shutdown failed", status == PCSL_NET_SUCCESS);

    while (1) {
	status = utilRead(handle, inbuf, sizeof(inbuf)-1);
	assertTrue("read failed", status != -1);
	if (status == 0) {
	    break;
	}
	inbuf[status] = '\0';
	printf("read => '%s'\n", inbuf);
    }

    (void)utilClose(handle);
}


/**
 * Test hostname lookup.
 */
void
testGetHostByName() {
    unsigned char addr[4];
    int len;
    int status;

    /* test of success */

    status = utilGetHostByName(testhost, addr, sizeof(addr), &len);
    assertTrue("gethostbyname failed", status);
    assertTrue("wrong length", len == 4);

    printf("address of %s is %d.%d.%d.%d\n",
	testhost, addr[0], addr[1], addr[2], addr[3]);

    /* failure - unknown host */

    status = utilGetHostByName(
	"foobar.xyzzy.example.com", addr, sizeof(addr), &len);
    assertTrue("gethostbyname didn't fail", status == 0);

    /* failure - buffer too short */

    status = utilGetHostByName(
	testhost, addr, sizeof(addr)-1, &len);
    assertTrue("gethostbyname didn't fail", status == 0);
}


/**
 * Test address to string conversion.
 */
void
testAddrToString() {
    unsigned char addr[4];
    unsigned short* result;
    int resultLen;
    int status;
    int i;

    /* test of success */
    addr[0] = 80;
    addr[1] = 80;
    addr[2] = 80;
    addr[3] = 80;

    status = pcsl_network_addrToString(addr, &result, &resultLen);
    assertTrue("gethostbyname failed", PCSL_NET_SUCCESS == status);
    if (PCSL_NET_SUCCESS != status) {
        return;
    }

    printf("Result of addr \"80.80.80.80\" = ");
    for (i = 0; i < resultLen, i++) {
        putchar(result[i]);
    }

    putchar('\n');

    pcsl_mem_free(result);
}

/**
 * Test retrieval of the number of bytes available.
 */
void
testAvailable()
{
    int status;
    void *handle;
    int avail;
    int ii;

    status = utilOpen(&handle, testaddr, PORT_CHARGEN);
    assertTrue("open failed", status);

    wait_on_handle(handle, 1);

    for (ii = 0; ii < 5; ii++) {
	status = pcsl_socket_available(handle, &avail);
	assertTrue("available failed", status == PCSL_NET_SUCCESS);
	printf("avail = %d\n", avail);
	usleep(1000);
    }

    (void)utilClose(handle);
}


/**
 * Main entry point for donuts.
 */
void
testSimpleNetwork_runTests()
{
    puts("===== testSocketOpen =====");
    testSocketOpen();
    puts("===== testSocketOpenFail =====");
    testSocketOpenFail();
    puts("===== testSocketRead =====");
    testSocketRead();
    puts("===== testSocketWrite =====");
    testSocketWrite();
    puts("===== testSocketReadWrite =====");
    testSocketReadWrite();
    puts("===== testGetHostByName =====");
    testGetHostByName();
    puts("===== testAddrToString =====");
    testAddrToString();
    puts("===== testAvailable =====");
    testAvailable();
    puts("===== testSimpleNetwork finished =====");
}


/*
 ***** BSD-specific stuff **********************************************
 */

#include <sys/types.h>

int
wait_on_handle(void *handle, int reading)
{
	int fd = (int)handle;
	fd_set rset;
	fd_set wset;
	fd_set eset;
	int r;

	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_ZERO(&eset);

	if (reading) {
		FD_SET(fd, &rset);
	} else {
		FD_SET(fd, &wset);
	}
	FD_SET(fd, &eset);
	
	r = select(fd+1, &rset, &wset, &eset, NULL);

/*
	printf("select=%d r=%d w=%d e=%d\n",
	    r, FD_ISSET(fd, &rset), FD_ISSET(fd, &wset), FD_ISSET(fd, &eset));
*/

	if (reading && FD_ISSET(fd, &rset)) {
		return 1;
	}

	if (!reading && FD_ISSET(fd, &wset)) {
		return 1;
	}

	if (FD_ISSET(fd, &eset)) {
		return 0;
	}

	printf("unexpected return from select!\n");
	return 0;
}
