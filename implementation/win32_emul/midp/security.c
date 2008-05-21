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
 * win32 implemenation for public keystore handling functions
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include "javacall_security.h"
#include "javacall_file.h"
#include "javacall_dir.h"
#include "javacall_logging.h"
#include "javacall_memory.h" 
#include "javautil_unicode.h"
#include "javacall_properties.h"

extern char* unicode_to_char(unsigned short* str);

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

static const char *VdomainPrefix="domain:";
static const char *VgroupPrefix="group:";
static       char *VpolicyFilename = NULL;

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
         javautil_debug_print (JAVACALL_LOG_ERROR, "security",
        "javacall_security.c : File name %d, is to long.\n",
               unicode_to_char(rootPath));
        return JAVACALL_FAIL;
    }

    javacall_set_local_variables();

    currentPosition = NULL;

    javautil_debug_print (JAVACALL_LOG_INFORMATION, "security", "Opening %s.\n", unicode_to_char(rootPath));
    result = javacall_file_open(rootPath, rootPathLen,
                                JAVACALL_FILE_O_RDONLY,
                                &handle);
    if(result == JAVACALL_FAIL) {
     javautil_debug_print (JAVACALL_LOG_ERROR, "security", "Can't open %s.\n", unicode_to_char(rootPath));
        return JAVACALL_FAIL;
    }

    main_ks_size = (int)javacall_file_sizeofopenfile(handle);
    if(-1 == main_ks_size) {
        javacall_file_close(handle);
     javautil_debug_print (JAVACALL_LOG_ERROR, "security",
        "Can't get javacall_file_sizeofopenfile() %s\n",
               unicode_to_char(rootPath));
        return JAVACALL_FAIL;
    }

    _main_ks_content = (unsigned char *) malloc(main_ks_size);
    if(_main_ks_content == NULL) {
        javacall_file_close(handle);
        return JAVACALL_FAIL;
    }

    res = javacall_file_read(handle, _main_ks_content, main_ks_size);
    if(res <= 0 ) {
     javautil_debug_print (JAVACALL_LOG_ERROR, "security","Can't read this key storage. Current version isn't correct\n");
        free(_main_ks_content);
        javacall_file_close(handle);
        return JAVACALL_FAIL;
    }

    javacall_file_close(handle);
    currentPosition  = _main_ks_content;

    if(*currentPosition !=  CURRENT_VERSION) {
     javautil_debug_print (JAVACALL_LOG_ERROR, "security","Can't read this key storage. Current version isn't correct\n");
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
     javautil_debug_print (JAVACALL_LOG_ERROR, "security","Can not allocate memory for owner.\n");
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
     javautil_debug_print (JAVACALL_LOG_ERROR, "security", "Can not allocate memory for modulus data\n");
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
     javautil_debug_print (JAVACALL_LOG_ERROR, "security", "Can not allocate memory for exponent data\n");
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
     javautil_debug_print (JAVACALL_LOG_ERROR, "security", "Can not allocate memory for domain data\n");
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
/*
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
*/
     javautil_debug_print (JAVACALL_LOG_ERROR, "security", "=================================================\n");
     javautil_debug_print (JAVACALL_LOG_ERROR, "security", "owner is: %s\n",owner);
     javautil_debug_print (JAVACALL_LOG_ERROR, "security", "ownerSize is: %d\n", ownerSize);
     javautil_debug_print (JAVACALL_LOG_ERROR, "security", "validityStartMillissec is: 0x%x\n", validityStartMillissec);
     javautil_debug_print (JAVACALL_LOG_ERROR, "security", "validityEndMillisec is: 0x%x\n", validityEndMillisec);
     javautil_debug_print (JAVACALL_LOG_ERROR, "security", "modulus is:\n");
     javautil_debug_print (JAVACALL_LOG_ERROR, "security", "modulusSize is: %d\n", modulusSize);
     javautil_debug_print (JAVACALL_LOG_ERROR, "security", "exponent is: [%c %c %c]\n",
                                                            exponent[0]+'0',exponent[1]+'0',exponent[2]+'0');
     javautil_debug_print (JAVACALL_LOG_ERROR, "security", "exponentSize is: %d\n", exponentSize);
     javautil_debug_print (JAVACALL_LOG_ERROR, "security", "domain is: %s\n", domain);
     javautil_debug_print (JAVACALL_LOG_ERROR, "security", "domainSize is: %d\n", domainSize);
     javautil_debug_print (JAVACALL_LOG_ERROR, "security", "=================================================\n");
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




static char* load_file(char *fname) {
    javacall_result  ret = 0;
    javacall_utf16   uni_fname[JAVACALL_MAX_FILE_NAME_LENGTH];
    javacall_handle  hFile;
    long  len, len1;
    char  *ptr = NULL;
    
    memset(uni_fname, 0, sizeof(uni_fname));
    javautil_unicode_utf8_to_utf16(fname, strlen(fname), uni_fname, JAVACALL_MAX_FILE_NAME_LENGTH, &len);
    ret = javacall_file_open(uni_fname, len, JAVACALL_FILE_O_RDONLY, &hFile);
    if (ret == JAVACALL_OK) {
        len = (long)javacall_file_sizeofopenfile(hFile);
        if (len > 0) {
            len += 1; //for null terminated byte
            ptr = (char*)javacall_malloc(len);
            if (ptr != NULL) {
                len1 = javacall_file_read(hFile, ptr, len);
                while (len1 < len) ptr[len1++] = 0; //pad with nulls
            }
        } else
            ptr = "";
        javacall_file_close(hFile);
    }

    return ptr;
}

//returns number of text lines 
static long count_lines(char* buffer) {
    char one_char;
    int  new_line=1;
    long lines=0;

    while (*buffer) {
        one_char = *buffer++;
        if (one_char == '\n' || one_char == '\r') {
            new_line = 1;
        } else if (new_line) {
            lines++;
            new_line = 0;
        }
    }

    return lines;
}

// fill dst with the next line from buffer0, returns bytes skipped
static int next_line(char *buffer0, char *dst) {
    char *buffer_ptr = buffer0;
    char one_char;
    int len = 0;

    //skip  preceding newlines
    while (*buffer_ptr && (*buffer_ptr == '\n' || *buffer_ptr == '\r')) buffer_ptr++; 
    while (*buffer_ptr) {
        one_char = *buffer_ptr++;
        if (one_char == '\n' || one_char == '\r') {
            while (*buffer_ptr && (*buffer_ptr == '\n' || *buffer_ptr == '\r'))
                buffer_ptr++;
            break;
        }
        dst[len++] = one_char;
    }

    dst[len] = 0;//terminate the line
    return buffer_ptr - buffer0;
}

static int check_prefix(char* buff, char *prefix) {
    if (buff == NULL)
        return 0;
    while (*prefix)
        if(*prefix++ != *buff++)
            return 0;

    return 1;
}

char *build_file_name(char* fname) {
    char *storage_path;
    char *full_name;
    int   storage_len, flen;

    javacall_get_property("system.default_storage",
                                      JAVACALL_APPLICATION_PROPERTY,
                                      &storage_path);

    storage_len = strlen(storage_path);
    flen = strlen(fname);
    full_name = javacall_malloc(storage_len + flen + 1);
    if (full_name != NULL) {
        strcpy(full_name, storage_path);
        strcat(&full_name[storage_len], fname);
        full_name[storage_len + flen + 1] = 0;
    } else
        full_name = fname;

    return full_name;
}

static char *getPolicyFilename(char *def_val) {
    char*tmpstr;
    if (javacall_get_property("security.policyfile",
                                      JAVACALL_APPLICATION_PROPERTY,
                                      &tmpstr) != JAVACALL_OK)
            tmpstr = def_val;
    return build_file_name(tmpstr);
}

static char *getFuncGroupFilename(char *def_val) {
    char*tmpstr;
    if (javacall_get_property("security.messagefile",
                                      JAVACALL_APPLICATION_PROPERTY,
                                      &tmpstr) != JAVACALL_OK)
            tmpstr = def_val;

    return build_file_name(tmpstr);
}

int javacall_load_domain_list(void **array) {
    char *file_str, *ptr0, *ptr1;
    char buff[128];
    int lines, offset, i1;
    char **str_list;

    if (VpolicyFilename == NULL)
        VpolicyFilename = getPolicyFilename("MSA.txt");

    file_str = load_file(VpolicyFilename);
    if (file_str == NULL)
        return 0;

    lines = 0;
    ptr0 = file_str;
    ptr1 = NULL;
    while ((offset = next_line(ptr0, buff))) {
        if (check_prefix(buff, (char*)VdomainPrefix)) {
            lines++;
            if (ptr1 == NULL)
                ptr1 = ptr0; //save the location of first "domain" line
        }
        ptr0 += offset;
    }

    do {
        //allocate one placeholder for pointers to domain strings and the string
        //we allocate more than needed but don't care - this buffer is immediatly freed
        str_list = (char**)javacall_malloc(lines*(sizeof(char*) + sizeof(buff)));
        if (str_list == NULL) {
            lines = 0;
            break;
        }

        ptr0 = ptr1;
        ptr1 = ((char*)str_list) + lines*sizeof(char*);
        for (i1 = 0; i1 < lines; i1++) {
            str_list[i1] = ptr1;
            while ((offset = next_line(ptr0, buff))) {
                if (check_prefix(buff, (char*)VdomainPrefix))
                    break;
                ptr0 += offset;
            }
            ptr0[strlen(buff)] = 0; //terminate the "domain" line
            ptr0 += strlen(VdomainPrefix);
            do {
                while(*ptr0 == ' ') ptr0++;//skip spaces
                *ptr1++ = *ptr0++;
            } while(*ptr0);
            *ptr1++ = 0; //add null terminated
            ptr0++;
        }

    } while (0);

    javacall_free(file_str);
    *array = str_list;
    return lines;
}

int javacall_load_group_list(void **array) {
    char *file_str, *ptr0, *ptr1;
    char buff[128];
    int lines, offset, i1;
    char **str_list;

    if (VpolicyFilename == NULL)
        VpolicyFilename = getPolicyFilename("MSA.txt");

    file_str = load_file(VpolicyFilename);
    if (file_str == NULL)
        return 0;

    lines = 0;
    ptr0 = file_str;
    while ((offset = next_line(ptr0, buff))) {
        if (check_prefix(buff, (char*)VgroupPrefix))
            lines++;
        else if (check_prefix(buff, (char*)VdomainPrefix)) {
            *ptr0 = 0; //groups at the beginning, no need for the rest of the file!!!
            break;
        }
        ptr0 += offset;
    }

    do {
        //allocate one placeholder for pointers to group strings and the string
        str_list = (char**)javacall_malloc(lines*(sizeof(char*) + sizeof(buff)));
        if (str_list == NULL) {
            lines = 0;
            break;
        }

        ptr0 = file_str;
        ptr1 = ((char*)str_list) + lines*sizeof(char*);
        for (i1 = 0; i1 < lines; i1++) {
            char *ptr2;
            str_list[i1] = ptr1;
            do {
                ptr0 += next_line(ptr0, buff);
                if (check_prefix(buff, (char*)VgroupPrefix))
                    break;
            } while (1);

            ptr2 = buff+strlen(VgroupPrefix);
            while(*ptr2 == ' ') ptr2++;//skip spaces
            while(*ptr2) *ptr1++ = *ptr2++;  //copy the group name
            *ptr1++ = 0; //add null terminated
        }

    } while (0);

    javacall_free(file_str);
    *array = str_list;
    return lines;
}

int javacall_load_group_permissions(void **list, char *group_name) {
    char *file_str, *ptr0, *ptr1;
    char buff[128];
    int lines, offset, i1;
    char **str_list;

    if (VpolicyFilename == NULL)
        VpolicyFilename = getPolicyFilename("MSA.txt");

    file_str = load_file(VpolicyFilename);
    if (file_str == NULL)
        return 0;

    lines = 0;
    ptr0 = file_str;
    while ((offset = next_line(ptr0, buff))) {
        ptr0 += offset;
        if (check_prefix(buff, (char*)VgroupPrefix)) {
            ptr1 = buff + strlen(VgroupPrefix);
            while(*ptr1 == ' ') ptr1++;//skip spaces
            if (check_prefix(ptr1, group_name) != 0) { // found, count the next permission lines
                ptr1 = ptr0;
                do {
                    ptr1 += next_line(ptr1, buff);
                    if (check_prefix(buff, (char*)VgroupPrefix) || // if reach next group or first domain
                        check_prefix(buff, (char*)VdomainPrefix))
                        break;
                    lines++;
                } while (1);
                break;
            }
        }
    }

    do {
        if (lines == 0) //group not found
            break;
        //allocate one placeholder for pointers to permission strings and the string
        str_list = (char**)javacall_malloc(lines*(sizeof(char*) + sizeof(buff)));
        if (str_list == NULL) {
            lines = 0;
            break;
        }
        ptr1 = ((char*)str_list) + lines*sizeof(char*);
        for (i1 = 0; i1 < lines; i1++) {
            char *ptr2;
            str_list[i1] = ptr1;
            ptr0 += next_line(ptr0, buff);
            ptr2 = buff;
            while (*ptr2) {
                if (*ptr2 != ' ' && *ptr2 != ',')
                    *ptr1++ = *ptr2;
                ptr2++;
            }
            *ptr1++ = 0;
        }
    } while (0);

    javacall_free(file_str);
    *list = str_list;
    return lines;
}

static int value_str_to_int (char *str) {
    if (check_prefix(str, JAVACALL_ALLOW_STR))
        return JAVACALL_ALLOW;
    if (check_prefix(str, JAVACALL_SESSION_STR))
        return JAVACALL_SESSION;
    if (check_prefix(str, JAVACALL_BLANKET_STR))
        return JAVACALL_BLANKET;
    if (check_prefix(str, JAVACALL_ONESHOT_STR))
        return JAVACALL_ONESHOT;
    return JAVACALL_NEVER;
}

static int get_group_value(char *domain_name, char *group_name, int getMaxValue) {
    char *file_str, *ptr0;
    char buff[128];
    int value;

    if (VpolicyFilename == NULL)
        VpolicyFilename = getPolicyFilename("MSA.txt");

    file_str = load_file(VpolicyFilename);
    if (file_str == NULL)
        return JAVACALL_NEVER;
    
    ptr0 = file_str;
    value = JAVACALL_NEVER;
    do {
        ptr0 += next_line(ptr0, buff);
        if (check_prefix(buff, (char*)VdomainPrefix) && (strstr(buff, domain_name) != NULL))
            break;
    } while (*ptr0);

    do {
        if (!*ptr0)
            break;
        ptr0 += next_line(ptr0, buff);
        if (strstr(buff, group_name)) {
            char *ptr1;
            if (getMaxValue)
                ptr1 = buff; 
            else {
                ptr1 = strchr(buff, '(');
                if (ptr1)
                    ptr1++;
                else
                    ptr1 = buff;
            }
            value = value_str_to_int(ptr1);
            break;
        }
    } while (!check_prefix(buff, (char*)VdomainPrefix));

    javacall_free(file_str);
    return value;
}

int javacall_get_default_value(char *domain_name, char *group_name) {
    return get_group_value(domain_name, group_name, 0);
}

int javacall_get_max_value(char *domain_name, char *group_name) {
    return get_group_value(domain_name, group_name, 1);
}

#define DEF_NUM_OF_LINES 6
int javacall_load_group_messages(void **list, char *group_name) {
    char *file_str, *ptr0, *ptr1;
    char buff[256];
    int  i1, found;
    char **str_list;

    file_str = getFuncGroupFilename("_function_groups.txt");

    if (file_str == NULL)
        return 0;

    file_str = load_file(file_str);
    if (file_str == NULL)
        return 0;

    ptr0 = file_str;
    found = 0;
    do {
        ptr0 += next_line(ptr0, buff);
        if (check_prefix(buff, group_name)) {
            found = 1;
            break;
        }
    } while (*ptr0);

    if (!found)
        return 0;

    str_list = (char**)javacall_malloc(DEF_NUM_OF_LINES*(sizeof(char*) + sizeof(buff)));
    if (str_list == NULL)
        return 0;
    memset(str_list, 0, DEF_NUM_OF_LINES*(sizeof(char*)));
    ptr1 = ((char*)str_list) + DEF_NUM_OF_LINES*sizeof(char*);
    for (i1 = 0; i1 < DEF_NUM_OF_LINES; i1++) {
        str_list[i1] = ptr1;
        ptr0 += next_line(ptr0, buff);
        if (buff[0] != ' ') // reaching next group
            break;
        strcpy(ptr1, &buff[1]);
        ptr1 += strlen(ptr1);
        *ptr1++ = 0; //null terminating
    }

    javacall_free(file_str);
    if (i1 == 0)
        javacall_free(str_list);
    
    *list = str_list;
    return i1;
}


#ifdef __cplusplus
}
#endif


