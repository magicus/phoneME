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
#include <stdio.h>
#include <pcsl_memory.h>
#include <pcsl_string.h>
#include <donuts.h>

#define LONG_SIZE_IN_BITS 64
#define MIN_LONG (((jlong)1) << (LONG_SIZE_IN_BITS - 1))
#define MAX_LONG (~MIN_LONG)

#define STATIC_ASCII_LITERAL        {'a','b','c','d','e','f','\0'}
#define EXTERN_ASCII_LITERAL        {'a','b','c','d','e','f','g','h','i','j','k','l','\0'}
#define STATIC_LOCAL_ASCII_LITERAL  {'a','a','a','b','b','b','\0'}
#define LOCAL_ASCII_LITERAL         {'a','a','a','b','b','b','c','c','c','d','d','d','\0'}

const jbyte malformed_utf8_bytes[] = { (jbyte)0xc0, (jbyte)0xc0 };
const jsize malformed_utf8_bytes_length = 
  sizeof(malformed_utf8_bytes) / sizeof(jbyte);

const jchar malformed_utf16_chars[] = { (jchar)0xd800, (jchar)0x1000 };
const jsize malformed_utf16_chars_length = 
  sizeof(malformed_utf16_chars) / sizeof(jchar);

// A UTF-16 code unit sequence that represents 1 abstract character.
const jchar surrogate_utf16_chars[] = { (jchar)0xd800, (jchar)0xdc00 };
const jsize surrogate_utf16_chars_length = 
  sizeof(surrogate_utf16_chars) / sizeof(jchar);
const jsize surrogate_string_length = 1;
const jint surrogate_code_point = 0x10000;

const jchar app_surrogate_utf16_chars[] = 
  { (jchar)0x30, (jchar)0xd800, (jchar)0xdc00 };
const jsize app_surrogate_utf16_chars_length = 
  sizeof(app_surrogate_utf16_chars) / sizeof(jchar);
const jsize app_surrogate_string_length = 2;

/* 07 bits: 0xxx xxxx  -- 7f */
/* 11 bits: 110x xxxx   10xx xxxx  -- 7ff */
/* 16 bits: 1110 xxxx  10xx xxxx  10xx xxxx  -- ffff */
/* 21 bits: 1111 0xxx  10xx xxxx  10xx xxxx  10xx xxxx -- 10_ffff */
/*
 * IMPL_NOTE not all the below code points are valid unicode characters,
 * but it's not important for testing our algorithm
 */
const jchar testutf_string_16[] =
  { (jchar)0x7f, (jchar)0x80, (jchar)0x81,
    (jchar)0x7ff, (jchar)0x800, (jchar)0x801,
    (jchar)0xffff, (jchar)0xd800, (jchar)0xdc00, (jchar)0xd800, (jchar)0xdc01,
    (jchar)0xdbff, (jchar)0xdfff
  };
const jsize testutf_string_16_length =
  sizeof(testutf_string_16) / sizeof(jchar);

const jbyte testutf_string_8[] =
  { 0x7f,  0xc2, 0x80,  0xc2, 0x81,
    0xdf, 0xbf,  0xe0, 0xa0, 0x80,  0xe0, 0xa0, 0x81,
    0xef, 0xbf, 0xbf,  0xf0, 0x90, 0x80, 0x80,  0xf0, 0x90, 0x80, 0x81,
    0xf4, 0x8f, 0xbf, 0xbf
  };
const jsize testutf_string_8_length =
  sizeof(testutf_string_8) / sizeof(jbyte);

const jchar bad16_string_16[] =
  { (jchar)0xd800, (jchar)0xdc00,
    (jchar)0xd800,
    (jchar)0xd800, (jchar)0xdc01,
    (jchar)0xdc00,
    (jchar)0xdbff, (jchar)0xdfff
  };
const jsize bad16_string_16_length =
  sizeof(bad16_string_16) / sizeof(jchar);

const jbyte bad16_string_8[] =
  { 0xf0, 0x90, 0x80, 0x80,
    '?',
    0xf0, 0x90, 0x80, 0x81,
    '?',
    0xf4, 0x8f, 0xbf, 0xbf
  };
const jsize bad16_string_8_length =
  sizeof(bad16_string_8) / sizeof(jbyte);

#define RC ((jchar)0xfffd)
const jbyte bad8_string_8[] =
  { 0xc2, 0xc2, 0x81, 0x80,
    0xc2, 'a', 0x80,

    0xe0, 0xa0,  0xe0, 0xa0, 0x81,  0x80,
    0xe0,  0xe0, 0xa0, 0x81,  0xa0, 0x80,
    0xe0, 0xa0,  'b',  0x80,
    0xe0,  'b',  0xa0, 0x80,
    0xf4, 0x8f, 0xbf, 0xbf,

    0xf0, 0x90, 0x80,  0xf0, 0x90, 0x80, 0x81,  0x80,
    0xf0, 0x90,  0xf0, 0x90, 0x80, 0x81,  0x80, 0x80,
    0xf0,  0xf0, 0x90, 0x80, 0x81,  0x90, 0x80, 0x80,
    0xf4, 0x8f, 0xbf, 0xbf
  };
const jsize bad8_string_8_length =
  sizeof(bad8_string_8) / sizeof(jbyte);

const jchar bad8_string_16[] =
  { RC, (jchar)0x81, RC,
    RC, 'a', RC,
    RC, (jchar)0x801, RC,
    RC, (jchar)0x801, RC, /* RC, */
    RC, 'b', RC,
    RC, 'b', RC, /* RC, */
    (jchar)0xdbff, (jchar)0xdfff,

    RC, (jchar)0xd800, (jchar)0xdc01, RC,
    RC, (jchar)0xd800, (jchar)0xdc01, RC, /* RC, */
    RC, (jchar)0xd800, (jchar)0xdc01, RC, /* RC, RC, */

    (jchar)0xdbff, (jchar)0xdfff
  };
const jsize bad8_string_16_length =
  sizeof(bad8_string_16) / sizeof(jchar);


PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(static_literal)
 STATIC_ASCII_LITERAL
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(static_literal);

PCSL_DEFINE_ASCII_STRING_LITERAL_START(extern_literal)
 EXTERN_ASCII_LITERAL
PCSL_DEFINE_ASCII_STRING_LITERAL_END(extern_literal);

static void test_pcsl_string_literal_length() {
  /*
   * Test case 3. Extern literal
   */
  {
    const jint extern_length1 = 
      pcsl_string_length(&extern_literal);
    const jint extern_length2 = 
      PCSL_STRING_LITERAL_LENGTH(extern_literal);

    assertTrue("pcsl_string_length and PCSL_STRING_LITERAL_LENGTH"
               " report different length", 
	       extern_length1 == extern_length2);
  }
}

static void test_pcsl_string_length() {
  /*
   * Test case 0. NULL string
   */
  {
    const jint null_length = pcsl_string_length(NULL);

    assertTrue("Invalid length of NULL string", null_length < 0);
  }

  /*
   * Test case 1. Empty string
   */
  {
    const jint empty_length = 
      pcsl_string_length(&PCSL_STRING_EMPTY);

    assertTrue("Invalid length of empty literal", 
	       empty_length == 0);
  }

  /*
   * Test case 2. Static literal
   */
  {
    const jint static_length = 
      pcsl_string_length(&static_literal);
    const jchar static_ascii_literal[] = STATIC_ASCII_LITERAL;
    const jint static_ascii_literal_length = 
      sizeof(static_ascii_literal) / sizeof(jchar);

    /* Terminating zero not included */
    assertTrue("Invalid length of static literal", 
	       static_length == static_ascii_literal_length - 1);
  }

  /*
   * Test case 3. Extern literal
   */
  {
    const jint extern_length = 
      pcsl_string_length(&extern_literal);
    const jchar extern_ascii_literal[] = EXTERN_ASCII_LITERAL;
    const jint extern_ascii_literal_length = 
      sizeof(extern_ascii_literal) / sizeof(jchar);


    /* Terminating zero not included */
    assertTrue("Invalid length of extern literal", 
	       extern_length == extern_ascii_literal_length - 1);
  }

  /*
   * Test case 4. Local static literal
   */
  {
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(static_local_literal)
      STATIC_LOCAL_ASCII_LITERAL
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(static_local_literal);

    const jint static_local_length = 
      pcsl_string_length(&static_local_literal);
    const jchar static_local_ascii_literal[] = STATIC_LOCAL_ASCII_LITERAL;
    const jint static_local_ascii_literal_length = 
      sizeof(static_local_ascii_literal) / sizeof(jchar);

    /* Terminating zero not included */
    assertTrue("Invalid length of static local literal", 
	       static_local_length == static_local_ascii_literal_length - 1);
  }

  /*
   * Test case 5. Local literal
   */
  {
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(local_literal)
      LOCAL_ASCII_LITERAL
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(local_literal);

    const jint local_length = pcsl_string_length(&local_literal);
    const jchar local_ascii_literal[] = LOCAL_ASCII_LITERAL;
    const jint local_ascii_literal_length = 
      sizeof(local_ascii_literal) / sizeof(jchar);


    /* Terminating zero not included */
    assertTrue("Invalid length of local literal", 
	       local_length == local_ascii_literal_length - 1);
  }

  /*
   * Test case 6. Malformed UTF8 string.
   */
  {
    pcsl_string malformed_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf8(malformed_utf8_bytes,
				    malformed_utf8_bytes_length,
				    &malformed_string);

    if (status == PCSL_STRING_OK) {
      const jint malformed_length = pcsl_string_length(&malformed_string);
    
      assertTrue("Invalid length of utf8 malformed string", 
		 malformed_length < 0);

      status = pcsl_string_free(&malformed_string);

      assertTrue("Failed to destroy utf8 malformed string", status == PCSL_STRING_OK);
    }
  }

  /*
   * Test case 7. Malformed UTF16 string.
   */
  {
    pcsl_string malformed_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf16(malformed_utf16_chars,
				     malformed_utf16_chars_length,
				     &malformed_string);

    if (status == PCSL_STRING_OK) {
      const jint malformed_length = pcsl_string_length(&malformed_string);
    
      assertTrue("Invalid length of utf16 malformed string", 
		 malformed_length < 0);

      status = pcsl_string_free(&malformed_string);

      assertTrue("Failed to destroy utf16 malformed string", status == PCSL_STRING_OK);
    }
  }

  /*
   * Test case 8. UTF16 string with surrogates.
   */
  {
    pcsl_string surrogate_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf16(surrogate_utf16_chars,
				     surrogate_utf16_chars_length,
				     &surrogate_string);

    if (status == PCSL_STRING_OK) {
      const jint surrogate_length = pcsl_string_length(&surrogate_string);
    
      /* Terminating zero not included */
      assertTrue("Invalid length of utf16 surrogate string", 
		 surrogate_length == surrogate_string_length);

      status = pcsl_string_free(&surrogate_string);

      assertTrue("Failed to destroy utf16 surrogate string", status == PCSL_STRING_OK);
    }
  }

  /*
   * Test case 9. UTF16 string with ASCII and surrogates.
   */
  {
    pcsl_string app_surrogate_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf16(app_surrogate_utf16_chars,
				     app_surrogate_utf16_chars_length,
				     &app_surrogate_string);

    if (status == PCSL_STRING_OK) {
      const jint app_surrogate_length = pcsl_string_length(&app_surrogate_string);
    
      /* Terminating zero not included */
      assertTrue("Invalid length of utf16 surrogate string", 
		 app_surrogate_length == app_surrogate_string_length);

      status = pcsl_string_free(&app_surrogate_string);

      assertTrue("Failed to destroy utf16 surrogate string", status == PCSL_STRING_OK);
    }
  }
}

static void test_pcsl_string_utf16_length() {
  /*
   * Test case 0. NULL string
   */
  {
    const jint null_length = pcsl_string_utf16_length(NULL);

    assertTrue("Invalid utf16 length of NULL string", null_length < 0);
  }

  /*
   * Test case 1. Empty string
   */
  {
    const jint empty_utf16_length = 
      pcsl_string_utf16_length(&PCSL_STRING_EMPTY);

    assertTrue("Invalid utf16 length of empty literal", 
	       empty_utf16_length == 0);
  }

  /*
   * Test case 2. Static literal
   */
  {
    const jint static_utf16_length = 
      pcsl_string_utf16_length(&static_literal);
    const jchar static_ascii_literal[] = STATIC_ASCII_LITERAL;
    const jint static_ascii_literal_length = 
      sizeof(static_ascii_literal) / sizeof(jchar);

    /* Terminating zero not included */
    assertTrue("Invalid utf16 length of static literal", 
	       static_utf16_length == static_ascii_literal_length - 1);
  }

  /*
   * Test case 3. Extern literal
   */
  {
    const jint extern_utf16_length = 
      pcsl_string_utf16_length(&extern_literal);
    const jchar extern_ascii_literal[] = EXTERN_ASCII_LITERAL;
    const jint extern_ascii_literal_length = 
      sizeof(extern_ascii_literal) / sizeof(jchar);


    /* Terminating zero not included */
    assertTrue("Invalid utf16 length of extern literal", 
	       extern_utf16_length == extern_ascii_literal_length - 1);
  }

  /*
   * Test case 4. Local static literal
   */
  {
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(static_local_literal)
      STATIC_LOCAL_ASCII_LITERAL
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(static_local_literal);

    const jint static_local_utf16_length = 
      pcsl_string_utf16_length(&static_local_literal);
    const jchar static_local_ascii_literal[] = STATIC_LOCAL_ASCII_LITERAL;
    const jint static_local_ascii_literal_length = 
      sizeof(static_local_ascii_literal) / sizeof(jchar);

    /* Terminating zero not included */
    assertTrue("Invalid utf16 length of static local literal", 
	       static_local_utf16_length == 
	       static_local_ascii_literal_length - 1);
  }

  /*
   * Test case 5. Local literal
   */
  {
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(local_literal)
      LOCAL_ASCII_LITERAL
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(local_literal);

    const jint local_utf16_length = pcsl_string_utf16_length(&local_literal);
    const jchar local_ascii_literal[] = LOCAL_ASCII_LITERAL;
    const jint local_ascii_literal_length = 
      sizeof(local_ascii_literal) / sizeof(jchar);


    /* Terminating zero not included */
    assertTrue("Invalid utf16 length of local literal", 
	       local_utf16_length == local_ascii_literal_length - 1);
  }

  /*
   * Test case 6. Malformed UTF8 string.
   */
  {
    pcsl_string malformed_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf8(malformed_utf8_bytes,
				    malformed_utf8_bytes_length,
				    &malformed_string);

    if (status == PCSL_STRING_OK) {
      const jint malformed_utf16_length = pcsl_string_utf16_length(&malformed_string);
    
      assertTrue("Invalid utf16 length of utf8 malformed string", 
		 malformed_utf16_length < 0);

      status = pcsl_string_free(&malformed_string);

      assertTrue("Failed to destroy utf8 malformed string", status == PCSL_STRING_OK);
    }
  }

  /*
   * Test case 7. Malformed UTF16 string.
   */
  {
    pcsl_string malformed_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf16(malformed_utf16_chars,
				     malformed_utf16_chars_length,
				     &malformed_string);

    if (status == PCSL_STRING_OK) {
      const jint malformed_utf16_length = pcsl_string_utf16_length(&malformed_string);
    
      assertTrue("Invalid utf16 length of utf16 malformed string", 
		 malformed_utf16_length == malformed_utf16_chars_length);

      status = pcsl_string_free(&malformed_string);

      assertTrue("Failed to destroy utf16 malformed string", status == PCSL_STRING_OK);
    }
  }

  /*
   * Test case 8. UTF16 string with surrogates.
   */
  {
    pcsl_string surrogate_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf16(surrogate_utf16_chars,
				     surrogate_utf16_chars_length,
				     &surrogate_string);

    if (status == PCSL_STRING_OK) {
      const jint surrogate_length = pcsl_string_utf16_length(&surrogate_string);
    
      assertTrue("Invalid utf16 length of utf16 surrogate string", 
		 surrogate_length == surrogate_utf16_chars_length);

      status = pcsl_string_free(&surrogate_string);

      assertTrue("Failed to destroy utf16 surrogate string", status == PCSL_STRING_OK);
    }
  }

  /*
   * Test case 9. UTF16 string with ASCII and surrogates.
   */
  {
    pcsl_string app_surrogate_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf16(app_surrogate_utf16_chars,
				     app_surrogate_utf16_chars_length,
				     &app_surrogate_string);

    if (status == PCSL_STRING_OK) {
      const jint app_surrogate_length = 
	pcsl_string_utf16_length(&app_surrogate_string);
    
      assertTrue("Invalid utf16 length of utf16 surrogate string", 
		 app_surrogate_length == app_surrogate_utf16_chars_length);

      status = pcsl_string_free(&app_surrogate_string);

      assertTrue("Failed to destroy utf16 surrogate string", status == PCSL_STRING_OK);
    }
  }
}

static void test_pcsl_string_utf8_length() {
  /*
   * Test case 0. NULL string
   */
  {
    const jint null_length = pcsl_string_utf8_length(NULL);

    assertTrue("Invalid utf8 length of NULL string", null_length < 0);
  }

  /*
   * Test case 1. Empty string
   */
  {
    const jint empty_utf8_length = 
      pcsl_string_utf8_length(&PCSL_STRING_EMPTY);

    assertTrue("Invalid utf8 length of empty literal", 
	       empty_utf8_length == 0);
  }

  /*
   * Test case 2. Static literal
   */
  {
    const jint static_utf8_length = 
      pcsl_string_utf8_length(&static_literal);
    const jchar static_ascii_literal[] = STATIC_ASCII_LITERAL;
    const jint static_ascii_literal_length = 
      sizeof(static_ascii_literal) / sizeof(jchar);

    /* Terminating zero not included */
    assertTrue("Invalid utf8 length of static literal", 
	       static_utf8_length == static_ascii_literal_length - 1);
  }

  /*
   * Test case 3. Extern literal
   */
  {
    const jint extern_utf8_length = 
      pcsl_string_utf8_length(&extern_literal);
    const jchar extern_ascii_literal[] = EXTERN_ASCII_LITERAL;
    const jint extern_ascii_literal_length = 
      sizeof(extern_ascii_literal) / sizeof(jchar);


    /* Terminating zero not included */
    assertTrue("Invalid utf8 length of extern literal", 
	       extern_utf8_length == extern_ascii_literal_length - 1);
  }

  /*
   * Test case 4. Local static literal
   */
  {
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(static_local_literal) 
      STATIC_LOCAL_ASCII_LITERAL
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(static_local_literal);

    const jint static_local_utf8_length = 
      pcsl_string_utf8_length(&static_local_literal);
    const jchar static_local_ascii_literal[] = STATIC_LOCAL_ASCII_LITERAL;
    const jint static_local_ascii_literal_length = 
      sizeof(static_local_ascii_literal) / sizeof(jchar);

    /* Terminating zero not included */
    assertTrue("Invalid utf8 length of static local literal", 
	       static_local_utf8_length == 
	       static_local_ascii_literal_length - 1);
  }

  /*
   * Test case 5. Local literal
   */
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(local_literal)
      LOCAL_ASCII_LITERAL
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(local_literal);

    const jint local_utf8_length = pcsl_string_utf8_length(&local_literal);
    const jchar local_ascii_literal[] = LOCAL_ASCII_LITERAL;
    const jint local_ascii_literal_length = 
      sizeof(local_ascii_literal) / sizeof(jchar);


    /* Terminating zero not included */
    assertTrue("Invalid utf8 length of local literal", 
	       local_utf8_length == local_ascii_literal_length - 1);
  }

  /*
   * Test case 6. Malformed UTF8 string.
   */
  {
    pcsl_string malformed_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf8(malformed_utf8_bytes,
				    malformed_utf8_bytes_length,
				    &malformed_string);

    if (status == PCSL_STRING_OK) {
      const jint malformed_utf8_length = pcsl_string_utf8_length(&malformed_string);
    
      assertTrue("Invalid utf8 length of malformed utf8 string", 
		 malformed_utf8_length == malformed_utf8_bytes_length);

      status = pcsl_string_free(&malformed_string);

      assertTrue("Failed to destroy malformed utf8 string", status == PCSL_STRING_OK);
    }
  }

  /*
   * Test case 7. Malformed UTF16 string.
   */
  {
    pcsl_string malformed_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf16(malformed_utf16_chars,
				     malformed_utf16_chars_length,
				     &malformed_string);

    if (status == PCSL_STRING_OK) {
      const jint malformed_utf8_length = pcsl_string_utf8_length(&malformed_string);
    
      assertTrue("Invalid utf8 length of malformed utf16 string", 
		 malformed_utf8_length < 0);

      status = pcsl_string_free(&malformed_string);

      assertTrue("Failed to destroy malformed utf16 string", status == PCSL_STRING_OK);
    }
  }
}

static void test_pcsl_string_convert_to_utf8() {
  /*
   * Test case 1. Empty string.
   */
  {
    jbyte buffer[1];
    jsize buffer_length = 1;
    jsize converted_length = 0;

    pcsl_string_status status = 
      pcsl_string_convert_to_utf8(&PCSL_STRING_EMPTY, 
				  buffer, 
				  buffer_length,
				  &converted_length);

    assertTrue("Unexpected conversion failure on empty string", 
	       status == PCSL_STRING_OK);

    assertTrue("Incorrect converted length on empty string", 
	       converted_length == 0);

    status = 
      pcsl_string_convert_to_utf8(&PCSL_STRING_EMPTY, 
				  NULL, 0,
				  &converted_length);

    assertTrue("Unexpected conversion success on empty string", 
	       status != PCSL_STRING_OK);
  }

  /*
   * Test case 2. Malformed UTF8 string.
   */
  {
    pcsl_string malformed_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf8(malformed_utf8_bytes,
				    malformed_utf8_bytes_length,
				    &malformed_string);

    if (status == PCSL_STRING_OK) {
      /*
       * It's ok if an implementation fails to create a string from malformed
       * characters, but if it is created, we should be able to convert it back
       * successfully.
       */
      jbyte * buffer = 
	pcsl_mem_malloc(malformed_utf8_bytes_length * sizeof(jbyte));

      if (buffer != NULL) {
	jsize converted_length = 0;
	status = pcsl_string_convert_to_utf8(&malformed_string, 
					     buffer, 
					     malformed_utf8_bytes_length,
					     &converted_length);

	assertTrue("Unexpected conversion failure on malformed utf8 string", 
		   status == PCSL_STRING_OK);

	assertTrue("Incorrect converted length on malformed utf8 string", 
		   converted_length == malformed_utf8_bytes_length);

	assertTrue("Incorrect converted length on malformed utf8 string", 
		   memcmp(buffer, malformed_utf8_bytes,
			  malformed_utf8_bytes_length) == 0);

	pcsl_mem_free(buffer);
      }

      {
	status = pcsl_string_free(&malformed_string);

	assertTrue("Failed to destroy malformed utf8 string", 
		   status == PCSL_STRING_OK);
      }
    }
  }

  /*
   * Test case 3. Malformed UTF16 string.
   */
  {
    pcsl_string malformed_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf16(malformed_utf16_chars,
				     malformed_utf16_chars_length,
				     &malformed_string);

    if (status == PCSL_STRING_OK) {
      /*
       * It's ok if an implementation fails to create a string from malformed
       * utf16 characters, but if it is created, we should be able to convert it
       * to utf8 successfully.
       */
      const jsize buffer_length = malformed_utf16_chars_length * 3;
      jbyte * buffer = pcsl_mem_malloc(buffer_length * sizeof(jbyte));
      
      if (buffer != NULL) {
	jsize converted_length = 0;
	status = pcsl_string_convert_to_utf8(&malformed_string, 
					     buffer, 
					     buffer_length,
					     &converted_length);

	assertTrue("Unexpected conversion status on malformed utf16 string", 
		   status != PCSL_STRING_OK);
	
	pcsl_mem_free(buffer);
      }

      {
	status = pcsl_string_free(&malformed_string);

	assertTrue("Failed to destroy malformed utf16 string", 
		   status == PCSL_STRING_OK);
      }
    }
  }
}

static void test_pcsl_string_convert_to_utf16() {
  /*
   * Test case 1. Empty string.
   */
  {
    jchar buffer[1];
    jsize buffer_length = 1;
    jsize converted_length = 0;

    pcsl_string_status status = 
      pcsl_string_convert_to_utf16(&PCSL_STRING_EMPTY, 
				   buffer, 
				   buffer_length,
				   &converted_length);

    assertTrue("Unexpected conversion failure on empty string", 
	       status == PCSL_STRING_OK);

    assertTrue("Incorrect converted length on empty string", 
	       converted_length == 0);

    status = 
      pcsl_string_convert_to_utf16(&PCSL_STRING_EMPTY, 
				   NULL, 0,
				   &converted_length);

    assertTrue("Unexpected conversion success on empty string", 
	       status != PCSL_STRING_OK);
  }

  /*
   * Test case 2. Malformed UTF8 string.
   */
  {
    pcsl_string malformed_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf8(malformed_utf8_bytes,
				    malformed_utf8_bytes_length,
				    &malformed_string);

    if (status == PCSL_STRING_OK) {
      /*
       * It's ok if an implementation fails to create a string from malformed
       * utf8 characters, but if it is created, we should be able to convert it
       * to utf16 successfully.
       */
      const jsize buffer_length = malformed_utf8_bytes_length;
      jchar * buffer = pcsl_mem_malloc(buffer_length * sizeof(jchar));

      if (buffer != NULL) {
	jsize converted_length = 0;
	status = pcsl_string_convert_to_utf16(&malformed_string, 
					      buffer, 
					      buffer_length,
					      &converted_length);

	assertTrue("Unexpected conversion status on malformed utf8 string", 
		   status != PCSL_STRING_OK);
	
	pcsl_mem_free(buffer);
      }

      {
	status = pcsl_string_free(&malformed_string);

	assertTrue("Failed to destroy malformed utf8 string", 
		   status == PCSL_STRING_OK);
      }
    }
  }

  /*
   * Test case 3. Malformed UTF16 string.
   */
  {
    pcsl_string malformed_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf16(malformed_utf16_chars,
				     malformed_utf16_chars_length,
				     &malformed_string);

    if (status == PCSL_STRING_OK) {
      /*
       * It's ok if an implementation fails to create a string from malformed
       * characters, but if it is created, we should be able to convert it back
       * successfully.
       */

      /* Reserve space for terminating zero */
      const jsize buffer_length = malformed_utf16_chars_length + 1;
      jchar * buffer = pcsl_mem_malloc(buffer_length * sizeof(jchar));
      
      if (buffer != NULL) {
	jsize converted_length = 0;

	status = pcsl_string_convert_to_utf16(&malformed_string, 
					      buffer, 
					      buffer_length,
					      &converted_length);

	assertTrue("Unexpected conversion failure on malformed utf16 string", 
		   status == PCSL_STRING_OK);

	assertTrue("Incorrect converted length on malformed utf16 string", 
		   converted_length == malformed_utf16_chars_length);

	assertTrue("Incorrect converted contents of malformed utf16 string", 
		   memcmp(buffer, malformed_utf16_chars,
			  malformed_utf16_chars_length * sizeof(jchar)) == 0);
	
	pcsl_mem_free(buffer);
      }

      {
	status = pcsl_string_free(&malformed_string);

	assertTrue("Failed to destroy malformed utf16 string", 
		   status == PCSL_STRING_OK);
      }
    }
  }
}

static void test_pcsl_string_convert_from_utf8() {
  /*
   * Test case 1. Null args.
   */
  {
    pcsl_string_status status = pcsl_string_convert_from_utf8(NULL, 0, NULL);

    assertTrue("Incorrect status on NULL args", 
	       status == PCSL_STRING_EINVAL);
  }

  /*
   * Test case 2. Null buffer.
   */
  {
    pcsl_string str = PCSL_STRING_NULL;
    pcsl_string_status status = pcsl_string_convert_from_utf8(NULL, 0, &str);

    assertTrue("Incorrect status on NULL buffer", 
	       status == PCSL_STRING_EINVAL);
  }
  
  /*
   * Test case 3. Null str.
   */
  {
    jbyte buffer[1];
    pcsl_string_status status = pcsl_string_convert_from_utf8(buffer, 0, NULL);

    assertTrue("Incorrect status on NULL str", 
	       status == PCSL_STRING_EINVAL);
  }
  

  /*
   * Test case 4. ASCII string.
   */
  {
    jbyte buffer[] = LOCAL_ASCII_LITERAL;
    jsize buffer_length = sizeof(buffer) / sizeof(jbyte);
    pcsl_string str = PCSL_STRING_NULL;
    pcsl_string_status status = pcsl_string_convert_from_utf8(buffer, buffer_length, &str);

    assertTrue("Incorrect status on common case", status == PCSL_STRING_OK);

    {
      jsize length = pcsl_string_utf8_length(&str);

      /* Terminating zero not included */
      assertTrue("Incorrect length on common case", 
		 length == buffer_length - 1);
    }

    {
      const jsize utf8_buffer_length = 100;
      jbyte * utf8_buffer = pcsl_mem_malloc(utf8_buffer_length * sizeof(jbyte));

      if (utf8_buffer != NULL) {
	jsize converted_length = 0;

	pcsl_string_status status = pcsl_string_convert_to_utf8(&str, utf8_buffer, 
						     utf8_buffer_length,
						     &converted_length);
      
	assertTrue("Incorrect status of conversion on common case", 
		   status == PCSL_STRING_OK);

	/* Terminating zero not included */
	assertTrue("Incorrect converted length on common case", 
		   converted_length == buffer_length - 1);

	assertTrue("Incorrect converted string on common case", 
		   memcmp(utf8_buffer, buffer, buffer_length) == 0);
	
	pcsl_mem_free(utf8_buffer);
      }
    }

    {
      const jchar utf16_golden_buffer[] = LOCAL_ASCII_LITERAL;
      jsize utf16_golden_buffer_length = 
	sizeof(utf16_golden_buffer) / sizeof(jchar);

      const jsize utf16_buffer_length = 100;
      jchar * utf16_buffer = pcsl_mem_malloc(utf16_buffer_length * sizeof(jchar));

      if (utf16_buffer != NULL) {
	jsize converted_length = 0;

	pcsl_string_status status = pcsl_string_convert_to_utf16(&str, utf16_buffer, 
						      utf16_buffer_length,
						      &converted_length);
      
	assertTrue("Incorrect status of conversion to utf16 on common case", 
		   status == PCSL_STRING_OK);

	/* Terminating zero not included */
	assertTrue("Incorrect converted length on common case", 
		   converted_length == utf16_golden_buffer_length - 1);

	assertTrue("Incorrect converted string on common case", 
		   memcmp(utf16_buffer, utf16_golden_buffer, 
			  utf16_golden_buffer_length * sizeof(jchar)) == 0);
	
	pcsl_mem_free(utf16_buffer);
      }
    }
    
    {
      status = pcsl_string_free(&str);

      assertTrue("Failed to destroy utf8 string", 
		 status == PCSL_STRING_OK);
    }
  }  
}

static void test_pcsl_string_convert_from_utf16() {
  /*
   * Test case 1. Null args.
   */
  {
    pcsl_string_status status = pcsl_string_convert_from_utf16(NULL, 0, NULL);

    assertTrue("Incorrect status on NULL args", 
	       status == PCSL_STRING_EINVAL);
  }

  /*
   * Test case 2. Null buffer.
   */
  {
    pcsl_string str = PCSL_STRING_NULL;
    pcsl_string_status status = pcsl_string_convert_from_utf16(NULL, 0, &str);

    assertTrue("Incorrect status on NULL buffer", 
	       status == PCSL_STRING_EINVAL);
  }
  
  /*
   * Test case 3. Null str.
   */
  {
    jchar buffer[1];
    pcsl_string_status status = pcsl_string_convert_from_utf16(buffer, 0, NULL);

    assertTrue("Incorrect status on NULL str", 
	       status == PCSL_STRING_EINVAL);
  }
  

  /*
   * Test case 4. ASCII string.
   */
  {
    jchar buffer[] = LOCAL_ASCII_LITERAL;
    jsize buffer_length = sizeof(buffer) / sizeof(jchar);
    pcsl_string str = PCSL_STRING_NULL;
    pcsl_string_status status = pcsl_string_convert_from_utf16(buffer, buffer_length, &str);

    assertTrue("Incorrect status on common case", status == PCSL_STRING_OK);

    {
      jsize length = pcsl_string_utf16_length(&str);

      /* Terminating zero not included */
      assertTrue("Incorrect length on common case", 
		 length == buffer_length - 1);
    }

    {
      const jsize utf16_buffer_length = 100;
      jchar * utf16_buffer = pcsl_mem_malloc(utf16_buffer_length * sizeof(jchar));

      if (utf16_buffer != NULL) {
	jsize converted_length = 0;

	pcsl_string_status status = pcsl_string_convert_to_utf16(&str, utf16_buffer, 
						      utf16_buffer_length,
						      &converted_length);
      
	assertTrue("Incorrect status of conversion on common case", 
		   status == PCSL_STRING_OK);

	/* Terminating zero not included */
	assertTrue("Incorrect converted length on common case", 
		   converted_length == buffer_length - 1);
      
	assertTrue("Incorrect converted string on common case", 
		   memcmp(utf16_buffer, buffer, buffer_length * sizeof(jchar)) == 0);

	pcsl_mem_free(utf16_buffer);
      }
    }

    {
      const jbyte utf8_golden_buffer[] = LOCAL_ASCII_LITERAL;
      jsize utf8_golden_buffer_length = 
	sizeof(utf8_golden_buffer) / sizeof(jbyte);

      const jsize utf8_buffer_length = 100;
      jbyte * utf8_buffer = pcsl_mem_malloc(utf8_buffer_length * sizeof(jbyte));

      if (utf8_buffer != NULL) {
	jsize converted_length = 0;

	pcsl_string_status status = pcsl_string_convert_to_utf8(&str, utf8_buffer, 
						     utf8_buffer_length,
						     &converted_length);
      
	assertTrue("Incorrect status of conversion to utf8 on common case", 
		   status == PCSL_STRING_OK);

	/* Terminating zero not included */
	assertTrue("Incorrect converted length on common case", 
		   converted_length == utf8_golden_buffer_length - 1);

	assertTrue("Incorrect converted string on common case", 
		   memcmp(utf8_buffer, utf8_golden_buffer, 
			  utf8_golden_buffer_length * sizeof(jbyte)) == 0);
	
	pcsl_mem_free(utf8_buffer);
      }
    }

    {
      status = pcsl_string_free(&str);

      assertTrue("Failed to destroy utf16 string", 
		 status == PCSL_STRING_OK);
    }
  }  
}

static void test_pcsl_string_equals() {
  /*
   * Test case 1. Null args.
   */
  {
    jboolean eq = pcsl_string_equals(NULL, NULL);

    assertTrue("Incorrect status on NULL args", eq == PCSL_TRUE);
  }

  /*
   * Test case 2. Null and empty string.
   */
  {
    jboolean eq = pcsl_string_equals(NULL, &PCSL_STRING_EMPTY);

    assertTrue("Incorrect status on NULL args", eq == PCSL_FALSE);
  }

  /*
   * Test case 3. Empty string and NULL.
   */
  {
    jboolean eq = pcsl_string_equals(&PCSL_STRING_EMPTY, NULL);

    assertTrue("Incorrect status on NULL args", eq == PCSL_FALSE);
  }

  /*
   * Test case 4. Empty string identity Differemt NULL and EMPTY strings.
   */
  {
    jboolean eq;
    pcsl_string str1, str2;
    /* Fill structures by same content */
    str1 = PCSL_STRING_NULL;
    str2 = PCSL_STRING_NULL;
    eq = pcsl_string_equals(&str1, &str2);
    assertTrue("Incorrect result on compare same args", eq == PCSL_TRUE);

    str1 = PCSL_STRING_EMPTY;
    str2 = PCSL_STRING_EMPTY;
    eq = pcsl_string_equals(&str1, &str2);

    assertTrue("Incorrect status on NULL args", eq == PCSL_TRUE);
  }

  /*
   * Test case 5. Common case equal strings.
   */
  {
    jchar buffer[] = LOCAL_ASCII_LITERAL;
    jsize buffer_length = sizeof(buffer) / sizeof(jchar);
    pcsl_string str1 = PCSL_STRING_NULL;
    pcsl_string str2 = PCSL_STRING_NULL;
    pcsl_string_status status1 = pcsl_string_convert_from_utf16(buffer, buffer_length, &str1);
    pcsl_string_status status2 = pcsl_string_convert_from_utf16(buffer, buffer_length, &str2);

    assertTrue("Incorrect status on common case", 
	       status1 == PCSL_STRING_OK && status2 == PCSL_STRING_OK);

    {
      jboolean eq = pcsl_string_equals(&str1, &str2);

      assertTrue("Incorrect result on equals strings", eq == PCSL_TRUE);
    }

    {
      pcsl_string_status status = pcsl_string_free(&str1);

      assertTrue("Failed to destroy utf16 string", 
		 status == PCSL_STRING_OK);

      status = pcsl_string_free(&str2);

      assertTrue("Failed to destroy utf16 string", 
		 status == PCSL_STRING_OK);
    }
  }

  /*
   * Test case 6. Common case unequal strings.
   */
  {
    jchar buffer1[] = LOCAL_ASCII_LITERAL;
    jsize buffer1_length = sizeof(buffer1) / sizeof(jchar);
    jchar buffer2[] = EXTERN_ASCII_LITERAL;
    jsize buffer2_length = sizeof(buffer2) / sizeof(jchar);
    pcsl_string str1 = PCSL_STRING_NULL;
    pcsl_string str2 = PCSL_STRING_NULL;
    pcsl_string_status status1 = pcsl_string_convert_from_utf16(buffer1, buffer1_length, &str1);
    pcsl_string_status status2 = pcsl_string_convert_from_utf16(buffer2, buffer2_length, &str2);

    assertTrue("Incorrect status on common case", 
	       status1 == PCSL_STRING_OK && status2 == PCSL_STRING_OK);

    {
      jboolean eq = pcsl_string_equals(&str1, &str2);

      assertTrue("Incorrect result on unequals strings", eq == PCSL_FALSE);
    }

    {
      pcsl_string_status status = pcsl_string_free(&str1);

      assertTrue("Failed to destroy utf16 string", 
		 status == PCSL_STRING_OK);

      status = pcsl_string_free(&str2);

      assertTrue("Failed to destroy utf16 string", 
		 status == PCSL_STRING_OK);
    }
  }

  /*
   * Test case 7. UTF8 and UTF16 equal strings.
   */
  {
    jchar utf16_buffer[] = LOCAL_ASCII_LITERAL;
    jsize utf16_buffer_length = sizeof(utf16_buffer) / sizeof(jchar);
    jbyte utf8_buffer[] = LOCAL_ASCII_LITERAL;
    jsize utf8_buffer_length = sizeof(utf8_buffer) / sizeof(jbyte);
    pcsl_string str1 = PCSL_STRING_NULL;
    pcsl_string str2 = PCSL_STRING_NULL;
    pcsl_string_status status1 = pcsl_string_convert_from_utf16(utf16_buffer, 
						     utf16_buffer_length, &str1);
    pcsl_string_status status2 = pcsl_string_convert_from_utf8(utf8_buffer, 
						    utf8_buffer_length, &str2);

    assertTrue("Incorrect status on common case", 
	       status1 == PCSL_STRING_OK && status2 == PCSL_STRING_OK);

    {
      jboolean eq = pcsl_string_equals(&str1, &str2);

      assertTrue("Incorrect result on equals strings", eq == PCSL_TRUE);
    }

    {
      pcsl_string_status status = pcsl_string_free(&str1);

      assertTrue("Failed to destroy utf16 string", 
		 status == PCSL_STRING_OK);

      status = pcsl_string_free(&str2);

      assertTrue("Failed to destroy utf8 string", 
		 status == PCSL_STRING_OK);
    }
  }
}

static void test_pcsl_string_compare() {
  jint cmp;
  /*
   * Test case 1. Null args.
   */
  {
    pcsl_string_status status = pcsl_string_compare(NULL, NULL, &cmp);

    assertTrue("Incorrect status on NULL args", 
	       status == PCSL_STRING_OK);
    assertTrue("Strings are not equal", 
	       cmp == 0);

    status = pcsl_string_compare(&PCSL_STRING_NULL, &PCSL_STRING_NULL, &cmp);
    assertTrue("Incorrect status on PCSL_STRING_NULL args", 
	       status == PCSL_STRING_OK);
    assertTrue("Strings are not equal", 
	       cmp == 0);
  }

  /*
   * Test case 2. Null and empty string.
   */
  {
    pcsl_string_status status = pcsl_string_compare(NULL, &PCSL_STRING_EMPTY, &cmp);

    assertTrue("Incorrect status on NULL args", 
	       status == PCSL_STRING_EINVAL);

    status = pcsl_string_compare(&PCSL_STRING_NULL, &PCSL_STRING_EMPTY, &cmp);
    assertTrue("Incorrect status on comparing NULL and EMPTY strings", 
	       status == PCSL_STRING_OK);
    assertTrue("PCSL_STRING_EMPTY is not greater then PCSL_STRING_NULL", 
	       cmp < 0);
  }

  /*
   * Test case 3. Empty string and NULL.
   */
  {
    pcsl_string_status status = pcsl_string_compare(&PCSL_STRING_EMPTY, NULL, &cmp);

    assertTrue("Incorrect status on NULL args", 
	       status == PCSL_STRING_EINVAL);

    status = pcsl_string_compare(&PCSL_STRING_EMPTY, &PCSL_STRING_NULL, &cmp);
    assertTrue("Incorrect status on comparing NULL and EMPTY strings", 
	       status == PCSL_STRING_OK);
    assertTrue("PCSL_STRING_EMPTY is not greater then PCSL_STRING_NULL", 
	       cmp > 0);
  }

  /*
   * Test case 4. Same string identity.
   */
  {
    pcsl_string str1, str2;
    pcsl_string_status status;
    /* Fill structures by same content */
    str1 = PCSL_STRING_NULL;
    str2 = PCSL_STRING_NULL;
    status = pcsl_string_compare(&str1, &str2, &cmp);

    assertTrue("Incorrect status on identical args", status == PCSL_STRING_OK);
    assertTrue("Incorrect comparison result on identical args", cmp == 0);

    /* Fill structures by same content */
    str1 = PCSL_STRING_EMPTY;
    str2 = PCSL_STRING_EMPTY;
    status = pcsl_string_compare(&str1, &str2, &cmp);

    assertTrue("Incorrect status on identical args", status == PCSL_STRING_OK);
    assertTrue("Incorrect comparison result on identical args", cmp == 0);
  }

  /*
   * Test case 5. Common case equal strings.
   */
  {
    jchar buffer[] = LOCAL_ASCII_LITERAL;
    jsize buffer_length = sizeof(buffer) / sizeof(jchar);
    pcsl_string str1 = PCSL_STRING_NULL;
    pcsl_string str2 = PCSL_STRING_NULL;
    pcsl_string_status status1 = pcsl_string_convert_from_utf16(buffer, buffer_length, &str1);
    pcsl_string_status status2 = pcsl_string_convert_from_utf16(buffer, buffer_length, &str2);

    assertTrue("Incorrect status on common case", 
	       status1 == PCSL_STRING_OK && status2 == PCSL_STRING_OK);

    {
      pcsl_string_status status = pcsl_string_compare(&str1, &str2, &cmp);

      assertTrue("Incorrect status on equals strings", status == PCSL_STRING_OK);
      assertTrue("Incorrect comparison result on equals strings", cmp == 0);

      status = pcsl_string_compare(&PCSL_STRING_EMPTY, &str2, &cmp);

      assertTrue("Incorrect status on comparing empty and not empty strings", 
        status == PCSL_STRING_OK);
      assertTrue("Incorrect comparison result on empty and not empty strings", 
        cmp < 0);

      status = pcsl_string_compare(&PCSL_STRING_NULL, &str2, &cmp);

      assertTrue("Incorrect status on comparing null and not empty strings", 
        status == PCSL_STRING_OK);
      assertTrue("Incorrect comparison result on null and not empty strings", 
        cmp < 0);
    }

    {
      pcsl_string_status status = pcsl_string_free(&str1);

      assertTrue("Failed to destroy utf16 string", 
		 status == PCSL_STRING_OK);

      status = pcsl_string_free(&str2);

      assertTrue("Failed to destroy utf16 string", 
		 status == PCSL_STRING_OK);
    }
  }

  /*
   * Test case 6. Common case unequal strings.
   */
  {
    jchar buffer1[] = LOCAL_ASCII_LITERAL;
    jsize buffer1_length = sizeof(buffer1) / sizeof(jchar);
    jchar buffer2[] = EXTERN_ASCII_LITERAL;
    jsize buffer2_length = sizeof(buffer2) / sizeof(jchar);
    pcsl_string str1 = PCSL_STRING_NULL;
    pcsl_string str2 = PCSL_STRING_NULL;
    pcsl_string_status status1 = pcsl_string_convert_from_utf16(buffer1, buffer1_length, &str1);
    pcsl_string_status status2 = pcsl_string_convert_from_utf16(buffer2, buffer2_length, &str2);

    assertTrue("Incorrect status on common case", 
	       status1 == PCSL_STRING_OK && status2 == PCSL_STRING_OK);

    {
      pcsl_string_status status = pcsl_string_compare(&str1, &str2, &cmp);

      assertTrue("Incorrect status on unequals strings", status == PCSL_STRING_OK);
      assertTrue("Incorrect comparison result on unequals strings", cmp != 0);
    }

    {
      const jint cmp1 = cmp;

      pcsl_string_status status = pcsl_string_compare(&str2, &str1, &cmp);

      assertTrue("Incorrect status on unequals strings", status == PCSL_STRING_OK);
      assertTrue("Incorrect comparison result on unequals strings", cmp * cmp1 < 0);
    }

    {
      pcsl_string_status status = pcsl_string_free(&str1);

      assertTrue("Failed to destroy utf16 string", 
		 status == PCSL_STRING_OK);

      status = pcsl_string_free(&str2);

      assertTrue("Failed to destroy utf16 string", 
		 status == PCSL_STRING_OK);
    }
  }

  /*
   * Test case 7. UTF8 and UTF16 equal strings.
   */
  {
    jchar utf16_buffer[] = LOCAL_ASCII_LITERAL;
    jsize utf16_buffer_length = sizeof(utf16_buffer) / sizeof(jchar);
    jbyte utf8_buffer[] = LOCAL_ASCII_LITERAL;
    jsize utf8_buffer_length = sizeof(utf8_buffer) / sizeof(jbyte);
    pcsl_string str1 = PCSL_STRING_NULL;
    pcsl_string str2 = PCSL_STRING_NULL;
    pcsl_string_status status1 = pcsl_string_convert_from_utf16(utf16_buffer, 
						     utf16_buffer_length, &str1);
    pcsl_string_status status2 = pcsl_string_convert_from_utf8(utf8_buffer, 
						    utf8_buffer_length, &str2);

    assertTrue("Incorrect status on common case", 
	       status1 == PCSL_STRING_OK && status2 == PCSL_STRING_OK);

    {
      pcsl_string_status status = pcsl_string_compare(&str1, &str2, &cmp);

      assertTrue("Incorrect result on equals strings", status == PCSL_STRING_OK);
      assertTrue("Incorrect result on equals strings", cmp == 0);
    }

    {
      pcsl_string_status status = pcsl_string_free(&str1);

      assertTrue("Failed to destroy utf16 string", 
		 status == PCSL_STRING_OK);

      status = pcsl_string_free(&str2);

      assertTrue("Failed to destroy utf8 string", 
		 status == PCSL_STRING_OK);
    }
  }
}

static void test_pcsl_string_cat() {
    pcsl_string str1, str2, result_string;
    pcsl_string_status status;
  /*
   * Test case 1. Null result string.
   */
  {
    status = pcsl_string_cat(NULL, NULL, NULL);

    assertTrue("Incorrect status on NULL result", 
	       status == PCSL_STRING_EINVAL);
  }
  /*
   * Test case 2. String1 is damaged.
   */
  {
    str1.length = 5;
    str1.data = NULL;
    status = pcsl_string_cat(&str1, NULL, &result_string);

    assertTrue("Incorrect status on damaged first argument", 
	       status == PCSL_STRING_EINVAL);
  }
  /*
   * Test case 3. String2 is damaged.
   */
  {
    str2.length = 5;
    str2.data = NULL;
    status = pcsl_string_cat(NULL, &str2, &result_string);

    assertTrue("Incorrect status on damaged second argument", 
	       status == PCSL_STRING_EINVAL);
  }
  /*
   * Test case 4. Concat 2 NULL (empty) strings.
   */
  {
    status = pcsl_string_cat(NULL, NULL, &result_string);

    assertTrue("Incorrect status on concat two NULL strings", 
	       status == PCSL_STRING_OK);
    assertTrue("The result string is not empty", 
               pcsl_string_length(&result_string) == (jsize)0);
    str1 = PCSL_STRING_NULL;
    str2 = PCSL_STRING_NULL;
    status = pcsl_string_cat(&str1, &str2, &result_string);

    assertTrue("Incorrect status on concat two NULL strings", 
	       status == PCSL_STRING_OK);
    assertTrue("The result string is not empty", 
               pcsl_string_length(&result_string) == (jsize)0);
    str1 = PCSL_STRING_EMPTY;
    str2 = PCSL_STRING_EMPTY;
    status = pcsl_string_cat(&str1, &str2, &result_string);

    assertTrue("Incorrect status on concat two NULL strings", 
	       status == PCSL_STRING_OK);
    assertTrue("The result string is not empty", 
               pcsl_string_length(&result_string) == (jsize)0);
  }
  /*
   * Test case 5. Concat NULL(empty) and not empty strings.
   */
  {
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(local_literal)
      LOCAL_ASCII_LITERAL
    PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(local_literal);

    const jint local_length = pcsl_string_length(&local_literal);
    jint comparison;

    status = pcsl_string_cat(&local_literal, NULL, &result_string);

    assertTrue("Incorrect status on concat NULL and not NULL strings", 
	       status == PCSL_STRING_OK);
    status = pcsl_string_compare(&local_literal, &result_string, &comparison);
    assertTrue("The result string is incorrect", comparison == 0);

    str2 = PCSL_STRING_NULL;
    status = pcsl_string_cat(&local_literal, &str2, &result_string);

    assertTrue("Incorrect status on concat NULL and not NULL strings", 
	       status == PCSL_STRING_OK);
    status = pcsl_string_compare(&local_literal, &result_string, &comparison);
    assertTrue("The result string is incorrect", comparison == 0);

    str2 = PCSL_STRING_EMPTY;
    status = pcsl_string_cat(&local_literal, &str2, &result_string);

    assertTrue("Incorrect status on concat NULL and not NULL strings", 
	       status == PCSL_STRING_OK);
    status = pcsl_string_compare(&local_literal, &result_string, &comparison);
    assertTrue("The result string is incorrect", comparison == 0);

    status = pcsl_string_cat(NULL, &local_literal, &result_string);

    assertTrue("Incorrect status on concat NULL and not NULL strings", 
	       status == PCSL_STRING_OK);
    status = pcsl_string_compare(&local_literal, &result_string, &comparison);
    assertTrue("The result string is incorrect", comparison == 0);

    str1 = PCSL_STRING_NULL;
    status = pcsl_string_cat(&str1, &local_literal, &result_string);

    assertTrue("Incorrect status on concat NULL and not NULL strings", 
	       status == PCSL_STRING_OK);
    status = pcsl_string_compare(&local_literal, &result_string, &comparison);
    assertTrue("The result string is incorrect", comparison == 0);

    str1 = PCSL_STRING_EMPTY;
    status = pcsl_string_cat(&str1, &local_literal, &result_string);

    assertTrue("Incorrect status on concat NULL and not NULL strings", 
	       status == PCSL_STRING_OK);
    status = pcsl_string_compare(&local_literal, &result_string, &comparison);
    assertTrue("The result string is incorrect", comparison == 0);
  }
}

static void test_pcsl_string_dup() {
    pcsl_string src, dst;
    pcsl_string_status status;
  /*
   * Test case 1. Call with wrong args
   */
  {
    status = pcsl_string_dup(NULL, &dst);

    assertTrue("Incorrect status on NULL src", 
	       status == PCSL_STRING_EINVAL);

    status = pcsl_string_dup(&src, NULL);

    assertTrue("Incorrect status on NULL dst", 
	       status == PCSL_STRING_EINVAL);
    src.length = 5;
    src.data = NULL;
    status = pcsl_string_dup(&src, &dst);

    assertTrue("Incorrect status on damaged src", 
	       status == PCSL_STRING_EINVAL);
  }
  /*
   * Test case 2. Duplicate null and empty strings
   */
  {
    status = pcsl_string_dup(&PCSL_STRING_NULL, &dst);
    assertTrue("Incorrect status on duplicating null string", 
	       status == PCSL_STRING_OK);
    assertTrue("Target string is not null", 
	       pcsl_string_length(&dst) == (jsize)-1);

    status = pcsl_string_dup(&PCSL_STRING_EMPTY, &dst);
    assertTrue("Incorrect status on duplicating empty string", 
	       status == PCSL_STRING_OK);
    assertTrue("Target string is not empty", 
	       pcsl_string_length(&dst) == (jsize)0);
  }
  /*
   * Test case 3. Duplicate non-empty string.
   */
  {
    jint comparison;
    jchar buffer[] = LOCAL_ASCII_LITERAL;
    jsize buffer_length = sizeof(buffer) / sizeof(jchar);
    status = pcsl_string_convert_from_utf16(buffer, buffer_length, &src);

    assertTrue("Incorrect status on common case", status == PCSL_STRING_OK);


    status = pcsl_string_dup(&src, &dst);

    assertTrue("Incorrect status on duplicate string", 
	       status == PCSL_STRING_OK);
    /* Check that internal pointers are different */
    assertTrue("Duplicate string contains the same data buffer", 
	       memcmp(&src, &dst, sizeof(pcsl_string)) != 0);
    status = pcsl_string_compare(&src, &dst, &comparison);
    assertTrue("The result string is incorrect", comparison == 0);
    pcsl_string_free(&src);
    pcsl_string_free(&dst);
  }
}

static void test_pcsl_string_index_of() {
  /*
   * Test case 1. NULL string
   */
  {
    const jint null_index = 
      pcsl_string_index_of(NULL, 1);

    assertTrue("Invalid index in NULL string", null_index < 0);
  }

  /*
   * Test case 2. Empty string
   */
  {
    const jint empty_index = 
      pcsl_string_index_of(&PCSL_STRING_EMPTY, 1);

    assertTrue("Invalid index of empty string", empty_index < 0);
  }

  /*
   * Test case 3. Invalid Unicode code point
   */
  {
    const jint index = 
      pcsl_string_index_of(&static_literal, 0xffff0000);

    assertTrue("Invalid index of invalid code point", index < 0);
  }

  /*
   * Test case 4. Common case
   */
  {
    const jbyte static_ascii_literal[] = STATIC_ASCII_LITERAL;
    const jint static_ascii_literal_length = 
      sizeof(static_ascii_literal) / sizeof(jbyte);
    jint i = 0;

    // NOTE: we assume that all characters are different in static_literal

    for (i = 0; i < static_ascii_literal_length; i++) {
      const jint index = 
	pcsl_string_index_of(&static_literal, static_ascii_literal[i]);

      assertTrue("Invalid index in common case", index == i);
    }
  }

  /*
   * Test case 5. UTF16 surrogate characters
   */
  {
    pcsl_string surrogate_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf16(surrogate_utf16_chars,
				     surrogate_utf16_chars_length,
				     &surrogate_string);

    if (status == PCSL_STRING_OK) {
      {
	const jint surrogate_index = 
	  pcsl_string_index_of(&surrogate_string, surrogate_code_point);
    
	assertTrue("Invalid surrogate index", 
		   surrogate_index == 0);
      }

      {
	const jint high_surrogate_index = 
	  pcsl_string_index_of(&surrogate_string, surrogate_utf16_chars[0]);
    
	assertTrue("Invalid high surrogate index", 
		   high_surrogate_index < 0);
      }

      {
	const jint low_surrogate_index = 
	  pcsl_string_index_of(&surrogate_string, surrogate_utf16_chars[1]);
    
	assertTrue("Invalid low surrogate index", 
		   low_surrogate_index < 0);
      }

      status = pcsl_string_free(&surrogate_string);

      assertTrue("Failed to destroy utf16 surrogate string", 
		 status == PCSL_STRING_OK);
    }
  }

  /*
   * Test case 6. ASCII char plus UTF16 surrogate characters
   */
  {
    pcsl_string app_surrogate_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf16(app_surrogate_utf16_chars,
				     app_surrogate_utf16_chars_length,
				     &app_surrogate_string);

    if (status == PCSL_STRING_OK) {
      {
	const jint surrogate_index = 
	  pcsl_string_index_of(&app_surrogate_string, surrogate_code_point);
    
	assertTrue("Invalid surrogate index", 
		   surrogate_index == 1);
      }

      {
	const jint high_surrogate_index = 
	  pcsl_string_index_of(&app_surrogate_string, surrogate_utf16_chars[0]);
    
	assertTrue("Invalid high surrogate index", 
		   high_surrogate_index < 0);
      }

      {
	const jint low_surrogate_index = 
	  pcsl_string_index_of(&app_surrogate_string, surrogate_utf16_chars[1]);
    
	assertTrue("Invalid low surrogate index", 
		   low_surrogate_index < 0);
      }

      status = pcsl_string_free(&app_surrogate_string);

      assertTrue("Failed to destroy utf16 surrogate string", 
		 status == PCSL_STRING_OK);
    }
  }
}

static void test_pcsl_string_index_of_from() {
  /*
   * Test case 1. NULL string
   */
  {
    jint null_index = 
      pcsl_string_index_of_from(NULL, 1, 0);

    assertTrue("Invalid index in NULL string", null_index < 0);

    null_index = 
      pcsl_string_index_of_from(NULL, 1, -1);

    assertTrue("Invalid index in NULL string", null_index < 0);
  }

  /*
   * Test case 2. Empty string
   */
  {
    jint empty_index = 
      pcsl_string_index_of_from(&PCSL_STRING_EMPTY, 1, 0);

    assertTrue("Invalid index of empty string", empty_index < 0);

    empty_index = 
      pcsl_string_index_of_from(&PCSL_STRING_EMPTY, 1, -1);

    assertTrue("Invalid index of empty string", empty_index < 0);
  }

  /*
   * Test case 3. Invalid Unicode code point
   */
  {
    const jint index = 
      pcsl_string_index_of_from(&static_literal, 0xffff0000, 0);

    assertTrue("Invalid index of invalid code point", index < 0);
  }

  /*
   * Test case 4. Common case
   */
  {
    const jbyte static_ascii_literal[] = STATIC_ASCII_LITERAL;
    const jint static_ascii_literal_length = 
      sizeof(static_ascii_literal) / sizeof(jbyte);
    jint i = 0;

    // NOTE: we assume that all characters are different in static_literal

    for (i = 0; i < static_ascii_literal_length; i++) {
      const jint index = 
	pcsl_string_index_of_from(&static_literal, static_ascii_literal[i], 0);

      assertTrue("Invalid index in common case", index == i);
    }

    for (i = 0; i < static_ascii_literal_length; i++) {
      const jint index = 
	pcsl_string_index_of_from(&static_literal, static_ascii_literal[i], i);

      assertTrue("Invalid index in common case", index == i);
    }

    for (i = 0; i < static_ascii_literal_length; i++) {
      const jint index = 
	pcsl_string_index_of_from(&static_literal, static_ascii_literal[i], i + 1);

      assertTrue("Invalid index in common case", index < 0);
    }
  }

  /*
   * Test case 5. UTF16 surrogate characters
   */
  {
    pcsl_string surrogate_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf16(surrogate_utf16_chars,
				     surrogate_utf16_chars_length,
				     &surrogate_string);

    if (status == PCSL_STRING_OK) {
      {
	const jint surrogate_index = 
	  pcsl_string_index_of_from(&surrogate_string, surrogate_code_point, 0);
    
	assertTrue("Invalid surrogate index", 
		   surrogate_index == 0);
      }

      {
	const jint surrogate_index = 
	  pcsl_string_index_of_from(&surrogate_string, surrogate_code_point, 1);
    
	assertTrue("Invalid surrogate index", 
		   surrogate_index < 0);
      }

      {
	const jint high_surrogate_index = 
	  pcsl_string_index_of_from(&surrogate_string, 
				    surrogate_utf16_chars[0], 0);
    
	assertTrue("Invalid high surrogate index", 
		   high_surrogate_index < 0);
      }

      {
	const jint low_surrogate_index = 
	  pcsl_string_index_of_from(&surrogate_string, 
				    surrogate_utf16_chars[1], 1);
    
	assertTrue("Invalid low surrogate index", 
		   low_surrogate_index < 0);
      }

      status = pcsl_string_free(&surrogate_string);

      assertTrue("Failed to destroy utf16 surrogate string", 
		 status == PCSL_STRING_OK);
    }
  }

  /*
   * Test case 6. ASCII char plus UTF16 surrogate characters
   */
  {
    pcsl_string app_surrogate_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf16(app_surrogate_utf16_chars,
				     app_surrogate_utf16_chars_length,
				     &app_surrogate_string);

    if (status == PCSL_STRING_OK) {
      {
	const jint surrogate_index = 
	  pcsl_string_index_of_from(&app_surrogate_string, 
				    surrogate_code_point, 0);
    
	assertTrue("Invalid surrogate index", 
		   surrogate_index == 1);
      }

      {
	const jint surrogate_index = 
	  pcsl_string_index_of_from(&app_surrogate_string, 
				    surrogate_code_point, 1);
    
	assertTrue("Invalid surrogate index", 
		   surrogate_index == 1);
      }

      {
	const jint high_surrogate_index = 
	  pcsl_string_index_of_from(&app_surrogate_string, 
			       surrogate_utf16_chars[0], 1);
    
	assertTrue("Invalid high surrogate index", 
		   high_surrogate_index < 0);
      }

      {
	const jint low_surrogate_index = 
	  pcsl_string_index_of_from(&app_surrogate_string, 
			       surrogate_utf16_chars[1], 2);
    
	assertTrue("Invalid low surrogate index", 
		   low_surrogate_index < 0);
      }

      status = pcsl_string_free(&app_surrogate_string);

      assertTrue("Failed to destroy utf16 surrogate string", 
		 status == PCSL_STRING_OK);
    }
  }
}

static void test_pcsl_string_last_index_of() {
  /*
   * Test case 1. NULL string
   */
  {
    const jint null_index = 
      pcsl_string_last_index_of(NULL, 1);

    assertTrue("Invalid index in NULL string", null_index < 0);
  }

  /*
   * Test case 2. Empty string
   */
  {
    const jint empty_index = 
      pcsl_string_last_index_of(&PCSL_STRING_EMPTY, 1);

    assertTrue("Invalid index of empty string", empty_index < 0);
  }

  /*
   * Test case 3. Invalid Unicode code point
   */
  {
    const jint index = 
      pcsl_string_last_index_of(&static_literal, 0xffff0000);

    assertTrue("Invalid index of invalid code point", index < 0);
  }

  /*
   * Test case 4. Common case
   */
  {
    const jbyte static_ascii_literal[] = STATIC_ASCII_LITERAL;
    const jint static_ascii_literal_length = 
      sizeof(static_ascii_literal) / sizeof(jbyte);
    jint i = 0;

    // NOTE: we assume that all characters are different in static_literal

    for (i = 0; i < static_ascii_literal_length; i++) {
      const jint index = 
	pcsl_string_last_index_of(&static_literal, static_ascii_literal[i]);

      assertTrue("Invalid index in common case", index == i);
    }
  }

  /*
   * Test case 5. UTF16 surrogate characters
   */
  {
    pcsl_string surrogate_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf16(surrogate_utf16_chars,
				     surrogate_utf16_chars_length,
				     &surrogate_string);

    if (status == PCSL_STRING_OK) {
      {
	const jint surrogate_index = 
	  pcsl_string_last_index_of(&surrogate_string, surrogate_code_point);
    
	assertTrue("Invalid surrogate index", 
		   surrogate_index == 0);
      }

      {
	const jint high_surrogate_index = 
	  pcsl_string_last_index_of(&surrogate_string, surrogate_utf16_chars[0]);
    
	assertTrue("Invalid high surrogate index", 
		   high_surrogate_index < 0);
      }

      {
	const jint low_surrogate_index = 
	  pcsl_string_last_index_of(&surrogate_string, surrogate_utf16_chars[1]);
    
	assertTrue("Invalid low surrogate index", 
		   low_surrogate_index < 0);
      }

      status = pcsl_string_free(&surrogate_string);

      assertTrue("Failed to destroy utf16 surrogate string", 
		 status == PCSL_STRING_OK);
    }
  }

  /*
   * Test case 6. ASCII char plus UTF16 surrogate characters
   */
  {
    pcsl_string app_surrogate_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf16(app_surrogate_utf16_chars,
				     app_surrogate_utf16_chars_length,
				     &app_surrogate_string);

    if (status == PCSL_STRING_OK) {
      {
	const jint surrogate_index = 
	  pcsl_string_last_index_of(&app_surrogate_string, 
				    surrogate_code_point);
    
	assertTrue("Invalid surrogate index", 
		   surrogate_index == 1);
      }

      {
	const jint high_surrogate_index = 
	  pcsl_string_last_index_of(&app_surrogate_string, 
				    surrogate_utf16_chars[0]);
    
	assertTrue("Invalid high surrogate index", 
		   high_surrogate_index < 0);
      }

      {
	const jint low_surrogate_index = 
	  pcsl_string_last_index_of(&app_surrogate_string, 
				    surrogate_utf16_chars[1]);
    
	assertTrue("Invalid low surrogate index", 
		   low_surrogate_index < 0);
      }

      status = pcsl_string_free(&app_surrogate_string);

      assertTrue("Failed to destroy utf16 surrogate string", 
		 status == PCSL_STRING_OK);
    }
  }
}

static void test_pcsl_string_last_index_of_from() {
  /*
   * Test case 1. NULL string
   */
  {
    jint null_index = 
      pcsl_string_last_index_of_from(NULL, 1, 0);

    assertTrue("Invalid index in NULL string", null_index < 0);

    null_index = 
      pcsl_string_last_index_of_from(NULL, 1, -1);

    assertTrue("Invalid index in NULL string", null_index < 0);
  }

  /*
   * Test case 2. Empty string
   */
  {
    jint empty_index = 
      pcsl_string_last_index_of_from(&PCSL_STRING_EMPTY, 1, 0);

    assertTrue("Invalid index of empty string", empty_index < 0);

    empty_index = 
      pcsl_string_last_index_of_from(&PCSL_STRING_EMPTY, 1, -1);

    assertTrue("Invalid index of empty string", empty_index < 0);
  }

  /*
   * Test case 3. Invalid Unicode code point
   */
  {
    const jint index = 
      pcsl_string_last_index_of_from(&static_literal, 0xffff0000, 0);

    assertTrue("Invalid index of invalid code point", index < 0);
  }

  /*
   * Test case 4. Common case
   */
  {
    const jbyte static_ascii_literal[] = STATIC_ASCII_LITERAL;
    const jint static_ascii_literal_length = 
      sizeof(static_ascii_literal) / sizeof(jbyte);
    jint i = 0;

    // NOTE: we assume that all characters are different in static_literal

    for (i = 0; i < static_ascii_literal_length; i++) {
      const jint index = 
	pcsl_string_last_index_of_from(&static_literal, 
				       static_ascii_literal[i], 
				       static_ascii_literal_length - 1);

      assertTrue("Invalid index in common case", index == i);
    }

    for (i = 0; i < static_ascii_literal_length; i++) {
      const jint index = 
	pcsl_string_last_index_of_from(&static_literal, 
				       static_ascii_literal[i], i);

      assertTrue("Invalid index in common case", index == i);
    }

    for (i = 0; i < static_ascii_literal_length; i++) {
      const jint index = 
	pcsl_string_last_index_of_from(&static_literal, 
				       static_ascii_literal[i], i - 1);

      assertTrue("Invalid index in common case", index < 0);
    }
  }

  /*
   * Test case 5. UTF16 surrogate characters
   */
  {
    pcsl_string surrogate_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf16(surrogate_utf16_chars,
				     surrogate_utf16_chars_length,
				     &surrogate_string);

    if (status == PCSL_STRING_OK) {
      {
	const jint surrogate_index = 
	  pcsl_string_last_index_of_from(&surrogate_string, 
					 surrogate_code_point, 0);
    
	assertTrue("Invalid surrogate index", 
		   surrogate_index == 0);
      }

      {
	const jint surrogate_index = 
	  pcsl_string_last_index_of_from(&surrogate_string, 
					 surrogate_code_point, 1);
    
	assertTrue("Invalid surrogate index", 
		   surrogate_index == 0);
      }

      {
	const jint high_surrogate_index = 
	  pcsl_string_last_index_of_from(&surrogate_string, 
				    surrogate_utf16_chars[0], 1);
    
	assertTrue("Invalid high surrogate index", 
		   high_surrogate_index < 0);
      }

      {
	const jint low_surrogate_index = 
	  pcsl_string_last_index_of_from(&surrogate_string, 
				    surrogate_utf16_chars[1], 2);
    
	assertTrue("Invalid low surrogate index", 
		   low_surrogate_index < 0);
      }

      status = pcsl_string_free(&surrogate_string);

      assertTrue("Failed to destroy utf16 surrogate string", 
		 status == PCSL_STRING_OK);
    }
  }

  /*
   * Test case 6. ASCII char plus UTF16 surrogate characters
   */
  {
    pcsl_string app_surrogate_string;
    pcsl_string_status status = 
      pcsl_string_convert_from_utf16(app_surrogate_utf16_chars,
				     app_surrogate_utf16_chars_length,
				     &app_surrogate_string);

    if (status == PCSL_STRING_OK) {
      {
	const jint surrogate_index = 
	  pcsl_string_last_index_of_from(&app_surrogate_string, 
				    surrogate_code_point, 1);
    
	assertTrue("Invalid surrogate index", 
		   surrogate_index == 1);
      }

      {
	const jint surrogate_index = 
	  pcsl_string_last_index_of_from(&app_surrogate_string, 
				    surrogate_code_point, 2);
    
	assertTrue("Invalid surrogate index", 
		   surrogate_index == 1);
      }

      {
	const jint high_surrogate_index = 
	  pcsl_string_last_index_of_from(&app_surrogate_string, 
			       surrogate_utf16_chars[0], 1);
    
	assertTrue("Invalid high surrogate index", 
		   high_surrogate_index < 0);
      }

      {
	const jint low_surrogate_index = 
	  pcsl_string_last_index_of_from(&app_surrogate_string, 
			       surrogate_utf16_chars[1], 2);
    
	assertTrue("Invalid low surrogate index", 
		   low_surrogate_index < 0);
      }

      status = pcsl_string_free(&app_surrogate_string);

      assertTrue("Failed to destroy utf16 surrogate string", 
		 status == PCSL_STRING_OK);
    }
  }
}

static void test_pcsl_string_trim() {
  pcsl_string src, dst;
  pcsl_string_status status;
  /*
   * Test case 1. Wrong arguments
   */
  status = pcsl_string_trim(NULL, &dst);
  assertTrue("Wrong status when source string is NULL", 
    status == PCSL_STRING_EINVAL);

  status = pcsl_string_trim(&src, NULL);
  assertTrue("Wrong status when destination string is NULL", 
    status == PCSL_STRING_EINVAL);
  /*
   * Test case 2. Damaged string
   */
  src.data = NULL;
  src.length = 5;
  status = pcsl_string_trim(&src, &dst);
  assertTrue("Wrong status when source string is damaged",
    status == PCSL_STRING_EINVAL);
  /*
   * Test case 3. Null and empty strings
   */
  src = PCSL_STRING_NULL;
  status = pcsl_string_trim(&src, &dst);
  assertTrue("Wrong status on trimming null string",
    status == PCSL_STRING_OK);
  assertTrue("Result string is not NULL", 
    memcmp(&dst, &PCSL_STRING_NULL, sizeof(pcsl_string)) == 0.);

  src = PCSL_STRING_EMPTY;
  status = pcsl_string_trim(&src, &dst);
  assertTrue("Wrong status when trimming empty string", 
    status == PCSL_STRING_OK);
  assertTrue("Result string is not empty", 
    memcmp(&dst, &PCSL_STRING_EMPTY, sizeof(pcsl_string)) == 0.);
  /*
   * Test case 4. Trim
   */
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(source_str)
    { ' ', ' ', 'H', 'e', 'l', 'l', 'o', ' ', ' ', ' ', ' ', '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(source_str);
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(reserved_str)
    { ' ', ' ', 'H', 'e', 'l', 'l', 'o', ' ', ' ', ' ', ' ', '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(reserved_str);
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(result_str)
    { 'H', 'e', 'l', 'l', 'o',  '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(result_str);
    status = pcsl_string_trim(&source_str, &dst);
    assertTrue("Wrong status on trimming string", 
      status == PCSL_STRING_OK);
    assertTrue("Strings are different", 
      pcsl_string_equals(&dst, &result_str) == PCSL_TRUE);
    status = pcsl_string_free(&dst);
    assertTrue("Free result string status is wrong", status == PCSL_STRING_OK);
    assertTrue("Source string has changed",
      pcsl_string_equals(&source_str, &reserved_str) == PCSL_TRUE);
  }
}

static void test_pcsl_string_trim_from_end() {
  pcsl_string src, dst;
  pcsl_string_status status;
  /*
   * Test case 1. Wrong arguments
   */
  status = pcsl_string_trim_from_end(NULL, &dst);
  assertTrue("Wrong status when source string is NULL",
    status == PCSL_STRING_EINVAL);

  status = pcsl_string_trim_from_end(&src, NULL);
  assertTrue("Wrong status when destination string is NULL",
    status == PCSL_STRING_EINVAL);
  /*
   * Test case 2. Damaged string
   */
  src.data = NULL;
  src.length = 5;
  status = pcsl_string_trim_from_end(&src, &dst);
  assertTrue("Wrong status when source string is damaged",
    status == PCSL_STRING_EINVAL);
  /*
   * Test case 3. Null and empty strings
   */
  src = PCSL_STRING_NULL;
  status = pcsl_string_trim_from_end(&src, &dst);
  assertTrue("Wrong status on trimming null string", 
    status == PCSL_STRING_OK);
  assertTrue("Result string is not NULL",
    memcmp(&dst, &PCSL_STRING_NULL, sizeof(pcsl_string)) == 0.);

  src = PCSL_STRING_EMPTY;
  status = pcsl_string_trim_from_end(&src, &dst);
  assertTrue("Wrong status on trimming empty string",
    status == PCSL_STRING_OK);
  assertTrue("Result string is not empty", 
    memcmp(&dst, &PCSL_STRING_EMPTY, sizeof(pcsl_string)) == 0.);
  /*
   * Test case 4. Trim
   */
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(source_str)
    { 'H', 'e', 'l', 'l', 'o', ' ', ' ', ' ', ' ', '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(source_str);
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(reserved_str)
    { 'H', 'e', 'l', 'l', 'o', ' ', ' ', ' ', ' ', '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(reserved_str);
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(result_str)
    { 'H', 'e', 'l', 'l', 'o',  '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(result_str);
    status = pcsl_string_trim_from_end(&source_str, &dst);
    assertTrue("Wrong status on trimming string", 
      status == PCSL_STRING_OK);
    assertTrue("Compare string is wrong",
      pcsl_string_equals(&dst, &result_str) == PCSL_TRUE);
    status = pcsl_string_free(&dst);
    assertTrue("Free result string is wrong", status == PCSL_STRING_OK);
    assertTrue("Source string has changed",
      pcsl_string_equals(&source_str, &reserved_str) == PCSL_TRUE);
  }
}

static void test_pcsl_string_convert_to_jint() {
  pcsl_string_status status;
  pcsl_string src;
  jint value;
  /*
   * Test case 1. Wrong arguments
   */
  status = pcsl_string_convert_to_jint(NULL, &value);
  assertTrue("Wrong status when string pointer is NULL", 
    status == PCSL_STRING_EINVAL);
  status = pcsl_string_convert_to_jint(&src, NULL);
  assertTrue("Wrong status when value pointer is NULL", 
    status == PCSL_STRING_EINVAL);
  src = PCSL_STRING_NULL;
  status = pcsl_string_convert_to_jint(&src, &value);
  assertTrue("Wrong status when data buffer is NULL", 
    status == PCSL_STRING_EINVAL);
  /*
   * Test case 2. Convert empty string
   */
  src = PCSL_STRING_EMPTY;
  status = pcsl_string_convert_to_jint(&src, &value);
  assertTrue("Wrong status when convert empty string", 
    status == PCSL_STRING_EINVAL);
  /*
   * Test case 3. Trailing zeros
   */
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(tr_zeros_str)
    {
      '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
      '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
      '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
      '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
      '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
      '5', '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(tr_zeros_str);
    status = pcsl_string_convert_to_jint(&tr_zeros_str, &value);
    assertTrue("Wrong status when convert trailing zeros string", 
      status == PCSL_STRING_OK);
    assertTrue("Value is incorrect", value == 5);
  }
  /*
   * Test case 4. Maximal and minimal values
   */
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(maxval_str)
    {'2', '1', '4', '7', '4', '8', '3', '6', '4', '7', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(maxval_str);
    status = pcsl_string_convert_to_jint(&maxval_str, &value);
    assertTrue("Wrong status on converting maxval string",
      status == PCSL_STRING_OK);
    assertTrue("Value is incorrect", value == 2147483647);
  }
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(minval_str)
    {'-', '2', '1', '4', '7', '4', '8', '3', '6', '4', '8', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(minval_str);
    status = pcsl_string_convert_to_jint(&minval_str, &value);
    assertTrue("Wrong status on converting minval string",
      status == PCSL_STRING_OK);
    assertTrue("Value is incorrect", value == 0x80000000);
  }
  /*
   * Test case 5. Overflow values
   */
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(pos_ovrw_str)
    {'2', '1', '4', '7', '4', '8', '3', '6', '4', '8', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(pos_ovrw_str);
    status = pcsl_string_convert_to_jint(&pos_ovrw_str, &value);
    assertTrue("Wrong status on converting positive overflow string",
      status == PCSL_STRING_OVERFLOW);
  }
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(neg_ovrw_str)
    {'-', '2', '1', '4', '7', '4', '8', '3', '6', '4', '8', '5', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(neg_ovrw_str);
    status = pcsl_string_convert_to_jint(&neg_ovrw_str, &value);
    assertTrue("Wrong status on converting negative overflow string",
      status == PCSL_STRING_OVERFLOW);
  }
  /*
   * Test case 6. Non-digit symbols
   */
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(non_dig_str)
    {'1', '2', '7', 'd', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(non_dig_str);
    status = pcsl_string_convert_to_jint(&non_dig_str, &value);
    assertTrue("Wrong status on converting non-digit string",
      status == PCSL_STRING_EINVAL);
  }
  /*
   * Test case 7. Plus symbol
   */
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(plus_str)
    {'+', '1', '2', '3', '4', '5', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(plus_str);
    status = pcsl_string_convert_to_jint(&plus_str, &value);
    assertTrue("Wrong status on converting maxval string",
      status == PCSL_STRING_OK);
    assertTrue("Value is incorrect", value == 12345);
  }
}

static void test_pcsl_string_convert_from_jint() {
  pcsl_string dst;
  jint cmp;
  pcsl_string_status status;
  /*
   * Test case 1. Wrong arguments
   */
  status = pcsl_string_convert_from_jint(0, NULL);
  assertTrue("Wrong status when destination string is NULL", 
    status == PCSL_STRING_EINVAL);
  /*
   * Test case 2. Convert zero
   */
  status = pcsl_string_convert_from_jint(0, &dst);
  assertTrue("Wrong status when zero converts", 
    status == PCSL_STRING_OK);
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(zero_str)
    { '0', '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(zero_str);
    status = pcsl_string_compare(&dst, &zero_str, &cmp);
    assertTrue("Wrong status on string comparing", 
      status == PCSL_STRING_OK);
    assertTrue("Strings are not equal", cmp == 0);
  }
  status = pcsl_string_free(&dst);
  assertTrue("Free string is not OK", status == PCSL_STRING_OK);
  /*
   * Test case 3. Convert positive value
   */
  status = pcsl_string_convert_from_jint(3512, &dst);
  assertTrue("Wrong status when positive value converts", 
    status == PCSL_STRING_OK);
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(positive_str)
    { '3', '5', '1', '2', '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(positive_str);
    status = pcsl_string_compare(&dst, &positive_str, &cmp);
    assertTrue("Wrong status when strings compares", 
      status == PCSL_STRING_OK);
    assertTrue("Strings are not equal", cmp == 0);
  }
  status = pcsl_string_free(&dst);
  assertTrue("Free string is not OK", status == PCSL_STRING_OK);
  /*
   * Test case 4. Convert negative value
   */
  status = pcsl_string_convert_from_jint(-53906, &dst);
  assertTrue("Wrong status when negative value converts", 
    status == PCSL_STRING_OK);
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(negative_str)
    { '-', '5', '3', '9', '0', '6', '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(negative_str);
    status = pcsl_string_compare(&dst, &negative_str, &cmp);
    assertTrue("Wrong status when strings compare", 
      status == PCSL_STRING_OK);
    assertTrue("Strings are not equal", cmp == 0);
  }
  status = pcsl_string_free(&dst);
  assertTrue("Free string is not OK", status == PCSL_STRING_OK);
  /*
   * Test case 5. Convert maximal integer value
   */
  status = pcsl_string_convert_from_jint(0x7fffffff, &dst);
  assertTrue("Wrong status when maximal integer value converts", 
    status == PCSL_STRING_OK);
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(max_int_str)
    { '2', '1', '4', '7', '4', '8', '3', '6', '4', '7', '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(max_int_str);
    status = pcsl_string_compare(&dst, &max_int_str, &cmp);
    assertTrue("Wrong status when strings compares", 
      status == PCSL_STRING_OK);
    assertTrue("Strings are not equal", cmp == 0);
  }
  status = pcsl_string_free(&dst);
  assertTrue("Free string is not OK", status == PCSL_STRING_OK);
  /*
   * Test case 6. Convert minimal integer value
   */
  status = pcsl_string_convert_from_jint(0x80000000, &dst);
  assertTrue("Wrong status when maximal integer value converts", 
    status == PCSL_STRING_OK);
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(min_int_str)
    { '-', '2', '1', '4', '7', '4', '8', '3', '6', '4', '8', '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(min_int_str);
    status = pcsl_string_compare(&dst, &min_int_str, &cmp);
    assertTrue("Wrong status when strings compare", 
      status == PCSL_STRING_OK);
    assertTrue("Strings are not equal", cmp == 0);
  }
  status = pcsl_string_free(&dst);
  assertTrue("Free string is not OK", status == PCSL_STRING_OK);
}

static void test_pcsl_string_convert_to_jlong() {
  pcsl_string_status status;
  pcsl_string src;
  jlong value;
  /*
   * Test case 1. Wrong arguments
   */
  status = pcsl_string_convert_to_jlong(NULL, &value);
  assertTrue("Wrong status when string pointer is NULL", 
    status == PCSL_STRING_EINVAL);
  status = pcsl_string_convert_to_jlong(&src, NULL);
  assertTrue("Wrong status when value pointer is NULL", 
    status == PCSL_STRING_EINVAL);
  src = PCSL_STRING_NULL;
  status = pcsl_string_convert_to_jlong(&src, &value);
  assertTrue("Wrong status when data buffer is NULL", 
    status == PCSL_STRING_EINVAL);
  /*
   * Test case 2. Convert empty string
   */
  src = PCSL_STRING_EMPTY;
  status = pcsl_string_convert_to_jlong(&src, &value);
  assertTrue("Wrong status when convert empty string", 
    status == PCSL_STRING_EINVAL);
  /*
   * Test case 3. Trailing zeros
   */
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(tr_zeros_str)
    {
      '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
      '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
      '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
      '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
      '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
      '5', '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(tr_zeros_str);
    status = pcsl_string_convert_to_jlong(&tr_zeros_str, &value);
    assertTrue("Wrong status when convert trailing zeros string", 
      status == PCSL_STRING_OK);
    assertTrue("Value is incorrect", value == 5);
  }
  /*
   * Test case 4. Maximal and minimal values
   */
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(maxval_str)
    { '9', '2', '2', '3', '3', '7', '2', '0', '3', '6', '8',
      '5', '4', '7', '7', '5', '8', '0', '7', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(maxval_str);
    status = pcsl_string_convert_to_jlong(&maxval_str, &value);
    assertTrue("Wrong status on converting maxval string",
      status == PCSL_STRING_OK);
    assertTrue("Value is incorrect", value ==  MAX_LONG);
  }
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(minval_str)
    { '-', '9', '2', '2', '3', '3', '7', '2', '0', '3', '6', '8',
      '5', '4', '7', '7', '5', '8', '0', '8', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(minval_str);
    status = pcsl_string_convert_to_jlong(&minval_str, &value);
    assertTrue("Wrong status on converting minval string",
      status == PCSL_STRING_OK);
    assertTrue("Value is incorrect", value == MIN_LONG);
  }
  /*
   * Test case 5. Overflow values
   */
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(pos_ovrw_str)
    { '9', '2', '2', '3', '3', '7', '2', '0', '3', '6', '8',
      '5', '4', '7', '7', '5', '8', '0', '8', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(pos_ovrw_str);
    status = pcsl_string_convert_to_jlong(&pos_ovrw_str, &value);
    assertTrue("Wrong status on converting positive overflow string",
      status == PCSL_STRING_OVERFLOW);
  }
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(neg_ovrw_str)
    { '-', '9', '2', '2', '3', '3', '7', '2', '0', '3', '6', '8',
      '5', '4', '7', '7', '5', '8', '0', '8', '5', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(neg_ovrw_str);
    status = pcsl_string_convert_to_jlong(&neg_ovrw_str, &value);
    assertTrue("Wrong status on converting negative overflow string",
      status == PCSL_STRING_OVERFLOW);
  }
  /*
   * Test case 6. Non-digit symbols
   */
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(non_dig_str)
    {'1', '2', '7', 'd', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(non_dig_str);
    status = pcsl_string_convert_to_jlong(&non_dig_str, &value);
    assertTrue("Wrong status on converting non-digit string",
      status == PCSL_STRING_EINVAL);
  }
  /*
   * Test case 7. Plus symbol
   */
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(plus_str)
    {'+', '1', '2', '3', '4', '5', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(plus_str);
    status = pcsl_string_convert_to_jlong(&plus_str, &value);
    assertTrue("Wrong status on converting maxval string",
      status == PCSL_STRING_OK);
    assertTrue("Value is incorrect", value == 12345);
  }
}

static void test_pcsl_string_convert_from_jlong() {
  pcsl_string dst;
  jint cmp;
  pcsl_string_status status;
  /*
   * Test case 1. Wrong arguments
   */
  status = pcsl_string_convert_from_jlong(0, NULL);
  assertTrue("Wrong status when destination string is NULL", 
    status == PCSL_STRING_EINVAL);
  /*
   * Test case 2. Convert zero
   */
  status = pcsl_string_convert_from_jlong(0, &dst);
  assertTrue("Wrong status when zero converts", 
    status == PCSL_STRING_OK);
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(zero_str)
    { '0', '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(zero_str);
    status = pcsl_string_compare(&dst, &zero_str, &cmp);
    assertTrue("Wrong status on string comparing", 
      status == PCSL_STRING_OK);
    assertTrue("Strings are not equal", cmp == 0);
  }
  status = pcsl_string_free(&dst);
  assertTrue("Free string is not OK", status == PCSL_STRING_OK);
  /*
   * Test case 3. Convert positive value
   */
  status = pcsl_string_convert_from_jlong(3512, &dst);
  assertTrue("Wrong status when positive value converts", 
    status == PCSL_STRING_OK);
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(positive_str)
    { '3', '5', '1', '2', '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(positive_str);
    status = pcsl_string_compare(&dst, &positive_str, &cmp);
    assertTrue("Wrong status when strings compares", 
      status == PCSL_STRING_OK);
    assertTrue("Strings are not equal", cmp == 0);
  }
  status = pcsl_string_free(&dst);
  assertTrue("Free string is not OK", status == PCSL_STRING_OK);
  /*
   * Test case 4. Convert negative value
   */
  status = pcsl_string_convert_from_jlong(-53906, &dst);
  assertTrue("Wrong status when negative value converts", 
    status == PCSL_STRING_OK);
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(negative_str)
    { '-', '5', '3', '9', '0', '6', '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(negative_str);
    status = pcsl_string_compare(&dst, &negative_str, &cmp);
    assertTrue("Wrong status when strings compare", 
      status == PCSL_STRING_OK);
    assertTrue("Strings are not equal", cmp == 0);
  }
  status = pcsl_string_free(&dst);
  assertTrue("Free string is not OK", status == PCSL_STRING_OK);
  /*
   * Test case 5. Convert maximal integer value
   */
  status = pcsl_string_convert_from_jlong(MAX_LONG, &dst);
  assertTrue("Wrong status when maximal integer value converts", 
    status == PCSL_STRING_OK);
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(max_int_str)
    { '9', '2', '2', '3', '3', '7', '2', '0', '3', '6', '8',
      '5', '4', '7', '7', '5', '8', '0', '7', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(max_int_str);
    status = pcsl_string_compare(&dst, &max_int_str, &cmp);
    assertTrue("Wrong status when strings compares", 
      status == PCSL_STRING_OK);
    assertTrue("Strings are not equal", cmp == 0);
  }
  status = pcsl_string_free(&dst);
  assertTrue("Free string is not OK", status == PCSL_STRING_OK);
  /*
   * Test case 6. Convert minimal integer value
   */
  status = pcsl_string_convert_from_jlong(MIN_LONG, &dst);
  assertTrue("Wrong status when maximal integer value converts", 
    status == PCSL_STRING_OK);
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(min_int_str)
    { '-', '9', '2', '2', '3', '3', '7', '2', '0', '3', '6', '8',
      '5', '4', '7', '7', '5', '8', '0', '8', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(min_int_str);
    status = pcsl_string_compare(&dst, &min_int_str, &cmp);
    assertTrue("Wrong status when strings compare", 
      status == PCSL_STRING_OK);
    assertTrue("Strings are not equal", cmp == 0);
  }
  status = pcsl_string_free(&dst);
  assertTrue("Free string is not OK", status == PCSL_STRING_OK);
}

static void test_pcsl_string_free() {
  pcsl_mem_finalize();
  pcsl_mem_initialize(NULL, 1024);

  /*
   * Test case 1. Memory leak test
   */
  {
    pcsl_string string;
    jbyte utf8_chars[] = {'0','1','2','3','4','5','6','7','8','9'};
    const jsize utf8_chars_length = sizeof(utf8_chars) / sizeof(jbyte);
    int i = 0;

    pcsl_string_status status = 
      pcsl_string_convert_from_utf8(utf8_chars, utf8_chars_length,
				    &string);

    if (status == PCSL_STRING_OK) {
      status = pcsl_string_free(&string);

      assertTrue("Failed to destroy string", status == PCSL_STRING_OK);

      for (i = 0; i < 1000; i++) {
	status = 
	  pcsl_string_convert_from_utf8(utf8_chars, utf8_chars_length,
					&string);

	assertTrue("Unexpectedly run out of memory - memory leak detected", 
		   status != PCSL_STRING_ENOMEM);

	if (status == PCSL_STRING_OK) {
	  status = pcsl_string_free(&string);

	  assertTrue("Failed to destroy string", status == PCSL_STRING_OK);
	}
      }
    }
  }

  pcsl_mem_finalize();
  pcsl_mem_initialize(NULL, 100*1024*1024);
}

static void test_pcsl_string_is_null() {
  jboolean cmp = PCSL_FALSE;
  pcsl_string null_string = PCSL_STRING_NULL;
  pcsl_string empty_string = PCSL_STRING_EMPTY;

  cmp = pcsl_string_is_null(&null_string);

  assertTrue("pcsl_string_is_null() returns false for null string", 
	     cmp == PCSL_TRUE);

  cmp = pcsl_string_is_null(&empty_string);

  assertTrue("pcsl_string_is_null() returns true for non-null string", 
	     cmp == PCSL_FALSE);
}

static void test_pcsl_string_predict_size() {
  jboolean cmp = PCSL_FALSE;
  pcsl_string acc = PCSL_STRING_NULL;
  PCSL_DEFINE_ASCII_STRING_LITERAL_START(tyry)
  { 'T', 'b', 'I', 'P', 'b', 'I',  '\0' }
  PCSL_DEFINE_ASCII_STRING_LITERAL_END(tyry);

  pcsl_string_dup(&tyry,&acc);
  pcsl_string_predict_size(&acc,100);
  cmp = pcsl_string_equals(&tyry,&acc);
  assertTrue("pcsl_string_predict_size() makes the string not equal to itself",
             cmp == PCSL_TRUE);
  pcsl_string_free(&acc);
}

static void test_pcsl_string_append() {
  jboolean cmp = PCSL_FALSE;
  pcsl_string acc = PCSL_STRING_NULL;
  jchar buf[]={' ','e','-'};
  int newlen = 0;
  PCSL_DEFINE_ASCII_STRING_LITERAL_START(tyry)
  { 'T', 'b', 'I', 'P', 'b', 'I',  '\0' }
  PCSL_DEFINE_ASCII_STRING_LITERAL_END(tyry);

  PCSL_DEFINE_ASCII_STRING_LITERAL_START(pyry)
  { 'n', 'b', 'I', 'p', 'b', 'I',  '\0' }
  PCSL_DEFINE_ASCII_STRING_LITERAL_END(pyry);

  PCSL_DEFINE_ASCII_STRING_LITERAL_START(moe)
  { 'M', 'O', 'E',  '\0' }
  PCSL_DEFINE_ASCII_STRING_LITERAL_END(moe);

  PCSL_DEFINE_ASCII_STRING_LITERAL_START(result)
  { 'T', 'b', 'I', 'P', 'b', 'I',  '-',
    'n', 'b', 'I', 'p', 'b', 'I',  ' ',
    'e', '-', 'M', 'O', 'E',  '\0' }
  PCSL_DEFINE_ASCII_STRING_LITERAL_END(result);

  cmp = pcsl_string_starts_with(&tyry,&tyry);
  assertTrue("test_pcsl_string_append pre-requisite 0 failure",
             cmp == PCSL_TRUE);

  cmp = pcsl_string_starts_with(&result,&tyry);
  assertTrue("test_pcsl_string_append pre-requisite 1 failure",
             cmp == PCSL_TRUE);

  pcsl_string_dup(&tyry,&acc);
  /* IMPL_NOTE: also test starting from null/empty/literal string */
  newlen = pcsl_string_length(&result);
  pcsl_string_predict_size(&acc,newlen); /* IMPL_NOTE: also test without this one */

  cmp = pcsl_string_starts_with(&result,&acc);
  assertTrue("test_pcsl_string_append pre-requisite failure",
             cmp == PCSL_TRUE);

  pcsl_string_append_char(&acc,'-');
  cmp = pcsl_string_starts_with(&result,&acc);
  assertTrue("pcsl_string_append_char() failure",
             cmp == PCSL_TRUE);

  pcsl_string_append(&acc,&pyry);
  cmp = pcsl_string_starts_with(&result,&acc);
  assertTrue("pcsl_string_append() failure",
             cmp == PCSL_TRUE);

  pcsl_string_append_buf(&acc,buf,sizeof(buf)/sizeof(buf[0]));
  cmp = pcsl_string_starts_with(&result,&acc);
  assertTrue("pcsl_string_append_buf() failure",
             cmp == PCSL_TRUE);

  pcsl_string_append(&acc,&moe);
  cmp = pcsl_string_equals(&result,&acc);
  assertTrue("pcsl_string_append*() final failure",
             cmp == PCSL_TRUE);

  pcsl_string_free(&acc);
}

static void test_pcsl_string_substring() {
  pcsl_string_status result;
  pcsl_string str, substr;
  /*
   * Test case 1. NULL args.
   */

  /* Sourse string pointer is NULL */
  result = pcsl_string_substring(NULL, 1, 2, &substr);
  assertTrue("Source string is NULL",
             result == PCSL_STRING_EINVAL);

  /* Destination string pointer is NULL */
  result = pcsl_string_substring(&str, 1, 2, NULL);
  assertTrue("Destination string is NULL",
             result == PCSL_STRING_EINVAL);

  /* Sourse string points to PCSL_STRING_NULL */
  str = PCSL_STRING_NULL;
  result = pcsl_string_substring(&str, 1, 2, &substr);
  assertTrue("Source string contains NULL data",
             result == PCSL_STRING_EINVAL);
  /*
   * Test case 2. Wrong bounds.
   */
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(source_str)
    { 'H', 'e', 'l', 'l', 'o', '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(source_str);

  /* Start index is negative */
    result = pcsl_string_substring(&source_str, -1, 2, &substr);
    assertTrue("First index is negative",
             result == PCSL_STRING_EINVAL);

  /* End index is negative */
    result = pcsl_string_substring(&source_str, 1, -2, &substr);
    assertTrue("Last index is negative",
             result == PCSL_STRING_EINVAL);

  /* Start index is more than end */
    result = pcsl_string_substring(&source_str, 3, 1, &substr);
    assertTrue("Last index is more than first",
             result == PCSL_STRING_EINVAL);

  /* Start index is more than string length */
    result = pcsl_string_substring(&source_str, 
             pcsl_string_length(&source_str) + 1, 2, &substr);
    assertTrue("First index is more than string length",
             result == PCSL_STRING_EINVAL);

  /* End index is more than string length */
    result = pcsl_string_substring(&source_str, 
             2, pcsl_string_length(&source_str) + 1, &substr);
    assertTrue("Last index is more than string length",
             result == PCSL_STRING_EINVAL);
  }
  /*
   * Test case 3. Correct args.
   */
  {
    jint comp, length;
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(source_str)
    { 'a', 'B', 'c', 'D', 'e', '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(source_str);
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(substr1)
    { 'a', '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(substr1);
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(substr2)
    { 'e', '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(substr2);
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(substr3)
    { 'c', 'D', '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(substr3);
    length = pcsl_string_length(&source_str);

    /* Get the empty string, start and last indexes are equal  */

    result = pcsl_string_substring(&source_str, 0, 0, &substr);
    assertTrue("Should be status OK",
             result == PCSL_STRING_OK);
    assertTrue("Should be empty string",
             pcsl_string_length(&substr) == 0);

    result = pcsl_string_substring(&source_str, 2, 2, &substr);
    assertTrue("Should be status OK",
             result == PCSL_STRING_OK);
    assertTrue("Should be empty string",
             pcsl_string_length(&substr) == 0);

    result = pcsl_string_substring(&source_str, length, length, &substr);
    assertTrue("Should be status OK",
             result == PCSL_STRING_OK);
    assertTrue("Should be empty string",
             pcsl_string_length(&substr) == 0);

    /* Get the first symbol */
    result = pcsl_string_substring(&source_str, 0, 1, &substr);
    assertTrue("Should be status OK",
             result == PCSL_STRING_OK);
    result = pcsl_string_compare(&substr1, &substr, &comp);
    assertTrue("Should be compare status OK",
             result == PCSL_STRING_OK);
    assertTrue("Should be 'a'", comp == 0);
    result = pcsl_string_free(&substr);
    assertTrue("Failed free string", result == PCSL_STRING_OK);

    /* Get the last symbol */
    result = pcsl_string_substring(&source_str, length - 1, length, &substr);
    assertTrue("Should be status OK",
             result == PCSL_STRING_OK);
    result = pcsl_string_compare(&substr2, &substr, &comp);
    assertTrue("Should be compare status OK",
             result == PCSL_STRING_OK);
    assertTrue("Should be 'e'", comp == 0);
    result = pcsl_string_free(&substr);
    assertTrue("Failed free string", result == PCSL_STRING_OK);

    /* Get the middle 2 symbols */
    result = pcsl_string_substring(&source_str, 2, 4, &substr);
    assertTrue("Should be status OK",
             result == PCSL_STRING_OK);
    result = pcsl_string_compare(&substr3, &substr, &comp);
    assertTrue("Should be compare status OK",
             result == PCSL_STRING_OK);
    assertTrue("Should be 'e'", comp == 0);
    result = pcsl_string_free(&substr);
    assertTrue("Failed free string", result == PCSL_STRING_OK);

    /* Get the whole string */
    result = pcsl_string_substring(&source_str, 0, length, &substr);
    assertTrue("Should be status OK",
             result == PCSL_STRING_OK);
    result = pcsl_string_compare(&source_str, &substr, &comp);
    assertTrue("Should be compare status OK",
             result == PCSL_STRING_OK);
    assertTrue("Should be whole string", comp == 0);
    assertTrue("Data buffers are different",
             memcmp(&source_str, &substr, sizeof(pcsl_string)) != 0);
    result = pcsl_string_free(&substr);
    assertTrue("Failed free string", result == PCSL_STRING_OK);

  }
}

static void test_pcsl_string_starts_with() {
  jboolean result;
  pcsl_string trg, prefix;
  /*
   * Test case 1. Null and empty args.
   */
  result = pcsl_string_starts_with(&trg, NULL);
  assertTrue("NULL is part of any string",
             result == PCSL_TRUE);
  result = pcsl_string_starts_with(NULL, &prefix);
  assertTrue("NULL couldn't contain any not-NULL prefix",
             result == PCSL_FALSE);
  trg = PCSL_STRING_NULL;
  prefix = PCSL_STRING_NULL;
  result = pcsl_string_starts_with(&trg, &prefix);
  assertTrue("Same contents of pcsl_string structures",
             result == PCSL_TRUE);
  trg = PCSL_STRING_EMPTY;
  prefix = PCSL_STRING_EMPTY;
  result = pcsl_string_starts_with(&trg, &prefix);
  assertTrue("Same contents of pcsl_string structures",
             result == PCSL_TRUE);
  trg = PCSL_STRING_EMPTY;
  prefix = PCSL_STRING_NULL;
  result = pcsl_string_starts_with(&trg, &prefix);
  assertTrue("NULL data is part of not-NULL data",
             result == PCSL_TRUE);
  trg = PCSL_STRING_NULL;
  prefix = PCSL_STRING_EMPTY;
  result = pcsl_string_starts_with(&trg, &prefix);
  assertTrue("NULL data couldn't be part of not-NULL data",
             result == PCSL_FALSE);
  /*
   * Test case 2. Not empty target and empty prefix.
   */
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(target_str)
    { 'H', 'e', 'l', 'l', 'o',  '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(target_str);
    prefix = PCSL_STRING_NULL;
    result = pcsl_string_starts_with(&target_str, &prefix);
    assertTrue("NULL string is part of not-NULL string",
             result == PCSL_TRUE);
    prefix = PCSL_STRING_EMPTY;
    result = pcsl_string_starts_with(&target_str, &prefix);
    assertTrue("Empty string is part of not-NULL string",
             result == PCSL_TRUE);
  }
  /*
   * Test case 3. Not empty args.
   */
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(target_str)
    { 'H', 'e', 'l', 'l', 'o',  '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(target_str);
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(prefix_str)
    { 'H', 'e', 'l',  '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(prefix_str);
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(suffix_str)
    { 'l', 'o',  '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(suffix_str);
    result = pcsl_string_starts_with(&target_str, &prefix_str);
    assertTrue("Hellow starts with Hel",
             result == PCSL_TRUE);
    result = pcsl_string_starts_with(&prefix_str, &target_str);
    assertTrue("hel doesn't start with Hellow",
             result == PCSL_FALSE);
    result = pcsl_string_starts_with(&target_str, &suffix_str);
    assertTrue("Hellow doesn't start with low",
             result == PCSL_FALSE);
  }
}

static void test_pcsl_string_ends_with() {
  jboolean result;
  pcsl_string trg, suffix;
  /*
   * Test case 1. Null and empty args.
   */
  result = pcsl_string_ends_with(&trg, NULL);
  assertTrue("NULL is part of any string",
             result == PCSL_TRUE);
  result = pcsl_string_ends_with(NULL, &suffix);
  assertTrue("NULL couldn't contain any not-NULL suffix",
             result == PCSL_FALSE);
  trg = PCSL_STRING_NULL;
  suffix = PCSL_STRING_NULL;
  result = pcsl_string_ends_with(&trg, &suffix);
  assertTrue("Same contents of pcsl_string structures",
             result == PCSL_TRUE);
  trg = PCSL_STRING_EMPTY;
  suffix = PCSL_STRING_EMPTY;
  result = pcsl_string_ends_with(&trg, &suffix);
  assertTrue("Same contents of pcsl_string structures",
             result == PCSL_TRUE);
  trg = PCSL_STRING_EMPTY;
  suffix = PCSL_STRING_NULL;
  result = pcsl_string_ends_with(&trg, &suffix);
  assertTrue("NULL data is part of not-NULL data",
             result == PCSL_TRUE);
  trg = PCSL_STRING_NULL;
  suffix = PCSL_STRING_EMPTY;
  result = pcsl_string_ends_with(&trg, &suffix);
  assertTrue("NULL data couldn't be part of not-NULL data",
             result == PCSL_FALSE);
  /*
   * Test case 2. Not empty target and empty suffix.
   */
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(target_str)
    { 'H', 'e', 'l', 'l', 'o',  '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(target_str);
    suffix = PCSL_STRING_NULL;
    result = pcsl_string_ends_with(&target_str, &suffix);
    assertTrue("NULL string is part of not-NULL string",
             result == PCSL_TRUE);
    suffix = PCSL_STRING_EMPTY;
    result = pcsl_string_ends_with(&target_str, &suffix);
    assertTrue("Empty string is part of not-NULL string",
             result == PCSL_TRUE);
  }
  /*
   * Test case 3. Not empty args.
   */
  {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(target_str)
    { 'H', 'e', 'l', 'l', 'o',  '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(target_str);
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(prefix_str)
    { 'H', 'e', 'l',  '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(prefix_str);
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(suffix_str)
    { 'l', 'o',  '\0' }
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(suffix_str);
    result = pcsl_string_ends_with(&target_str, &prefix_str);
    assertTrue("Hellow doesn't end with Hel",
             result == PCSL_FALSE);
    result = pcsl_string_ends_with(&suffix_str, &target_str);
    assertTrue("low doesn't end with Hellow",
             result == PCSL_FALSE);
    result = pcsl_string_ends_with(&target_str, &suffix_str);
    assertTrue("Hellow ends with low",
             result == PCSL_TRUE);
  }
}

static void test_conv8to16(const jbyte * buf8, const jsize len8, const jchar * buf16, const jsize len16, pcsl_string_status rc_exp) {
    jchar utf16buf[100];
    jsize len=0;
    pcsl_string_status rc = 0x12345678;
    int i;
    rc = pcsl_utf8_convert_to_utf16(buf8,len8,utf16buf,100,&len);
    if (len!=len16) {printf("rc=%i, length %i %i\n",rc, len, len16); }
    if (rc!=rc_exp) {printf("rc=%i, not %i\n",rc, rc_exp); }
    assertTrue("return value not as expected",rc==rc_exp);
    assertTrue("wrong length",len==len16);
    for (i=0;i<len;i++) {
        if (utf16buf[i]!=buf16[i]) {
            printf("at %i: %x %x",i,0xffff&utf16buf[i],0xffff&buf16[i]);
            assertTrue("data do not match",0);
        }
    }
}
static void test_conv16to8(const jchar * buf16, const jsize len16, const jbyte * buf8, const jsize len8, pcsl_string_status rc_exp) {
    jbyte utf8buf[100];
    jsize len=0;
    pcsl_string_status rc = 0x12345678;
    int i;
    rc = pcsl_utf16_convert_to_utf8(buf16,len16,utf8buf,100,&len);
    if (len!=len8) { printf("rc=%i, length %i %i\n",rc, len, len8); }
    assertTrue("return value not as expected",rc==rc_exp);
    assertTrue("wrong length",len==len8);
    for (i=0;i<len8;i++) {
        if (utf8buf[i]!=buf8[i]) {
            printf("at %i: %x %x",i,0xff&utf8buf[i],0xff&buf8[i]);
            assertTrue("data do not match",0);
        }
    }
}
static void test_pcsl_utf() {
  /* test utf8 to utf16 conversion */
  /* Test case 1: valid utf8 */
  test_conv8to16(testutf_string_8,testutf_string_8_length,testutf_string_16,testutf_string_16_length,PCSL_STRING_OK);
  /* Test case 2: invalid utf8 */
  test_conv8to16(bad8_string_8,bad8_string_8_length,bad8_string_16,bad8_string_16_length,PCSL_STRING_EILSEQ);
  /* test utf16 to utf8 conversion*/
  /* Test case 1: valid utf16 */
  test_conv16to8(testutf_string_16,testutf_string_16_length,testutf_string_8,testutf_string_8_length,PCSL_STRING_OK);
  /* Test case 2: invalid utf16 */
  test_conv16to8(bad16_string_16,bad16_string_16_length,bad16_string_8,bad16_string_8_length,PCSL_STRING_EILSEQ);

}

/**
 * Unit test framework entry point for this set of unit tests.
 *
 */
void testString_runTests() {

    pcsl_mem_initialize(NULL, 100*1024*1024);

    assertTrue("String system is active before initialization", 
	       pcsl_string_is_active() == PCSL_FALSE);

    pcsl_string_initialize();

    assertTrue("String system is not active after initialization", 
	       pcsl_string_is_active() == PCSL_TRUE);

    /**
     * jsize pcsl_string_length(const pcsl_string str);
     */
    test_pcsl_string_length();

    /**
     * PCSL_STRING_LITERAL_LENGTH(literal_name)
     */
    test_pcsl_string_literal_length();

    /**
     * jsize pcsl_string_utf16_length(const pcsl_string str);
     */
    test_pcsl_string_utf16_length();

    /**
     * jsize pcsl_string_utf8_length(const pcsl_string str);
     */
    test_pcsl_string_utf8_length();

    /**
     * pcsl_string_status pcsl_string_convert_to_utf8(const pcsl_string string, 
     *                                     jbyte * buffer, 
     *                                     jsize buffer_length);
     */
    test_pcsl_string_convert_to_utf8();

    /**
     * pcsl_string_status pcsl_string_convert_to_utf16(const pcsl_string str, 
     *                                      jchar * buffer, 
     *                                      jsize buffer_length);
     */
    test_pcsl_string_convert_to_utf16();

    /**
     * pcsl_string_status pcsl_string_convert_from_utf8(const jbyte * buffer, 
     *                                       jsize buffer_length, 
     *                                       pcsl_string * string);
     */
    test_pcsl_string_convert_from_utf8();

    /**
     * pcsl_string_status pcsl_string_convert_from_utf16(const jchar * buffer, 
     *                                        jsize buffer_length, 
     *                                        pcsl_string * string);
     */
    test_pcsl_string_convert_from_utf16();

    /**
     * jboolean pcsl_string_equals(const pcsl_string str1, 
     *                             const pcsl_string str2);
     */
    test_pcsl_string_equals();

    /**
     * jint pcsl_string_compare(const pcsl_string str1, const pcsl_string str2);
     */
    test_pcsl_string_compare();

    /**
     * pcsl_string_status pcsl_string_cat(const pcsl_string str1, 
     *                         const pcsl_string str2,
     *                         pcsl_string * str);
     */
    test_pcsl_string_cat();

    /**
     * pcsl_string_status pcsl_string_dup(const pcsl_string src, 
     *                         pcsl_string * dst);
     */
    test_pcsl_string_dup();

    /**
    * void pcsl_string_predict_size(jint capacity, pcsl_string* str);
    */
    test_pcsl_string_predict_size();
    /**
    * pcsl_string_status pcsl_string_append(pcsl_string* dst,
    *                                       const pcsl_string* src);
    * pcsl_string_status pcsl_string_append_char(pcsl_string* dst,
    *                                            const jchar newchar);
    * pcsl_string_status pcsl_string_append_buf(pcsl_string* dst,
    *                                           const jchar* newtext,
    *                                           const jint textsize);
    */
    test_pcsl_string_append();
    /**
     * pcsl_string_status pcsl_string_substring(const pcsl_string str, 
     *                               jint begin_index, jint end_index,
     *                               pcsl_string * dst);
     */
    test_pcsl_string_substring();

    /**
     * jboolean pcsl_string_starts_with(const pcsl_string str, 
     *                                  const pcsl_string prefix);
     */
    test_pcsl_string_starts_with();

    /**
     * jboolean pcsl_string_ends_with(const pcsl_string str, 
     *                                const pcsl_string suffix);
     */
    test_pcsl_string_ends_with();

    /**
     * jint pcsl_string_index_of(const pcsl_string str, jint c);
     */
    test_pcsl_string_index_of();

    /**
     * jint pcsl_string_index_of_from(const pcsl_string str, jint c, 
     *                                jint from_index);
     */
    test_pcsl_string_index_of_from();

    /**
     * jint pcsl_string_last_index_of(const pcsl_string str, jint c);
     */
    test_pcsl_string_last_index_of();

    /**
     * jint pcsl_string_last_index_of_from(const pcsl_string str, jint c, 
     *                                     jint from_index);
     */
    test_pcsl_string_last_index_of_from();

    /**
     * pcsl_string_status pcsl_string_trim_from_end(const pcsl_string str, pcsl_string * dst);
     */
    test_pcsl_string_trim_from_end();

    /**
     * pcsl_string_status pcsl_string_convert_to_jint(const pcsl_string str, 
     *                                     jint * value);
     */
    test_pcsl_string_convert_to_jint();

    /**
     * pcsl_string_status pcsl_string_convert_from_jint(jint integer_value, 
     *                                      pcsl_string * str);
     */
    test_pcsl_string_convert_from_jint();

    /**
     * pcsl_string_status pcsl_string_convert_to_jlong(const pcsl_string str, 
     *                                      jlong * value);
     */
    test_pcsl_string_convert_to_jlong();

    /**
     * pcsl_string_status pcsl_string_convert_from_jlong(jlong integer_value, 
     *                                        pcsl_string * str);
     */
    test_pcsl_string_convert_from_jlong();

    /**
     * pcsl_string_status pcsl_string_free(pcsl_string str);
     */
    test_pcsl_string_free();

    /**
     * pcsl_string_status pcsl_string_is_null(pcsl_string * str);
     */
    test_pcsl_string_is_null();

    /*
    *
    */
    test_pcsl_utf();

    assertTrue("String system is not active before finalization", 
	       pcsl_string_is_active() == PCSL_TRUE);

    pcsl_string_finalize();

    assertTrue("String system is active after finalization", 
	       pcsl_string_is_active() == PCSL_FALSE);

    pcsl_mem_finalize();

    printf("testString PASSED\n");
}
