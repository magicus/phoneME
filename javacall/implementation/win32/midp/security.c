/*
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
 * win32 implemenation for public keystore handling functions
 */

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include "javacall_security.h"
#include "javacall_file.h"
#include "javacall_dir.h"
#include "javacall_logging.h"

typedef enum {
    OWNER_TAG      = 0x1,
    NOT_BEFORE_TAG = 0x2,
    NOT_AFTER_TAG  = 0x3,
    MODULUS_TAG    = 0x4,
    EXPONENT_TAG   = 0x5,
    DOMAIN_TAG     = 0x6
} javacall_keystore_tag;

typedef enum {
    CURRENT_VERSION = 0x1,
    BINARY_TYPE = 0x1,
    STRING_TYPE = 0x2,
    LONG_TYPE = 0x3
} javacall_datatype;

typedef union {
    unsigned char size[8];
    javacall_int64 vdate;
} jc_expire_date;

typedef union {
    unsigned char size[2];
    unsigned short dsize;
} jc_date_size;

static unsigned short  getFieldSize(void);
static javacall_int64  javacall_read_int64_value(void);
static void            javacall_set_local_variables(void);
static void            javacall_reset_local_variables(void);
static javacall_result verify_mainks_fields(javacall_keystore_tag tag,
                                            javacall_datatype datatype);

static unsigned char  *_main_ks_content;
static int            main_ks_size;
static unsigned char  *currentPosition;

static unsigned short *local_owner;
static unsigned char  *local_modulus;
static unsigned char  *local_exponent;
static unsigned char  *local_domain;

javacall_result javacall_security_keystore_start(javacall_handle* jc_h) {

    javacall_utf16 rootPath[JAVACALL_MAX_FILE_NAME_LENGTH];

    unsigned short unicode_main_ks[] =
    {'\\','_','m','a','i','n','.','k','s'};

    int unicode_main_ks_name_length = sizeof(unicode_main_ks)/sizeof(unsigned short);

    javacall_result    result;
    javacall_handle    handle;

    int    rootPathLen = JAVACALL_MAX_FILE_NAME_LENGTH;
    int    res;
    int    i;


    memset(rootPath, 0, JAVACALL_MAX_FILE_NAME_LENGTH);

    result = javacall_dir_get_root_path(rootPath, &rootPathLen);
    if(result == JAVACALL_FAIL) {
        return JAVACALL_FAIL;
    }
    if(rootPathLen < (JAVACALL_MAX_FILE_NAME_LENGTH - unicode_main_ks_name_length)) {
        for(i = 0; i < unicode_main_ks_name_length; i++) {
            rootPath[rootPathLen+i] = unicode_main_ks[i];
        }
        rootPathLen+=i;
    } else {
        printf("javacall_security.c : File name %ls, is to long.\n",
               rootPath);
        return JAVACALL_FAIL;
    }

    javacall_set_local_variables();

    currentPosition = NULL;

    printf("Opening %ls.\n", rootPath);
    result = javacall_file_open(rootPath, rootPathLen,
                                JAVACALL_FILE_O_RDONLY,
                                &handle);
    if(result == JAVACALL_FAIL) {
        printf("Can't open %ls.\n", rootPath);
        return JAVACALL_FAIL;
    }

    main_ks_size = (int)javacall_file_sizeofopenfile(handle);
    if(-1 == main_ks_size) {
        javacall_file_close(handle);
        printf("Can't get javacall_file_sizeofopenfile() %ls\n",
               rootPath);
        return JAVACALL_FAIL;
    }

    _main_ks_content = (unsigned char *) malloc(main_ks_size);
    if(_main_ks_content == NULL) {
        javacall_file_close(handle);
        return JAVACALL_FAIL;
    }

    res = javacall_file_read(handle, _main_ks_content, main_ks_size);
    if(res <= 0 ) {
        printf("Can't read %ls\n", rootPath);
        free(_main_ks_content);
        javacall_file_close(handle);
        return JAVACALL_FAIL;
    }

    javacall_file_close(handle);
    currentPosition  = _main_ks_content;

    if(*currentPosition !=  CURRENT_VERSION) {
        printf("Can't read this key storage. Current version isn't correct\n");
        free(_main_ks_content);
        currentPosition = NULL;
        return JAVACALL_FAIL;
    }

    currentPosition++;

    *jc_h = (javacall_handle)(&currentPosition);

    return JAVACALL_OK;

} /* end of javacall_security_keystore_start */

javacall_result
javacall_security_keystore_has_next(javacall_handle keyStoreHandle) {
    javacall_result res = JAVACALL_OK;

    /* end of buffer reached */
    if((currentPosition - _main_ks_content) >= main_ks_size) {
        return JAVACALL_FAIL;
    }

    /* verify_mainks_fields() moves currentPosition forward */
    res = verify_mainks_fields(OWNER_TAG, STRING_TYPE);
    return res;

} /* end of javacall_security_keystore_has_next */

javacall_result
javacall_security_keystore_get_next(javacall_handle keyStoreHandle,
                                    unsigned short** /*OUT*/ owner,
                                    int*             /*OUT*/ ownerSize,
                                    javacall_int64*  /*OUT*/ validityStartMillissec,
                                    javacall_int64*  /*OUT*/ validityEndMillisec,
                                    unsigned char**  /*OUT*/ modulus,
                                    int*             /*OUT*/ modulusSize,
                                    unsigned char**  /*OUT*/ exponent,
                                    int*             /*OUT*/ exponentSize,
                                    char**           /*OUT*/ domain,
                                    int*             /*OUT*/ domainSize) {

    int tmp_size;
    javacall_result res = JAVACALL_OK;

    javacall_reset_local_variables();

    /* getFieldSize moves currentPosition forward */
    *ownerSize = getFieldSize();
    tmp_size = (*ownerSize)+2;

    *owner = (unsigned short*) malloc(tmp_size);
    if(*owner == NULL) {
        printf("Can not allocate memory for owner.\n");
        return JAVACALL_FAIL;
    }

    local_owner = *owner;

    memset(*owner, 0, tmp_size);
    memcpy(*owner, currentPosition, *ownerSize);

    /* move currentPosition forward */
    currentPosition+=*ownerSize;

    /* get validity Start time */
    /* verify_mainks_fields() moves currentPosition forward */
    res = verify_mainks_fields(NOT_BEFORE_TAG, LONG_TYPE);
    if(res != JAVACALL_OK) {
        return res;
    }

    /* javacall_read_int64_value moves currentPosition forward */
    *validityStartMillissec = javacall_read_int64_value();


    /* get validity end time */
    /* verify_mainks_fields() moves currentPosition forward */
    res = verify_mainks_fields(NOT_AFTER_TAG, LONG_TYPE);
    if(res != JAVACALL_OK) {
        return res;
    }

    /* javacall_read_int64_value moves currentPosition forward */
    *validityEndMillisec = javacall_read_int64_value();


    /* verify_mainks_fields() moves currentPosition forward */
    res = verify_mainks_fields(MODULUS_TAG, BINARY_TYPE);
    if(res != JAVACALL_OK) {
        return res;
    }

    /* getFieldSize moves currentPosition forward */
    *modulusSize = getFieldSize();

    tmp_size = (*modulusSize)+2;
    *modulus = (unsigned char*)malloc(tmp_size);
    if(*modulus == NULL) {
        printf("Can not allocate memory for modulus data\n");
        return JAVACALL_FAIL;
    }

    local_modulus = *modulus;

    memset(*modulus, 0, tmp_size);
    memcpy(*modulus,currentPosition,*modulusSize);
    currentPosition+=*modulusSize;

    /* verify_mainks_fields() moves currentPosition forward */
    res = verify_mainks_fields(EXPONENT_TAG, BINARY_TYPE);
    if(res != JAVACALL_OK) {
        return res;
    }

    /* getFieldSize moves currentPosition forward */
    *exponentSize = getFieldSize();

    tmp_size = (*exponentSize)+2;
    *exponent = (unsigned char*)malloc(tmp_size);
    if(*exponent == NULL) {
        printf("Can not allocate memory for exponent data\n");
        return JAVACALL_FAIL;
    }

    local_exponent = *exponent;

    memset(*exponent, 0, tmp_size);
    memcpy(*exponent,currentPosition,*exponentSize);
    currentPosition+=*exponentSize;


    /* verify_mainks_fields() moves currentPosition forward */
    res = verify_mainks_fields(DOMAIN_TAG, STRING_TYPE);
    if(res != JAVACALL_OK) {
        return res;
    }

    /* getFieldSize moves currentPosition forward */
    *domainSize = getFieldSize();

    tmp_size = (*domainSize)+2;
    *domain = malloc(tmp_size);
    if(*domain == NULL) {
        printf("Can not allocate memory for domain data\n");
        return JAVACALL_FAIL;
    }

    local_domain = *domain;

    memset(*domain, 0, tmp_size);
    memcpy(*domain, currentPosition, *domainSize);
    currentPosition+=*domainSize;

    return JAVACALL_OK;
} /* end of javacall_security_keystore_get_next */


javacall_result
javacall_security_keystore_end(javacall_handle keyStoreHandle) {

    javacall_reset_local_variables();
    free(_main_ks_content);
    currentPosition = NULL;
    main_ks_size = 0;

    return JAVACALL_OK;

} /* end of javacall_security_keystore_end */


static unsigned short getFieldSize(void) {
    jc_date_size ds;

    ds.size[0] = currentPosition[1];
    ds.size[1] = currentPosition[0];

    currentPosition+=(sizeof(short));

    return ds.dsize;

} /* end of getFieldSize */

static javacall_int64 javacall_read_int64_value(void) {

    jc_expire_date ed;
    ed.size[0] = currentPosition[7];
    ed.size[1] = currentPosition[6];
    ed.size[2] = currentPosition[5];
    ed.size[3] = currentPosition[4];
    ed.size[4] = currentPosition[3];
    ed.size[5] = currentPosition[2];
    ed.size[6] = currentPosition[1];
    ed.size[7] = currentPosition[0];

    currentPosition+=sizeof(javacall_int64);

    return ed.vdate;

} /* end of javacall_read_int64_value */

static void javacall_set_local_variables(void) {

    local_owner = NULL;
    local_modulus = NULL;
    local_exponent = NULL;
    local_domain = NULL;

}  /* end of javacall_reset_local_variables */

static void javacall_reset_local_variables(void) {

    if(local_owner != NULL) {
        free(local_owner);
        local_owner = NULL;
    }

    if(local_modulus != NULL) {
        free(local_modulus);
        local_modulus = NULL;
    }

    if(local_exponent != NULL) {
        free(local_exponent);
        local_exponent = NULL;
    }

    if(local_domain != NULL) {
        free(local_domain);
        local_domain = NULL;
    }

}  /* end of javacall_reset_local_variables */

static javacall_result verify_mainks_fields(javacall_keystore_tag tag,
                                            javacall_datatype datatype) {

    if(*currentPosition != tag) {
        return JAVACALL_FAIL;
    }
    currentPosition++;

    if(*currentPosition != datatype) {
        return JAVACALL_FAIL;
    }
    currentPosition++;

    return JAVACALL_OK;

} /* end of verify_mainks_fields */

#ifdef USE_MAIN_KS_EXAMPLE_MAIN
int example_main(void) {

    javacall_handle h;
    unsigned short*  owner;
    int    ownerSize;
    int    modulusSize;
    int    exponentSize;
    int    domainSize;

    javacall_int64   validityStartMillissec;
    javacall_int64   validityEndMillisec;

    unsigned char*   modulus;
    unsigned char*   exponent;
    char*            domain;

    h = (javacall_handle)javacall_security_keystore_start();

    while(javacall_security_keystore_has_next(h)==JAVACALL_OK) {

        javacall_security_keystore_get_next(h,
                                            &owner, &ownerSize,
                                            &validityStartMillissec,
                                            &validityEndMillisec,
                                            &modulus,
                                            &modulusSize,
                                            &exponent, &exponentSize,
                                            &domain, &domainSize);

        printf("=================================================\n");
        printf("owner is: %s\n",owner);
        printf("ownerSize is: %d\n", ownerSize);
        printf("validityStartMillissec is: 0x%x\n", validityStartMillissec);
        printf("validityEndMillisec is: 0x%x\n", validityEndMillisec);
        printf("modulus is:\n");
        printf("modulusSize is: %d\n", modulusSize);
        printf("exponent is: [%c %c %c]\n",
               exponent[0]+'0',exponent[1]+'0',exponent[2]+'0');
        printf("exponentSize is: %d\n", exponentSize);
        printf("domain is: %s\n", domain);
        printf("domainSize is: %d\n", domainSize);
        printf("=================================================\n");

    }

    javacall_security_keystore_end(h);

} /* end of main */
#endif // USE_MAIN_KS_EXAMPLE_MAIN

/**
 * Invoke the native permission dialog.
 * When the native permission dialog is displayed, Java guarantees
 * no attempt will be made to refresh the screen from Java and the
 * LCD control will be passed to the platform.
 *
 * This function is asynchronous.
 * Return JAVACALL_WOULD_BLOCK. The notification for the dismissal
 * of the permission dialog will be sent later via notify function,
 * see javanotify_security_permission_dialog_finish().
 *
 * @param message the message the platform should display to the user.
 *                The platform must copy the message string to its own buffer.
 * @param messageLength length of message string
 * @param options the combination of permission level options
 *                to present to the user.
 *                The options flags are any combination (bitwise-OR)
 *                of the following:
 *                <ul>
 *                  <li> JAVACALL_SECURITY_ALLOW_SESSION </li>
 *                  <li> JAVACALL_SECURITY_ALLOW_ ONCE </li>
 *                  <li> JAVACALL_SECURITY_ALLOW_ALWAYS </li>
 *                  <li> JAVACALL_SECURITY_DENY_SESSION </li>
 *                  <li> JAVACALL_SECURITY_DENY_ ONCE </li>
 *                  <li> JAVACALL_SECURITY_DENY_ALWAYS </li>
 *                </ul>
 *
 * The platform is responsible for providing the coresponding strings
 * for each permission level option according to the locale.
 *
 * @retval JAVACALL_WOULD_BLOCK this indicates that the permission
 *         dialog will be displayed by the platform.
 * @retval JAVACALL_FAIL in case prompting the permission dialog failed.
 * @retval JAVACALL_NOT_IMPLEMENTED in case the native permission dialog
 *         is not implemented by the platform.
 */
javacall_result javacall_security_permission_dialog_display(javacall_utf16* message,
                                                            int messageLength,
                                                            int options) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * Checks for security permission.
 * @param suite_id      the MIDlet Suite the permission should be checked with
 * @param permission_str permission string
 * @param permission_len length of the permission string
 * @param enable_block  enable user interaction. If it is
 *                      JAVACALL_FALSE the call should never be blocked.
 * @param result        address of variable to receive the
 *                      security status (JAVACALL_SECURITY_GRANT
 *                      - granted,  JAVACALL_SECURITY_DENY -
 *                      denied), or the handle of check session
 *                      that blocks this call. Actual result
 *                      will be notified through
 *                      <code>javanotify_security_permission_check_result</code>.
 * 
 * @return JAVACALL_OK if check was performed correctly and
 *         result is stored at <code>result</code>,
 *         JAVACALL_FALSE if the function fails to perform
 *         checking, JAVACALL_WOULD_BLOCK if user interaction
 *         dialog is created and actual result will be delivered
 *         later.
 * @note the function MUST NOT return JAVACALL_WOULD_BLOCK if
 *       <code>enable_block</code> equals to JAVACALL_FALSE
 * @note it is possible to have several security session in
 *       parallel.
 * @note this function is alternate to
 *       <code>javacall_security_permission_dialog_display/code>
 *       that is deprecated
 */
javacall_result
javacall_security_check_permission(const javacall_suite_id suite_id,
                                   javacall_const_utf16_string permission_str,
                                   const int permission_len,
                                   const javacall_bool enable_block,
                                   unsigned int* const result) {
    (void)suite_id;
    (void)permission_str;
    (void)permission_len;
    (void)enable_block;
    *result = JAVACALL_SECURITY_DENY;
    return JAVACALL_NOT_IMPLEMENTED;
}
