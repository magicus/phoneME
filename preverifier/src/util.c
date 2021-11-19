/*
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

/*=========================================================================
 * SYSTEM:    Verifier
 * SUBSYSTEM: Utility functions.
 * FILE:      util.c
 * OVERVIEW:  Utility routines needed by both the compiler and the interpreter.
 *
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>

#include <oobj.h>
#include <utf.h>
#include <sys_api.h>
#include <path.h>       /* DIR_SEPARATOR */

/*=========================================================================
 * Globals and extern declarations
 *=======================================================================*/

extern struct StrIDhash *nameTypeHash;
extern struct StrIDhash *stringHash;

unicode *
str2unicode(char *str, unicode *ustr, long len)
{
    unicode *dst = ustr;

    memset((char *) dst, 0, len * sizeof(*dst));
    while (*str && --len >= 0)
    *ustr++ = 0xff & *str++;
    return dst;
}

void
unicode2str(unicode *src, char *dst, long len)
{
    while (--len >= 0)
    *dst++ = (char)(*src++);
    *dst = '\0';
}

int
jio_printf (const char *format, ...)
{
    int len;

    va_list args;
    va_start(args, format);
    len = jio_vfprintf(stdout, format, args);
    va_end(args);

    return len;
}

int
jio_fprintf (FILE *handle, const char *format, ...)
{
    int len;

    va_list args;
    va_start(args, format);
    len = jio_vfprintf(handle, format, args);
    va_end(args);

    return len;
}

int
jio_vfprintf (FILE *handle, const char *format, va_list args)
{
  return vfprintf(handle, format, args);
}

/*
 * Print s null terminated C-style character string.
 */
void
prints(char *s)
{
    jio_fprintf(stdout, "%s", s);
}

#undef  HTSIZE            /* Avoid conflict on PC */
#define HTSIZE 2003        /* Default size -- must be prime */
#define HTOVERFLOWPOINT(h)    (h->size * 4 / 5)

/*
 * We store most of the result of the hash in hash_bits to quickly
 * reject impossible matches because strcmp() is fairly expensive,
 * especially when many strings will have common prefixes.
 */
typedef struct {
    char *hash;            /* the string for this entry */
    unsigned is_malloced :  1;    /* 1 if hash was malloced */
    unsigned hash_bits   : 31;    /* low 31 bits of the hash */
} StrIDhashSlot;

typedef void (*hash_fn)(const char *s, unsigned *h1, unsigned *h2);

typedef struct StrIDhash {
    int size;            /* number of entries */
    hash_fn hash;        /* hash function for this table */
    struct StrIDhash *next;    /* next bucket */
    short used;            /* number of full entries */
    short baseid;        /* ID for item in slot[0] */
    void **params;        /* param table, if needed */
    StrIDhashSlot slot[1];    /* expanded to be number of slots */
} StrIDhash;

/* Hash a string into a primary and secondary hash value */
static void
default_hash(const char *s, unsigned *h1, unsigned *h2)
{
    int i;
    unsigned raw_hash;
    for (raw_hash = 0; (i = *s) != '\0'; ++s) 

    raw_hash = raw_hash * 37 + i;
    *h1 = raw_hash;
    *h2 = (raw_hash & 7) + 1; /* between 1 and 8 */
}

/* Create a hash table of the specified size */
static StrIDhash *
createHash(int entries)
{
    StrIDhash *h;
    int size = offsetof(struct StrIDhash, slot) +  (entries * sizeof(h->slot));
    h = (StrIDhash *)sysCalloc(1, size); 
    if (h != NULL) { 
    h->size = entries;
    h->hash = default_hash;    /* only custom tables get something else */
    h->next = NULL;
    }
    return h;
}

/*
 * Given a string, return a unique 16 bit id number.  
 * If param isn't null, we also set up an array of void * for holding info
 *     about each object.  The address of this void * is stored into param
 * If CopyNeeded is true, then the name argument "s" must be dup'ed, since
 * the current item is allocated on the stack.
 *
 * Note about returning 0 in the case of out of memory errors: 0 is *not*
 * a valid ID!  The out of memory error should be thrown by the calling code.
 */
unsigned short
Str2ID(StrIDhash **hash_ptr, register char *s, void ***param,
          int CopyNeeded)
{
    /*
     * The database is a hash table.  When the hash table overflows, a new
     * hashtable is created chained onto the previous one.  This is done so
     * that we can use the hashtable slot index as the ID, without worrying
     * about having the IDs change when the hashtable grows.
     */
    StrIDhash *h = *hash_ptr;
    long i;
    unsigned primary_hash;
    unsigned secondary_hash;
    unsigned hash_bits;
    hash_fn current_hash_func = NULL;

    if (h == NULL) 
    goto not_found;

    /* Create the hash values */
    current_hash_func = h->hash;
    current_hash_func(s, &primary_hash, &secondary_hash);
    hash_bits = primary_hash & ((1u << 31) - 1);

    for (;;) {
    char *s2;
    int bucketSize = h->size;
    
    /* See if the new hash table has a different hash function */
    if (h->hash != current_hash_func) {
        current_hash_func = h->hash;
        current_hash_func(s, &primary_hash, &secondary_hash);
        hash_bits = primary_hash & ((1u << 31) - 1);
    }
    i = primary_hash % bucketSize;
    
    while ((s2 = h->slot[i].hash) != NULL) {
        if (h->slot[i].hash_bits == hash_bits && strcmp(s2, s) == 0)
        goto found_it;
        if ((i -= secondary_hash) < 0)
        i += bucketSize;
    }
    /* Not found in this table.  Try the next table. */
    if (h->next == NULL)
        break;
    h = h->next;
    }
    
not_found:
    /* Either the hash table is empty, or the item isn't yet found. */
    if (h == NULL || (h->used >= HTOVERFLOWPOINT(h))) {
    /* Need to create a new bucket */
    StrIDhash *next;
        if (h && h->baseid > 30000 && *hash_ptr != stringHash) {
        panic("16-bit string hash table overflow");
    }
    next = createHash(HTSIZE);
    if (next == NULL) {
        /* Calling code should signal OutOfMemoryError */
        return 0;
    }
    if (h == NULL) { 
        /* Create a new table */
        *hash_ptr = h = next; 
        h->baseid = 1;  /* guarantee that no ID is 0 */
    } else { 
        next->baseid = h->baseid + h->size;
        h->next = next;
        h = next;
    }
    if (h->hash != current_hash_func) {
        current_hash_func = h->hash;
        current_hash_func(s, &primary_hash, &secondary_hash);
        hash_bits = primary_hash & ((1u << 31) - 1);
    }
    i = primary_hash % h->size;
    }
    if (CopyNeeded) {
    char *d = strdup(s);
    if (d == NULL) {
        /* Calling code should signal OutOfMemoryError */
            return 0;
    }
    s = d;
    h->slot[i].is_malloced = 1;
    } 
    h->slot[i].hash = s;
    h->slot[i].hash_bits = hash_bits;
    h->used++;

found_it:
    /* We have found or created slot "i" in hash bucket "h" */
    if (param != NULL) { 
    if (h->params == NULL) { 
        h->params = sysCalloc(h->size, sizeof(void *));
        if (h->params == NULL) {
        /* Calling code should signal OutOfMemoryError */
        return 0;
        }
    }
    *param = &(h->params[i]);
    }
    return (unsigned short)(h->baseid + i);
}

/* Free an StrIDhash table and all the entries in it */
void Str2IDFree(StrIDhash **hash_ptr)
{
    StrIDhash *hash = *hash_ptr;

    while (hash != NULL) {
    StrIDhash *next = hash->next;
    StrIDhashSlot *ptr, *endPtr;
    for (ptr = &hash->slot[0], endPtr = ptr + hash->size; 
              ptr < endPtr; ptr++) { 
        if (ptr->is_malloced) 
        sysFree(ptr->hash);
    }
    if (hash->params != NULL) 
        sysFree(hash->params);
    sysFree(hash);
    hash = next;
    }
    *hash_ptr = 0;
}

/*
 * Call the callback function on every entry in the table.  This
 * should only be invoked holding the string hash table lock.
 */
void Str2IDCallback(StrIDhash **hash_ptr, void (*callback)(char *, void *))
{
    StrIDhash *hash, *next;
    int i;

    hash = *hash_ptr;
    while (hash) {
    void **params = hash->params;
    next = hash->next;
    for (i = 0; i < hash->size; i++) {
        if (hash->slot[i].hash != 0) { 
        callback(hash->slot[i].hash, params ? params[i] : NULL);
        }
    }
    hash = next;
    }
}

/*
 * Returns NULL in the case of an error.
 */
char *
ID2Str(StrIDhash *h, unsigned short ID, void ***param)
{
    int entry;

    while ((long)(ID - h->baseid) >= h->size) {
    h = h->next;
    }
    entry = ID - h->baseid;
    if (param != NULL) { 
    if (h->params == NULL) {
        h->params = (void **)sysCalloc(h->size, sizeof(*param));
        if (h->params == NULL) {
        /* Calling code should signal OutOfMemoryError */
        return NULL;
        }
    }
    *param = &h->params[entry];
    } 
    return h->slot[entry].hash;
}

char *
addstr(char *s, char *buf, char *limit, char term)
{
    char c;
    while ((c = *s) && c != term && buf < limit) {
    *buf++ = c;
    s++;
    }
    return buf;
}

char *
unicode2rd(unicode *s, long len)
{       /* unicode string to readable C string */
#define CSTRLEN 40
    static char buf[CSTRLEN+1];
    char *dp = buf;
    int c;
    if (s == 0)
    return "NULL";
    *dp++ = '"';
    while (--len>=0 && (c = *s++) != 0 && dp < buf + sizeof buf - 10)
    if (040 <= c && c < 0177)
        *dp++ = c;
    else
        switch (c) {
        case '\n':
        *dp++ = '\\';
        *dp++ = 'n';
        break;
        case '\t':
        *dp++ = '\\';
        *dp++ = 't';
        break;
        case '\r':
        *dp++ = '\\';
        *dp++ = 'r';
        break;
        case '\b':
        *dp++ = '\\';
        *dp++ = 'b';
        break;
        case '\f':
        *dp++ = '\\';
        *dp++ = 'f';
        break;
        default:
        /* Should not be possible to overflow, truncate if so */
        (void) jio_snprintf(dp, CSTRLEN+1 - (dp - buf), "\\%X", c);
        dp += strlen(dp);
        break;
        }
    *dp++ = '"';
    if (len >= 0 && c != 0)
    *dp++ = '.', *dp++ = '.', *dp++ = '.';
    *dp++ = 0;
    return buf;
}

/*
 * WARNING: out_of_memory() aborts the runtime!  It should not be used
 * except in the case of out of memory situations that are clearly not
 * survivable, like running out of memory before the runtime is initialized.
 * If the runtime has finished initializing and you run out of memory, you
 * should throw an OutOfMemoryError instead.
 */
void
out_of_memory()
{
    jio_fprintf(stderr, "**Out of memory, exiting**\n");
        exit(1);
}

void
panic(const char *format, ...)
{
    va_list ap;
    char buf[256];

    printCurrentClassName();
    va_start(ap, format);
    
    /* If buffer overflow, quietly truncate */
    (void) jio_vsnprintf(buf, sizeof(buf), format, ap);

    jio_fprintf(stdout, "\nERROR: %s\n", buf);

    exit(1);
}

char *
classname2string(char *src, char *dst, int size) {
    char *buf = dst;
    for (; (--size > 0) && (*src != '\0') ; src++, dst++) {
    if (*src == '/') {
        *dst = '.';
    }  else {
        *dst = *src;
    }
    }
    *dst = '\0';
    return buf;
}

typedef struct InstanceData {
    char *buffer;
    char *end;
} InstanceData;

#define ERROR_RETVAL -1
#undef  SUCCESS
#define SUCCESS 0
#undef  CheckRet
#define CheckRet(x) { if ((x) == ERROR_RETVAL) return ERROR_RETVAL; }

static int 
put_char(InstanceData *this, int c)
{
    if (iscntrl(0xff & c) && c != '\n' && c != '\t') {
        c = '@' + (c & 0x1F);
        if (this->buffer >= this->end) {
            return ERROR_RETVAL;
        }
        *this->buffer++ = '^';
    }
    if (this->buffer >= this->end) {
        return ERROR_RETVAL;
    }
    *this->buffer++ = c;
    return SUCCESS;
}

static int
format_string(InstanceData *this, char *str, int left_justify, int min_width,
              int precision)
{
    int pad_length;
    char *p;

    if (str == 0) {
    return ERROR_RETVAL;
    }

    if ((int)strlen(str) < precision) {
        pad_length = min_width - strlen(str);
    } else {
        pad_length = min_width - precision;
    }
    if (pad_length < 0)
        pad_length = 0;
    if (left_justify) {
        while (pad_length > 0) {
            CheckRet(put_char(this, ' '));
            --pad_length;
        }
    }

    for (p = str; *p != '\0' && --precision >= 0; p++) {
        CheckRet(put_char(this, *p));
    }

    if (!left_justify) {
        while (pad_length > 0) {
            CheckRet(put_char(this, ' '));
            --pad_length;
        }
    }
    return SUCCESS;
}

#define MAX_DIGITS 32

static int
format_number(InstanceData *this, long value, int format_type, int left_justify,
              int min_width, int precision, bool_t zero_pad)
{
    int sign_value = 0;
    unsigned long uvalue;
    char convert[MAX_DIGITS+1];
    int place = 0;
    int pad_length = 0;
    static char digits[] = "0123456789abcdef";
    int base = 0;
    bool_t caps = FALSE;
    bool_t add_sign = FALSE;

    switch (format_type) {
      case 'o': case 'O':
          base = 8;
          break;
      case 'd': case 'D':
          add_sign = TRUE; /* fall through */
      case 'u': case 'U':
          base = 10;
          break;
      case 'X':
          caps = TRUE; /* fall through */
      case 'x':
          base = 16;
          break;
    }
    sysAssert(base > 0 && base <= 16);

    uvalue = value;
    if (add_sign) {
        if (value < 0) {
            sign_value = '-';
            uvalue = -value;
        }
    }

    do {
        convert[place] = digits[uvalue % (unsigned)base];
        if (caps) {
            convert[place] = toupper(convert[place]);
        }
        place++;
        uvalue = (uvalue / (unsigned)base);
        if (place > MAX_DIGITS) {
            return ERROR_RETVAL;
        }
    } while(uvalue);
    convert[place] = 0;

    pad_length = min_width - place;
    if (pad_length < 0) {
        pad_length = 0;
    }
    if (left_justify) {
        if (zero_pad && pad_length > 0) {
            if (sign_value) {
                CheckRet(put_char(this, sign_value));
                --pad_length;
                sign_value = 0;
            }
            while (pad_length > 0) {
                CheckRet(put_char(this, '0'));
                --pad_length;
            }
        } else {
            while (pad_length > 0) {
                CheckRet(put_char(this, ' '));
                --pad_length;
            }
        }
    }
    if (sign_value) {
        CheckRet(put_char(this, sign_value));
    }

    while (place > 0 && --precision >= 0) {
        CheckRet(put_char(this, convert[--place]));
    }

    if (!left_justify) {
        while (pad_length > 0) {
            CheckRet(put_char(this, ' '));
            --pad_length;
        }
    }
    return SUCCESS;
}

int
jio_vsnprintf(char *str, size_t count, const char *fmt, va_list args)
{
    char *strvalue;
    long value;
    InstanceData this;
    bool_t left_justify, zero_pad, long_flag, add_sign, fPrecision;
    int min_width, precision, ch;

    if (str == NULL) {
    return ERROR_RETVAL;
    }
    str[0] = '\0';

    this.buffer = str;
    this.end = str + count - 1;
    *this.end = '\0';        /* ensure null-termination in case of failure */

    while ((ch = *fmt++) != 0) {
        if (ch == '%') {
            zero_pad = long_flag = add_sign = fPrecision = FALSE;
            left_justify = TRUE;
            min_width = 0;
            precision = this.end - this.buffer;
        next_char:
            ch = *fmt++;
            switch (ch) {
              case 0:
                  return ERROR_RETVAL;
              case '-':
                  left_justify = FALSE;
                  goto next_char;
              case '0': /* set zero padding if min_width not set */
                  if (min_width == 0)
                      zero_pad = TRUE;
              case '1': case '2': case '3':
              case '4': case '5': case '6':
              case '7': case '8': case '9':
                  if (fPrecision == TRUE) {
                      precision = precision * 10 + (ch - '0');
                  } else {
                      min_width = min_width * 10 + (ch - '0');
                  }
                  goto next_char;
              case '.':
                  fPrecision = TRUE;
                  precision = 0;
                  goto next_char;
              case 'l':
                  long_flag = TRUE;
                  goto next_char;
              case 's':
                  strvalue = va_arg(args, char *);
                  CheckRet(format_string(&this, strvalue, left_justify,
                                         min_width, precision));
                  break;
              case 'c':
                  ch = va_arg(args, int);
                  CheckRet(put_char(&this, ch));
                  break;
              case '%':
                  CheckRet(put_char(&this, '%'));
                  break;
              case 'd': case 'D':
              case 'u': case 'U':
              case 'o': case 'O':
              case 'x': case 'X':
                  value = long_flag ? va_arg(args, long) : va_arg(args, int);
                  CheckRet(format_number(&this, value, ch, left_justify,
                                         min_width, precision, zero_pad));
                  break;
              default:
                  return ERROR_RETVAL;
            }
        } else {
            CheckRet(put_char(&this, ch));
        }
    }
    *this.buffer = '\0';
    return strlen(str);
}

int 
jio_snprintf(char *str, size_t count, const char *fmt, ...)
{
    va_list args;
    int len;

    va_start(args, fmt);
    len = jio_vsnprintf(str, count, fmt, args);
    va_end(args);
    return len;
}

/* Return true if the two classes are in the same class package */

bool_t
IsSameClassPackage(ClassClass *class1, ClassClass *class2) 
{
    if (cbLoader(class1) != cbLoader(class2)) 
    return FALSE;
    else {
    char *name1 = cbName(class1);
    char *name2 = cbName(class2);
    char *last_slash1 = strrchr(name1, DIR_SEPARATOR);
    char *last_slash2 = strrchr(name2, DIR_SEPARATOR);
    if ((last_slash1 == NULL) || (last_slash2 == NULL)) {
        /* One of the two doesn't have a package.  Only return true
         * if the other one also doesn't have a package. */
        return (last_slash1 == last_slash2); 
    } else {
        int length1, length2;
        if (*name1 == SIGNATURE_ARRAY) { 
        do name1++; while (*name1 == SIGNATURE_ARRAY);
        if (*name1 != SIGNATURE_CLASS) { 
            /* Something is terribly wrong.  Shouldn't be here */
            return FALSE;
        }
        name1++;
        }
        if (*name2 == SIGNATURE_ARRAY) { 
        do name2++; while (*name2 == SIGNATURE_ARRAY);
        if (*name2 != SIGNATURE_CLASS) { 
            /* Something is terribly wrong.  Shouldn't be here */
            return FALSE;
        }
        name2++;
        }
        length1 = last_slash1 - name1;
        length2 = last_slash2 - name2;
        return ((length1 == length2) 
            && (strncmp(name1, name2, length1) == 0));
    }
    }
}
