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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <util/utf.h>
#include <pcsl_string.h>
#include <pcsl_memory.h>

/*
 * Radix value for pcsl convertings.
 */
#define RADIX 10

static jboolean pcsl_string_active = PCSL_FALSE;

/**
 * Returns whether the string system is active.
 * The string subsystem is active after it is initialized and
 * before it is finalized.
 *
 * @return PCSL_TRUE if the string system is active,
 *         PCSL_FALSE otherwise
 */
jboolean pcsl_string_is_active(void) {
  return pcsl_string_active;
}

/**
 * Performs platform-specific initialization of the string system.
 * This function must be invoked before any other functions of
 * the string subsystem.
 *
 * @return initialization status
 */
pcsl_string_status pcsl_string_initialize(void) {
  pcsl_string_active = PCSL_TRUE;
  return PCSL_STRING_OK;
}

/**
 * Performs platform-specific finalization of the string system.
 * No functions of the string subsystem should be used after
 * the string system is finalized.
 *
 * @return finalization status
 */
pcsl_string_status pcsl_string_finalize() {
  pcsl_string_active = PCSL_FALSE;
  return PCSL_STRING_OK;
}

/**
 * Returns the number of abstract characters in the specified string,
 * not including the terminating zero character.
 * Returns -1 if str is NULL or doesn't represent a valid abstract character
 * sequence.
 * See Unicode Glossary at http://www.unicode.org/glossary/.
 *
 * @param str string
 * @return number of abstract characters in the string
 */
jsize pcsl_string_length(const pcsl_string * str) {
  if (str == NULL || str->data == NULL) {
    return -1;
  }

  return utf16_string_length(str->data, str->length) - 1;
}

/**
 * Returns the number of 16-bit units in the UTF-16 representation
 * of the specified string, not including the terminating zero character.
 * Returns -1 if str is NULL.
 *
 * @param str string
 * @return length of UTF-16 representation of the string
 */
jsize pcsl_string_utf16_length(const pcsl_string * str) {
  if (str == NULL || str->data == NULL) {
    return -1;
  }

  /* Do not count terminating '\0' */
  return str->length - 1;
}

/**
 * Returns the number of bytes in the UTF-8 representation
 * of the specified string, not including the terminating zero character.
 * Returns -1 if str is NULL.
 *
 * @param str string
 * @return length of UTF-8 representation of the string
 */
jsize pcsl_string_utf8_length(const pcsl_string * str) {
  if (str == NULL || str->data == NULL) {
    return -1;
  }

  {
    jsize utf8_length = 0;
    pcsl_string_status status = pcsl_utf16_convert_to_utf8(str->data, str->length,
					   NULL, 0, &utf8_length);

    if (status == PCSL_STRING_OK) {
      /* Do not count terminating '\0' */
      return utf8_length - 1;
    } else {
      return -1;
    }
  }
}


/**
 * Converts the string to UTF-16.
 * If the buffer length is not sufficient, the conversion is not performed and
 * the function returns PCSL_STRING_BUFFER_OVERFLOW.
 * If converted_length is not NULL and the specified string has a valid UTF-8
 * representation, the number of 16-bit units in the UTF-16 representation
 * of the string is written to converted_length.
 *
 * @param str           string to convert
 * @param buffer        buffer to store the result of conversion
 * @param buffer_length length of buffer (number of 16-bit units)
 * @param converted_length
 *            storage for the number of 16-bit units in UTF-16 representation
 *            of the string
 * @return status of the operation
 */
pcsl_string_status pcsl_string_convert_to_utf16(const pcsl_string * str,
				     jchar * buffer,
				     jsize buffer_length,
				     jsize * converted_length) {
  if (str == NULL || str->data == NULL || buffer == NULL) {
    return PCSL_STRING_EINVAL;
  }

  /* Do not count the terminating zero character. */
  if (converted_length != NULL) {
    * converted_length = str->length - 1;
  }

  if (str->length > buffer_length) {
    return PCSL_STRING_BUFFER_OVERFLOW;
  }

  (void)memcpy(buffer, str->data, str->length * sizeof(jchar));

  return PCSL_STRING_OK;
}

/**
 * Converts the string to UTF-8.
 * If the buffer length is not sufficient, the conversion is not performed and
 * the function returns PCSL_STRING_BUFFER_OVERFLOW.
 * If converted_length is not NULL and the specified string has a valid UTF-8
 * representation, the number of bytes in the UTF-8 representation
 * of the string is written to converted_length.
 *
 * @param str           string to convert
 * @param buffer        buffer to store the result of conversion
 * @param buffer_length length of buffer (number of bytes)
 * @param converted_length
 *            storage for the number of bytes in UTF-8 representation
 *            of the string
 * @return status of the operation
 */
pcsl_string_status pcsl_string_convert_to_utf8(const pcsl_string * str,
				    jbyte * buffer,
				    jsize buffer_length,
				    jsize * converted_length) {
  if (str == NULL || str->data == NULL || buffer == NULL) {
    return PCSL_STRING_EINVAL;
  }

  {
    pcsl_string_status status =
      pcsl_utf16_convert_to_utf8(str->data, str->length,
			    buffer, buffer_length, converted_length);

    /* Do not count the terminating zero character. */
    if (status == PCSL_STRING_OK && converted_length != NULL) {
      (*converted_length)--;
    }

    return status;
  }
}

/**
 * Creates a new string from the specified array of UTF-16 characters
 * and saves a pointer to that string into the specified storage 'string'.
 * Fails if either 'buffer' or 'string' is NULL.
 * If the last character in the specified array is not zero,
 * a zero character is appended at the end of string.
 *
 * @param buffer        array of UTF-16 characters
 * @param buffer_length number of 16-bit units in the array
 * @param string        storage for the created string
 * @return status of the operation
 */
pcsl_string_status pcsl_string_convert_from_utf16(const jchar * buffer,
				       jsize buffer_length,
				       pcsl_string * string) {
  jchar * new_buffer = NULL;

  if (buffer == NULL || string == NULL) {
    return PCSL_STRING_EINVAL;
  }

  * string = PCSL_STRING_NULL;

  if (buffer_length < 0) {
    return PCSL_STRING_EINVAL;
  }

  {
    jsize new_buffer_length = buffer_length;

    /* Strip trailing zero characters. */
    for ( ; new_buffer_length > 0 && buffer[new_buffer_length - 1] == 0;
	  new_buffer_length--) {}

    /* IMPL_NOTE: decide what to do with possible internal zeroes.
     * Now we remove all trailing zeroes, but if some garbage text follows
     * the eol terminator in the buffer, we will append all that stuff.
     * On the other hand, we can just say DON'T DO THAT (universal advice),
     * that is, do not include garbage into the string length */

    /* We are going to append one terminating zero. */
    new_buffer_length++;

    new_buffer = pcsl_mem_malloc(sizeof(jchar) * new_buffer_length);

    if (new_buffer == NULL) {
      return PCSL_STRING_ENOMEM;
    }

    (void)memcpy(new_buffer, buffer, sizeof(jchar) * (new_buffer_length - 1));

    /* Append the terminating zero. */
    new_buffer[new_buffer_length - 1] = 0;

    string->data    = new_buffer;
    string->length  = new_buffer_length;
    string->flags   = PCSL_STRING_IN_HEAP;

    return PCSL_STRING_OK;
  }
}

/**
 * Creates a new string from the specified array of UTF-8 characters
 * and saves a pointer to that string into the specified storage 'string'.
 * Fails if either 'buffer' or 'string' is NULL.
 * If the last character in the specified array is not zero,
 * a zero character is appended at the end of string.
 *
 * @param buffer        array of UTF-8 characters
 * @param buffer_length number of bytes in the array
 * @param string        storage for the created string
 * @return status of the operation
 */
pcsl_string_status pcsl_string_convert_from_utf8(const jbyte * buffer,
				      jsize buffer_length,
				      pcsl_string * string) {
  jchar * utf16_buffer = NULL;
  const jsize max_utf16_length = buffer_length * 2 + 1;

  if (buffer == NULL || string == NULL) {
    return PCSL_STRING_EINVAL;
  }

  if (buffer_length < 0) {
    * string = PCSL_STRING_NULL;
    return PCSL_STRING_EINVAL;
  }

  utf16_buffer = pcsl_mem_malloc(sizeof(jchar) * max_utf16_length);

  if (utf16_buffer == NULL) {
    * string = PCSL_STRING_NULL;
    return PCSL_STRING_ENOMEM;
  }

  /* Strip trailing zero characters. */
  for ( ; buffer_length > 0 && buffer[buffer_length - 1] == 0;
	buffer_length--) {}

  {
    jsize utf16_length = 0;
    /* we specify max_utf16_length - 1 to leave room for a terminating zero */
    pcsl_string_status status = pcsl_utf8_convert_to_utf16(buffer, buffer_length,
						      utf16_buffer,
						      max_utf16_length - 1,
						      &utf16_length);

    if (status != PCSL_STRING_OK) {
      pcsl_mem_free(utf16_buffer);
      * string = PCSL_STRING_NULL;
      return status;
    } else {
      /* Append terminating zero character. */
      utf16_buffer[utf16_length] = 0;
      utf16_length++;

      /* Resize the buffer down to the actual string length. */
      utf16_buffer = pcsl_mem_realloc(utf16_buffer,
				      utf16_length * sizeof(jchar));

      if (utf16_buffer == NULL) {
	* string = PCSL_STRING_NULL;
	return PCSL_STRING_ENOMEM;
      }

      string->data    = utf16_buffer;
      string->length  = utf16_length;
      string->flags   = PCSL_STRING_IN_HEAP;

      return PCSL_STRING_OK;
    }
  }
}

/**
 * Tests if two strings are equal.
 * <p>
 * Returns PCSL_FALSE if either of the two strings is NULL.
 *
 * @param str1 first string to compare
 * @param str2 second string to compare
 * @return PCSL_TRUE if both strings are not NULL and are equal,
 *         PCSL_FALSE otherwise
 */
jboolean pcsl_string_equals(const pcsl_string * str1, const pcsl_string * str2) {

  if (str1 == str2) { /* include NULL = NULL */
    return PCSL_TRUE;
  }

  if (str1 == NULL || str2 == NULL) {
    return PCSL_FALSE;
  }

  /* Check that input structures are equals */
  if (memcmp(str1, str2, sizeof(pcsl_string)) == 0) {
    return PCSL_TRUE;
  }

  if (str1->data == NULL || str2->data == NULL) {
    return PCSL_FALSE;
  }

  if (str1->length != str2->length) {
    return PCSL_FALSE;
  }

  if (str1->data == str2->data) {
    return PCSL_TRUE;
  }

  return memcmp(str1->data, str2->data,
		str1->length * sizeof(jchar)) == 0 ? PCSL_TRUE : PCSL_FALSE;
}

/**
 * Compares two strings. The result of comparison is an integer less than,
 * equal to, or greater than zero if str1 is found, respectively, to be
 * less than, to match, or be greater than str2.
 *
 * @param str1 first string to compare
 * @param str2 second string to compare
 * @param comparison storage for the result of comparison
 * @return status of operation comparison
 */
pcsl_string_status pcsl_string_compare(const pcsl_string * str1, const pcsl_string * str2,
			    jint * comparison) {

  if (comparison == NULL) {
    return PCSL_STRING_EINVAL;
  }

  if (str1 == str2) { /* include NULL = NULL */
    *comparison = 0;
    return PCSL_STRING_OK;
  }

  if (str1 == NULL || str2 == NULL) {
    return PCSL_STRING_EINVAL;
  }

  /* Check that input structures are equals */
  if (memcmp(str1, str2, sizeof(pcsl_string)) == 0) {
    *comparison = 0;
    return PCSL_STRING_OK;
  }

  /* Damaged strings */
  if ((str1->data == NULL && str1->length > 0) ||
     (str2->data == NULL && str2->length > 0)) {
    return PCSL_STRING_EINVAL;
  }

  {
    const jsize min_length =
      str1->length < str2->length ? str1->length : str2->length;

    jint cmp = 0;
    if (min_length > 1) { /* length = 1 in case of empty string */
      cmp = memcmp(str1->data, str2->data, sizeof(jchar) * min_length);
    }

    if (cmp == 0) {
      cmp = str1->length - str2->length;
    }

    * comparison = cmp;

    return PCSL_STRING_OK;
  }
}

/**
 * Creates a new string that is a concatenation of the two specified strings.
 * NULL string is interpreted as empty string
 *
 * @param str1 first string to concatenate
 * @param str2 second string to concatenate
 * @param dst  storage for the created concatenation
 * @return status of the operation
 */
pcsl_string_status pcsl_string_cat(const pcsl_string * str1,
			const pcsl_string * str2,
			pcsl_string * str) {
  pcsl_string *src1;
  pcsl_string *src2;
  if (str == NULL) {
    return PCSL_STRING_EINVAL;
  }
  if (str1 == NULL) {
    src1 = (pcsl_string *)&PCSL_STRING_EMPTY;
  } else {
    src1 = (pcsl_string *)str1;
    if (src1->length > 0 && src1->data == NULL) {
      return PCSL_STRING_EINVAL;
    }
  }
  if (str2 == NULL) {
    src2 = (pcsl_string *)&PCSL_STRING_EMPTY;
  } else {
    src2 = (pcsl_string *)str2;
    if (src2->length > 0 && src2->data == NULL) {
      return PCSL_STRING_EINVAL;
    }
  }
  {
    /* Strip the terminating zero at the end of the first string. */
    jsize length1, length2, cat_length;
    jchar * cat_buffer;
    length1 = src1->length;
    length2 = src2->length;
    if (length1 < 2 && length2 < 2) { /* both strings are empty or NULL */
      *str = PCSL_STRING_EMPTY;
      return PCSL_STRING_OK;
    }
    if (length1 > 0 && length2 > 0) length1--;
    cat_length = length1 + length2;
    cat_buffer = pcsl_mem_malloc(cat_length * sizeof(jchar));

    if (cat_buffer == NULL) {
      * str = PCSL_STRING_NULL;
      return PCSL_STRING_ENOMEM;
    }

    if (length1 > 0) {
      memcpy(cat_buffer, src1->data, length1 * sizeof(jchar));
    }

    if (length2 > 0) {
      memcpy(cat_buffer + length1, src2->data, length2 * sizeof(jchar));
    }

    str->data = cat_buffer;
    str->length = cat_length;
    str->flags = PCSL_STRING_IN_HEAP;

    return PCSL_STRING_OK;
  }
}

/**
 * Creates a new string that is a duplicate of this string.
 *
 * @param str         string
 * @param dst         storage for the created duplicate
 * @return status of the operation
 */
pcsl_string_status pcsl_string_dup(const pcsl_string * src,
			pcsl_string * dst) {
  /* Return error status when any argument is NULL */
  if (dst == NULL || src == NULL) {
    return PCSL_STRING_EINVAL;
  }
  /* Return error status when string is damaged */
  if (src->length > 0 && src->data == NULL) {
    return PCSL_STRING_EINVAL;
  }
  /* When src is not in heap (frequently NULL and EMPTY string),
   * it is not need to duplicate source string
   */
  if (!(src->flags & PCSL_STRING_IN_HEAP)) {
    *dst = *src;
    return PCSL_STRING_OK;
  }
  switch (src->length) {

    case 0: /* NULL string */
      *dst = PCSL_STRING_NULL;
      break;

    case 1: /* EMPTY string */
      *dst = PCSL_STRING_EMPTY;
      break;

    default: /* String is not empty */
      {
        const jsize dup_length = src->length;
        jchar * dup_buffer = pcsl_mem_malloc(dup_length * sizeof(jchar));

        if (dup_buffer == NULL) {
          * dst = PCSL_STRING_NULL;
          return PCSL_STRING_ENOMEM;
        }

        memcpy(dup_buffer, src->data, dup_length * sizeof(jchar));

        dst->data = dup_buffer;
        dst->length = dup_length;
        dst->flags = PCSL_STRING_IN_HEAP;
      } /* End of default */
  } /* End of switch */

  return PCSL_STRING_OK;
}

/**
 * Append a string to an existing destination string.
 * All buffers obtained from the string must be released before the string
 * is changed by this function.
 *
 * @param dst the destination string, that grows
 * @param src the source string whose text gets appended to the destination string
 * @return status code
 */
pcsl_string_status pcsl_string_append(pcsl_string* dst, const pcsl_string* src) {
    /* IMPL_NOTE: write up an optimized implementation */
    pcsl_string tmpres;
    pcsl_string_status rc;
    rc = pcsl_string_cat(dst,src,&tmpres);
    if (PCSL_STRING_OK != rc) {
        return rc;
    }
    pcsl_string_free(dst);
    *dst = tmpres;
    return rc;
}

/**
 * Append a character to an existing destination string.
 * All buffers obtained from the string must be released before the string
 * is changed by this function.
 *
 * @param dst the destination string, that grows
 * @param newchar the character that gets appended to the destination string
 * @return status code
 */
pcsl_string_status pcsl_string_append_char(pcsl_string* dst, const jchar newchar) {
    /* IMPL_NOTE: write up an optimized implementation */
    jchar tmp[1];
    tmp[0] = newchar;
    return pcsl_string_append_buf(dst, tmp, 1);
}

/**
 * Append text in a jchar array to an existing destination string.
 * All buffers obtained from the string must be released before the string
 * is changed by this function.
 *
 * @param dst the destination string, that grows
 * @param newtext the character array that gets appended to the destination string
 * @param textsize the size of the text in the character array
 * @return status code
 */
pcsl_string_status pcsl_string_append_buf(pcsl_string* dst,
                                          const jchar* newtext,
                                          const jint textsize) {
    /* IMPL_NOTE: write up an optimized implementation */
    pcsl_string text_str;
    pcsl_string_status rc;
    rc = pcsl_string_convert_from_utf16(newtext,textsize,&text_str);
    if (PCSL_STRING_OK == rc) {
        rc = pcsl_string_append(dst,&text_str);
        pcsl_string_free(&text_str);
    }
    return rc;
}

/**
 * Set string buffer capacity to the specified value.
 * All buffers obtained from a string must be released before the string
 * is changed by this function.
 *
 * This function is a hint that allows an efficient implementation
 * of concatenation of multiple strings using the append functions
 * (see pcsl_string_append, pcsl_string_append_buf, pcsl_string_append_char).
 * Before appending, invoke this function to specify the final string length.
 *
 * Nevertheless, an implementation may choose to implement this function
 * as a nothing-doer. There is no guarantee that the append functions
 * will not cause buffer reallocation.
 *
 * This function does not fail: if the resize request fails, it leaves
 * the string unchanged.
 *
 * @param str the string whose buffers's capacity is desired to be changed
 * @param size the desired new string buffer capacity
 *
 */
void pcsl_string_predict_size(pcsl_string* str, jint size) {
    /* IMPL_NOTE: write up an optimized implementation */
    (void) str;
    (void) size;
}

/**
 * Creates a new string that is a substring of this string. The
 * substring begins with the character at the specified begin_index
 * and extends to the character at the index end_index-1.
 *
 * @param str         string to look in
 * @param begin_index the beginning index, inclusive
 * @param end_index   the ending index, exclusive
 * @param dst         storage for the created substring
 * @return status of the operation
 */
pcsl_string_status pcsl_string_substring(const pcsl_string * str,
			      jint begin_index, jint end_index,
			      pcsl_string * dst) {
  jsize sub_length = end_index - begin_index;

  if (dst == NULL || str == NULL || str->data == NULL) {
    return PCSL_STRING_EINVAL;
  }

  {
    jsize src_length = pcsl_string_length(str);
    if (src_length < 0 || begin_index < 0 ||
      sub_length < 0 || end_index > src_length) {
      return PCSL_STRING_EINVAL;
    }
  }

  if (sub_length == 0) {
    * dst = PCSL_STRING_EMPTY;
    return PCSL_STRING_OK;
  } else {
    const jchar * str_data = str->data;
    jchar * sub_buffer = NULL;

    sub_buffer = pcsl_mem_malloc((sub_length + 1) * sizeof(jchar));

    if (sub_buffer == NULL) {
      * dst = PCSL_STRING_NULL;
      return PCSL_STRING_ENOMEM;
    }

    memcpy(sub_buffer, str->data + begin_index,
	   sub_length * sizeof(jchar));

    /* Append the terminating zero. */
    sub_buffer[sub_length] = 0;

    dst->data = sub_buffer;
    dst->length = sub_length + 1;
    dst->flags = PCSL_STRING_IN_HEAP;

    return PCSL_STRING_OK;
  }
}

/**
 * Tests if this string starts with the specified prefix
 *
 * @param str    string to look in
 * @param prefix prefix to look for
 * @param offset offset from start
 * @return PCSL_TRUE if the string and the prefix are not NULL and
 *         the starts with the specified prefix, PCSL_FALSE otherwise
 */
jboolean pcsl_string_starts_with(const pcsl_string * str,
				 const pcsl_string * prefix) {
  if (prefix == NULL) { /* NULL is part of any string */
    return PCSL_TRUE;
  }

  if (str == NULL) { /* Null string couldn't contain not-null substring */
    return PCSL_FALSE;
  }

  /* NULL data is part of any other data */
  if (prefix->data == NULL) {
    return PCSL_TRUE;
  }

  if (str->data == NULL) { /* Null data couldn't contain not-null subdata */
    return PCSL_FALSE;
  }

  /* Substring couldn't be longer than target string */
  if (str->length < prefix->length) {
    return PCSL_FALSE;
  }

  /* Empty or null prefix is a part of any string */
  if (prefix->length < 2) {
    return PCSL_TRUE;
  }


  return (0 == memcmp(str->data, prefix->data,
                     /* Do not count terminating '\0' */
		     (prefix->length - 1) * sizeof(jchar)))
              ? PCSL_TRUE : PCSL_FALSE;
}

/**
 * Tests if this string ends with the specified suffix.
 *
 * @param str    string to look in
 * @param suffix suffix to look for
 * @return PCSL_TRUE if the string and the suffix are not NULL and
 *         the ends with the specified suffix, PCSL_FALSE otherwise
 */
jboolean pcsl_string_ends_with(const pcsl_string * str,
			       const pcsl_string * suffix) {
  if (suffix == NULL) { /* NULL is part of any string */
    return PCSL_TRUE;
  }

  if (str == NULL) { /* Null string couldn't contain not-null substring */
    return PCSL_FALSE;
  }

  /* NULL data is part of any other data */
  if (suffix->data == NULL) {
    return PCSL_TRUE;
  }

  if (str->data == NULL) { /* Null data couldn't contain not-null subdata */
    return PCSL_FALSE;
  }

  /* Substring couldn't be longer than target string */
  if (str->length < suffix->length) {
    return PCSL_FALSE;
  }

  /* Empty or null suffix is a part of any string */
  if (suffix->length < 2) {
    return PCSL_TRUE;
  }

  return memcmp(str->data + str->length - suffix->length, suffix->data,
		  suffix->length * sizeof(jchar)) == 0 ? PCSL_TRUE : PCSL_FALSE;
}

/**
 * Returns an index of the first occurrence of the abstract character
 * in the abstract character sequence specified by the given string.
 * The abstract character is specified by the Unicode code point.
 * <p>
 * Returns -1 if the specified integer value is not in the Unicode codespace,
 * or the specified integer value is a surrogate code point,
 * or the string is NULL, or the character doesn't occur.
 * Otherwise,
 * the returned index must be 0 <= index <= pcsl_string_length(str)-1.
 *
 * @param str   string to look in
 * @param c     Unicode code point to look for
 * @return index of the first occurence of the character
 */
jint pcsl_string_index_of(const pcsl_string * str, jint ch) {
  return pcsl_string_index_of_from(str, ch, 0);
}

/**
 * Returns an index of the first occurrence of the abstract character
 * in the abstract character sequence specified by the given string,
 * starting the search at the specified from_index.
 * The abstract character is specified by the Unicode code point.
 * <p>
 * Returns -1 if the specified integer value is not in the Unicode codespace,
 * or the specified integer value is a surrogate code point,
 * or the string is NULL, or the character doesn't occur.
 * Otherwise,
 * the returned index must be from_index <= index <= pcsl_string_length(str)-1.
 * <p>
 *
 * @param str   string to look in
 * @param c     Unicode code point to look for
 * @return index of the first occurence of the character in the string
 */
jint pcsl_string_index_of_from(const pcsl_string * str, jint ch,
			       jint from_index) {
  if (str == NULL || str->data == NULL || str->length == 0) {
    return -1;
  }

  if (from_index >= str->length) {
    return -1;
  }

  if (from_index < 0) {
    from_index = 0;
  }

  if (IS_BMP_CODE_POINT(ch)) {
    if (IS_SURROGATE_CODE_POINT(ch)) {
      return -1;
    } else {
      /* handle most cases here */
      const jchar * data = str->data + from_index;
      const jchar * data_end = str->data + str->length;
      for ( ; data < data_end; data++) {
	if (*data == ch) {
	  return data - str->data;
	}
      }
    }
  } else if (IS_UNICODE_CODE_POINT(ch)) {
    jchar code_unit[2];
    jsize unit_length;
    pcsl_string_status status = pcsl_code_point_to_utf16_code_unit(ch, code_unit, &unit_length);

    if (status != PCSL_STRING_OK) {
      return -1;
    } else {
      const jchar high_surrogate = code_unit[0];
      const jchar low_surrogate = code_unit[1];
      const jchar * data = str->data + from_index;
      const jchar * data_end = str->data + str->length - 1;
      for ( ; data < data_end; data++) {
	if (*data == high_surrogate && *(data + 1) == low_surrogate) {
	  return data - str->data;
	}
      }
    }
  } else {
    return -1;
  }

  return -1;
}

/**
 * Returns an index of the last occurrence of the abstract character
 * in the abstract character sequence specified by the given string.
 * The abstract character is specified by the Unicode code point.
 * <p>
 * Returns -1 if the specified integer value is not in the Unicode codespace,
 * or the specified integer value is a surrogate code point,
 * or the string is NULL, or the character doesn't occur.
 * Otherwise,
 * the returned index must be 0 <= index <= pcsl_string_length(str)-1.
 *
 * @param  str   string to look in
 * @param  c     Unicode code point to look for
 * @return index of the last occurence of the character in the string
 */
jint pcsl_string_last_index_of(const pcsl_string * str, jint ch) {
  if (str == NULL || str->data == NULL || str->length == 0) {
    return -1;
  }
  return pcsl_string_last_index_of_from(str, ch, str->length - 1);
}

/**
 * Returns an index of the last occurrence of the abstract character
 * in the abstract character sequence specified by the given string,
 * searching backward starting at the specified from_index.
 * The abstract character is specified by the Unicode code point.
 * <p>
 * Returns -1 if the specified integer value is not in the Unicode codespace,
 * or the specified integer value is a surrogate code point,
 * or the string is NULL, or the character doesn't occur.
 * Otherwise,
 * the returned index must be 0 <= index <= from_index.
 *
 * @param  str   string to look in
 * @param  c     Unicode code point to look for
 * @return index of the last occurence of the character in the string
 */
jint pcsl_string_last_index_of_from(const pcsl_string * str, jint ch,
				    jint from_index) {
  if (str == NULL || str->data == NULL || str->length == 0) {
    return -1;
  }

  if (from_index >= str->length) {
    from_index = str->length - 1;
  }

  if (from_index < 0) {
    return -1;
  }

  if (IS_BMP_CODE_POINT(ch)) {
    if (IS_SURROGATE_CODE_POINT(ch)) {
      return -1;
    } else {
      /* handle most cases here */
      const jchar * data = str->data + from_index;
      for ( ; data >= str->data; data--) {
	if (*data == ch) {
	  return data - str->data;
	}
      }
    }
  } else if (IS_UNICODE_CODE_POINT(ch)) {
    jchar code_unit[2];
    jsize unit_length;
    pcsl_string_status status = pcsl_code_point_to_utf16_code_unit(ch, code_unit, &unit_length);

    if (from_index >= str->length - 1) {
      from_index = str->length - 2;
    }

    if (status != PCSL_STRING_OK) {
      return -1;
    } else {
      const jchar high_surrogate = code_unit[0];
      const jchar low_surrogate = code_unit[1];
      const jchar * data = str->data + from_index;
      for ( ; data >= str->data; data--) {
	if (*data == high_surrogate && *(data + 1) == low_surrogate) {
	  return data - str->data;
	}
      }
    }
  } else {
    return -1;
  }

  return -1;
}

/**
 * Removes white spaces from both sides of the string.
 *
 * @param  str string to handle
 * @param  dst storage for the trimmed string
 * @return status of trimming
 */
pcsl_string_status pcsl_string_trim(const pcsl_string * str,
				  pcsl_string * dst) {
 if (str == NULL || dst == NULL ||
    (str->length > 0 && str->data == NULL)) {
    return PCSL_STRING_EINVAL;
  }

  if (str->length < 2) { /* null or empty string */
    if(pcsl_string_is_null(str)) {
      *dst = PCSL_STRING_NULL;
    } else {
      *dst = PCSL_STRING_EMPTY;
    }
    return PCSL_STRING_OK;
  }

  {
    /* the character that precedes the end-of-string zero */
    const jchar * data_start = str->data;
    const jchar * data_end = str->data + str->length - 2;
    jsize trimmed_length = 0;
    jchar * trimmed_buffer = NULL;

    // start spaces
    for ( ; data_end >= data_start &&
        ((*data_start == 0x9) || (*data_start == 0x20)); data_start++) {}
    // end spaces
    for ( ; data_end >= data_start &&
        ((*data_end == 0x9) || (*data_end == 0x20)); data_end--) {}

    trimmed_length = data_end - data_start + 2;

    trimmed_buffer = pcsl_mem_malloc(trimmed_length * sizeof(jchar));

    if (trimmed_buffer == NULL) {
      * dst = PCSL_STRING_NULL;
      return PCSL_STRING_ENOMEM;
    }

    (void)memcpy(trimmed_buffer, data_start, (trimmed_length - 1) * sizeof(jchar));
    /* Terminating zero */
    trimmed_buffer[trimmed_length - 1] = 0;

    dst->data   = trimmed_buffer;
    dst->length = trimmed_length;
    dst->flags  = PCSL_STRING_IN_HEAP;

    return PCSL_STRING_OK;
  }
}

/**
 * Removes white spaces from the end of the string.
 *
 * @param  str string to handle
 * @param  dst storage for the trimmed string
 * @return status of trimming
 */
pcsl_string_status pcsl_string_trim_from_end(const pcsl_string * str,
				  pcsl_string * dst) {
  if (str == NULL || dst == NULL ||
    (str->length > 0 && str->data == NULL)) {
    return PCSL_STRING_EINVAL;
  }

  if (str->length < 2) { /* null or empty string */
    if(pcsl_string_is_null(str)) {
      *dst = PCSL_STRING_NULL;
    } else {
      *dst = PCSL_STRING_EMPTY;
    }
    return PCSL_STRING_OK;
  }

  {
    /* the character that precedes the end-of-string zero */
    const jchar * data = str->data + str->length - 2;
    jsize trimmed_length = 0;
    jchar * trimmed_buffer = NULL;

    for ( ; data >= str->data && ((*data == 0x9) || (*data == 0x20)); data--) {}

    trimmed_length = data - str->data + 2;

    trimmed_buffer = pcsl_mem_malloc(trimmed_length * sizeof(jchar));

    if (trimmed_buffer == NULL) {
      * dst = PCSL_STRING_NULL;
      return PCSL_STRING_ENOMEM;
    }

    (void)memcpy(trimmed_buffer, str->data, (trimmed_length - 1) * sizeof(jchar));
    /* Terminating zero */
    trimmed_buffer[trimmed_length - 1] = 0;

    dst->data   = trimmed_buffer;
    dst->length = trimmed_length;
    dst->flags  = PCSL_STRING_IN_HEAP;

    return PCSL_STRING_OK;
  }
}

/**
 * Parses the string argument as a signed decimal integer. The
 * characters in the string must all be decimal digits, except that
 * the first character may be an ASCII minus sign to indicate
 * a negative value or plus to indicate positive value.
 * The value must fit into 32-bit signed integer.
 * <p>
 * The parsed integer value is written to the memory location specified by
 * 'value'. Fails if 'value' is NULL.
 * <p>
 * If parsing fails, nothing is written and the return value indicates the
 * failure status.
 *
 * @param str    the string to be parsed
 * @param value  storage for the parsed value
 * @return parsing status
 */
pcsl_string_status pcsl_string_convert_to_jint(const pcsl_string * str, jint * value) {
  jlong value_long;
  pcsl_string_status ret_val;
  if (value == NULL) {
    return PCSL_STRING_EINVAL;
  }
  ret_val = pcsl_string_convert_to_jlong(str, &value_long);
  if (ret_val == PCSL_STRING_OK) { /* check result */
    jint value_int = (jint)value_long;
    if ((jlong)value_int == value_long) {
      *value = value_int;
    } else {
      ret_val = PCSL_STRING_OVERFLOW;
    }
  }
  return ret_val;
}

/**
 * Creates a new string representing the specified 32-bit signed integer value.
 * The integer value is converted to signed decimal representation.
 * <p>
 * An address of the created string is written to the memory location specified
 * by 'str'. Fails if 'str' is NULL.
 *
 * @param   value the value to be converted
 * @param   str   storage for the created string
 * @return  conversion status
 */
pcsl_string_status pcsl_string_convert_from_jint(jint value, pcsl_string * str) {
  return pcsl_string_convert_from_jlong((jlong)value, str);
}

/**
 * Parses the string argument as a signed decimal integer. The
 * characters in the string must all be decimal digits, except that
 * the first character may be an ASCII minus sign to indicate
 * a negative value. The value must fit into 64-bit signed integer.
 * <p>
 * The parsed integer value is written to the memory location specified by
 * 'value'. Fails if 'value' is NULL.
 * <p>
 * If parsing fails, nothing is written and the return value indicates the
 * failure status.
 *
 * @param str    the string to be parsed
 * @param value  storage for the parsed value
 * @return parsing status
 */
/* 0xCCC,CCCC,CCCC,CCCC */
/* we don't write (jlong)0xCcccCccc to avoid conversion from negative signed */
#define MAXINT_DIV_10 ( (((jlong)0x0cccCccc) << 32 ) \
                      | (((jlong)0x33333333) << 2 ) \
                      )

#define LONG_SIZE_IN_BITS 64
#define MIN_LONG (((jlong)1) << (LONG_SIZE_IN_BITS - 1))

pcsl_string_status pcsl_string_convert_to_jlong(const pcsl_string * str, jlong * value) {
  jint length;
  if (str == NULL || str->data == NULL || value == NULL) {
    return PCSL_STRING_EINVAL;
  }
  {
    length = pcsl_string_length(str);
    /* String length doesn't include trailing zero character */
    if (length == 0) { /* Empty string */
      return PCSL_STRING_EINVAL;
    }
  }

  {
    const jchar* src = str->data;
    jlong ret_val = 0;
    int isNegative = 0;
    int i = 0;
    /* maybe first symbol is '-' or '+' */
    switch (src[0]) {
      case '-':
        isNegative = 1;
      case '+':
        i++;
        /* no break here */
    }
    /* loop along string */
    for ( ; i < length; i++) {
      if (ret_val > MAXINT_DIV_10 || ret_val < 0) {
        return PCSL_STRING_OVERFLOW;
      }
      if ( '0' <= src[i] && src[i] <= '9') {
        ret_val = ret_val * RADIX + (jlong)(src[i] - '0');
        if (ret_val < 0 && ret_val != MIN_LONG ) {
          /* arithmetic overflow */
          return PCSL_STRING_OVERFLOW;
        }
      } else {
        return PCSL_STRING_EINVAL; /* non-digit symbol */
      }
    }
    if (isNegative) {
      ret_val = - ret_val; /* once */
    } else if (ret_val == MIN_LONG) {
      /* arithmetic overflow */
      return PCSL_STRING_OVERFLOW;
    }
    *value = ret_val;
    return PCSL_STRING_OK;
  }
}

/**
 * Creates a new string representing the specified 64-bit signed integer value.
 * The integer value is converted to signed decimal representation.
 * <p>
 * An address of the created string is written to the memory location specified
 * by 'str'. Fails if 'str' is NULL.
 *
 * @param   value the value to be converted
 * @param   str   storage for the created string
 * @return  conversion status
 */
#define DIGIT(x) ('0'+((char)(x)))
/* Max length has MIN_LONG = -9223372036854775808 */
#define TEXT_BUF_FOR_LONG_SIZE 20
pcsl_string_status pcsl_string_convert_from_jlong(jlong value, pcsl_string * str) {
  if (str == NULL) {
    return PCSL_STRING_EINVAL;
  }

  {
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(PCSL_STRING_MIN_JLONG)
    { '-', '9', '2', '2', '3', '3', '7', '2', '0', '3', '6', '8',
      '5', '4', '7', '7', '5', '8', '0', '8', 0}
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(PCSL_STRING_MIN_JLONG);
    jchar buf[TEXT_BUF_FOR_LONG_SIZE];
    /* filling buffer from the end */
    int charPos = TEXT_BUF_FOR_LONG_SIZE - 1 ;
    int negative = (value < 0);
    jchar * new_buffer = NULL;
    int buf_length;
    /* MIN_LONG 0x8000000000000000 */
    if (negative && value == MIN_LONG) {
        *str = PCSL_STRING_MIN_JLONG;
        return PCSL_STRING_OK;
    }
    /* Platform-undependent division: dividend and divisor must be positive */
    if (negative) {
        value = -value;
    }
    /* Trailing 0 */
    buf[charPos--] = 0;
    while (value >= RADIX) {
      buf[charPos--] = DIGIT(value % RADIX);
      value /= RADIX;
    }
    buf[charPos] = DIGIT(value);
    if (negative) {
      buf[--charPos] = '-';
    }

    /* Allocate need memory */
    buf_length = sizeof(jchar) * (TEXT_BUF_FOR_LONG_SIZE - charPos);
    new_buffer = pcsl_mem_malloc(buf_length);
    if (new_buffer == NULL) {
      return PCSL_STRING_ENOMEM;
    }
    /* Filling the buffer */
    memcpy(new_buffer, &buf[charPos], buf_length);

    str->data    = new_buffer;
    str->length  = TEXT_BUF_FOR_LONG_SIZE - charPos;
    str->flags   = PCSL_STRING_IN_HEAP;

    return PCSL_STRING_OK;
  }
}

/**
 * Frees the string.
 * Sets the freed pcsl_string to PCSL_STRING_NULL.
 *
 * @param   str string to free
 * @return  status
 */
pcsl_string_status pcsl_string_free(pcsl_string * str) {
  if (str == NULL) {
    return PCSL_STRING_EINVAL;
  }

  if (str->flags & PCSL_STRING_IN_HEAP) {
    if (str->data != NULL) {
      pcsl_mem_free(str->data);
    }
  }

  * str = PCSL_STRING_NULL;

  return PCSL_STRING_OK;
}

/**
 * Returns UTF-8 representation for the specified string.
 * Returns NULL in case of failure.
 * <p>
 * If not NULL, the returned pointer points to an internal buffer that contains
 * UTF-8 representation of the string. You can safely read up to
 * <code>pcsl_string_utf8_length(str)</code> bytes from this buffer.
 * <p>
 * Do not write into the returned buffer, honor the 'const' modifier.
 * The implementation is not required to synchronize the buffer contents with
 * the actual string value.
 * <p>
 * To release the buffer, invoke <code>pcsl_string_release_utf8_data()</code>
 * when you are done and pass this pointer as an argument.
 *
 * @param str the string
 * @return UTF-8 representation for the specified string
 */
const jbyte * pcsl_string_get_utf8_data(const pcsl_string * str) {
  if (str == NULL) {
    return NULL;
  } else {
    /* The returned length doesn't include the terminating zero. */
    const jsize length = pcsl_string_utf8_length(str) + 1;
    jbyte * buffer = pcsl_mem_malloc(length * sizeof(jbyte));

    if (buffer == NULL) {
      return NULL;
    }

    if (pcsl_string_convert_to_utf8(str, buffer, length, NULL)
	!= PCSL_STRING_OK) {
      pcsl_mem_free(buffer);
      return NULL;
    }

    return buffer;
  }
}

/**
 * Releases the internal buffer that pointed to by <code>buf</code>.
 * The pointer must have been returned by a previous call to
 * <code>pcsl_string_get_utf8_data()</code>, otherwise the behavior is not
 * specified.
 *
 * @param buf pointer to the buffer
 * @param str the string from which this buffer was obtained
 */
void pcsl_string_release_utf8_data(const jbyte * buf,
				    const pcsl_string * str) {
  pcsl_mem_free((void*)buf);
}

/**
 * Returns UTF-16 representation for the specified string.
 * Returns NULL in case of failure.
 * <p>
 * If not NULL, the returned pointer points to an internal buffer that contains
 * UTF-16 representation of the string. You can safely read up to
 * <code>pcsl_string_utf16_length(str)</code> 16-bit units from this buffer.
 * <p>
 * Do not write into the returned buffer, honor the 'const' modifier.
 * The implementation is not required to synchronize the buffer contents with
 * the actual string value.
 * <p>
 * To release the buffer, invoke <code>pcsl_string_release_utf16_data()</code>
 * when you are done and pass this pointer as an argument.
 *
 * @param str the string
 * @return UTF-8 representation for the specified string
 */
const jchar * pcsl_string_get_utf16_data(const pcsl_string * str) {
  if (str != NULL) {
    return str->data;
  }
  return NULL;
}

/**
 * Releases the internal buffer that pointed to by <code>buf</code>.
 * The pointer must have been returned by a previous call to
 * <code>pcsl_string_get_data_utf16()</code>, otherwise the behavior is not
 * specified.
 *
 * @param buf pointer to the buffer
 * @param str the string from which this buffer was obtained
 */
void pcsl_string_release_utf16_data(const jchar * buf,
				    const pcsl_string * str) {
  (void)buf;
  (void)str;
}

/**
 * Compares the given string with PCSL_STRING_NULL.
 *
 * @param str the string to compare
 * @return PCSL_TRUE if str is not NULL and is equal to PCSL_STRING_NULL,
 * PCSL_FALSE otherwise
 */
jboolean pcsl_string_is_null(const pcsl_string * str) {
  return (str != NULL && str->data == NULL) ? PCSL_TRUE : PCSL_FALSE;
}

static jchar empty_string_data = 0;

/* Empty zero-terminated string */
const pcsl_string PCSL_STRING_EMPTY =
  { &empty_string_data, 1, 0 };

/* NULL string */
const pcsl_string PCSL_STRING_NULL =
  { NULL, 0, 0 };

