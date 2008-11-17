/*
 * @(#)jcov_file.c	1.40 06/10/10
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
 *
 */

#include "javavm/include/porting/ansi/stdlib.h"
#include "javavm/include/porting/ansi/string.h"
#include "javavm/include/porting/ansi/stdio.h"

#include "jcov.h"
#include "jcov_types.h"
#include "jcov_util.h"
#include "jcov_htables.h"
#include "jcov_setup.h"
#include "jcov_error.h"

typedef enum {
    MERGE = 0,
    TAKE_BOTH,
    TAKE_FROM_DISK,
    TAKE_FROM_MEM,
    SKIP_BOTH
} merge_action_t;

static char *KEYWORD_HEADER    = "JCOV-DATA-FILE-VERSION:";
static char *KEYWORD_CLASS     = "CLASS:";
static char *KEYWORD_SRCFILE   = "SRCFILE:";
static char *KEYWORD_TIMESTAMP = "TIMESTAMP:";
static char *KEYWORD_DATA      = "DATA:";
static char *KEYWORD_METHOD    = "METHOD:";
static char *KEYWORD_FILTER    = "FILTER:";
static char *COMMENT_CHAR      = "#";
static char *SECT_PATTERN      = "%d\t%d\t%d\t\t%d\n";
static char *DELIM             = " \t";
static char *HEADER_COV        = "#kind\tline\tposition\tcount\n";
static char *HEADER_CRT        = "#kind\tstart\tend\t\tcount\n";

static FILE *temp_file;
static FILE *result_file;

#define MAX_JCOV_LINE_LEN 1024
#define F_OK 0

static char temp_file_name[MAX_PATH_LEN];
static char line[MAX_JCOV_LINE_LEN];
static char line_copy[MAX_JCOV_LINE_LEN];
static char *line_copy_ptr;
static char buf[2*MAX_PATH_LEN];

static char *token;
static INT32 line_number;

static SSIZE_T filters_max;
static SSIZE_T filters_total;
static char **filters;

static void iterate_all_classes(void *(f)(void*, void*)) {
    hash_table_iterate(class_key_table, f);
}

static jcov_class_t *find_class(jcov_class_t *class) {
    return lookup_class_by_key(class_key_table, class);
}

static jcov_method_t *find_method(jmethodID method_id) {
    return lookup_method(method_table, method_id);
}

static void write_class_header(jcov_class_t *cls, char *modifiers) {
    char *tmp;
    if (modifiers == NULL) {
        decode_modifiers(buf, cls->access_flags);
        tmp = buf;
    } else {
        tmp = modifiers;
    }
    fprintf(temp_file, "%s %s [%s]\n", KEYWORD_CLASS, cls->name, tmp);
    fprintf(temp_file, "%s %s\n", KEYWORD_SRCFILE, cls->src_name);
    fprintf(temp_file, "%s %s\n", KEYWORD_TIMESTAMP, cls->timestamp);
    fprintf(temp_file, "%s %c\n", KEYWORD_DATA, cls->data_type);
    tmp = (cls->data_type == JCOV_DATA_C) ? HEADER_CRT : HEADER_COV;
    fprintf(temp_file, tmp);
}

static Bool class_suits_filters(jcov_class_t *class) {
    int i;
    if (class == NULL)
        return 0;
    if (filters_total == 0)
        return 1;
    for (i = 0 ; i < filters_total; i++) {
        if (!strncmp(class->name, filters[i], strlen(filters[i]))) {
            return 1;
        }
    }
    return 0;
}

static void *clear_unloaded(void *_cls, void *dummy) {
    jcov_class_t *cls = *(jcov_class_t**)_cls;
    cls->unloaded = 0;
    return NULL;
}

static void *write_class(void *_cls, void *dummy) {
    int i, j;
    jcov_method_t *m;
    cov_item_t *ci;
    jcov_class_t *cls = *(jcov_class_t**)_cls;

    if (cls->unloaded || cls->num_methods < 1 ||
        !class_suits_filters(cls) || cls->data_type == JCOV_SKIP_CLASS) {
        return NULL;
    }
    write_class_header(cls, NULL);
    for (i = cls->num_methods - 1; i >= 0; i--) {
        m = find_method((cls->methods[i]).method_id);
        if (!m) {
            if (verbose_mode > 0) {
                sprintf(buf, "(internal) unknown method : %s.%s%s",
                        cls->name,
                        cls->methods[i].method_name,
                        cls->methods[i].method_signature);
                jcov_error(buf);
            }
        } else {
            Bool all_zero = FALSE;
            if (!include_abstracts && m->access_flags & JVM_ACC_ABSTRACT)
                continue;
            decode_modifiers(buf, m->access_flags);
            fprintf(temp_file, "%s %s%s [%s]\n", KEYWORD_METHOD, m->name, m->signature, buf);
            all_zero = m->covtable_size > 0 && m->covtable[0].count == 0 && caller_filter != NULL;
            for (j = 0; j < m->covtable_size; j++) {
                ci = &(m->covtable[j]);
                if (ci->type == CT_CASE && ci->where_line == 0)
                    continue;
                fprintf(temp_file, SECT_PATTERN, ci->type,
                        ci->where_line, ci->where_pos, (all_zero ? 0 : ci->count));
            }
        }
    }
    return NULL;
}

#define ERROR_AND_EXIT(msg) \
    jcov_close(&temp_file); \
    jcov_close(&result_file); \
    jcov_remove(temp_file_name); \
    jcov_error_stop(msg)

static Bool read_line(void) {
    char *tmp = line;
    int ch = getc(result_file);
    if (ch == EOF) {
        return 0;
    }
    line_number++;
    while (ch != '\n') {
        if (tmp - line >= MAX_JCOV_LINE_LEN) {
            sprintf(buf, "line #%d is too long", (int)line_number);
            ERROR_AND_EXIT(buf);
        }
        *(tmp++) = (char)ch;
        ch = getc(result_file);
        if (ch == EOF) {
            jcov_error_stop("unexpected EOF");
        }
    }
    *tmp = '\0';
    strcpy(line_copy, line);
    line_copy_ptr = line_copy;
    return 1;
}

#define STARTS_WITH(S1, S2) !strncmp(S1, S2, strlen(S2))
#define EQUALS(S1, S2) !strcmp(S1, S2)

static void get_next_token(const char *delimiters) {
    token = strtok(line_copy_ptr, delimiters);
    line_copy_ptr = NULL;
    if (token && STARTS_WITH(token, COMMENT_CHAR)) {
        token = NULL;
    }
}

static int guess_merge_action(jcov_class_t *class_from_mem, jcov_class_t *class_from_disk) {
    char data_mem;
    char data_disk;

    if (!class_suits_filters(class_from_disk) || 
        (caller_filter != NULL && !string_suits_filter(class_filter, class_from_disk->name))) {
        return SKIP_BOTH;
    }
    if (class_from_mem == NULL) { 
        return TAKE_FROM_DISK;
    }
    data_mem  = class_from_mem->data_type;
    data_disk = class_from_disk->data_type;
    if (data_mem == data_disk) {
        if (class_from_mem->unloaded) {
            sprintf(buf, "duplicated class : %s", class_from_mem->name);
            jcov_warn(buf);
        }
        return MERGE;
    }
    if ((data_mem == JCOV_DATA_B && data_disk == JCOV_DATA_C) ||
        (data_mem == JCOV_DATA_C && data_disk == JCOV_DATA_B)) {
        char *s = "can't merge class %s (diff data types: %c and %c).";
        sprintf(buf, s, class_from_mem->name, JCOV_DATA_C, JCOV_DATA_B);
        jcov_warn(buf);
        return TAKE_FROM_MEM;
    }
    if ((data_mem == JCOV_DATA_B || data_mem == JCOV_DATA_C) && data_disk == JCOV_DATA_A) {
        return class_from_mem->unloaded ? SKIP_BOTH : TAKE_FROM_MEM;
    }
    return class_from_mem->unloaded ? TAKE_FROM_DISK : TAKE_BOTH;
}

#define CHK_STATE(old_state, new_state) \
    if (state != old_state) { \
        jcov_close(&temp_file); \
        jcov_remove(temp_file_name); \
        sprintf(buf, "malformed jcov file (line %d)", (int)line_number); \
        jcov_error_stop(buf); \
    } state = new_state
#define CHK_TOKEN \
    if (token == NULL) { \
        jcov_close(&temp_file); \
        jcov_remove(temp_file_name); \
        sprintf(buf, "malformed jcov file (line %d)", (int)line_number); \
	jcov_error_stop(buf); \
    }

static void read_and_merge_data(void) {
    jcov_class_t   last_class;
    JVMPI_Method   last_method;
    cov_item_t     jcov_item;
    cov_item_t     *ji;
    int            jcov_item_ind = 0;
    int            ind;
    char           *tmp, *m_nam, *m_sig;
    char           last_modifiers[256];
    char           ch;
    int            state = 6;
    JVMPI_Method   *m = NULL;
    jcov_class_t   *found_class = NULL;
    jcov_method_t  *found_method = NULL;
    merge_action_t merge_action = MERGE;
    int ct_size;

    last_class.name              = (char*)jcov_calloc(MAX_PATH_LEN);
    last_class.src_name          = (char*)jcov_calloc(MAX_PATH_LEN);
    last_method.method_name      = (char*)jcov_calloc(MAX_PATH_LEN);
    last_method.method_signature = (char*)jcov_calloc(MAX_PATH_LEN);

    while (read_line()) {
        get_next_token(DELIM);
        if (!token || strlen(token) == 0) {
            continue;
        }
        if (EQUALS(token, KEYWORD_CLASS)) {
            CHK_STATE(6, 1);
            get_next_token(DELIM);
            CHK_TOKEN;
            strcpy(last_class.name, token);
            get_next_token("[]");
            if (token == NULL) {
                last_modifiers[0] = '\0';
            } else {
                strcpy(last_modifiers, token);
            }
        } else if (EQUALS(token, KEYWORD_SRCFILE)) {
            CHK_STATE(1, 2);
            get_next_token(DELIM);
            if (token == NULL) {
                tmp = dummy_src_name(last_class.name);
                strcpy(last_class.src_name, tmp);
                jcov_free(tmp);
            } else {
                strcpy(last_class.src_name, token);
            }
        } else if (EQUALS(token, KEYWORD_TIMESTAMP)) {
            CHK_STATE(2, 3);
            get_next_token(DELIM);
            CHK_TOKEN;
            last_class.timestamp = strdup(token);
        } else if (EQUALS(token, KEYWORD_DATA)) {
            CHK_STATE(3, 4);
            get_next_token(DELIM);
            CHK_TOKEN;
            ch = token[0];
            if (strlen(token) > 1 || 
                (ch != JCOV_DATA_B && ch != JCOV_DATA_C && ch != JCOV_DATA_M && ch != JCOV_DATA_A)) {
                sprintf(buf, "bad DATA section (line %d)", (int)line_number);
                ERROR_AND_EXIT(buf);
            }
            last_class.data_type = ch;
            found_class = find_class(&last_class);
            merge_action = guess_merge_action(found_class, &last_class);

            switch (merge_action) {
            case MERGE:
                write_class_header(&last_class, last_modifiers);
                found_class->unloaded = 1;
                break;
            case TAKE_FROM_MEM:
                write_class(&found_class, NULL);
                found_class->unloaded = 1;
                break;
            case TAKE_FROM_DISK:
                write_class_header(&last_class, last_modifiers);
                break;
            case TAKE_BOTH:
                write_class(&found_class, NULL);
                found_class->unloaded = 1;
                write_class_header(&last_class, last_modifiers);
                break;
            case SKIP_BOTH:
                break;
            }
        } else if (EQUALS(token, KEYWORD_METHOD)) {
            if (state != 6 && state != 4) {
                sprintf(buf, "malformed jcov file (line %d)", (int)line_number);
                ERROR_AND_EXIT(buf);
            }
            state = 5;
            if (merge_action == SKIP_BOTH || merge_action == TAKE_FROM_MEM) {
                continue;
            }
            if (found_class == NULL) {
                fprintf(temp_file, "%s\n", line);
                continue;
            }
            get_next_token(DELIM);
            CHK_TOKEN;
            tmp = strchr(token, '(');
            if (tmp == NULL) {
                sprintf(buf, "bad method signature : %s (line %d)",
                        token, (int)line_number);
                ERROR_AND_EXIT(buf);
            }
            m_nam = last_method.method_name;
            m_sig = last_method.method_signature;
            memcpy(m_nam, token, tmp - token);
            m_nam[tmp - token] = '\0';
            strcpy(m_sig, tmp);

            for (ind = found_class->num_methods-1; ind >= 0; ind--) {
                m = &(found_class->methods[ind]);
                if (!strcmp(m_nam, m->method_name) && !strcmp(m_sig, m->method_signature)) {
                    break;
                }
            }
            if (ind == -1) {
                sprintf(buf, "unexpected method %s%s (%s)", m_nam, m_sig, last_class.name);
                ERROR_AND_EXIT(buf);
            }
            found_method = find_method(found_class->methods[ind].method_id);
            if (found_method == NULL) {
                sprintf(buf, "(internal) cannot find method %s.%s%s",
                        last_class.name, m_nam, m_sig);
                ERROR_AND_EXIT(buf);
            }
            jcov_item_ind = 0;
            fprintf(temp_file, "%s\n", line);
        } else if (EQUALS(token, KEYWORD_FILTER)) {
            get_next_token(DELIM);
            CHK_TOKEN;
            if (filters_total > filters_max) {
                char **tmp_filters;
                int i;
                filters_max *= 2;
                tmp_filters = jcov_calloc(sizeof(char*) * filters_max);
                for (i = 0; i < filters_total; i++) {
		    tmp_filters[i] = filters[i];
		}
                jcov_free(filters);
                filters = tmp_filters;
            }
            filters[filters_total++] = jcov_strdup(token);
            fprintf(temp_file, "%s\n", line);
        } else { /* else parse jcov item data */
            if (state != 5 && state != 6) {
                sprintf(buf, "malformed jcov file (line %d)", (int)line_number);
                ERROR_AND_EXIT(buf);
            }
            state = 6;
            switch (merge_action) {
            case TAKE_FROM_MEM:
            case SKIP_BOTH:
                continue;
            case TAKE_FROM_DISK:
            case TAKE_BOTH:
                fprintf(temp_file, "%s\n", line);
                continue;
            case MERGE:
                break;
            }

            ct_size = found_method->covtable_size;
            if (jcov_item_ind > ct_size - 1) {
                sprintf(buf, "numbers of jcov items don't match (line %d)", (int)line_number);
                ERROR_AND_EXIT(buf);
            }
            do {
                ji = &(found_method->covtable[jcov_item_ind++]);
            } while (ji->type == 5 && ji->where_line == 0 && jcov_item_ind < ct_size);
            jcov_item.type = (unsigned char)strtoul(token, NULL, 10);
            get_next_token(DELIM);
            CHK_TOKEN;
            jcov_item.where_line = strtoul(token, NULL, 10);
            get_next_token(DELIM);
            CHK_TOKEN;
            jcov_item.where_pos = strtoul(token, NULL, 10);
            get_next_token(DELIM);
            CHK_TOKEN;
            jcov_item.count = strtoul(token, NULL, 10);
            if (jcov_item.type != ji->type || jcov_item.where_line != ji->where_line ||
                jcov_item.where_pos != ji->where_pos) {
                sprintf(buf, "jcov items don't match (line %d)", (int)line_number);
                ERROR_AND_EXIT(buf);
            }
            fprintf(temp_file, SECT_PATTERN, ji->type, ji->where_line, ji->where_pos,
                    ji->count + jcov_item.count);
        }
    }
    CHK_STATE(6, 0);
    iterate_all_classes(write_class);
}

void save_jcov_data(char *filename) {
    iterate_all_classes(clear_unloaded);
    do {
        sprintf(buf, "%s.%d", filename, rand());
    } while(jcov_file_exists(buf));

    strcpy(temp_file_name, buf);
    if (verbose_mode > 0) {
        sprintf(buf, "Saving jcov data : file %s, temp file %s",
                filename, temp_file_name);
        jcov_info(buf);
    }

    if ((temp_file = fopen(temp_file_name, "w+")) == NULL ) {
        sprintf(buf, "cannot create file : %s\n", temp_file_name);
        jcov_error_stop(buf);
    }

    fprintf(temp_file, "%s %d.%d\n", KEYWORD_HEADER, 
            JCOV_FILE_MAJOR_VER, JCOV_FILE_MINOR_VER);
    /* does the file exist or do we have to overwrite it? */
    if (!jcov_file_exists(filename) || overwrite_jcov_file) {
        /* yes - write new jcov data file */
        iterate_all_classes(write_class);
        if (fclose(temp_file) != 0) {
            jcov_error("cannot close file");
        }
    } else {
        /* no - merge jcov data with existing data file */
        if ((result_file = fopen(filename, "rb")) == NULL) {
            sprintf(buf,"cannot open file : %s\n", filename);
            jcov_error_stop(buf);
        }
        read_line();
        get_next_token(DELIM);
        CHK_TOKEN;
        if (!EQUALS(token, KEYWORD_HEADER)) {
            ERROR_AND_EXIT("malformed jcov file header");
        }

        get_next_token(".");
        CHK_TOKEN;
        if (strtoul(token, NULL, 10) > JCOV_FILE_MAJOR_VER) {
            sprintf(buf, "jcov file version is higher than current (%d.%d)",
                    JCOV_FILE_MAJOR_VER, JCOV_FILE_MINOR_VER);
            ERROR_AND_EXIT(buf);
        }
        get_next_token(DELIM);
        CHK_TOKEN;
        if (strtoul(token, NULL, 10) > JCOV_FILE_MINOR_VER) {
            sprintf(buf, "jcov file version is higher than current (%d.%d)",
                    JCOV_FILE_MAJOR_VER, JCOV_FILE_MINOR_VER);
            ERROR_AND_EXIT(buf);
        }

        read_and_merge_data();
        jcov_close(&result_file);
        jcov_close(&temp_file);
        jcov_remove(filename);
    }
    if (rename(temp_file_name, filename) == -1) {
        sprintf(buf,"cannot rename file : %s -> %s\n", temp_file_name, filename);
        jcov_error_stop(buf);
    }
}

void jcov_file_init(void) {
    line_number = 0;
    filters_max = 32;
    filters_total = 0;
    filters = (char**)jcov_calloc(sizeof(char*) * filters_max);
}
