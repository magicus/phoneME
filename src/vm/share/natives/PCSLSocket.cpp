/*
 *   
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

#include "incls/_precompiled.incl"
#include "incls/_PCSLSocket.cpp.incl"

// Note the "&& 0" here. As of 2004/08/25 this code compiles with the
// PCSL network library on linux/i386. However, the only existing PCSL
// implementation requires QT, which is not used by the vanilla
// VM, so it will fail in the linking stage.  As a result, I
// have temporarily disabled PCSLSocket.cpp.
//
// If you wish to link a stand-alone VM with the PCSL library
// on your platform, please remove the "&& 0" in this file and the "||
// 1" on top of BSDSocket.cpp.
//
// Usually the PCSL network library is used in the MIDP layer. The
// only reason for you to compile on this file is to build a
// stand-alone VM, without MIDP, in order to execute the CLDC-TCK.

#if ENABLE_PCSL && 0
/*
 * *** WARNING WARNING WARNING ***
 *
 * Socket implementation on top of PCSL -- Portable Common Services
 * Library.  It is intended only for running CLDC-TCK in "standalone
 * VM" mode. Here we implement just as much socket functionality as
 * possible to communicate with the CLDC-TCK test harness. For
 * simplicity, we use a polling mechanism to implement all the socket
 * methods in a blocking style.
 *
 * This socket implementation exists strictly for testing purpose. It
 * must NOT be used in a production MIDP environment.
 */

extern "C" {
#include <pcsl_network.h>

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_io_j2me_socket_Protocol_open0() {
  static int inited = 0;
  if (!inited) {
    pcsl_network_init();
    inited = 1;
  }

  int fd = -1;

  // (1) Get the hostname, and convert it to IP address
  KNI_StartHandles(1);
  {
    KNI_DeclareHandle(hostname_object);
    KNI_GetParameterAsObject(1, hostname_object);
    // hostname is always NUL terminated. See socket/Protocol.java for detail.
    char *hostname = (char*) SNI_GetRawArrayPointer(hostname_object);
    int port = KNI_GetParameterAsInt(2);

    unsigned char addr[4];
    int len;
    int status;
    void *context;
    void *handle;

    status = pcsl_network_gethostbyname_start(
          hostname, addr, sizeof(addr), &len, &handle, &context);
    if (status == PCSL_NET_WOULDBLOCK) {
      do {
        status =  pcsl_network_gethostbyname_finish(addr,
                                                    sizeof(addr), &len, 
                                                    &handle, &context);
      } while (status == PCSL_NET_WOULDBLOCK);
    }

    if (status != PCSL_NET_SUCCESS) {
      goto done;
    }

    // (2) Make the connection
    status = pcsl_socket_open_start(
          addr, port, &handle, &context);
    if (status == PCSL_NET_WOULDBLOCK) {
      do {
        status = pcsl_socket_open_finish(handle, context);
      } while (status == PCSL_NET_WOULDBLOCK);
    }

    if (status != PCSL_NET_SUCCESS) {
      goto done;
    }

    fd = (int)handle;
  }
done:
  KNI_EndHandles();

  KNI_ReturnInt(fd);
}

static int do_pcsl_read(void *handle, char *buffer, int length) {
  void *context;
  int nread;
  int status = pcsl_socket_read_start(
	handle, (unsigned char*)buffer, length, &nread, &context);
  if (status == PCSL_NET_WOULDBLOCK) {
    do {
      status = pcsl_socket_read_finish(handle, (unsigned char*) buffer, length,
                                       &nread, context);
    } while (status == PCSL_NET_WOULDBLOCK);
  }

  if (status == PCSL_NET_SUCCESS && nread > 0) {
    return nread;
  } else {
    return -1;
  }
}

static int do_pcsl_write(void *handle, char *buffer, int length) {
  void *context;
  int nwrite;
  int status = pcsl_socket_write_start(
	handle, buffer, length, &nwrite, &context);
  if (status == PCSL_NET_WOULDBLOCK) {
    do {
      status = pcsl_socket_write_finish(handle, buffer, length,
                                       &nwrite, context);
    } while (status == PCSL_NET_WOULDBLOCK);
  }

  if (status == PCSL_NET_SUCCESS && nwrite > 0) {
    return nwrite;
  } else {
    return -1;
  }
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_io_j2me_socket_Protocol_readBuf() {
  void *handle = (void*)KNI_GetParameterAsInt(1);
  int offset = KNI_GetParameterAsInt(3);
  int length = KNI_GetParameterAsInt(4);
  int result;

  KNI_StartHandles(1);
  {
    KNI_DeclareHandle(buffer_object);
    KNI_GetParameterAsObject(2, buffer_object);
    char *buffer = (char *) SNI_GetRawArrayPointer(buffer_object) + offset;

    result = do_pcsl_read(handle, buffer, length);
  }
  KNI_EndHandles();
  KNI_ReturnInt(result);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_io_j2me_socket_Protocol_readByte() {
  void *handle = (void*)KNI_GetParameterAsInt(1);
  unsigned char c;
  if (do_pcsl_read(handle, (char*)&c, 1)) {
    KNI_ReturnInt((int)c); // Do not sign-extend: 0 - 255
  } else {
    KNI_ReturnInt(-1);
  }
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_io_j2me_socket_Protocol_writeBuf() {
  void *handle = (void*)KNI_GetParameterAsInt(1);
  int offset = KNI_GetParameterAsInt(3);
  int length = KNI_GetParameterAsInt(4);
  int result;

  KNI_StartHandles(1);
  {
    KNI_DeclareHandle(buffer_object);
    KNI_GetParameterAsObject(2, buffer_object);
    char *buffer = (char *) SNI_GetRawArrayPointer(buffer_object) + offset;

    result = do_pcsl_write(handle, buffer, length);
  }
  KNI_EndHandles();
  KNI_ReturnInt(result);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_io_j2me_socket_Protocol_writeByte() {
  void *handle = (void*)KNI_GetParameterAsInt(1);
  char byte = (char) KNI_GetParameterAsInt(2);

  return do_pcsl_write(handle, &byte, 1);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_cldc_io_j2me_socket_Protocol_available0() {
  // unsupported. Not used by CLDC TCK
  KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_cldc_io_j2me_socket_Protocol_close0() {
  void *handle = (void*)KNI_GetParameterAsInt(1);
  void *context;
  int status = pcsl_socket_close_start(handle, &context);
  if (status == PCSL_NET_WOULDBLOCK) {
    do {
      status = pcsl_socket_close_finish(handle, context);
    } while (status == PCSL_NET_WOULDBLOCK);
  }
}


/*
 * This function is called by the VM periodically. It has to check if
 * any of the blocked threads are ready for execution, and call
 * SNI_UnblockThread() on those threads that are ready.
 *
 * Values for the <timeout> paramater:
 *  >0 = Block until an event happens, or until <timeout> milliseconds
 *       has elapsed.
 *   0 = Check the events sources but do not block. Return to the
 *       caller immediately regardless of the status of the event sources.
 *  -1 = Do not timeout. Block until an event happens.
 */
void JVMSPI_CheckEvents(JVMSPI_BlockedThreadInfo *blocked_threads,
                        int blocked_threads_count,
                        jlong timeout) {
  GUARANTEE(blocked_threads_count == 0, 
            "Vanilla VM with PCSL socket never calls SNI_BlockThread");
  if (timeout > 0) {
    Os::sleep(timeout);
  }
}

} // extern "C"
#endif //ENABLE_PCSL
