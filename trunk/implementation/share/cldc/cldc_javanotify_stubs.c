/*
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

/**
 * @file
 *
 * CLDC stubs for javanotify_*.
 */


#include <javacall_datagram.h>
#include <javacall_socket.h>
#include <javacall_lifecycle.h>
#include <javacall_keypress.h>
#include <javacall_lcd.h>
#include <javacall_penevent.h>

#if ENABLE_NATIVE_AMS_UI
#include <javacall_ams_installer.h>
#endif

#if ENABLE_JSR_120
#include <javacall_sms.h>
#include <javacall_cbs.h>
#endif  /* ENABLE_JSR_120 */

#if ENABLE_JSR_205
#include <javacall_mms.h>
#endif  /* ENABLE_JSR_205 */

#if ENABLE_JSR_75
#include <javanotify_fileconnection.h>
#endif

#if ENABLE_JSR_179
#include <javanotify_location.h>
#endif

void javanotify_datagram_event(
                             javacall_datagram_callback_type type, 
                             javacall_handle handle,
                             javacall_result operation_result) {}

void javanotify_server_socket_event(
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

/**
 * The platform should invoke this function in platform context
 * to select another running application to be the foreground.
 */
void javanotify_select_foreground_app(void) {}

void javanotify_key_event(javacall_key key, javacall_keypress_type type) {}

void javanotify_rotation(int hardwareId) {}

void javanotify_display_device_state_changed(int hardwareId, javacall_lcd_display_device_state state) {}

void javanotify_clamshell_state_changed(javacall_lcd_clamshell_state state) {}

void javanotify_pen_event(int x, int y, javacall_penevent_type type) {}

void javanotify_start(void) {}

void javanotify_start_java_with_arbitrary_args(int argc, char* argv[]) {}


/* The 4 functions below are only called from implementation\win32\midp\main.c */ 

void javanotify_start_i3test(char* arg1, char* arg2) {}

void javanotify_start_handler(char* handlerID, char* url, char* action) {}

void javanotify_start_tck(char* url, javacall_lifecycle_tck_domain domain) {}

void javanotify_install_content(const char * httpUrl,
                                const javacall_utf16* descFilePath,
                                unsigned int descFilePathLen,
                                javacall_bool isJadFile,
                                javacall_bool isSilent) {}


#if ENABLE_NATIVE_AMS_UI

javacall_result
javanotify_ams_install_suite(javacall_app_id appId,
                             javacall_ams_install_source_type srcType,
                             javacall_const_utf16_string installUrl,
                             javacall_storage_id storageId,
                             javacall_folder_id folderId) {
    return JAVACALL_NOT_IMPLEMENTED;
}

#endif /* ENABLE_NATIVE_AMS_UI */


void JavaTask(void) {}


//extern "C" {
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
//} /* extern "C" */

#if ENABLE_JSR_120
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
#endif /* ENABLE_JSR_120 */

#if ENABLE_JSR_205
void javanotify_incoming_mms(
        char* fromAddress, char* appID, char* replyToAppID,
        int             bodyLen, 
        unsigned char*  body) {}
void javanotify_mms_send_completed(
                        javacall_result result, 
                        int handle) {}
#endif /* ENABLE_JSR_205 */

#if ENABLE_ON_DEVICE_DEBUG
/**
 * The platform calls this function to inform VM that
 * ODTAgent midlet must be enabled.
 */
void javanotify_enable_odd(void) {}
#endif

#if ENABLE_JSR_75
void javanotify_fileconnection_root_changed() {}
#endif

#if ENABLE_JSR_179
void javanotify_location_proximity(
        javacall_handle provider,
        double latitude,
        double longitude,
        float proximityRadius,
        javacall_location_location* pLocationInfo,
        javacall_location_result operation_result) {}

void javanotify_location_event(
        javacall_location_callback_type event,
        javacall_handle provider,
        javacall_location_result operation_result) {}
#endif
