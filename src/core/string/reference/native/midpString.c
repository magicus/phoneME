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

#include <string.h>
#include <midpMalloc.h>
#include <midpString.h>

#include <pcsl_memory.h>

/**
 * @file
 *
 * This is the native Unicode string API. It is modeled after the
 * java.lang.String class.
 */

/** Global NULL string. */
const MidpString NULL_MIDP_STRING = { NULL_LEN, NULL };

/** Global empty string. */
const MidpString EMPTY_MIDP_STRING = { 0, NULL };

/**
 * Concatenates 2 unicode strings into 1 string. Free the results data
 * field with midpFree.
 * 
 * @param str1 first string
 * @param str2 second string
 *
 * @return result of first + second, len is OUT_OF_MEM_STR_LEN if out of memory
 */
MidpString midpStringCatImpl(MidpString str1, MidpString str2,
        char* filename, int line) {
    MidpString result;

    (void)filename;                               /* Avoid compiler warnings */
    (void)line;                                   /* Avoid compiler warnings */

    if (str1.len < 0) {
        str1.len = 0;
    }

    if (str2.len < 0) {
        str2.len = 0;
    }

    result.len = str1.len + str2.len;
    if (result.len == 0) {
        result.data = NULL;
        return result;
    }

    result.data = (jchar*)midpMallocImpl(result.len * sizeof (jchar),
                                         filename, line);
    if (result.data == NULL) {
        result.len = OUT_OF_MEM_LEN;
        return result;
    }

    memcpy(result.data, str1.data, str1.len * sizeof (jchar));
    memcpy(result.data + str1.len, str2.data, str2.len * sizeof (jchar));
    return result;
}

/**
 * Compares 2 unicode strings.
 * 
 * @param str1 first string
 * @param str2 second string
 *
 * @return 0 if str1 = str2, < 0 if str1 < str2, > 0 if str1 > str2
 */
int midpStringCmp(MidpString str1, MidpString str2) {
    int i;

    for (i = 0; i < str1.len && i < str2.len; i++) {
        if (str1.data[i] < str2.data[i]) {
            return -1;
        }

        if (str1.data[i] > str2.data[i]) {
            return 1;
        }
    }

    if (str1.len < str2.len) {
        return -1;
    }

    if (str1.len > str2.len) {
        return 1;
    }

    return 0;
}

/**
 * Check to see if 2 unicode strings are equal.
 * 
 * @param str1 first string
 * @param str2 second string
 *
 * @return true if equal false if not
 */
int midpStringEquals(MidpString str1, MidpString str2) {
    return midpStringCmp(str1, str2) == 0;
}

/**
 * Check to see if one string ends with another.
 * 
 * @param str1 first string
 * @param str2 second string
 *
 * @return 1 if str1 ends with str2, else 0
 */
int midpStringEndsWith(MidpString str1, MidpString str2) {
    int i;
    int j;

    if (str1.len < str2.len) {
        return 0;
    }

    if (str2.len == 0) {
        return 1;
    }

    for (i = str1.len - 1, j = str2.len - 1; j >= 0; i--, j--) {
        if (str1.data[i] != str2.data[j]) {
            return 0;
        }
    }

    return 1;
}

/**
 * Convert a jchar string to a C string.
 * This function should not be used directly,
 * use the midpJcharsToChars macro.
 *
 * @param in jchar string
 * @param filename provided by the midpJcharsToChars macro
 * @param line provided by the midpJcharsToChars macro
 *
 * @return C string or NULL
 */
char* midpJcharsToCharsImpl(MidpString in, char* filename, int line) {
    char* out;
    int i;

    (void)filename;                               /* Avoid compiler warnings */
    (void)line;                                   /* Avoid compiler warnings */

    if (in.len < 0) {
        return NULL;
    }

    out = (char*)midpMallocImpl(in.len + 1, filename, line);
    if (out == NULL) {
        return NULL;
    }

    for (i = 0; i < in.len; i++) {
        out[i] = (char)in.data[i];
    }

    out[i] = 0;

    return out;
}

/**
 * Convert a C string to a jchar string.
 * This function should not be used directly,
 * use the midpCharsToJchars macro.
 *
 * @param in C string
 * @param filename provided by the midpCharsToJchars macro
 * @param line provided by the midpCharsToJchars macro
 *
 * @return jchar string
 */
MidpString midpCharsToJcharsImpl(char* in, char* filename, int line) {
    MidpString out;
    int i;

    (void)filename;                               /* Avoid compiler warnings */
    (void)line;                                   /* Avoid compiler warnings */

    if (in == NULL) {
        out.len = NULL_LEN;
        out.data = NULL;
        return out;
    }

    out.len = strlen(in);
    if (out.len == 0) {
        out.data = NULL;
        return out;
    }

    out.data = (jchar*)midpMallocImpl(out.len * sizeof (jchar), filename,
                                      line);
    if (out.data == NULL) {
        out.len = OUT_OF_MEM_LEN;
        return out;
    }

    for (i = 0; i < out.len; i++) {
        out.data[i] = (jchar)in[i];
    }

    return out;
}

/**
 * Duplicates a unicode strings. Free the results data
 * field with midpFree.
 * This function should not be used directly,
 * use the midpStringDup macro.
 * 
 * @param in input string
 * @param filename provided by the midpStringDup macro
 * @param line provided by the midpStringDup macro
 *
 * @return copy of the input string
 */
MidpString midpStringDupImpl(MidpString in, char* filename, int line) {
    return midpSubStringImpl(in, 0, in.len, filename, line);
}

/**
 * Frees a MIDP string.
 *
 * @param str string to free
 */
void midpFreeString(MidpString str) {
    if (str.len <= 0 || str.data == NULL) {
        return;
    }

    midpFree(str.data);
}

/**
 * Returns a new string that is a substring of this string. The
 * substring begins at the specified <code>beginIndex</code> and
 * extends to the character at index <code>endIndex - 1</code>.
 * Thus the length of the substring is <code>endIndex-beginIndex</code>.
 * <p>
 * Examples:
 * <blockquote><pre>
 * "hamburger".substring(4, 8) returns "urge"
 * "smiles".substring(1, 5) returns "mile"
 * </pre></blockquote>
 *
 * This function should not be used directly,
 * use the midpStringDup macro.
 *
 * @param in input string
 * @param beginIndex the beginning index, inclusive.
 * @param endIndex the ending index, exclusive.
 * @param filename provided by the midpSubString macro
 * @param line provided by the midpSubString macro
 *
 * @return     the specified substring, or a string with a length of
 *             OUT_OF_MEM_LEN.
 */
MidpString midpSubStringImpl(MidpString in, int beginIndex, int endIndex,
        char* filename, int line) {
    MidpString result;

    (void)filename;                               /* Avoid compiler warnings */
    (void)line;                                   /* Avoid compiler warnings */

    result.data = NULL;
    if (in.len < 0 || endIndex > in.len || beginIndex < 0 ||
            beginIndex >= endIndex) {
        result.len = NULL_LEN;
        return result;
    }

    if (in.len == 0) {
        result.len = 0;
        return result;
    }

    result.len = endIndex - beginIndex;
    result.data = (jchar*)midpMallocImpl(result.len * sizeof (jchar),
                  filename, line);
    if (result.data == NULL) {
        result.len = OUT_OF_MEM_LEN;
        return result;
    }

    memcpy(result.data, in.data + beginIndex, result.len * sizeof (jchar));
    return result;
}

/**
 * Returns the index within this string of the last occurrence of the
 * specified character. That is, the index returned is the largest
 * value <i>k</i> such that:
 * <blockquote><pre>
 * string.data[<i>k</i>] == ch
 * </pre></blockquote>
 * is true.
 * The String is searched backwards starting at the last character.
 *
 * @param   ch   a character.
 * @return  the index of the last occurrence of the character in the
 *          character sequence represented by this object, or
 *          <code>-1</code> if the character does not occur.
 */
int midpStringLastIndexOf(MidpString str, jchar ch) {
    int i;

    if (str.len <= 0) {
        return  -1;
    }

    for (i = str.len - 1; i >= 0; i--) {
        if (str.data[i] == ch) {
            return i;
        }
    }

    return -1;
}

 /**
 * looks for first occurrence of <PARAM>c</PARAM> in the <PARAM>line</PARAM> 
 * 
 * @param c    character to look for
 * @param line   MidpString to search in
 * @return index of c
 */
int midpStringIndexOf(MidpString line, jchar c) {
    int index = 0;

    if (line.len <= 0) {
        return -1;
    }

    while ((index < line.len) && (*(line.data+index) != c)) {
        index++;
    }

    if (index == line.len) { /* c not found */
        return -1;
    }

    return index;
} /* end of midpStringIndexOf */

/**
 * removes white spaces from the end of string
 * 
 * @param line   string to trim
 * @return Trimmed string and error code
 */
MidpString midpStringTrimFromEnd(MidpString line) {

    if (line.len <= 0) {
        line.len = BAD_PARAMS;
        return line;
    }

    while (IS_WHITE_SPACE((line.data+(line.len-1))) && (line.len > 0)) {
        *(line.data+(line.len-1)) = 0x00;
        line.len--;
    }
    return line;
}/* end of midpStringTrimFromEnd */

/**
 * Removes white spaces from the both ends of a MidpString.
 * <p>
 * Memory allocation done here, even if the result is the same as the input.
 * <p>
 * This function should not be used directly,
 * use the midpNewString macro.
 * 
 * @param line   MidpString to handle
 * @param filename provided by the midpStringTrim macro
 * @param line provided by the midpStringTrim macro
 *
 * @return new string
 */
MidpString midpStringTrimImpl(MidpString in, char* filename, int line) {
    int i;
    int end;
    int begin;

    if (in.len <= 0) {
        return NULL_MIDP_STRING;
    }

    for (i = 0; i < in.len; i++) {
        if (!(IS_CHAR_WHITE_SPACE(in.data[i]))) {
            break;
        }
    }

    begin = i;
    end = in.len;

    for (i = in.len; i > begin; i--) {
        if (!(IS_CHAR_WHITE_SPACE(in.data[i - 1]))) {
            break;
        }
    }

    end = i;

    return midpSubStringImpl(in, begin, end, filename, line);
}/* end of midpStringTrimImpl */

/**
 * Converts integer to MidpString in decimal format.
 * Memory allocation done here.
 * 
 * @param inputInt integer to convert
 * @return MidpString representation of the inputInt
 */
MidpString midpStringValueOfInt(int inputInt) {
    int base = 10;
    int neg = 0;
    int len = 0;
    jchar buffer[21];
    jchar *pstr = buffer;
    MidpString res = {0, NULL};

    if (inputInt < 0) {
        neg = 1;
        inputInt*=(-1);
    }

    do {
        *pstr++ = ((inputInt % base)+'0');
        inputInt = inputInt/base;
        len++;
    } while (inputInt > 0);

    pstr--;

    if (neg) {
        *++pstr = '-';
        len++;
    }

    res.data = (jchar*)midpMalloc((len+1)*sizeof(jchar));
    if (!res.data) {
        return res;
    }
    memset(res.data,0,(len+1)*sizeof(jchar));

    res.len = len;
    base = 0;
    while (len-- > 0) {
        *(res.data+base++) = *pstr--;
    }
    return res;
} /* midpStringValueOfInt */

/**
 * Converts MidpString representation of decimal integer to integer.
 * 
 * @param str    MidpString representation of integer
 * @return integer value of str
 */
int midpIntegerParseInt(MidpString str) {
    int res = 0;
    int td = 1;
    jchar* p = str.data;

    if (str.len <= 0) {
        return NUMBER_ERROR;
    }

    p+=str.len-1;

    while (p >= str.data) {

        if ((*p >= '0') && (*p <= '9')) { /* range between 0 to 9 */
            res += ((*p)-'0')*td;
            td*=10;
        } else {
            return NUMBER_ERROR;
        }
        p--;
    } /* end of while */
    return res;
} /* midpIntegerParseInt */


/**
 * Frees a string list.
 *
 * @param pStrings point to an array of suites
 * @param numberOfStrings number of elements in pStrings
 */
void
freeStringList(MidpString* pStrings, int numberOfStrings) {
    int i;

    if (pStrings == NULL) {
        return;
    }

    for (i = 0; i < numberOfStrings; i++) {
        midpFree(pStrings[i].data);
    }

    midpFree(pStrings);
}
void
free_pcsl_string_list(pcsl_string* pStrings, int numberOfStrings) {
    int i;
    if (pStrings == NULL) {
        return;
    }

    for (i = 0; i < numberOfStrings; i++) {
        pcsl_string_free(&pStrings[i]);
    }

    midpFree(pStrings);
}

/**
 * Allocate an array of pcsl_string's.
 * Initialize the strings with PCSL_STRING_NULL.
 *
 * @param numberOfStrings desired array size
 * @return address of the array, or NULL if out-of-memory error happened
 */
pcsl_string* alloc_pcsl_string_list(int numberOfStrings)
{
    pcsl_string* res = (pcsl_string*)midpMalloc(numberOfStrings * sizeof (pcsl_string));
    if (NULL != res) {
        int i;
        for(i=0; i<numberOfStrings; i++) {
            res[i] = PCSL_STRING_NULL;
        }
    }
    return res;
}

/**
 * Create pcsl_string from the specified MidpString.
 * The caller is responsible for freeing the created string when done.
 *
 * @param midpString pointer to the MidpString instance
 * @param pcslString pointer to the pcsl_string instance
 * @return status of the operation
 */
pcsl_string_status midp_string_to_pcsl_string(const MidpString * midpString,
					      pcsl_string * pcslString) {
  if (pcslString == NULL) {
    return PCSL_STRING_EINVAL;
  } 

  if (NULL == midpString) {
    *pcslString = PCSL_STRING_NULL;
    return PCSL_STRING_EINVAL;
  } else if(NULL == midpString->data) {
    *pcslString = PCSL_STRING_NULL;
    return PCSL_STRING_OK;
  } else {
    const jsize length = midpString->len;
    if (length > 0) {
      return pcsl_string_convert_from_utf16(midpString->data, midpString->len, 
					    pcslString);
    } else if (length == 0) {
      *pcslString = PCSL_STRING_EMPTY;
    } else {
      /* need revisit: what we should return in this case. */
      /* A negative length means a run-time error. */
      *pcslString = PCSL_STRING_NULL;
    }
    return PCSL_STRING_OK;
  }
}

/**
 * Create MidpString from the specified pcsl_string.
 * The caller is responsible for freeing the created string when done.
 *
 * @param pcslString pointer to the pcsl_string instance
 * @param midpString pointer to the MidpString instance
 * @return status of the operation
 */
pcsl_string_status midp_string_from_pcsl_string(const pcsl_string * pcslString,
						MidpString * midpString) {
  if (midpString == NULL || pcslString == NULL) {
    return PCSL_STRING_EINVAL;
  } else if (pcsl_string_is_null(pcslString)) {
      * midpString = NULL_MIDP_STRING;
      return PCSL_STRING_OK;
  } else {
    const jsize length = pcsl_string_utf16_length(pcslString);

    if (length == 0) {
      * midpString = EMPTY_MIDP_STRING;
      return PCSL_STRING_OK;
    } else {
      pcsl_string_status status = PCSL_STRING_OK;
      /* Reserve space for terminating zero */
      const jsize buffer_length = length + 1;
      jchar * buffer = pcsl_mem_malloc(buffer_length * sizeof(jchar));

      if (buffer == NULL) {
        return PCSL_STRING_ENOMEM;
      }

      status = pcsl_string_convert_to_utf16(pcslString,
                                            buffer, buffer_length, NULL);

      if (status != PCSL_STRING_OK) {
        * midpString = NULL_MIDP_STRING;
        pcsl_mem_free(buffer);
      } else {
        midpString->data = buffer;
        midpString->len = length;
      }

      return status;
    }
  }
}

/**
 * Allocate a buffer of jchars with the text from the specified pcsl_string.
 * The caller is responsible for freeing the buffer with midpFree when done.
 *
 * @param pcslString pointer to the pcsl_string instance
 * @param pBuf receives the allocated buffer
 * @param pBufLen receives the size (in jchars) of the buffer
 * @return status of the operation
 */
pcsl_string_status text_buffer_from_pcsl_string(const pcsl_string * pcslString,
						jchar * * pBuf, int * pBufLen) {
    *pBuf = NULL;
    *pBufLen = -1;
    if (pcsl_string_is_null(pcslString)) {
        return PCSL_STRING_OK;
    } else {
        const jsize length = pcsl_string_utf16_length(pcslString);
        pcsl_string_status status = PCSL_STRING_OK;
        /* Reserve space for terminating zero */
        const jsize buffer_length = length + 1;
        jchar * buffer = midpMalloc(buffer_length * sizeof(jchar));

        if (buffer == NULL) {
            return PCSL_STRING_ENOMEM;
        }

        status = pcsl_string_convert_to_utf16(pcslString,
                                            buffer, buffer_length, NULL);

        if (status == PCSL_STRING_OK) {
            *pBuf = buffer;
            *pBufLen = length;
        } else {
            midpFree(buffer);
        }
        return status;
    }
}
/**
  An analog of midp_string_from_pcsl_string that works only with utf-16 implementation
  of pcsl_string.
  DO NOT FREE THE RETURNED STRING!!!!
  Sometimes we need to get a MidpString for a string whose pcsl_string representation
  is stored into some structure.
  Such strings are not free'd by the code that accesses them.
  TO BE REMOVED AS SOON AS POSSIBLE.
  (mg's note: I had to choose between memory leaks, inventing a MidpString cache for
  array of pcsl_strings, and accessing the internal details. I chose the latter.)
*/
pcsl_string_status midp_string_from_pcsl_string_hack(const pcsl_string * pcslString,
						MidpString * midpString) {
  if (midpString == NULL || pcslString == NULL) {
    return PCSL_STRING_EINVAL;
  } else if (pcsl_string_is_null(pcslString)) {
      * midpString = NULL_MIDP_STRING;
      return PCSL_STRING_OK;
  } else if (0 == pcsl_string_length(pcslString)) {
      * midpString = EMPTY_MIDP_STRING;
      return PCSL_STRING_OK;
  } else {
    /* this is absoluteyl incorrect.... */
    midpString->data = pcslString->data;
    midpString->len = pcsl_string_length(pcslString);
    return PCSL_STRING_OK;
  }
}

