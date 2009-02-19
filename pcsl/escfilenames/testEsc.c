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

#include <stdio.h>
#include <pcsl_esc.h>
#include <donuts.h>
#include <pcsl_string.h>

/* for debugging only */
#if 0
#define VERBOSE(x) x
#else
#define VERBOSE(x)
#endif

/**
 * Given variable <code>id</code> of type pcsl_string*,
 * declare: <ul>
 * <li>id_data of type jchar const * const pointing to the string UTF-16 data, </li>
 * <li>id_len of type jint containing the string length.</li>  </ul>
 *
 * The scope of the above two names is the
 *  GET_PCSL_STRING_DATA_AND_LENGTH...RELEASE_PCSL_STRING_DATA_AND_LENGTH
 * construct.
 *
 * @param id the variable name, from which new names are derived.
 */
#define GET_PCSL_STRING_DATA_AND_LENGTH(id) \
    { \
        const jint id##_len = pcsl_string_utf16_length(id); \
        const jchar * const id##_data = pcsl_string_get_utf16_data(id); \
        const jchar * const * const last_pcsl_string_data = & id##_data; \
        const pcsl_string* const last_pcsl_string_itself = id; \
        {

/**
 * closes the
 *  GET_PCSL_STRING_DATA_AND_LENGTH...RELEASE_PCSL_STRING_DATA_AND_LENGTH
 * construct.
 */
#define RELEASE_PCSL_STRING_DATA_AND_LENGTH \
        } pcsl_string_release_utf16_data(*last_pcsl_string_data, last_pcsl_string_itself); \
    }

/**
 * Print a pcsl_string to the standard output. Useful for debugging.
 * Uses <code>printf</code>, requires <code>#include &lt;stdio.h&gt;</code>.
 *
 * @param fmt string literal, specifying the printf format,
 *            for example, "result=[%s]\n"
 * @param id  address of a string, of type pcsl_string*
 *
*/
#define PRINTF_PCSL_STRING(fmt,id) \
    { \
        const jbyte * const __data = pcsl_string_get_utf8_data(id); \
        printf(fmt,__data?__data:(jbyte*)"<null>"); \
        pcsl_string_release_utf8_data(__data, id); \
    }
// enough digits for chosen radix

// num -> digit -> num == lambda x.x

// digits_per_tuple
// ndigits_to_nbytes
// case-insensitive: num -> str -> uppercase -> num

#define amount(array) (sizeof(array)/sizeof(array[0]))

int pcsl_string_equal_text(const pcsl_string* a, const pcsl_string* b) {
    return pcsl_string_equals(a,b) 
	|| (0 >= pcsl_string_length(a) && 0 >= pcsl_string_length(b));
}

/* ==== testing assumptions ==== */

void testUnsigned32(unsigned x, unsigned x1, unsigned x2, unsigned x3, unsigned x4) {
    /* these tests will fail if unsigned is less than 32-bit */
    assertTrue("bytes per tuple, check range"
	      , 1 <= PCSL_ESC_BYTES_PER_TUPLE && PCSL_ESC_BYTES_PER_TUPLE <= 4);
    if(PCSL_ESC_BYTES_PER_TUPLE >= 4)
    {
	assertTrue("testing 32-bit maths",x>>24 == x1);
    }
    if(PCSL_ESC_BYTES_PER_TUPLE >= 3)
    {
	assertTrue("testing 24-bit maths",x>>16 == 256*x1+x2);
    }
    assertTrue("testing 16-bit maths",(x>>8 & 0xff) == x3);
    assertTrue("testing 16-bit maths",(x & 0xff) == x4);
}
void testMacros() {
    assertTrue("right radix"
		,  PCSL_ESC_RADIX==16
		|| PCSL_ESC_RADIX==41
		|| PCSL_ESC_RADIX==64
		|| PCSL_ESC_RADIX==85
	      );
    assertTrue("bytes/digits per tuple"
	       , PCSL_ESC_BYTES_PER_TUPLE + 1 == PCSL_ESC_DIGITS_PER_TUPLE
	       );
    assertTrue("bytes per tuple and tuple mask"
	       , (jlong)1 << 8*PCSL_ESC_BYTES_PER_TUPLE == (jlong)PCSL_ESC_FULL_TUPLE_MASK+1
	       );
#ifdef PCSL_ESC_EXPR_IF_EXTENDED_SHIFT_MD
    assertTrue("PCSL_ESC_EXPR_IF_EXTENDED_SHIFT_MD works? (1)",
	PCSL_ESC_EXPR_IF_EXTENDED_SHIFT_MD(11,10)==(PCSL_ESC_EXTENDED_SHIFT?11:10));
    assertTrue("PCSL_ESC_EXPR_IF_EXTENDED_SHIFT_MD works? (2)",
	PCSL_ESC_EXPR_IF_EXTENDED_SHIFT_MD(11,10)==PCSL_ESC_EXTENDED_SHIFT+10);
#endif
    assertTrue("PCSL_ESC_EXPR_IF_CASE_SENSITIVE_MD works? (1)",
	PCSL_ESC_EXPR_IF_CASE_SENSITIVE_MD(11,10)==10+PCSL_ESC_CASE_SENSITIVE);
    assertTrue("PCSL_ESC_EXPR_IF_CASE_SENSITIVE_MD works? (2)",
	PCSL_ESC_EXPR_IF_CASE_SENSITIVE_MD(11,12)==PCSL_ESC_CASE_SENSITIVE?11:12);
}
void testRadix() {
    jlong maximum1=1,maximum2=1;
    int i;
    for(i=0;i<PCSL_ESC_DIGITS_PER_TUPLE;i++) maximum1 *= PCSL_ESC_RADIX;
    for(i=0;i<PCSL_ESC_BYTES_PER_TUPLE;i++) maximum2 <<= 8;
    maximum2--;
    assertTrue("checking that radix^(n+1) >= 2^(8*n)",maximum1>=maximum2);
    assertTrue("checking maximum1 and maximum2 have not overflown"
	      , maximum1>0 && maximum2>0);
}
void testTupleBits() {
    int i;
    unsigned x=0x80;
    for(i=1;i<PCSL_ESC_BYTES_PER_TUPLE;i++) x <<= 8;
    assertTrue("testing that the highest bit in tuple is not lost", x != 0);
}
void testExtraUpper()
{
    char*s,*S;
    int i;
    if(!PCSL_ESC_EXTENDED_SHIFT) return;
    S = PCSL_ESC_EXTRA_UPPERCASE;
    s = PCSL_ESC_EXTRA_LOWERCASE;
    assertTrue("sizeof(PCSL_ESC_EXTRA_UPPER)==sizeof(PCSL_ESC_EXTRA_LOWER)",sizeof(PCSL_ESC_EXTRA_UPPERCASE)==sizeof(PCSL_ESC_EXTRA_LOWERCASE));
    for(i=0;S[i];i++) {
	if(PCSL_ESC_LOWER(S[i])!=s[i]) printf("err:%c (%x) -> %c (%x) != %c (%x)\n",S[i],S[i],PCSL_ESC_LOWER(S[i]),PCSL_ESC_LOWER(S[i]),s[i],s[i]);
	assertTrue("do extra upper&lower match?",PCSL_ESC_LOWER(S[i])==s[i]);
	assertTrue("uppercase must not cange extra-upper",PCSL_ESC_UPPER(S[i])==S[i]);
	assertTrue("premature end?",s[i]);
    }
    for(i=0;s[i];i++) {
	assertTrue("lowercase must not chnge extra lower",PCSL_ESC_LOWER(s[i])==s[i]);
	assertTrue("do extra upper&lower match?",PCSL_ESC_UPPER(s[i])==S[i]);
	assertTrue("premature end?",S[i]);
    }
}
void test_assumptions() {
    testUnsigned32(0x12345678,0x12,0x34,0x56,0x78);
    testMacros();
    testRadix();
    testTupleBits();
    testExtraUpper();
}
/* ==== testing digits ==== */

/* the whole 0..radix-1 range covered? */
void testEnoughDigits() {
    int i;

    assertTrue("not enough digits",
	       (amount(PCSL_ESC_MOREDIGITS)-1) + 10 + 26 + (PCSL_ESC_CASE_SENSITIVE?26:0) >= PCSL_ESC_RADIX
	      );

    for (i=0;i<PCSL_ESC_RADIX;i++) {
	int c=pcsl_esc_num2digit(i);
	if(' ' > (signed)c) {
	    printf("pcsl_esc_num2digit(%i) --> 0x%x\n",i,(unsigned)c);
	    printf("PCSL_ESC_MOREDIGITS=[%s]\n",PCSL_ESC_MOREDIGITS);
	}
	assertTrue("a control character (less than (char)32) cannot be used as a digit",
		   ' ' <= (unsigned)c);
        switch (c) {
//        case PCSL_ESC_SHIFT_TOGGLE:
//        case PCSL_ESC_SHIFT1:
        case PCSL_ESC_PREV_BLOCK:
        case PCSL_ESC_NEW_BLOCK:
        case PCSL_ESC_TOGGLE:
        case PCSL_ESC_FULL_CODES:
            printf("char='%c' (0x%x)",c,c);
            assertTrue("an escape character is used as a digit",0);
        }
    }
}
/* test that conversion to digit and back produces the same number  */
void testDigitConv() {
    int i;

    for (i=0;i<PCSL_ESC_RADIX;i++) {
	int c=pcsl_esc_num2digit(i);
	int n=pcsl_esc_digit2num(c);
	if(n!=i) {
	    printf("error converting number %i --> char '%c' (%i) --> number %i\n",i,c,c,n);
	    assertTrue("conversion of a number to digit and back must yield the original number",i==n);
	}
    }
}
/* test that conversion of numbers out of 0..(radix-1) range to digits produces -1 (error) */
void testNum2DigitRange() {
    int i;
    assertTrue("-1 == pcsl_esc_num2digit(-1)", -1 == pcsl_esc_num2digit(-1));
    assertTrue("-1 == pcsl_esc_num2digit(PCSL_ESC_RADIX)", -1 == pcsl_esc_num2digit(PCSL_ESC_RADIX));
//    assertTrue("-1 == pcsl_esc_digit2num(PCSL_ESC_SHIFT_TOGGLE)", -1 == pcsl_esc_digit2num(PCSL_ESC_SHIFT_TOGGLE));
//    assertTrue("-1 == pcsl_esc_digit2num(PCSL_ESC_SHIFT1      )", -1 == pcsl_esc_digit2num(PCSL_ESC_SHIFT1      ));
    assertTrue("-1 == pcsl_esc_digit2num(PCSL_ESC_PREV_BLOCK  )", -1 == pcsl_esc_digit2num(PCSL_ESC_PREV_BLOCK  ));
    assertTrue("-1 == pcsl_esc_digit2num(PCSL_ESC_NEW_BLOCK   )", -1 == pcsl_esc_digit2num(PCSL_ESC_NEW_BLOCK   ));
    assertTrue("-1 == pcsl_esc_digit2num(PCSL_ESC_TOGGLE      )", -1 == pcsl_esc_digit2num(PCSL_ESC_TOGGLE      ));
    assertTrue("-1 == pcsl_esc_digit2num(PCSL_ESC_FULL_CODES  )", -1 == pcsl_esc_digit2num(PCSL_ESC_FULL_CODES  ));

#define MAX_TO_TEST 4000
    for(i=PCSL_ESC_RADIX;i<MAX_TO_TEST;i++) {
	int c=pcsl_esc_num2digit(i);
	if(c!=-1) {
	    printf("error converting number %i --> char '%c' (%i) -- must return -1\n",i,c,c);
	    assertTrue("conversion of a number not less than radix must produce -1",c==-1);
	}
    }

    for(i=1;i<MAX_TO_TEST;i++) {
	int c=pcsl_esc_num2digit(-i);
	if(c!=-1) {
	    printf("error converting number %i --> char '%c' (%i) -- must return -1\n",-i,c,c);
	    assertTrue("conversion of a negative number must produce -1",c==-1);
	}
    }
#undef MAX_TO_TEST
}
/* test that attempts to convert non-digits to numbers produce -1 */
void testDigit2NumRange() {
    int c;
    assertTrue("-1 == pcsl_esc_digit2num(-1)", -1 == pcsl_esc_digit2num(-1));
    assertTrue("-1 == pcsl_esc_digit2num(0)", -1 == pcsl_esc_digit2num(0));
#define MAX_TO_TEST 4000
    for(c=0;c<MAX_TO_TEST;c++) {
	int n=pcsl_esc_digit2num(c);
	int cc;
	int test = n == -1
		|| c == (cc = pcsl_esc_num2digit(n))
                || ( !PCSL_ESC_CASE_SENSITIVE &&'A'<=c && c<='Z' && cc == c + 'a' - 'A');
	if(!test) {
	    printf("error converting char '%c' (%i) --> number %i -- must return -1"
		   " or the corresponding number (radix %i)\n",c,c,n,PCSL_ESC_RADIX);
	    assertTrue("conversion of a non-digit char must produce -1",test);
	}
    }
#undef MAX_TO_TEST
}
/* test that a-z and A-Z are the same only for case-insensitive systems */
void testDigitCaseSensitivity() {
    {
	int cs = PCSL_ESC_CASE_SENSITIVE;
	int cs1 = 0;
	PCSL_ESC_ONLY_IF_CASE_SENSITIVE( cs1 = 1 );
	assertTrue("PCSL_ESC_ONLY_IF_CASE_SENSITIVE must work",cs1 == cs);
    }
    {
	int C,c;
	for(C='A',c='a';C<='Z';C++,c++) {
	    assertTrue("uppercase and lowercase letters, different?",
		(pcsl_esc_digit2num(C) != pcsl_esc_digit2num(c)) == PCSL_ESC_CASE_SENSITIVE
	    );
	    assertTrue("uppercase and lowercase letters, different how much?",
		(pcsl_esc_digit2num(C) - pcsl_esc_digit2num(c)) == 26*PCSL_ESC_CASE_SENSITIVE
	    );
	}
    }
}

void test_all_digits() {
    testEnoughDigits();
    testDigitConv();
    testNum2DigitRange();
    testDigit2NumRange();
    testDigitCaseSensitivity();
}

/* ====  testing tuples ==== */
// incomplete tuple
// two tuples
void checkOneTuple(pcsl_string*ps,unsigned num, unsigned nbytes) {
    int num2 = -1;
    GET_PCSL_STRING_DATA_AND_LENGTH(ps);
        const jchar* ptr = ps_data;
        pcsl_esc_extract_encoded_tuple(&num2,&ptr);
        if(num2 != num) { printf("original=%x, converted=%x\n",num, num2); };
        assertTrue("bytes before and after conversion must match",num == num2);
        assertTrue("pointer must be advanced", ptr == ps_data + nbytes + 1);
    RELEASE_PCSL_STRING_DATA_AND_LENGTH;
}

void testSingleTupleInString(jchar suffix) {
    pcsl_string s = PCSL_STRING_NULL_INITIALIZER;
    pcsl_string*ps=&s;
    int nbytes;
//    printf("testSingleTupleInString\n");
    for(nbytes=1;nbytes<=PCSL_ESC_BYTES_PER_TUPLE;nbytes++) {
	/* I am very sorry, but the expression (1<<8*nbytes)-1 does not work,
	* maybe due to a gcc bug */
        unsigned int maxnum = 0xff;
        int i;
	for(i=1;i<nbytes;i++) maxnum = maxnum << 8 | 0xff;
//printf("!! %x %x %x\n",1<<31,1<<32,1<<33);
//printf("!! nbytes=%x, 8*nbytes=%x, 1<<8*nbytes=%x, (1<<8*nbytes)-1, maxnum=%x\n",nbytes,8*nbytes,1<<8*nbytes,(1<<8*nbytes)-1,maxnum);
	for(i=0;i<256;i++) {
            int k;
            int num = i;
            for(k=1;k<nbytes;k++) {
                num = num << 8 | 0xff & k * 0x11 + i;
            }
            s = PCSL_STRING_NULL;
//printf("testSingleTupleInString i=%x, num=%x, nbytes=%x,maxnum=%x radix=%i\n",i,num,nbytes,maxnum,PCSL_ESC_RADIX);
            pcsl_esc_append_encoded_tuple(&s, num, maxnum);
            if(suffix) {
                pcsl_string_append_char(ps,suffix);
            }
//PRINTF_PCSL_STRING("[%s]\n",ps)
            checkOneTuple(ps,num,nbytes);
            pcsl_string_free(ps);
            s=PCSL_STRING_NULL;
        }
	{
	    int special[] = { 0, 1, maxnum ^ maxnum>>8, maxnum };
            for(i=0;i<amount(special);i++) {
	        s = PCSL_STRING_NULL;
//printf("testSingleTupleInString i=%x, num=%x, nbytes=%x,maxnum=%x radix=%i\n",i,special[i],nbytes,maxnum,PCSL_ESC_RADIX);
    	        pcsl_esc_append_encoded_tuple(&s, special[i], maxnum);
        	checkOneTuple(ps,special[i],nbytes);
        	pcsl_string_free(ps);
        	s=PCSL_STRING_NULL;
	    }
        }
    }
}

void checkTwoTuples(pcsl_string*ps,unsigned numx, unsigned numy, unsigned nbytes) {
    int num2 = -1;
    GET_PCSL_STRING_DATA_AND_LENGTH(ps);
        const jchar* ptr = (jchar*)ps_data;
        pcsl_esc_extract_encoded_tuple(&num2,&ptr);
        if(num2 != numx) { printf("original=%x, converted=%x\n",numx, num2); };
        assertTrue("bytes before and after conversion must match",numx == num2);
        assertTrue("pointer must be advanced", ptr == ps_data + nbytes + 1);
        pcsl_esc_extract_encoded_tuple(&num2,&ptr);
        if(num2 != numy) { printf("original=%x, converted=%x\n",numy, num2); };
        assertTrue("bytes before and after conversion must match",numy == num2);
        assertTrue("pointer must be advanced", ptr == ps_data + 2*(nbytes + 1));
    RELEASE_PCSL_STRING_DATA_AND_LENGTH;
}

void testTwoTuplesInString(jchar suffix) {
    int special [][2]
        = { { 0, 0 }, { 0, -1 } , { -1, 0 }, { -1, -1 }, { 0x12345678, 0x90abcdef } };
    int maxnum = PCSL_ESC_FULL_TUPLE_MASK;
    int nbytes = PCSL_ESC_BYTES_PER_TUPLE;
    int m = PCSL_ESC_FULL_TUPLE_MASK;
    int i;
    pcsl_string s,*ps=&s,t,*pt=&t;
    for(i=0;i<amount(special);i++) {
        s = PCSL_STRING_NULL;
//printf("test2TuplesInString#1 i=%x, num=%x, nbytes=%x,maxnum=%x radix=%i\n",i,special[i][0],nbytes,maxnum,PCSL_ESC_RADIX);
//printf("test2TuplesInString#2 i=%x, num=%x, nbytes=%x,maxnum=%x radix=%i\n",i,special[i][1],nbytes,maxnum,PCSL_ESC_RADIX);
    	pcsl_esc_append_encoded_tuple(&s, special[i][0]&m, maxnum);
    	pcsl_esc_append_encoded_tuple(&s, special[i][1]&m, maxnum);
            if(suffix) {
                pcsl_string_append_char(ps,suffix);
            }
        checkTwoTuples(ps,special[i][0]&m,special[i][1]&m,nbytes);
        pcsl_string_free(ps);
        s=PCSL_STRING_NULL;
    }
}

void test_tuples()
{
    testSingleTupleInString(0);
    testSingleTupleInString('#');
    testTwoTuplesInString(0);
    testTwoTuplesInString('#');
}

/* ==== testing strings ==== */
//你好世界
PCSL_DEFINE_ASCII_STRING_LITERAL_START(str1)
{ 'a', 'B', 'c', 'D', 'e', '\0' }
PCSL_DEFINE_ASCII_STRING_LITERAL_END(str1);

PCSL_DEFINE_ASCII_STRING_LITERAL_START(str2)
{ 'Z', 'a', 'B', 'c', 'D', 'e', '\0' }
PCSL_DEFINE_ASCII_STRING_LITERAL_END(str2);

PCSL_DEFINE_ASCII_STRING_LITERAL_START(str3)
{ 'a', 'b', 'c', 'D', 'E', 'F', ' ', 'G', 'h', 'i', 'J', 'K', 'l', 'M', 'N', 'o', 'p', 'Q', 'r', ' ', 's', 'T', 'U', 'V', 'w', '\0' }
PCSL_DEFINE_ASCII_STRING_LITERAL_END(str3);

PCSL_DEFINE_ASCII_STRING_LITERAL_START(str4)
{ 'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!', '\0' }
PCSL_DEFINE_ASCII_STRING_LITERAL_END(str4);

PCSL_DEFINE_ASCII_STRING_LITERAL_START(str5)
{ 0x41f, 0x440, 0x438, 0x432, 0x435, 0x442, ',', ' ', 0x43c, 0x438, 0x440, '!', '\0' }
PCSL_DEFINE_ASCII_STRING_LITERAL_END(str5);

PCSL_DEFINE_ASCII_STRING_LITERAL_START(str6)
{0x4f60, 0x597d, 0x2c, 0x4e16, 0x754c, 0x21, 0} 
PCSL_DEFINE_ASCII_STRING_LITERAL_END(str6);

PCSL_DEFINE_ASCII_STRING_LITERAL_START(str7)
{ PCSL_ESC_SHIFT_TOGGLE_MD, PCSL_ESC_SHIFT1_MD,
  PCSL_ESC_PREV_BLOCK_MD, PCSL_ESC_NEW_BLOCK_MD, PCSL_ESC_TOGGLE_MD, PCSL_ESC_FULL_CODES_MD,
  0 }
PCSL_DEFINE_ASCII_STRING_LITERAL_END(str7);

pcsl_string str8 = PCSL_STRING_NULL_INITIALIZER;

const pcsl_string* str[] = {&PCSL_STRING_NULL, &PCSL_STRING_EMPTY, &str1, &str2, &str3, &str4, &str5, &str6, &str7, &str8 };

void init_str8() {
    int i=0;
    str8 = PCSL_STRING_NULL;
    for(i=' ';i<128;i++) {
	pcsl_string_append_char(&str8,i);
    }
}

void showStrings() {
    pcsl_string s;

    int i,j;
    for (i=0;i<amount(str);i++) {
	PRINTF_PCSL_STRING("%s\n",str[i]);
	s = PCSL_STRING_NULL;
	pcsl_esc_attach_string(str[i],&s);
	PRINTF_PCSL_STRING("%s\n",&s);
	pcsl_string_free(&s);
    }
}

void test_esc_string(const pcsl_string* s0, const pcsl_string* s) {
    pcsl_string t;
    int l0 = pcsl_string_utf16_length(s0);
    if(l0<0)l0=0;
    VERBOSE(
	PRINTF_PCSL_STRING("prefix=[%s]\n",s0);        
	PRINTF_PCSL_STRING("data=[%s]\n",s);        
    )
    pcsl_string_dup(s0, &t);
    pcsl_esc_attach_string(s,&t);
    VERBOSE(
	PRINTF_PCSL_STRING("whole=[%s]\n",&t);        
    )
#define UPC(c) ('a'<=(c)&&(c)<='z'?(c)+'A'-'a':(c))
#define LWC(c) ('A'<=(c)&&(c)<='Z'?(c)+'a'-'A':(c))
    assertTrue("bug in UPC()?",UPC('x')=='X');
    assertTrue("bug in LWC()?",LWC('X')=='x');
    {
        //int lt = pcsl_string_utf16_length(&t);
        pcsl_string p1 = PCSL_STRING_NULL, p2 = PCSL_STRING_NULL;
        pcsl_string tt = PCSL_STRING_NULL, TT = PCSL_STRING_NULL;
	pcsl_string *pt=&t;
        pcsl_string_substring(&t,0,l0,&p1);
        //pcsl_string_substring(&t,l0,lt,&p2);
	VERBOSE(
	    PRINTF_PCSL_STRING("substr=[%s]\n",&p1);
	    printf("substr length = %i\n",pcsl_string_length(&p1));
	)
        assertTrue("part1 must be equal to s0", pcsl_string_equal_text(&p1,s0));

    if(!PCSL_ESC_CASE_SENSITIVE){
    
	GET_PCSL_STRING_DATA_AND_LENGTH(pt);
	    int i;
	    for(i=0;i<pt_len;i++) {
		int c = pt_data[i];
		pcsl_string_append_char(&TT,UPC(c));
		pcsl_string_append_char(&tt,LWC(c));
	    }
	RELEASE_PCSL_STRING_DATA_AND_LENGTH;

        pcsl_esc_extract_attached(l0,&tt,&p2);
	VERBOSE(
	    PRINTF_PCSL_STRING("attached(lwc)=[%s]\n",&tt);        
	    PRINTF_PCSL_STRING("decoded=[%s]\n",&p2);        
	)
        assertTrue("part2 must be equal to s",pcsl_string_equal_text(&p2,s));
        pcsl_string_free(&p2);
	p2 = PCSL_STRING_NULL;

        pcsl_esc_extract_attached(l0,&TT,&p2);
	VERBOSE(
	    PRINTF_PCSL_STRING("attached(upc)=[%s]\n",&TT);        
	    PRINTF_PCSL_STRING("decoded=[%s]\n",&p2);        
	)
        assertTrue("part2 must be equal to s",pcsl_string_equal_text(&p2,s));
        pcsl_string_free(&p2);
	p2 = PCSL_STRING_NULL;
    }
        pcsl_esc_extract_attached(l0,&t,&p2);
	VERBOSE(
	    PRINTF_PCSL_STRING("attached(orig)=[%s]\n",&t);        
	    PRINTF_PCSL_STRING("decoded=[%s]\n",&p2);        
	)
        assertTrue("part2 must be equal to s",pcsl_string_equal_text(&p2,s));
        pcsl_string_free(&p2);
	p2 = PCSL_STRING_NULL;

        pcsl_string_free(&p1);
        pcsl_string_free(&tt);
        pcsl_string_free(&TT);
    }
    pcsl_string_free(&t);
}
void test_esc() {
    int i,j;
    for (i=0;i<amount(str);i++) {
        for (j=0;j<amount(str);j++) {
            test_esc_string(str[i],str[j]);
        }
    }
}

void test_strings() {
    init_str8();
    showStrings();
    test_esc();
}

void testEsc_runTests() {
//    printf("===escfilenames===================\n");
#ifdef PCSL_ESC_TESTING
    int validRadix[] = { 16, 41, 64, 85 };
    int caseSens[]   = {  0,  0,  1,  1 };
    int i;
    for (i=0;i<amount(validRadix);i++) {
	for (esc_extended_shift=0;esc_extended_shift<=1;esc_extended_shift++) {
	    esc_radix = validRadix[i];
	    esc_case_sensitive =  caseSens[i];
#else
	    printf("can test only one radix=%i, case_sensitive=%i, extended shift mode=%i\n", PCSL_ESC_RADIX, PCSL_ESC_CASE_SENSITIVE, PCSL_ESC_EXTENDED_SHIFT);
#endif
	    printf(VERBOSE("***\n") "*** testing radix=%i, case_sensitive=%i, extended shift mode=%i\n", PCSL_ESC_RADIX, PCSL_ESC_CASE_SENSITIVE, PCSL_ESC_EXTENDED_SHIFT);
    	    pcsl_esc_init();
	    test_assumptions();
	    test_all_digits();
    	    test_tuples();
    	    test_strings();
#ifdef PCSL_ESC_TESTING
    }	}
#endif
}

// TODO: test PCSL_ESC_CONVERT_CASE()
// test which case it starts with
