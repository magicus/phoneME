/*
 * 
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
 * @brief Content Handler Registry implementation based on MIDP functions
 */
#include "jsr211_registry.h"

#include <string.h>

#include "midpString.h"
#include "midpStorage.h"
#include "midpMalloc.h"
#include <suitestore_common.h>
#include <pcsl_string.h>

#define JSR211_TABLE_NAME jsr211_file_name

#define JSR211_FILE_HANDLER void*
#define JSR211_MALLOC   midpMalloc
#define JSR211_FREE     midpFree
#define JSR211_MAX_RESULT_SET 256

/**
 * Internal enum for defining string comparision conditions
 */
typedef enum {
  find_exact = 1,   /**< Exact strings comparision */
  find_first,       /**< Comparision by beginnings of strings */
  find_test,        /**< Special comparision mode for testing new IDs */
  find_last         /**< Comparision by endings of strings */
} find_condition;

/**
 * Contetnt Handler Record field indexes.
 */
typedef enum {
    JSR211_CHR_ID = 0,      /**< Handler ID */
    JSR211_CHR_FLAG,        /**< installation flag */
    JSR211_CHR_SUITE,       /**< Suite Id */
    JSR211_CHR_CLASSNAME,    /**< class name  */
    JSR211_CHR_TYPES,       /**< Types supported by a handler */
    JSR211_CHR_SUFFIXES,    /**< Suffixes supported by a handler */
    JSR211_CHR_ACTIONS,     /**< Actions supported by a handler */
    JSR211_CHR_LOCALES,     /**< Locales supported by a handler */
    JSR211_CHR_ACTION_MAP,  /**< Handler action map */
    JSR211_CHR_ACCESSES,     /**< Access list */
    JSR211_CHR_COUNT
} chr_index;

/**
 * Translate JSR 211 field index enum to chr_index.
 */
#define CHR_INDEX(jsr_fld) \
    (jsr_fld < 0? -1: \
     jsr_fld < JSR211_FIELD_TYPES? jsr_fld: \
     jsr_fld < JSR211_FIELD_COUNT? jsr_fld + JSR211_CHR_CLASSNAME: \
     -1)

/**
 * Internal enum defining fields types
 */
typedef enum {
  field_int,    /**< Integer field */
  field_string, /**< String field */
  field_array   /**< String array field */
} field_type;

/**
 * Fields types definition
 */
static field_type record_struct[] = {
  field_string, /* id */
  field_int,    /* state */
  field_int,     /* suite_id */
  field_string, /* class_name */
  field_array,  /* types */
  field_array,  /* suffixes */
  field_array,  /* actions */
  field_array,  /* locales */
  field_array,  /* action map */
  field_array,  /* accesses */
  -1            /* invalid-valued fence */
};


/* "_jsr211_reg" - filename for registry storage */
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(jsr211_file_name)
{'_','j','s','r','2','1','1','_','r','e','g','\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(jsr211_file_name);

static int table_file;
static char* io_error_message;

/**
 * The current version supports the Latin Abc only.
 */
static char to_lower_case(jchar ch) {
  const jchar a = 'a' - 'A';
  return ch < 'A' || ch > 'Z'? ch: ch + a;
}

/**
 * Compares two MIDP strings.
 *
 * The distinction from midpStringCmp is a posibility of
 * partly equal strings recognition
 *
 * @param str1 the first string to be comapred
 * @param str2 the second string to be comapred
 * @param case_sensitive indicates case sensivity
 * @return 0 if strings are equals; if strings are
 * different returns number of the first different symbol
 */
static int compare_two_strings(const pcsl_string *str1, const pcsl_string *str2, 
                                            jsr211_boolean case_sensitive) {
  const jchar *buf1, *buf2, *cur1, *cur2;
  int i, n, res;

  cur1 = buf1 = pcsl_string_get_utf16_data(str1);
  cur2 = buf2 = pcsl_string_get_utf16_data(str2);
  n = pcsl_string_utf16_length(str1);
  i = pcsl_string_utf16_length(str2);

  res = n == i? 0: n < i? n: i;
  if (res != 0) {
    n = res++;
  }
  i = 0;

  if (!case_sensitive) {
    while (i++ < n) {
      if (to_lower_case(*cur1++) != to_lower_case(*cur2++)) {
        res = i;
        break;
      }
    }
  } else {
    while (i++ < n) {
      if (*cur1++ != *cur2++) {
        res = i;
        break;
      }
    }
  }

  pcsl_string_release_utf16_data(buf1, str1);
  pcsl_string_release_utf16_data(buf2, str2);
  return res;
}

/**
 * Finds specified string in a string array.
 *
 * @param array string array
 * @param array_len length of array
 * @param key string to be found
 * @return 0 if string is not found;
 */
static int findStringInArray(const pcsl_string *array, int array_len, 
                                                    const pcsl_string *key) {
  const pcsl_string *current, *last;
  
  for (current = array, last = array + array_len; current < last; current++) {
    if (pcsl_string_equals(current, key)) {
      return 1;
    }
  }
  return 0;
}

/**
 * Moves file position to next record.
 */
static void goto_next_record(void) {
  int    current_size;
  
  storageRead(&io_error_message, table_file, (char*)&current_size, sizeof(int));
  storageRelativePosition(&io_error_message, table_file, current_size - 
                                                                  sizeof(int));
}

/**
 * Writes string to the file.
 *
 * @param string string to be written
 * @return number of bytes written
 */
static int write_string(const pcsl_string* string) {
  int size = pcsl_string_utf16_length(string) * sizeof(jchar);
  const jchar *buf = pcsl_string_get_utf16_data(string);

  storageWrite(&io_error_message, table_file, (char *)&size, sizeof(int));
  storageWrite(&io_error_message, table_file, (char *)buf, size);

  pcsl_string_release_utf16_data(buf, string);
  
  return sizeof(int) + size;
}

/**
 * Writes integer to the file.
 *
 * @param data integer to be written
 * @return number of bytes written
 */
static int write_int(int data) {
  storageWrite(&io_error_message, table_file, (char *)&data, sizeof(int));
  return sizeof(int);
}

/**
 * Writes string array to the file.
 *
 * @param array_len length of array to be written
 * @param array array to be written
 * @return number of bytes written
 */
static int write_string_array(int array_len, const pcsl_string* array) {
  long current_pos;
  int write_count;
  const pcsl_string *current_string, *end;

  current_pos = storageRelativePosition(&io_error_message, table_file, 0);
  /* array size placeholder */
  storageWrite(&io_error_message, table_file, (char*)&write_count, sizeof(int));

  /* array size saving */
  storageWrite(&io_error_message, table_file, (char *)&array_len, sizeof(int));
  write_count = sizeof(int);

  for (current_string = array, end = array + array_len;
       current_string < end;
       current_string ++) {
    write_count += write_string(current_string);
  }
  storagePosition(&io_error_message, table_file, current_pos);
  storageWrite(&io_error_message, table_file, (char*)&write_count, sizeof(int));
  storageRelativePosition(&io_error_message, table_file, write_count);

  return write_count + sizeof(int);
}

/**
 * Reads string from the file.
 *
 * @param string pointer to result string
 * @return number of bytes read
 */
static int read_string(pcsl_string* string) {
  jint string_len;
  jchar *string_data;

  storageRead(&io_error_message, table_file, (char*)&string_len, sizeof(int));
  string_data = (jchar*)JSR211_MALLOC(string_len);
  storageRead(&io_error_message, table_file, (char*)string_data, string_len);
  pcsl_string_convert_from_utf16(string_data, string_len/sizeof(jchar), string);

  return string_len + sizeof(int);
}

/**
 * Reads string array from the file.
 *
 * @param array_size pointer to result array length
 * @param strings pointer to result array
 * @return number of bytes read
 */
static int read_string_array(int* array_size, pcsl_string** strings) {
  int array_len;
  int bytes_read;
  pcsl_string* current_string;

  storageRead(&io_error_message, table_file, (char *)&array_len, sizeof(int));
  storageRead(&io_error_message, table_file, (char *)array_size, sizeof(int));
  bytes_read = sizeof(int) * 2;

  if (*array_size > 0) {
    *strings = alloc_pcsl_string_list(*array_size);
    for (current_string = *strings; bytes_read < array_len; current_string++) {
      bytes_read += read_string(current_string);
    }
  }

  return bytes_read;
}

/**
 * Searches fields in records.
 *
 * @param fld_num internal ID of field to be found
 * @return number of bytes read
 */
static int position_field(int fld_num) {
  int record_size;
  int field_cnt;
  field_type* cur_type;

  if (fld_num < 0 || fld_num >= JSR211_CHR_COUNT) {
    return -1;
  }

  /* skip header */
  if (storageRead(&io_error_message, table_file, (char *)&record_size, 
                                                        sizeof(int)) == -1) {
    return 0;
  }
  for (field_cnt = 0, cur_type = record_struct; field_cnt < fld_num; 
                                                    field_cnt++, cur_type++) {
    int field_size;

    if (*cur_type != field_int) {
      storageRead(&io_error_message, table_file, 
                                            (char*)&field_size, sizeof(int));
    }
    else {
      field_size = sizeof(int);
    }
    storageRelativePosition(&io_error_message, table_file, field_size);
  }
  return record_size;
}

/**
 * compare_two_strings wrapper.
 * Reads a string from a current field of a current
 * record and compares it with a passed value.
 *
 * @param key compare key
 * @param case_sensitive case sensivity flag
 * @param cond internal ID of comparision mode
 * @return 1 if strings are equal (for the given mode)
 */
static int compare_string(const pcsl_string* search_key, 
                        jsr211_boolean case_sensitive, find_condition cond) {
  pcsl_string string;
  int compare_result;
  int exact_result, first_result;
  int equal_count, string_len;

  read_string(&string);

  string_len = pcsl_string_utf16_length(&string);
  compare_result = cond != find_last ? 
                   compare_two_strings(&string, search_key, case_sensitive):
                   pcsl_string_ends_with(search_key, &string);

  pcsl_string_free(&string);
  exact_result = compare_result == 0;
  
  if (cond == find_last) {
    return compare_result;
  }

  if (cond == find_exact) {
    return exact_result;
  }

  equal_count = compare_result - 1;
  first_result = (equal_count == pcsl_string_utf16_length(search_key)) 
                 || exact_result;

  if (cond == find_first) {
    return compare_result == 0 || equal_count == string_len;
  }

  if (cond == find_test) {
    return first_result || (equal_count == string_len);
  }
  return 0;
}

/**
 * Searches specified string in the array record field
 * Reads strings from a current field of a current
 * record and compares it with a passed value.
 *
 * @param key compare key
 * @param cond internal ID of comparision mode
 * @return 1 if strings are equal (for the given mode)
 */
static int compare_array(const pcsl_string* key, jsr211_boolean case_sensitive, 
                                                        find_condition cond) {
  int array_size, array_len, current_string;

  storageRead(&io_error_message, table_file, (char *)&array_size, sizeof(int));
  storageRead(&io_error_message, table_file, (char *)&array_len, sizeof(int));

  for (current_string = 0; current_string < array_len; current_string++) {
    if (compare_string(key, case_sensitive, cond)) {
      return 1;
    }
  }
  return 0;
}

/**
 * Verifies access of caller for current content handler.
 * Reads access restrictions from a current record and
 * compares it with a passed caller_id.
 *
 * @param caller_id caller
 * @param current_position current position of the file
 * @param current_size [out] size of current record
 * @return 1 if access is permitted
 */
static int check_access(const pcsl_string* caller_id, long current_position, 
                                                            int* current_size) {
  int    access_len, access_ok;

  if ((*current_size = position_field(JSR211_CHR_ACCESSES)) <= 0) {
    return -1;
  }
  if (caller_id != NULL && pcsl_string_length(caller_id) > 0) {
    storageRelativePosition(&io_error_message, table_file, sizeof(int));
    storageRead(&io_error_message, table_file, (char*)&access_len, sizeof(int));
    if (!access_len) {
      access_ok = 1;
    }
    else {
      storageRelativePosition(&io_error_message, table_file, -2 * (long)sizeof(int));
      access_ok = compare_array(caller_id, find_first, JSR211_TRUE);
    }
  }
  else {
    access_ok = 1;
  }
  storagePosition(&io_error_message, table_file, current_position);
  return access_ok;
}

/**
 * Searches the next content handler in the file by specified conditions.
 *
 * @param caller_id caller ID
 * @param number number of key field
 * @param key search key
 * @param case_sensitive case sensivity flag
 * @param cond comparision mode
 * @return offset of found record or -1
 */
static long find_next_by_field(const pcsl_string* caller_id, 
                        jsr211_field number, const pcsl_string* key, 
                        jsr211_boolean case_sensitive, find_condition cond) {
  long current_position;
  int    current_size, access;
  int    found = 0;
  int chr = CHR_INDEX((int)(number));  // Content Handler Record index
  int isString;

  switch (record_struct[chr]) {
    case field_string:
      isString = 1;
      break;
    case field_array:
      isString = 0;
      break;
    default:
      return -1;
  }

  current_position = storageRelativePosition(&io_error_message, table_file, 0);
  for (;;) {
    /* check access */
    access = check_access(caller_id, current_position, &current_size);
    if (access == -1) {
      return -1;
    }
    else if (access) {
      position_field(chr);
      found = (isString? 
                    compare_string(key, case_sensitive, cond):
                    compare_array(key, case_sensitive, cond));
    }

    storagePosition(&io_error_message, table_file, current_position);
    if (found) {
      break;
    }
    current_position += current_size;
    goto_next_record();
  }

  return current_position;
}

/**
 * Searches the next content handler in the file by its suite id.
 *
 * @param suite search suite ID
 * @return offset of found record or -1
 */
static long find_next_by_suite(int suite) {
  long current_position;
  int record_size;
  int testSuite;
  int found = 0;

  current_position = storageRelativePosition(&io_error_message, table_file, 0);
  do {
    record_size = position_field(JSR211_CHR_SUITE);
    if (record_size <= 0) {
      current_position = -1;
      break;
    }

    storageRead(&io_error_message, table_file, (char*)&testSuite, sizeof(int));
    found = (testSuite == suite? 1: 0);
    storagePosition(&io_error_message, table_file, current_position);
    if (found) {
      break;
    }
    current_position = storageRelativePosition(&io_error_message, table_file, 
                                    record_size);
  } while (current_position > 0);

  return current_position;
}

/**
 * Opens registry file
 *
 * @param write write access flag
 * @return operation result
 */
static jsr211_result open_table_file(int write) {
  pcsl_string fileName = PCSL_STRING_NULL;
  int io_mode = OPEN_READ | (write ? OPEN_WRITE : 0);

  if (PCSL_STRING_OK !=
      pcsl_string_cat(storage_get_root(INTERNAL_STORAGE_ID), &JSR211_TABLE_NAME, &fileName)) {
    return JSR211_FAILED;
  }
  
  table_file = storage_open(&io_error_message, &fileName, io_mode);
  
  pcsl_string_free(&fileName);

  return io_error_message? JSR211_FAILED: JSR211_OK;
}

/**
 * Closes registry file
 *
 * @return operation result
 */
static jsr211_result close_table_file(void) {
  storageClose(&io_error_message, table_file);
  
  if (io_error_message) {
    return JSR211_FAILED;
  }
  
  return JSR211_OK;
}

/**
 * Frees pcsl strings allocated for the JSR211_CH.
 * @param ch cleaned handler pointer.
 */
static void free_CH(JSR211_CH* ch) {
    pcsl_string_free(&(ch->id));
    pcsl_string_free(&(ch->class_name));
}

/**
 * Allocates and intializes memory for JSR211_CH array.
 * @param size requested number of JSR211_CH elements.
 * @return allocated and intialized buffer pointer 
 8         or <code>NULL<code> if no memory.
 */
static JSR211_CH* alloc_CH_buffer(int size) {
    JSR211_CH* buf = (JSR211_CH*)JSR211_MALLOC(size * sizeof(JSR211_CH));

    if (buf != NULL) {
        JSR211_CH initializer = {
            PCSL_STRING_NULL_INITIALIZER,
            0,
            PCSL_STRING_NULL_INITIALIZER,
            (jsr211_flag)-1
        };
        JSR211_CH* ptr = buf;
        
        while (size--) {
            *ptr++ = initializer;
        }
    }

    return buf;
}

/**
 * Frees memory allocated for the JSR211_CH array and all incorporated strings.
 * @param buf cleaned buffer pointer.
 * @param size the buffer size
 */
static void free_CH_buffer(JSR211_CH* buf, int size) {
    if (buf != NULL) {
        while (size--) {
            free_CH(buf+size);
        }
        JSR211_FREE(buf);
    }
}

/**
 * Loads current handler's main fields (ID, suite_ID, class_name, flag).
 * @param handler loaded structure pointer.
 * @return operation result
 */
static void load_current_handler(JSR211_CH* ch) {
    long pos = storageRelativePosition(&io_error_message, table_file, 
                                       sizeof(int)) - sizeof(int);
    read_string(&(ch->id));
    storageRead(&io_error_message, table_file, (char*)&(ch->flag), sizeof(int));
    storageRead(&io_error_message, table_file, (char*)&(ch->suite_id), sizeof(int));
    read_string(&(ch->class_name));
    storagePosition(&io_error_message, table_file, pos);
}

/**
 * Initializes content handler registry.
 *
 * @return JSR211_OK if content handler registry initialized successfully
 */
jsr211_result jsr211_initialize(void) {
  return JSR211_OK;  
}

/**
 * Finalizes content handler registry.
 *
 * @return JSR211_OK if content handler registry finalized successfully
 */
jsr211_result jsr211_finalize(void) {
  return JSR211_OK;  
}

/**
 * Store content handler information into a registry.
 *
 * @param handler description of a registering handler. Implementation MUST NOT 
 * retain pointed object
 * @return JSR211_OK if content handler registered successfully
 */
jsr211_result jsr211_register_handler(const JSR211_content_handler* handler) {
  long current_position;
  unsigned int record_size = sizeof(int);

  if (open_table_file(1) != JSR211_OK) {
    return JSR211_FAILED;
  }
  storagePosition(&io_error_message, table_file, 
                                storageSizeOf(&io_error_message, table_file));
  current_position = storageRelativePosition(&io_error_message, table_file, 0);
  storageWrite(&io_error_message, table_file, (char *)&record_size, 
                        sizeof(record_size)); /* Placeholder for record size */

  record_size += write_string(&(handler->id));
  record_size += write_int(handler->flag);
  record_size += write_int(handler->suite_id);
  record_size += write_string(&(handler->class_name));
  record_size += write_string_array(handler->type_num, handler->types);
  record_size += write_string_array(handler->suff_num, handler->suffixes);
  record_size += write_string_array(handler->act_num, handler->actions);
  record_size += write_string_array(handler->locale_num, handler->locales);
  record_size += write_string_array(handler->locale_num * handler->act_num, 
                                                           handler->action_map);
  record_size += write_string_array(handler->access_num, handler->accesses);

  storagePosition(&io_error_message, table_file, current_position);
  storageWrite(&io_error_message, table_file, (char *)&record_size, 
                                                           sizeof(record_size));
  storageRelativePosition(&io_error_message, table_file, record_size - 
                                                                   sizeof(int));
  
  close_table_file();
  return JSR211_OK;
}

/**
 * Deletes content handler information from a registry.
 *
 * @param handler_id content handler ID
 * @return JSR211_OK if content handler unregistered successfully
 */
jsr211_result jsr211_unregister_handler(const pcsl_string* handler_id) {
  long current_position, file_size, buffer_size;
  int record_size;
  char *buffer;

  if (open_table_file(1) != JSR211_OK) {
    return JSR211_FAILED;
  }
  if (find_next_by_field(NULL, JSR211_FIELD_ID, handler_id, JSR211_TRUE,
                                                            find_exact) == -1) {
    close_table_file();
    return JSR211_FAILED;
  }
  
  storageRead(&io_error_message, table_file, (char *)&record_size, sizeof(int));
  current_position = storageRelativePosition(&io_error_message, table_file, 
                                                            -(long)sizeof(int));
  file_size = storageSizeOf(&io_error_message, table_file);
  goto_next_record();
  
  if (!(buffer = JSR211_MALLOC((buffer_size = file_size - current_position - 
                                                            record_size)))) {
    close_table_file();
    return JSR211_FAILED;
  }
  
  storageRead(&io_error_message, table_file, buffer, buffer_size);
  storagePosition(&io_error_message, table_file, current_position);
  storageWrite(&io_error_message, table_file, buffer, buffer_size);
  storageTruncate(&io_error_message, table_file, current_position + 
                                                                   buffer_size);
  
  close_table_file();
  return JSR211_OK;
}

/**
 * Gets the registered content handler for the ID.
 * The query can be for an exact match or for the handler
 * matching the prefix of the requested ID.
 *  <BR>Only a content handler which is visible to and accessible to the 
 * given @link caller_id should be returned.
 *
 * @param caller_id calling application identifier.
 * @param id handler ID.
 * @param mode flag indicating whether exact, prefixed or test search should be 
 * performed according to @link JSR211_SEARCH_MODE constants.
 * @param handler output value - requested handler.
 *  <br>Use @link jsr211_fillHandler function to fill this structure.
 * @return status of the operation
 */
jsr211_result jsr211_get_handler(const pcsl_string* caller_id, 
                        const pcsl_string* id, jsr211_search_flag mode,
                        /*OUT*/ JSR211_RESULT_CH* handler) {
  JSR211_CH ch;
  find_condition cond = mode == JSR211_SEARCH_TEST?     find_test:
                        mode == JSR211_SEARCH_PREFIX?   find_first:
                                                        find_exact; 

  if (open_table_file(0) != JSR211_OK) {
    return JSR211_FAILED;
  }

  if (find_next_by_field(caller_id, JSR211_FIELD_ID, id, JSR211_TRUE, cond) != -1) {
    load_current_handler(&ch);
    jsr211_fillHandler(&ch, handler);
    free_CH(&ch);
  }

  close_table_file();
  return JSR211_OK;
}

/**
 * Loads the handler's data field. Allowed fields are: <UL>
 *  <LI> JSR211_FIELD_TYPES, <LI> JSR211_FIELD_SUFFIXES, 
 *  <LI> JSR211_FIELD_ACTIONS, <LI> JSR211_FIELD_LOCALES, 
 *  <LI> JSR211_FIELD_ACTION_MAP, <LI> and JSR211_FIELD_ACCESSES. </UL>
 *
 * @param id requested handler ID.
 * @param field_id requested field.
 * @param result output structure where requested array is placed to.
 *  <br>Use @link jsr211_fillStringArray function to fill this structure.
 * @return status of the operation
 */
jsr211_result jsr211_get_handler_field(const pcsl_string* id, 
                                    jsr211_field field_id, 
                                    /*OUT*/ JSR211_RESULT_STRARRAY* result) {
  int result_len = 0;
  pcsl_string* buf = NULL;

  if (record_struct[CHR_INDEX((int)(field_id))] != field_array) {
    return JSR211_FAILED;
  }

  if (open_table_file(0) != JSR211_OK) {
    return JSR211_FAILED;
  }

  if (find_next_by_field(NULL, JSR211_FIELD_ID, id, JSR211_TRUE, find_exact) 
                                                                        == -1) {
    close_table_file();
    return JSR211_FAILED;
  }

  /* reading field */
  position_field(CHR_INDEX((int)(field_id)));
  read_string_array(&result_len, &buf);
  if (buf != NULL) {
    jsr211_fillStringArray(buf, result_len, result);
    free_pcsl_string_list(buf, result_len);
  }

  close_table_file();
  return JSR211_OK;
}

/**
 * Searches content handler using specified key and value.
 *
 * @param caller_id calling application identifier
 * @param key search field id. Valid keys are: <ul> 
 *   <li>JSR211_FIELD_TYPES, <li>JSR211_FIELD_SUFFIXES, 
 *   <li>JSR211_FIELD_ACTIONS. </ul>
 * @param value search value
 * @param result the buffer for Content Handlers result array. 
 *  <br>Use @link jsr211_fillHandlerArray function to fill this structure.
 * @return status of the operation
 */
jsr211_result jsr211_find_handler(const pcsl_string* caller_id,
                        jsr211_field key, const pcsl_string* value,
                        /*OUT*/ JSR211_RESULT_CHARRAY* result) {
  int found_num;
  JSR211_CH *res_buffer;
  JSR211_CH *current_result, *last;
  jsr211_boolean case_sens;
  find_condition cond = find_exact;

  if (open_table_file(0) != JSR211_OK) {
    return JSR211_FAILED;
  }

  switch (key) {
    case JSR211_FIELD_TYPES:
    case JSR211_FIELD_SUFFIXES:
        case_sens = JSR211_FALSE;
        break;
    case JSR211_FIELD_ACTIONS:
        case_sens = JSR211_TRUE;
        break;
    default:
        close_table_file();
        return JSR211_FAILED;
  }

  res_buffer = alloc_CH_buffer(JSR211_MAX_RESULT_SET);
  if (res_buffer == NULL) {
    close_table_file();
    return JSR211_FAILED;
  }

  for (current_result = res_buffer, last = res_buffer + JSR211_MAX_RESULT_SET;
       current_result < last;
       current_result++) {
    if (-1 == find_next_by_field(caller_id, key, value, case_sens, cond)) {
      break;
    }

    load_current_handler(current_result);
    goto_next_record();
  }

  if ((found_num = (int)(current_result - res_buffer))) {
    jsr211_fillHandlerArray(res_buffer, found_num, result);
  }
  free_CH_buffer(res_buffer, found_num);
  close_table_file();
  return JSR211_OK;
}

/**
 * Fetches handlers registered for the given suite.
 *
 * @param suiteId requested suite ID.
 * @param result the buffer for Content Handlers result array. 
 *  <br>Use @link jsr211_fillHandlerArray function to fill this structure.
 * @return status of the operation
 */
jsr211_result jsr211_find_for_suite(SuiteIdType suiteId, 
                        /*OUT*/ JSR211_RESULT_CHARRAY* result) {
  int found_num;
  JSR211_CH *res_buffer;
  JSR211_CH *current_result, *last;

  if (open_table_file(0) != JSR211_OK) {
    return JSR211_FAILED;
  }

  res_buffer = alloc_CH_buffer(JSR211_MAX_RESULT_SET);
  if (res_buffer == NULL) {
    close_table_file();
    return JSR211_FAILED;
  }

  for (current_result = res_buffer, last = res_buffer + JSR211_MAX_RESULT_SET;
       current_result < last;
       current_result++) {
    if (-1 == find_next_by_suite(suiteId)) {
      break;
    }
    load_current_handler(current_result);
    goto_next_record();
  }

  if ((found_num = (int)(current_result - res_buffer))) {
    jsr211_fillHandlerArray(res_buffer, found_num, result);
  }
  free_CH_buffer(res_buffer, found_num);
  close_table_file();
  return JSR211_OK;
}

/**
 * Searches content handler using content URL. This function MUST define
 * content type and return default handler for this type if any.
 *
 * @param caller_id calling application identifier
 * @param url content URL
 * @param action requested action
 * @param handler output value - requested handler.
 *  <br>Use @link jsr211_fillHandler function to fill this structure.
 * @return JSR211_OK if the appropriate handler found
 */
jsr211_result jsr211_handler_by_URL(const pcsl_string* caller_id, 
                        const pcsl_string* url, const pcsl_string* action, 
                        /*OUT*/ JSR211_RESULT_CH* handler) {
  (void)caller_id;
  (void)url;
  (void)action;
  (void)handler;
  return JSR211_FAILED;
}

/**
 * Returns all found values for specified field. Tha allowed fields are: <ul>
 *    <li> JSR211_FIELD_ID, <li> JSR211_FIELD_TYPES, <li> JSR211_FIELD_SUFFIXES,
 *    <li> and JSR211_FIELD_ACTIONS. </ul>
 * Values should be selected only from handlers accessible for given caller_id.
 *
 * @param caller_id calling application identifier.
 * @param field search field id
 * @param result output structure where result is placed to.
 *  <br>Use @link jsr211_fillStringArray function to fill this structure.
 * @return status of the operation
 */
jsr211_result jsr211_get_all(const pcsl_string* caller_id, jsr211_field field,
                        /*OUT*/ JSR211_RESULT_STRARRAY* result) {
  long current_position;
  int found_num, access, current_size;
  pcsl_string *res_buffer;
  pcsl_string *current_result, *last;
  int chr = CHR_INDEX((int)(field));
  int isString;

  switch (record_struct[chr]) {
    case field_string:
      isString = 1;
      break;
    case field_array:
      isString = 0;
      break;
    default:
      return JSR211_FAILED;
  }

  if (open_table_file(0) != JSR211_OK) {
    return JSR211_FAILED;
  }
  res_buffer = alloc_pcsl_string_list(JSR211_MAX_RESULT_SET);
  if (res_buffer == NULL) {
    close_table_file();
    return JSR211_FAILED;
  }

  current_position = 0;
  for (current_result = res_buffer, last = res_buffer + JSR211_MAX_RESULT_SET;
       current_result < last;) {
    
    access = check_access(caller_id, current_position, &current_size);
    if (access == -1) {
      break;
    }
    
    if (access && position_field(chr) > 0) {
      if (isString) {
        read_string(current_result);
        if (!findStringInArray(res_buffer, current_result - res_buffer, 
                                                            current_result)) {
          current_result++;
        }
      }
      else {
        pcsl_string *array_res, *results, *last_res;
        int array_length;

        read_string_array(&array_length, &array_res);
        for (results = array_res, last_res = array_res + array_length;
             results < last_res && current_result < last;
             results++) {
          if (!findStringInArray(res_buffer, current_result - res_buffer, 
                                                                    results)) {
            *(current_result++) = *results;
            *results = PCSL_STRING_NULL;
          }
        }
        free_pcsl_string_list(array_res, array_length);
      }
    }
    
    storagePosition(&io_error_message, table_file, current_position);
    current_position += current_size;
    goto_next_record();
  }
  
  if ((found_num = (int)(current_result - res_buffer))) {
    jsr211_fillStringArray(res_buffer, found_num, result);
  }

  free_pcsl_string_list(res_buffer, JSR211_MAX_RESULT_SET);
  close_table_file();
  return JSR211_OK;
}


/**
 * Executes specified non-java content handler.
 * @param handler_id content handler ID
 * @return codes accordingly jsr211_launch_result enums:
 * <ul>
 * <li> JSR211_LAUNCH_OK or JSR211_LAUNCH_OK_SHOULD_EXIT if content handler 
 *   executed successfully
 * <li> other code from the enum according to error codition
 * </ul>
 */
jsr211_launch_result jsr211_execute_handler(const pcsl_string* handler_id) {
  handler_id = handler_id;
  return JSR211_LAUNCH_ERR_NOTSUPPORTED;
}
