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

/**
 * @file
 * @defgroup chapi JSR 211 Content Handler API (CHAPI)
 * @ingroup stack
 * @brief This is the API definition for exchange of the invocation data
 * between JVM and platform applications.
 * 
 * @{
 */

#include "javacall_chapi_invoke.h"

#ifndef _JSR211_PLATFORM_INVOC_H_
#define _JSR211_PLATFORM_INVOC_H_

#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/

typedef struct {
    javacall_utf16_string handler_id;
    javacall_chapi_invocation_status status;
    javacall_chapi_invocation invocation;
} jsr211_platform_event;

void
jsr211_process_platform_finish_notification (int invoc_id, 
                                             jsr211_platform_event *event);

void 
jsr211_process_java_invoke_notification (int invoc_id,
                                         jsr211_platform_event *event);

#ifdef __cplusplus
}
#endif/*__cplusplus*/

/** @} */

#endif  /* _JSR211_PLATFORM_INVOC_H_ */
