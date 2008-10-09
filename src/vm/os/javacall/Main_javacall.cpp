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

#include "incls/_precompiled.incl"
#include "incls/_Main_javacall.cpp.incl"
#include <stdio.h>
#include <stdlib.h>

#if ENABLE_PCSL
extern "C" {
#include <pcsl_memory.h>
#include <pcsl_print.h>
}
#endif

#include "../utilities/JVM_Malloc.hpp"

#include <javacall_logging.h>


#include <javacall_datagram.h>
#include <javacall_socket.h>
#include <javacall_lifecycle.h>
#include <javacall_keypress.h>
#include <javacall_lcd.h>
#include <javacall_penevent.h>

//#if ENABLE_JSR_120
#include <javacall_sms.h>
#include <javacall_cbs.h>
//#endif

//#if ENABLE_JSR_205
#include <javacall_mms.h>
//#endif


void JVMSPI_PrintRaw(const char* s) {
  /* Print the string to the standard output device */
#if ENABLE_PCSL
  pcsl_print(s);
#else
  javacall_print(s);
#endif
}

void JVMSPI_Exit(int code) {
  /* Terminate the current process */
  return ;
}

int main(int argc, char **argv) {

  int   size = 0x00200000;
  int code;

#if ENABLE_PCSL
  pcsl_mem_initialize(NULL, -1);
#endif

  JVM_Initialize();

  argc --;
  argv ++;

  while (true) {
    int n = JVM_ParseOneArg(argc, argv);
    if (n < 0) {
      printf("Unknown argument: %s\n", argv[0]);
      JVMSPI_DisplayUsage(NULL);
      code = -1;
      goto end;
    } else if (n == 0) {
      break;
    }
    argc -= n;
    argv += n;
  }

  if (JVM_GetConfig(JVM_CONFIG_SLAVE_MODE) == KNI_FALSE) {
    // Run the VM in regular mode -- JVM_Start won't return until
    // the VM completes execution.
    code = JVM_Start(NULL, NULL, argc, argv);
  } else {
    // Run the VM in slave mode -- we keep calling JVM_TimeSlice(),
    // which executes bytecodes for a small amount and returns. This
    // mode is necessary for platforms that need to keep the main
    // control loop outside of of the VM.

    JVM_Start(NULL, NULL, argc, argv);

    for (;;) {
      jlong timeout = JVM_TimeSlice();
      if (timeout <= -2) {
        break;
      } else {
        int blocked_threads_count;
        JVMSPI_BlockedThreadInfo * blocked_threads;

        blocked_threads = SNI_GetBlockedThreads(&blocked_threads_count);
        JVMSPI_CheckEvents(blocked_threads, blocked_threads_count, timeout);
      }
    }

    code = JVM_CleanUp();
  }

end:
#if ENABLE_PCSL
  pcsl_mem_finalize();
#endif

  return code;
}

void javanotify_datagram_event(
                             javacall_datagram_callback_type type, 
                             javacall_handle handle,
                             javacall_result operation_result) {}

void /* OPTIONAL*/ javanotify_server_socket_event(
                             javacall_server_socket_callback_type type, 
                             javacall_handle socket_handle,
                             javacall_handle new_socket_handle,
                             javacall_result operation_result) {}

void javanotify_socket_event(
                             javacall_socket_callback_type type, 
                             javacall_handle socket_handle,
                             javacall_result operation_result) {}

void javanotify_shutdown(void) {}

void javanotify_change_locale(short languageCode, short regionCode) {}

void javanotify_resume(void) {}

void javanotify_pause(void) {}

void javanotify_switch_to_ams(void) {}

void javanotify_key_event(javacall_key key, javacall_keypress_type type) {}

void javanotify_rotation(void) {}

void javanotify_pen_event(int x, int y, javacall_penevent_type type) {}

void javanotify_start(void) {}

void javanotify_start_java_with_arbitrary_args(int argc, char* argv[]) {}


void JavaTask(void) {}


extern "C" {
void javanotify_list_storageNames(void) {}
void javanotify_remove_suite(char* suite_id) {}
void javanotify_transient(char* url) {}
void javanotify_list_midlets(void) {}
void javanotify_start_suite(char* suiteId) {}
void javanotify_install_midlet_wparams(const char* httpUrl,
                                       int silentInstall, int forceUpdate) {}
void javanotify_start_local(char* classname, char* descriptor,
                            char* classpath, javacall_bool debug) {}
void javanotify_set_vm_args(int argc, char* argv[]) {}
void javanotify_set_heap_size(int heapsize) {}
} /* extern "C" */

//#if ENABLE_JSR_120
void javanotify_incoming_sms(
        javacall_sms_encoding   msgType,
        char*                   sourceAddress,
        unsigned char*          msgBuffer,
        int                     msgBufferLen,
        unsigned short          sourcePortNum,
        unsigned short          destPortNum,
        javacall_int64          timeStamp
        ){}
void javanotify_incoming_cbs(
        javacall_cbs_encoding  msgType,
        unsigned short         msgID,
        unsigned char*         msgBuffer,
        int                    msgBufferLen) {}
void javanotify_sms_send_completed(
                        javacall_result result, 
                        int handle) {}
//#endif
//#if ENABLE_JSR_205
void javanotify_incoming_mms(
        char* fromAddress, char* appID, char* replyToAppID,
        int             bodyLen, 
        unsigned char*  body) {}
void javanotify_mms_send_completed(
                        javacall_result result, 
                        int handle) {}
//#endif

