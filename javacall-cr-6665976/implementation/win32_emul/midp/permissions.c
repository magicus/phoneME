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
 * common implemenation for loading policy files functions
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include "javacall_security.h"
#include "javacall_dir.h"
#include "javacall_file.h"
#include "javacall_logging.h"
#include "javacall_memory.h" 
#include "javautil_unicode.h"
#include "javacall_properties.h"


static const char *VdomainPrefix="domain:";
static const char *VgroupPrefix="alias:";
static       char *VpolicyFilename = NULL;


static char* load_file(char *fname) {
    javacall_result  ret = 0;
    javacall_utf16   uni_fname[JAVACALL_MAX_FILE_NAME_LENGTH];
    javacall_handle  hFile;
    long  len, len1;
    char  *ptr = NULL;
    
    memset(uni_fname, 0, sizeof(uni_fname));
    javautil_unicode_utf8_to_utf16(fname, strlen(fname), uni_fname, 
                                        JAVACALL_MAX_FILE_NAME_LENGTH, &len);
    ret = javacall_file_open(uni_fname, len, JAVACALL_FILE_O_RDONLY, &hFile);
    if (ret == JAVACALL_OK) {
        len = (long)javacall_file_sizeofopenfile(hFile);
        if (len > 0) {
            len += 1; /* for null terminated byte */
            ptr = (char*)javacall_malloc(len);
            if (ptr != NULL) {
                len1 = javacall_file_read(hFile, ptr, len);
                while (len1 < len) ptr[len1++] = 0; /* pad with nulls */
            }
        } else
            ptr = "";
        javacall_file_close(hFile);
    }

    return ptr;
}

/* returns number of text lines */
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

/* fill dst with the next line from buffer0, returns bytes skipped */
static int next_line(char *buffer0, char *dst) {
    char *buffer_ptr = buffer0;
    char one_char;
    int len = 0;

    /* skip  preceding newlines */
    while (*buffer_ptr && (*buffer_ptr == '\n' || *buffer_ptr == '\r'))
        buffer_ptr++; 
    while (*buffer_ptr) {
        one_char = *buffer_ptr++;
        if (one_char == '\n' || one_char == '\r') {
            while (*buffer_ptr && (*buffer_ptr == '\n' || *buffer_ptr == '\r'))
                buffer_ptr++;
            break;
        }
        dst[len++] = one_char;
    }

    dst[len] = 0; /* terminate the line */
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
    javacall_utf16 configPath[256];
    unsigned char storage_path[256];
    char *full_name;
    int   config_len, flen;
	javacall_int32 storage_len;

    config_len = sizeof(configPath)/2;
    storage_path[0] = 0;
    if (javacall_dir_get_config_path(configPath, &config_len) ==
                                                            JAVACALL_OK) {
		javacall_utf16 sep = javacall_get_file_separator();
		if (configPath[config_len-1] != sep) {
			configPath[config_len++] = sep;
		}
        javautil_unicode_utf16_to_utf8(configPath, config_len,
                                       storage_path, sizeof(storage_path),
                                       &storage_len);
    }

    storage_path[storage_len] = 0;
    flen = strlen(fname);
    full_name = javacall_malloc(storage_len + flen + 1);
    if (full_name != NULL) {
        strcpy(full_name, storage_path);
        strcpy(&full_name[storage_len], fname);
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

int javacall_permissions_load_domain_list(javacall_utf8_string* array) {
    char *file_str, *ptr0, *ptr1, *ptr2;
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
                ptr1 = ptr0;  /* save the location of first "domain" line */
        }
        ptr0 += offset;
    }

    do {
        /* 
         * allocate one placeholder for pointers to domain strings and the 
         * string we allocate more than needed but don't care - this buffer 
         * is immediatly freed
         */
        str_list = (char**)javacall_malloc(lines*(sizeof(char*) + 
                                                    sizeof(buff)));
        if (str_list == NULL) {
            lines = 0;
            break;
        }

        ptr0 = ptr1;
        ptr1 = ((char*)str_list) + lines*sizeof(char*);
        for (i1 = 0; i1 < lines; i1++) {
            str_list[i1] = ptr1;
            while ((offset = next_line(ptr0, buff))) {
                ptr0 += offset;
                if (check_prefix(buff, (char*)VdomainPrefix))
                    break;
            }
            ptr2 = buff+strlen(VdomainPrefix);
            do {
                if (*ptr2 != ' ')
                    *ptr1++ = *ptr2;
                ptr2++;
            } while(*ptr2);
            *ptr1++ = 0; /* add null terminated */
        }

    } while (0);

    javacall_free(file_str);
    *array = (void*)str_list;
    return lines;
}

int javacall_permissions_load_group_list(javacall_utf8_string* array) {
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
            /*groups at the beginning, rest of the file in no needed */
            *ptr0 = 0;
            break;
        }
        ptr0 += offset;
    }

    do {
        /* allocate one placeholder for pointers to group strings and the 
         * string
         */
        str_list = (char**)javacall_malloc(lines*(sizeof(char*) + 
                                                        sizeof(buff)));
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
            while(*ptr2 == ' ') ptr2++; /* skip spaces */
            while(*ptr2) *ptr1++ = *ptr2++;  /*copy the group name */
            *ptr1++ = 0; /* add null terminated */
        }

    } while (0);

    javacall_free(file_str);
    *array = (void*)str_list;
    return lines;
}

int javacall_permissions_load_group_permissions(javacall_utf8_string* list,
                                    javacall_utf8_string group_name) {
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
            while(*ptr1 == ' ') ptr1++; /* skip spaces */
            if (check_prefix(ptr1, group_name) != 0) {
                /* found, count the next permission lines */
                ptr1 = ptr0;
                do {
                    ptr1 += next_line(ptr1, buff);
                    /* if reach next group or first domain */
                    if (check_prefix(buff, (char*)VgroupPrefix) ||
                        check_prefix(buff, (char*)VdomainPrefix))
                        break;
                    lines++;
                } while (1);
                break;
            }
        }
    }

    do {
        if (lines == 0) /* group not found */
            break;
        /* allocate one placeholder for pointers to permission strings and
         * the string
         */
        str_list = (char**)javacall_malloc(lines*(sizeof(char*) + 
                                                    sizeof(buff)));
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
    *list = (void*)str_list;
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

static int get_group_value(javacall_utf8_string domain_name,
                           javacall_utf8_string group_name,
                           int getMaxValue) {
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
        if (check_prefix(buff, (char*)VdomainPrefix) && 
                            (strstr(buff, domain_name) != NULL))
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

int javacall_permissions_get_default_value(javacall_utf8_string domain_name, 
                               javacall_utf8_string group_name) {
    return get_group_value(domain_name, group_name, 0);
}

int javacall_permissions_get_max_value(javacall_utf8_string domain_name,
                           javacall_utf8_string group_name) {
    return get_group_value(domain_name, group_name, 1);
}

#define DEF_NUM_OF_LINES 6
int javacall_permissions_load_group_messages(javacall_utf8_string* list,
                                 javacall_utf8_string group_name) {
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

    str_list = (char**)javacall_malloc(DEF_NUM_OF_LINES*(sizeof(char*)
                                                          + sizeof(buff)));
    if (str_list == NULL)
        return 0;
    memset(str_list, 0, DEF_NUM_OF_LINES*(sizeof(char*)));
    ptr1 = ((char*)str_list) + DEF_NUM_OF_LINES*sizeof(char*);
    for (i1 = 0; i1 < DEF_NUM_OF_LINES; i1++) {
        str_list[i1] = ptr1;
        ptr0 += next_line(ptr0, buff);
        if (buff[0] != ' ') /* reaching next group */
            break;
        strcpy(ptr1, &buff[1]);
        ptr1 += strlen(ptr1);
        *ptr1++ = 0; /* null terminating */
    }

    javacall_free(file_str);
    if (i1 == 0)
        javacall_free(str_list);
    
    *list = (void*)str_list;
    return i1;
}


#ifdef __cplusplus
}
#endif


