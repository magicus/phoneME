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

#ifndef _JSR120_JUMPDRIVER_H
#define _JSR120_JUMPDRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#define STUB_NAME(function_)  D##function_

#define INVOKE_REMOTELY_VOID(function_, args_)  \
    STUB_NAME(function_) args_

#define INVOKE_REMOTELY(result_, function_, args_)  \
    (result_ = STUB_NAME(function_) args_)

#ifdef JSR120_KNI_LAYER
// cbs
#define jsr120_cbs_is_midlet_msgID_registered   Djsr120_cbs_is_midlet_msgID_registered
#define jsr120_cbs_register_midlet_msgID        Djsr120_cbs_register_midlet_msgID
#define jsr120_cbs_unregister_midlet_msgID      Djsr120_cbs_unregister_midlet_msgID
#define jsr120_cbs_unblock_thread               Djsr120_cbs_unblock_thread
#define jsr120_cbs_delete_midlet_suite_msg      Djsr120_cbs_delete_midlet_suite_msg
#define jsr120_cbs_pool_peek_next_msg           Djsr120_cbs_pool_peek_next_msg
#define jsr120_cbs_pool_retrieve_next_msg       Djsr120_cbs_pool_retrieve_next_msg
#define jsr120_cbs_pool_peek_next_msg1          Djsr120_cbs_pool_peek_next_msg1
#define jsr120_cbs_delete_msg                   Djsr120_cbs_delete_msg
#define jsr120_cbs_pool_add_msg                 Djsr120_cbs_pool_add_msg

// sms
#define jsr120_is_sms_midlet_port_registered    Djsr120_is_sms_midlet_port_registered
#define jsr120_register_sms_midlet_port         Djsr120_register_sms_midlet_port
#define jsr120_unregister_sms_midlet_port       Djsr120_unregister_sms_midlet_port
#define jsr120_sms_unblock_thread               Djsr120_sms_unblock_thread
#define jsr120_sms_delete_midlet_suite_msg      Djsr120_sms_delete_midlet_suite_msg
#define jsr120_send_sms                         Djsr120_send_sms
#define jsr120_number_of_sms_segments           Djsr120_number_of_sms_segments
#define jsr120_sms_pool_peek_next_msg           Djsr120_sms_pool_peek_next_msg
#define jsr120_sms_pool_retrieve_next_msg       Djsr120_sms_pool_retrieve_next_msg
#define jsr120_sms_pool_peek_next_msg1          Djsr120_sms_pool_peek_next_msg1
#define jsr120_sms_delete_msg                   Djsr120_sms_delete_msg
#define jsr120_sms_pool_add_msg                 Djsr120_sms_pool_add_msg
#endif

enum {
    // cbs
    ID_jsr120_cbs_is_midlet_msgID_registered = 0x100,
    ID_jsr120_cbs_register_midlet_msgID,
    ID_jsr120_cbs_unregister_midlet_msgID,
    ID_jsr120_cbs_unblock_thread,
    ID_jsr120_cbs_delete_midlet_suite_msg,
    ID_jsr120_cbs_pool_peek_next_msg,
    ID_jsr120_cbs_pool_retrieve_next_msg,
    ID_jsr120_cbs_pool_peek_next_msg1,
    ID_jsr120_cbs_delete_msg,
    ID_jsr120_cbs_pool_add_msg,

    ID_KILL_SHMEM = 0x200,

    // sms
    ID_jsr120_is_sms_midlet_port_registered = 0x300,
    ID_jsr120_register_sms_midlet_port,
    ID_jsr120_unregister_sms_midlet_port,
    ID_jsr120_sms_unblock_thread,
    ID_jsr120_sms_delete_midlet_suite_msg,
    ID_jsr120_send_sms,
    ID_jsr120_number_of_sms_segments,
    ID_jsr120_sms_pool_peek_next_msg,
    ID_jsr120_sms_pool_retrieve_next_msg,
    ID_jsr120_sms_pool_peek_next_msg1,
    ID_jsr120_sms_delete_msg,
    ID_jsr120_sms_pool_add_msg,
    
    ID_javanotify_incoming_sms = 0x400,
    ID_javanotify_incoming_cbs
};

#ifdef __cplusplus
}
#endif

#endif /* #ifdef _JSR120_JUMPDRIVER_H */
