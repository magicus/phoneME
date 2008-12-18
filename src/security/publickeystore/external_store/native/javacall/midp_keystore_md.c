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

#ifdef __cplusplus
extern "C" {
#endif

#include "midp_pks_security.h"
#include "javacall_security.h"

int midp_security_keystore_start_md(/*OUT*/ int* handle) {
    javacall_result res;
    res = javacall_security_keystore_start((javacall_handle)handle);
    if(res != JAVACALL_OK) {
        return -1;
    }
    return 1;
}

int midp_security_keystore_has_next_md(int keyStoreHandle) {
    javacall_result res;
    res = javacall_security_keystore_has_next((javacall_handle)keyStoreHandle);
    if(res != JAVACALL_OK) {
        return 0;
    }
    return 1;
}

int midp_security_keystore_get_next_md(int   keyStoreHandle,
                                       unsigned short**  owner,
                                       int*              ownerSize,
                                       jlong*      validityStartMillissec,
                                       jlong*      validityEndMillisec,
                                       unsigned char**   modulus,
                                       int*              modulusSize,
                                       unsigned char**   exponent,
                                       int*              exponentSize,
                                       char**            domain,
                                       int*              domainSize) {


    javacall_result res;

    res = javacall_security_keystore_get_next((javacall_handle) keyStoreHandle,
                                              owner,
                                              ownerSize,
                                              (javacall_int64*)validityStartMillissec,
                                              (javacall_int64*)validityEndMillisec,
                                              modulus,
                                              modulusSize,
                                              exponent,   
                                              exponentSize,
                                              domain,     
                                              domainSize);
    if(res != JAVACALL_OK) {
        return -1;
    }
    return 1;
}

int midp_security_keystore_end_md(int keyStoreHandle) {
    javacall_result res;
    res = javacall_security_keystore_end((javacall_handle)keyStoreHandle);
    if(res != JAVACALL_OK) {
        return -1;
    }
    return 1;
}


#ifdef __cplusplus
}
#endif


