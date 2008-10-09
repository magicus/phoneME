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
#include <utf.h>

#define UTF16_ERROR_CHAR (0xFFFD)
pcsl_string_status pcsl_utf8_convert_to_utf16(const jbyte * str, jsize str_length,
			      jchar * buffer, jsize buffer_length,
			      jsize * converted_length) {
  const jbyte * const str_end = str + str_length;

  jbyte byte1 = 0;
  jbyte byte2 = 0;
  jbyte byte3 = 0;
  jbyte byte4 = 0;
  jchar output_char[2] = { 0 };
  jsize output_size = 0;
  jsize output_off = 0;
  jboolean buffer_overflow = PCSL_FALSE;
  jboolean bad_sequence = PCSL_FALSE;

  if (str == NULL) {
    return PCSL_STRING_EINVAL;
  }

  while (str < str_end) {
    byte1 = *str++;

    if ((byte1 & 0x80) == 0){
      /*  7 bits: 0xxx xxxx */
      output_char[0] = (jchar)byte1;
      output_size = 1;
    } else if ((byte1 & 0xe0) == 0xc0) {
      /* 11 bits: 110x xxxx   10xx xxxx */
      if (str >= str_end) {
        bad_sequence = PCSL_TRUE;
        output_char[0] = UTF16_ERROR_CHAR;
        output_size = 1;
      } else {
          byte2 = *str++;
          if ((byte2 & 0xc0) != 0x80) {
            --str;
            bad_sequence = PCSL_TRUE;
            output_char[0] = UTF16_ERROR_CHAR;
            output_size = 1;
          } else {
            output_char[0] = (jchar)(((byte1 & 0x1f) << 6) | (byte2 & 0x3f));
            output_size = 1;
        }
      }
    } else if ((byte1 & 0xf0) == 0xe0){
      /* 16 bits: 1110 xxxx  10xx xxxx  10xx xxxx */
      if (str + 1 >= str_end) {
        bad_sequence = PCSL_TRUE;
        output_char[0] = UTF16_ERROR_CHAR;
        output_size = 1;
      } else {
          byte2 = *str++;
          byte3 = *str++;
          if ((byte2 & 0xc0) != 0x80 || (byte3 & 0xc0) != 0x80) {
            if ((byte2 & 0xc0) != 0x80) { str-=2; }
            else { --str; }
            bad_sequence = PCSL_TRUE;
            output_char[0] = UTF16_ERROR_CHAR;
            output_size = 1;
          } else {
              output_char[0] = (jchar)(((byte1 & 0x0f) << 12)
                         | ((byte2 & 0x3f) << 6)
                         | (byte3 & 0x3f));
              output_size = 1;
          }
      }
    } else if ((byte1 & 0xf8) == 0xf0) {
      /* 21 bits: 1111 0xxx  10xx xxxx  10xx xxxx  10xx xxxx */
      if (str + 2 >= str_end) {
        bad_sequence = PCSL_TRUE;
        output_char[0] = UTF16_ERROR_CHAR;
        output_size = 1;
      } else {
          byte2 = *str++;
          byte3 = *str++;
          byte4 = *str++;

          if ((byte2 & 0xc0) != 0x80 ||
              (byte3 & 0xc0) != 0x80 ||
              (byte4 & 0xc0) != 0x80) {
            if ((byte2 & 0xc0) != 0x80) { str-=3; }
            else if ((byte3 & 0xc0) != 0x80) { str-=2; }
            else { --str; }

            bad_sequence = PCSL_TRUE;
            output_char[0] = UTF16_ERROR_CHAR;
            output_size = 1;
          } else {
            // this byte sequence is UTF16 character
            jint ucs4 = (jint)(0x07 & byte1) << 18 |
              (jint)(0x3f & byte2) << 12 |
              (jint)(0x3f & byte3) <<  6 |
              (jint)(0x3f & byte4);
            output_char[0] = (jchar)((ucs4 - 0x10000) / 0x400 + 0xd800);
            output_char[1] = (jchar)((ucs4 - 0x10000) % 0x400 + 0xdc00);
            output_size = 2;
          }
      }
    } else {
      /* remove up to two more follow-up bytes: */
      /* total at most 3 follow-up bytes may belong to the same character */
      const int remaining = str_end-str;
      int n = remaining > 2 ? 2 : remaining;
      while (n-- && (0xc0 & *str) == 0x80) str++;

      bad_sequence = PCSL_TRUE;
      output_char[0] = UTF16_ERROR_CHAR;
      output_size = 1;
    }

    if (buffer_overflow == PCSL_FALSE && buffer != NULL) {
      if (output_off + output_size > buffer_length) {
        buffer_overflow = PCSL_TRUE;
      } else {
        /* do not need any loop because output_size is either 1 or 2 */
        buffer[output_off] = output_char[0];
        if (output_size == 2) {
            buffer[output_off + 1] = output_char[1];
        }
      }
    }

    output_off += output_size;
  } /* while */

  if (converted_length != NULL) {
    *converted_length = output_off;
  }

  return buffer_overflow == PCSL_TRUE ? PCSL_STRING_BUFFER_OVERFLOW
       : bad_sequence == PCSL_TRUE ? PCSL_STRING_EILSEQ
       : PCSL_STRING_OK;
}

#define UTF8_ERROR_CHAR ('?')
pcsl_string_status pcsl_utf16_convert_to_utf8(const jchar * str, jsize str_length,
			      jbyte * buffer, jsize buffer_length,
			      jsize * converted_length) {
  const jchar * const str_end = str + str_length;
  const jbyte * const buffer_end = buffer + buffer_length;

  jchar input_char = 0;
  jbyte output_byte[6] = { 0 };
  jsize output_size = 0;
  jsize output_off = 0;
  jboolean buffer_overflow = PCSL_FALSE;
  jboolean bad_sequence = PCSL_FALSE;

  if (str == NULL) {
    return PCSL_STRING_EINVAL;
  }

  while(str < str_end) {
    input_char = *str++;
    if (input_char < 0x80) {
      output_byte[0] = (jbyte)input_char;
      output_size = 1;
    } else if (input_char < 0x800) {
      output_byte[0] = (jbyte)(0xc0 | ((input_char >> 6) & 0x1f));
      output_byte[1] = (jbyte)(0x80 | (input_char & 0x3f));
      output_size = 2;
    } else if (input_char >= 0xd800 && input_char <= 0xdbff) {
      /* this is <high-half zone code> in UTF-16 */
      if (str >= str_end) {
        bad_sequence = PCSL_TRUE;
        output_byte[0] = (jbyte)UTF8_ERROR_CHAR;
        output_size = 1;
      } else {
        /* check next char is valid <low-half zone code> */
        jchar low_char = *str++;

        if (low_char < 0xdc00 || low_char > 0xdfff) {
          --str;
          bad_sequence = PCSL_TRUE;
          output_byte[0] = (jbyte)UTF8_ERROR_CHAR;
          output_size = 1;
        } else {
          int ucs4 =
            (input_char - 0xd800) * 0x400 + (low_char - 0xdc00) + 0x10000;
          output_byte[0] = (jbyte)(0xf0 | ((ucs4 >> 18)) & 0x07);
          output_byte[1] = (jbyte)(0x80 | ((ucs4 >> 12) & 0x3f));
          output_byte[2] = (jbyte)(0x80 | ((ucs4 >> 6) & 0x3f));
          output_byte[3] = (jbyte)(0x80 | (ucs4 & 0x3f));
          output_size = 4;
        }
      }
    } else if (input_char >= 0xdc00 && input_char <= 0xdfff) {
        bad_sequence = PCSL_TRUE;
        output_byte[0] = (jbyte)UTF8_ERROR_CHAR;
        output_size = 1;
    } else {
      output_byte[0] = (jbyte)(0xe0 | ((input_char >> 12)) & 0x0f);
      output_byte[1] = (jbyte)(0x80 | ((input_char >> 6) & 0x3f));
      output_byte[2] = (jbyte)(0x80 | (input_char & 0x3f));
      output_size = 3;
    }

    if (buffer_overflow == PCSL_FALSE && buffer != NULL) {
      if (output_off + output_size > buffer_length) {
        buffer_overflow = PCSL_TRUE;
      } else {
        int i;
        for (i = 0; i < output_size; i++) {
          buffer[output_off + i] = output_byte[i];
        }
      }
    }


    output_off += output_size;
  }

  if (converted_length != NULL) {
    *converted_length = output_off;
  }

  return buffer_overflow == PCSL_TRUE ? PCSL_STRING_BUFFER_OVERFLOW
       : bad_sequence == PCSL_TRUE ? PCSL_STRING_EILSEQ
       : PCSL_STRING_OK;
}

/**
 * Converts the Unicode code point to UTF-16 code unit.
 * See Unicode Glossary at http://www.unicode.org/glossary/.
 * High surrogate is stored in code_unit[0],
 * low surrogate is stored in code_unit[1].
 *
 * @param code_point  Unicode code point
 * @param code_unit   Storage for UTF-16 code unit
 * @param unit_length Storage for the number of 16-bit units
 *                    in the UTF-16 code unit
 * @return status of the conversion
 */
pcsl_string_status pcsl_code_point_to_utf16_code_unit(jint code_point,
				      jchar code_unit[2],
				      jsize * unit_length) {

  if ((code_point & 0xffff0000) == 0) {
    // handle most cases here (ch is a BMP code point)
    * unit_length = 1;
    code_unit[0] = (jchar)(code_point & 0xffff);
    return PCSL_STRING_OK;
  } else if (code_point < 0 || code_point > 0x10ffff) {
    return PCSL_STRING_EINVAL;
  } else {
    const jint offset = code_point - 0x10000;
    code_unit[0] = (jchar)((offset >> 10) + 0xd800);
    code_unit[1] = (jchar)((offset & 0x3ff) + 0xdc00);
    * unit_length = 2;
    return PCSL_STRING_OK;
  }
}

/**
 * Returns the number of abstract characters in the string specified
 * by the UTF-16 code unit sequence.
 * Returns -1 if str is NULL or is not a valid UTF-16 code unit sequence.
 * See Unicode Glossary at http://www.unicode.org/glossary/.
 *
 * @param str           UTF-16 code unit sequence
 * @param str_length    number of UTF-16 code units in the sequence
 * @return number of abstract characters in the string
 */
jsize utf16_string_length(jchar * str, jsize str_length) {
  const jchar * const str_end = str + str_length;
  jsize char_count = 0;
  jchar input_char = 0;

  if (str == NULL) {
    return -1;
  }

  while(str < str_end) {
    input_char = *str++;
    char_count++;
    if (input_char >= 0xd800 && input_char <= 0xdbff) {
      // this is <high-half zone code> in UTF-16
      if (str >= str_end) {
	return -1;
      } else {
	// check next char is valid <low-half zone code>
	jchar low_char = *str++;

	if (low_char < 0xdc00 || low_char > 0xdfff) {
	  return -1;
	}
      }
    }
  }

  return char_count;
}
