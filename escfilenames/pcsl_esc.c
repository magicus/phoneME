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

#include <pcsl_esc.h>
#include <pcsl_string.h>
#include <stddef.h>

#if 0
/* for debugging only */
#include <stdio.h>
#define PRINTF_PCSL_STRING(fmt,id) \
    { \
        const jbyte * const __data = pcsl_string_get_utf8_data(id); \
        printf(fmt,__data?__data:(jbyte*)"<null>"); \
        pcsl_string_release_utf8_data(__data, id); \
    }
#endif

/**
 * @defgroup escaped_unicode Conversion of Unicode strings into valid file names
 */

/**
 * @file
 * @ingroup escaped_unicode
 *
 * The purpose of this subsystem is to convert resource names
 * into valid file names, taking into consideration that the
 * file path length may be limited. Of course, the general case
 * solution to this problem is a file that keeps the mapping
 * between resource names and file names... probably at the
 * cost of doubling the file access time.
 *
 * There are two main ideas behind the encoding.
 *
 * At first, in real life, the characters in an Unicode string
 * all belong to the same language and therefore share the same
 * most-significant byte. Therefore, we do not need to encode
 * the high-order byte for each character. (The things get a bit
 * more complicated with Unicode code points greater than 2^16...
 * Anyway, the encoding supports compact selection of two
 * values of the most significant byte.)
 *
 * (The previous statement should be a bit corrected: for CJK,
 * there are thousands of characters that just cannot happen to share
 * the same high-order byte; therefore, we also support a mode
 * requiring both bytes for each character.)
 *
 * The second idea is grouping bytes into "tuples" and encoding
 * them as numbers in some relatively large radix.
 * The supported radix values are 16, 41, 64, 85, and in each
 * case N bytes are encoded with N+1 digits (N being,
 * correspondingly, 1, 2, 3, 4).
 *
 * For each of these radix values,
 * (radix-1)^(N+1) < 256^N <= radix^(N+1)
 *
 * <pre>
 * radix                  16    41    64   85
 * bytes in tuple          1     2     3    4
 * digits per tuple        2     3     4    5
 * case-sensitive?        no    no   yes  yes
 * non-alphanum digits     0     5     2   23
 * </pre>
 *
 * Larger radix values give more compact encoding for long
 * Unicode strings, but require the file system to allow
 * more characters in file names (remember that in addition
 * to these we need several escape characters that cannot
 * be digits, and that at least one character is used as
 * the path separator), for example, radix 85 requires
 * that almost any character of available 95 character
 * codes between 32 and 127 be allowed in file names.
 *
 */


#ifndef PCSL_ESC_ONLY_IF_CASE_SENSITIVE
#error PCSL_ESC_ONLY_IF_CASE_SENSITIVE is not defined!!
#endif

char pcsl_esc_mapchar(char x, char* from, char* to) {
    int i;
    for(i=0;from[i];i++) {
	if (x==from[i]) return to[i];
    }
    return x;
}

/**
 * Convert a number in PCSL_ESC_RADIX system into a digit.
 * For example, 9 gets converted to '9', and 15 gets converted to 'f'.
 * Two error conditions are possible: the input number is out of
 * the [0..radix) range,
 * or there is not enough digits (this does not normally happen at
 * run-time and is covered by donuts testing; the engineer has to check
 * the macros PCSL_ESC_MOREDIGITS and PCSL_ESC_CASE_SENSITIVE).
 *
 * @param n a number specifying a digit in base PCSL_ESC_RADIX system, 0 <= n < PCSL_ESC_RADIX
 * @return the code of character that serves as digit representing n, or -1 in the case of an error.
*/
int pcsl_esc_num2digit(unsigned int n) {
    if (n < PCSL_ESC_RADIX) {

	if (n<10) return '0'+n;
	n -= 10;
	if (n<26) return 'a'+n;
	n -= 26;
      PCSL_ESC_ONLY_IF_CASE_SENSITIVE(
	if (n<26) return 'A'+n;
	n -= 26;
      )
	if (n<sizeof(PCSL_ESC_MOREDIGITS)-1) return PCSL_ESC_MOREDIGITS[n];
	n -= sizeof(PCSL_ESC_MOREDIGITS);
	/* error */
    }
    return -1;
}
/**
 * Convert a character that serves as a digit into the corresponding number.
 * Returns -1 if the argument is a character that is not a digit
 * @param c the character code
 * @return a number in the range 0 ... PCSL_ESC_RADIX-1, or -1 if c is not a digit
 */
int pcsl_esc_digit2num(unsigned int c) {
    int at = 0;
    int res = -1;
    do {
        if ('0'<=c && c <= '9') { res = at+c-'0'; break; }
        at+=10;
        if ('a'<=c && c <= 'z') { res = at+c-'a'; break; }
      PCSL_ESC_ONLY_IF_CASE_SENSITIVE(
        at+=26;
      )
        if ('A'<=c && c <= 'Z') { res = at+c-'A'; break; }
        at+=26;
        { int i;
          for(i=0;PCSL_ESC_MOREDIGITS[i];i++)
          {
            if (((unsigned char*)PCSL_ESC_MOREDIGITS)[i]==c) { res = at+i; break /*for*/; }
          }
        }
    } while (0);
    if (res>=PCSL_ESC_RADIX) { res = -1; }
    return res;
}

/**
 * Convert bytes to text representation, and append that representation to a string.
 * The bytes to be converted are packed in an unsigned number, the
 * first byte is the most significant one.
 * The number of bytes to be converted is specified by the value maxnum
 * containing ones in all bits of bytes provided in num, for example, 0xffff
 * specify two bytes. The maxnum value also determines the number of digits 
 * in the text representation, for example, in hexadecimal, 0 and maxnum=0xff
 * will produce 00 while 0 with the maxnum=0xffff will produce 0000.
 *
 * Usage note. The encoding algorithm calls this function to encode a "tuple"
 * of bytes, for example, when radix=41 is used, 3 digits encode a tuple of two bytes.
 * Sometimes the tuple is incomplete, for example, two radix 41 digits encode one byte;
 * this may happen only at the end of escaped byte sequence. Nevertheless, the encoding
 * algorithm never calls this function to encode more than one tuple at a time,
 * for example, four bytes are converted to radix 41 in two function calls, both times
 * with the maxnum=0xffff specifying two bytes (rather than in one function call with
 * maxnum=0xffffffff, which would produce a different result).
 *
 * @param str the string to append converted bytes to
 * @param num unsigned number that contains the bytes to be converted,
 *              for example, 0x1234 for bytes 0x12 0x34
 * @param maxnum a value containing as many 0xff bytes, as there are bytes
 *             packed into num. For example, 0xffff for two bytes like 0x12 0x34
 */
pcsl_string_status pcsl_esc_append_encoded_tuple(pcsl_string* str, unsigned int num, unsigned int maxnum)
{
#define TMPSIZE (2*sizeof(int))
    jchar tmp[TMPSIZE];
    jchar* p=tmp+TMPSIZE;
    while (maxnum!=0) {
	unsigned int r = num % PCSL_ESC_RADIX;
	num /= PCSL_ESC_RADIX;
	maxnum /= PCSL_ESC_RADIX;
	*--p = pcsl_esc_num2digit(r);
    }
    return pcsl_string_append_buf(str, p, tmp+TMPSIZE-p);
#undef TMPSIZE 
}

/**
 * Convert digits (in PCSL_ESC_RADIX-based system) pointed to by *pptext to a number.
 * An error is reported if the encountered number of digits cannot represent a whole number of bytes.
 * @param pnum points to a variable that receives converted bytes, the first byte is most-significant
 * @param pptext points to a variable that points to the text; the pointer
 *               is updated as the number conversion progresses
 * @return number of decoded bytes, 0 for end of escaped byte sequence, -1 for the error
 */
int pcsl_esc_extract_encoded_tuple(unsigned int *pnum, const jchar**pptext) {
    int d;
    int ndigits = 0;
    *pnum = 0;
    while (-1 != (d = pcsl_esc_digit2num(**pptext)) && ndigits < PCSL_ESC_DIGITS_PER_TUPLE) {
	*pnum = PCSL_ESC_RADIX * *pnum + d;
	++ndigits;
	++*pptext;
    }
    if (ndigits <= 1) { /* 0 (nothing to convert) or 1 (less than one byte) */
	return - ndigits; /* 0 (nothing to convert) or -1 (incomplete byte)*/
    } else {
	return ndigits - 1;
    }
}



typedef enum {
    goes_as_is = 1,
    goes_shifted = 2,
    goes_escaped = 3,
} char_sort;
#define LAST_TAB_CHAR 127
static unsigned char char_sort_tab [LAST_TAB_CHAR+1];

#define CHAR_SORT_OF(c) ( ((unsigned)(c)) > LAST_TAB_CHAR ? goes_escaped : char_sort_tab[c] )
#define BLOCK_OF(c) (0xff & ((c)>>8))
/* 
* @return 0 on success, -1 otherwise                                                                              
*/

void pcsl_esc_init() {

#define FOR_RANGE(var,from,to) { unsigned var; for(var=from;var<=to;var++) {
#define FOR_EACH(var,string) { char* __##var=string; unsigned var; for(var = *__##var; var; var = *++__##var) {
#define END_FOR } }

    /* by default, all characters need escaping */
    FOR_RANGE(c,0,LAST_TAB_CHAR) char_sort_tab[c] = goes_escaped; END_FOR
    /* 0-9 a-z go as is */
    FOR_RANGE(c,'0','9') char_sort_tab[c] = goes_as_is; END_FOR
    FOR_RANGE(c,'a','z') char_sort_tab[c] = goes_as_is; END_FOR
    /* the individual characters that go as is: */
/*
    FOR_EACH(c,"-_") char_sort_tab[c] = goes_as_is; END_FOR
*/
    /* if file names are case-sensitive, A-Z go as is
     * if file names are not case-sensitive, A-Z go shifted */
    FOR_RANGE(c,'A','Z') char_sort_tab[c] = PCSL_ESC_CASE_SENSITIVE?goes_as_is:goes_shifted; END_FOR

    if (PCSL_ESC_EXTENDED_SHIFT) {
	FOR_EACH(c,PCSL_ESC_EXTRA_NEEDXCASE) char_sort_tab[c] = goes_shifted; END_FOR
    }

#undef FOR_RANGE
#undef FOR_EACH
#undef END_FOR

}

/**
 * Append the escaped-unicode representation of the text specified by
 * the parameters in and len to the string out.
 * The word "attach" is used in the function name to underline that
 * the text is attached as a whole and can be extracted only as a whole;
 * in addition, in the general case it is incorrect to decode two
 * attachments as one attachment.
 *
 * @param in points to the utf-16 text to be attached
 * @param len length of the text to be attached
 * @param out the string to which the attachment is appended
 * @return error code
 */
pcsl_string_status pcsl_esc_attach_buf(const jchar* in, jsize len, pcsl_string* out) {
    /* typically we save a few bytes in the output string
     * by setting these initially to block 0 */
    pcsl_string_status rc;
    int currentBlock = 0, previousBlock = 0;
    int shiftMode = goes_as_is;
    unsigned buffer = 0, bufMask = 0;
    const jchar* limit = in+len;
    unsigned c;
    if (!in||!len) return PCSL_STRING_OK; /* nothing to do */
    while (c=*in,in<limit) {
	int b = BLOCK_OF(c);
	char_sort s = CHAR_SORT_OF(c) ;
	switch (s) {
	case goes_as_is:
	case goes_shifted:
	    {
		if (s==shiftMode) {
		    rc = pcsl_string_append_char(out, (unsigned char)PCSL_ESC_CONVERT_CASE((char)c));
                    if (rc != PCSL_STRING_OK) return rc;
		    ++in;
		} else {
		    int cc = *++in;
                    if (s==CHAR_SORT_OF(cc)) { /* multiple shifted */
			rc = pcsl_string_append_char(out, PCSL_ESC_SHIFT_TOGGLE);
                        if (rc != PCSL_STRING_OK) return rc;
			shiftMode = s;
		    } else {
			rc = pcsl_string_append_char(out, PCSL_ESC_SHIFT1);
                        if (rc != PCSL_STRING_OK) return rc;
		    }
		    rc = pcsl_string_append_char(out, (unsigned char)PCSL_ESC_CONVERT_CASE((char)c));
                    if (rc != PCSL_STRING_OK) return rc;
		}
	    }   break;
	case goes_escaped:
	default:
	    if (in+1 < limit
             && b != BLOCK_OF(in[1])
             && goes_escaped == CHAR_SORT_OF(in[1])
               ) {
	        rc = pcsl_string_append_char(out, PCSL_ESC_FULL_CODES);
                if (rc != PCSL_STRING_OK) return rc;
		buffer = bufMask = 0;
		do {
	            previousBlock = currentBlock;
		    currentBlock = b;

		    buffer <<= 8;
		    buffer |= b & 0xff;
		    bufMask <<= 8;
		    bufMask |= 0xff;

		    if (bufMask == PCSL_ESC_FULL_TUPLE_MASK) {
			rc = pcsl_esc_append_encoded_tuple(out,buffer,bufMask);
                        if (rc != PCSL_STRING_OK) return rc;
			buffer = bufMask = 0;
		    }

		    buffer <<= 8;
		    buffer |= c & 0xff;
		    bufMask <<= 8;
		    bufMask |= 0xff;

		    if (bufMask == PCSL_ESC_FULL_TUPLE_MASK) {
			rc = pcsl_esc_append_encoded_tuple(out,buffer,bufMask);
                        if (rc != PCSL_STRING_OK) return rc;
			buffer = bufMask = 0;
		    }

		    c = *++in;
		    b = BLOCK_OF(c);
                    s = CHAR_SORT_OF(c);
		} while (in<limit && currentBlock != b && s == goes_escaped);
		if (bufMask) {
		    rc = pcsl_esc_append_encoded_tuple(out,buffer,bufMask);
                    if (rc != PCSL_STRING_OK) return rc;
		    buffer = bufMask = 0;
		}
		if (s != goes_escaped) {
		    rc = pcsl_string_append_char(out, PCSL_ESC_TOGGLE);
                    if (rc != PCSL_STRING_OK) return rc;
                }
            } else {
		if (currentBlock == b) {
		    rc = pcsl_string_append_char(out, PCSL_ESC_TOGGLE);
                    if (rc != PCSL_STRING_OK) return rc;
		    buffer = bufMask = 0;
		} else if (previousBlock == b) {
		    rc = pcsl_string_append_char(out, PCSL_ESC_PREV_BLOCK);
                    if (rc != PCSL_STRING_OK) return rc;
		    previousBlock = currentBlock;
		    currentBlock = b;
		    buffer = bufMask = 0;
		} else {
		    rc = pcsl_string_append_char(out, PCSL_ESC_NEW_BLOCK);
                    if (rc != PCSL_STRING_OK) return rc;
		    previousBlock = currentBlock;
		    currentBlock = b;
		    buffer = b;
		    bufMask = 0xff;
		}

		do {
		    if (bufMask == PCSL_ESC_FULL_TUPLE_MASK) {
			rc = pcsl_esc_append_encoded_tuple(out,buffer,bufMask);
                        if (rc != PCSL_STRING_OK) return rc;
			buffer = bufMask = 0;
		    }
		    buffer <<= 8;
		    buffer |= c & 0xff;
		    bufMask <<= 8;
		    bufMask |= 0xff;
		    c = *++in;
		    b = BLOCK_OF(c);
                    s = CHAR_SORT_OF(c);
		} while (in<limit && currentBlock == b && s == goes_escaped);
		if (bufMask) {
		    rc = pcsl_esc_append_encoded_tuple(out,buffer,bufMask);
                    if (rc != PCSL_STRING_OK) return rc;
		    buffer = bufMask = 0;
		}
		if (s != goes_escaped) {
		    rc = pcsl_string_append_char(out, PCSL_ESC_TOGGLE);
                    if (rc != PCSL_STRING_OK) return rc;
                }
	    } 	break;
	} /* switch */
    } /* while */
    return PCSL_STRING_OK;
}

/**
 * Append the escaped-unicode representation of the text specified by
 * the parameter data to the string out.
 * The word "attach" is used in the function name to underline that
 * the text is attached as a whole and can be extracted only as a whole;
 * in addition, in the general case it is incorrect to decode two
 * attachments as one attachment.
 *
 * @param data the text to be escaped and attached
 * @param out the string to which the attachment is appended
 * @return error code
 */
pcsl_string_status pcsl_esc_attach_string(const pcsl_string* data, pcsl_string*dst) {
    pcsl_string_status rc = PCSL_STRING_OK;
    const jchar* x = pcsl_string_get_utf16_data(data);
    int l = pcsl_string_utf16_length(data);
    if (x==NULL && l>0) return PCSL_STRING_ENOMEM;
    rc = pcsl_esc_attach_buf(x,l,dst);
    pcsl_string_release_utf16_data(x,data);
    return rc;
}

pcsl_string_status pcsl_esc_extract_attached(const int offset, const pcsl_string *src, pcsl_string* dst) {
    int shiftMode = 0;
    int b = 0, b_prev = 0;
    const jchar*s = pcsl_string_get_utf16_data(src);
    const int len = pcsl_string_utf16_length(src);
    const jchar*p = s+offset;
    const jchar*plimit = s+len;
    *dst = PCSL_STRING_NULL;
    if (len<=0) return PCSL_STRING_OK;
    if (s==NULL) return PCSL_STRING_ENOMEM;
    while (p<plimit) {
        int c = *p;
	pcsl_string_status rc = PCSL_STRING_OK;
        switch (c) {
        default:
            c = shiftMode ? (unsigned char)PCSL_ESC_UPPER((char)c) : (unsigned char)PCSL_ESC_LOWER((char)c);
            rc = pcsl_string_append_char(dst,(unsigned char)c);
            if (rc != PCSL_STRING_OK) return rc;
            break;
        case PCSL_ESC_SHIFT_TOGGLE:
            shiftMode ^= 1;
            c=*++p;
            c = shiftMode ? (unsigned char)PCSL_ESC_UPPER((char)c) : (unsigned char)PCSL_ESC_LOWER((char)c);
            rc = pcsl_string_append_char(dst,(unsigned char)c);
            if (rc != PCSL_STRING_OK) return rc;
            break;
        case PCSL_ESC_SHIFT1:
            c=*++p;
            c = shiftMode ? (unsigned char)PCSL_ESC_LOWER((char)c) : (unsigned char)PCSL_ESC_UPPER((char)c);
            rc = pcsl_string_append_char(dst,(unsigned char)c);
            if (rc != PCSL_STRING_OK) return rc;
            break;
        case PCSL_ESC_PREV_BLOCK:
        case PCSL_ESC_NEW_BLOCK:
        case PCSL_ESC_TOGGLE:
        case PCSL_ESC_FULL_CODES:
	  {
            unsigned int tuple;
            int nbytes = 0, utf16, utf16_incomplete = 0;
            int cmd = c;
            ++p;
            do {
                int byte;
                if (nbytes == 0) {
                    /* fetch a tuple, advancing p */
                    nbytes = pcsl_esc_extract_encoded_tuple(&tuple,&p);
                }

		if (nbytes < 0) nbytes = 0;

                if (nbytes == 0) {
                    cmd = *p++;
                } else {
                    byte = 0xff & tuple >> 8*--nbytes;
                    switch (cmd) {
                    case PCSL_ESC_NEW_BLOCK:
                        b_prev = b;
                        b = byte;
                        utf16_incomplete = 1;
                        cmd = -1;
                        break;
                    case PCSL_ESC_FULL_CODES:
                        if (utf16_incomplete) {
                            utf16 = b << 8 | byte;
                        } else {
                            b = byte;
                        }
                        utf16_incomplete ^= 1;
                        break;
                    case PCSL_ESC_PREV_BLOCK:
                        {
                            int tmp = b_prev;
                            b_prev = b;
                            b = tmp;
                        }
                        /* no break */
                    default:
                    case PCSL_ESC_TOGGLE:
                        utf16 = b << 8 | byte;
                        utf16_incomplete = 0;
                        cmd = -1;
                    }
                    if (!utf16_incomplete) {
                        rc = pcsl_string_append_char(dst, (jchar)utf16);
                        if (rc != PCSL_STRING_OK) return rc;
                    }
                }
            } while (nbytes != 0 || p < plimit && *p != PCSL_ESC_TOGGLE);
	  }
        }
        p++;
    }
    pcsl_string_release_utf16_data(s,src);
    return PCSL_STRING_OK;
}


