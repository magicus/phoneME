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
 * SUBSYSTEM: Verifies ClassClass structure.
 * FILE:      check_class.c
 * OVERVIEW:  Code for verifying the ClassClass structure for internal
 *            consistency.
 *            Initial implementation based on the Classic VM Verifier.
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <ctype.h>

#include <oobj.h>
#include <utf.h>
#include <tree.h>
#include <sys_api.h>

/*=========================================================================
 * Globals and extern declarations
 *=======================================================================*/

extern bool_t verify_class_codes(ClassClass *cb);

static bool_t verify_constant_pool(ClassClass *cb);

static bool_t is_legal_fieldname(ClassClass *cb, char *name, int type);
static bool_t is_legal_method_signature(ClassClass *cb, char *name, char *signature);
static bool_t is_legal_field_signature(ClassClass *cb, char *name, char *signature);

static char *skip_over_fieldname(char *name, bool_t slash_okay);
static char *skip_over_field_signature(char *name, bool_t void_okay);

static void CCerror (ClassClass *cb, char *format, ...);

/* Argument for is_legal_fieldname */
enum { LegalClass, LegalField, LegalMethod };

/*=========================================================================
 * FUNCTION:      VerifyClass
 * OVERVIEW:      Verifies a class given a pointer to the ClassClass struct. 
 *                Returns true if the class is ok. 
 * INTERFACE:
 *   parameters:  pointer to the ClassClass structure. 
 *                
 *   returns:     boolean type
 *=======================================================================*/
bool_t
VerifyClass(ClassClass *cb)
{
    bool_t result = TRUE;
    struct methodblock *mb;
    struct fieldblock *fb;
    int i;
    if (!verify_constant_pool(cb)) 
        return FALSE;
    /* Make sure all the method names and signatures are okay */
    for (i = cbMethodsCount(cb), mb = cbMethods(cb); --i >= 0; mb++) {
    char *name = mb->fb.name;
    char *signature = mb->fb.signature;
    if (! (is_legal_fieldname(cb, name, LegalMethod)  &&
           is_legal_method_signature(cb, name, signature)))
        result = FALSE;
    }
    /* Make sure all the field names and signatures are okay */
    for (i = cbFieldsCount(cb), fb = cbFields(cb); --i >= 0; fb++) {
    if (!  (is_legal_fieldname(cb, fb->name, LegalField) &&
        is_legal_field_signature(cb, fb->name, fb->signature))) 
        result = FALSE;
    }
    /* Make sure we are not overriding any final methods or classes*/
    if (cbIsInterface(cb)) { 
    struct methodblock *mb;
    if ((cbSuperclass(cb) == NULL) ||
        (cbSuperclass(cb) != classJavaLangObject)) { 
        CCerror(cb, "Interface %s has bad superclass", cbName(cb));
        result = FALSE;
    }
    for (i = cbMethodsCount(cb), mb = cbMethods(cb); --i >= 0; mb++) {
        if (mb->fb.access & ACC_STATIC) {
        if (mb->fb.name[0] != '<') { 
            /* Only internal methods can be static */
            CCerror(cb, "Illegal static method %s in interface %s",
                mb->fb.name, cbName(cb));
            result = FALSE;
        }
        }
    }
    } else if (cbSuperclass(cb)) { 
    ClassClass *super_cb;
    unsigned bitvector_size = (unsigned)(cbMethodTableSize(cb) + 31) >> 5;
    long *bitvector = sysCalloc(bitvector_size, sizeof(long));
    for (super_cb = cbSuperclass(cb); ; super_cb = cbSuperclass(super_cb)) {
        if (cbAccess(super_cb) & ACC_FINAL) {
        CCerror(cb, "Class %s is subclass of final class %s",
            cbName(cb), cbName(super_cb));
        result = FALSE;
        }
        mb = cbMethods(super_cb);
        for (i = cbMethodsCount(super_cb); --i >= 0; mb++) {
        if (mb->fb.access & ACC_FINAL) {
            unsigned offset = mb->fb.u.offset;
            bitvector[offset >> 5] |= (1 << (offset & 0x1F));
        }
        }
        if (cbSuperclass(super_cb) == NULL) break;
    }
    for (i = cbMethodsCount(cb), mb = cbMethods(cb); --i >= 0; mb++) {
        unsigned offset = mb->fb.u.offset;
        if ((offset > 0) 
           && bitvector[offset >> 5] & (1 << (offset & 0x1F))) {
        CCerror(cb, "Class %s overrides final method %s.%s",
            cbName(cb), mb->fb.name, mb->fb.signature);
        result = FALSE;
        }
    }
    sysFree(bitvector);
    } else if (cb != classJavaLangObject) {
    CCerror(cb, "Class %s does not have superclass", cbName(cb));
    result = FALSE;
    }
    
    if (result)
    result = verify_class_codes(cb);
    return result;
}

/*=========================================================================
 * FUNCTION:      verify_constant_pool 
 * OVERVIEW:      Verifies the constant pool given a pointer to the 
 *                ClassClass structure. 
 *                Makes two quick passes over the constant pool. The first
 *                pass ensures that everything is of the right type.
 *                Returns true if the constant pool is ok. 
 * INTERFACE:
 *   parameters:  pointer to the ClassClass structure. 
 *                
 *   returns:     boolean type
 *=======================================================================*/
static bool_t
verify_constant_pool(ClassClass *cb)
{
    union cp_item_type *cp = cbConstantPool(cb);
    long cp_count = cbConstantPoolCount(cb);
    unsigned char *type_table;
    int i, type;
    
    const int utf8_resolved = (CONSTANT_Utf8 | CONSTANT_POOL_ENTRY_RESOLVED);

    if (cp_count == 0) /* Primitive classes */
        return TRUE;
    type_table = cp[CONSTANT_POOL_TYPE_TABLE_INDEX].type;
    /* Let's make two quick passes over the constant pool. The first one 
     * checks that everything is of the right type.            */
    for (i = 1; i < cp_count; i++) {
    switch(type = type_table[i]) {
        case CONSTANT_String:
        case CONSTANT_Class: {
        int index = cp[i].i;
        if (   (index < 1) 
               || (index >= cp_count)
               || (type_table[index] != utf8_resolved)) {
            CCerror(cb, "Bad index in constant pool #%d", i);
            return FALSE;
        }
        break;
        }
        
        case CONSTANT_String | CONSTANT_POOL_ENTRY_RESOLVED:
        /* This can only happen if a string is the "initial" value of
         * some final static String.  We assume that the checking has
         * already been done.
         */
        break;

        case CONSTANT_Fieldref:
        case CONSTANT_Methodref:
        case CONSTANT_InterfaceMethodref: 
        case CONSTANT_NameAndType: {
        unsigned index = (unsigned)(cp[i].i);
        int key1 = index >> 16;
        int key2 = index & 0xFFFF;
        if (key1 < 1 || key1 >= cp_count 
              || key2 < 1 || key2 >= cp_count) {
            CCerror(cb, "Bad index in constant pool #%d", i);
            return FALSE;
        }
        if (type == CONSTANT_NameAndType) {
            if (   (type_table[key1] != utf8_resolved) 
            || (type_table[key2] != utf8_resolved)) {
            CCerror(cb, "Bad index in constant pool.");
            return FALSE;
            }
        } else {
            if (     ((type_table[key1] & CONSTANT_POOL_ENTRY_TYPEMASK) 
                    != CONSTANT_Class)
              || ((type_table[key2] != CONSTANT_NameAndType))) {
            CCerror(cb, "Bad index in constant pool #%d", i);
            return FALSE;
            }
        }
        break;
        }
        
        case CONSTANT_Fieldref | CONSTANT_POOL_ENTRY_RESOLVED:
        case CONSTANT_Methodref | CONSTANT_POOL_ENTRY_RESOLVED:
        case CONSTANT_InterfaceMethodref | CONSTANT_POOL_ENTRY_RESOLVED:
        case CONSTANT_NameAndType | CONSTANT_POOL_ENTRY_RESOLVED:
            CCerror(cb, "Improperly resolved constant pool #%d", i);
            return FALSE;

        case CONSTANT_Class | CONSTANT_POOL_ENTRY_RESOLVED:
        case CONSTANT_Utf8 | CONSTANT_POOL_ENTRY_RESOLVED:
        case CONSTANT_Integer | CONSTANT_POOL_ENTRY_RESOLVED:
        case CONSTANT_Float | CONSTANT_POOL_ENTRY_RESOLVED:
        break;

        case CONSTANT_Long | CONSTANT_POOL_ENTRY_RESOLVED:
        case CONSTANT_Double | CONSTANT_POOL_ENTRY_RESOLVED:
        if ((i + 1 >= cp_count) || 
            (type_table[i + 1] != CONSTANT_POOL_ENTRY_RESOLVED)) {
            CCerror(cb, "Improper constant pool long/double #%d", i);
            return FALSE;
        } else {
            i++;    
            break;        
        }

        case CONSTANT_Integer:
        case CONSTANT_Float:
        case CONSTANT_Long:
        case CONSTANT_Double:
        case CONSTANT_Utf8:
            CCerror(cb, "Improperly unresolved constant pool #%d", i);
            return FALSE;

        default:
            CCerror(cb, "Illegal constant pool type at #%d", i);
            return FALSE;

    }
    }
    for (i = 1; i < cp_count; i++) {
    switch(type = type_table[i]) {
        case CONSTANT_Class: {
        int index = cp[i].i;
        if (!is_legal_fieldname(cb, cp[index].cp, LegalClass)) 
            return FALSE;
        break;
        }
          
        case CONSTANT_Fieldref:
        case CONSTANT_Methodref:
        case CONSTANT_InterfaceMethodref: {
        unsigned index = (unsigned)(cp[i].i);
        int name_type_index = index & 0xFFFF;
        int name_type_key = cp[name_type_index].i;
        int name_index = name_type_key >> 16;
        int signature_index = name_type_key & 0xFFFF;
        char *name = cp[name_index].cp;
        char *signature = cp[signature_index].cp;

        if (type == CONSTANT_Fieldref) {
            if (! (is_legal_fieldname(cb, name, LegalField) &&
               is_legal_field_signature(cb, name, signature)))
                return FALSE;
        } else {
            if (! (is_legal_fieldname(cb, name, LegalMethod) &&
               is_legal_method_signature(cb, name, signature)))
                return FALSE;
        }
        break;
        }
    }
    }
    return TRUE;
}
        

/*=========================================================================
 * FUNCTION:      is_legal_fieldname 
 * OVERVIEW:      Returns true if the given name within the given ClassClass 
 *                structure consists of a legal fieldname or a classname 
 *                if the third argument is LegalClass.
 * INTERFACE:
 *   parameters:  pointer to the ClassClass structure. 
 *                char*: legal field name or a classname
 *                int: type of name (class or field name) 
 *                
 *   returns:     boolean type
 *=======================================================================*/
static bool_t 
is_legal_fieldname(ClassClass *cb, char *name, int type)
{
    bool_t result;
    if (name[0] == '<') {
    result = (type == LegalMethod) && 
             ((strcmp(name, "<init>") == 0) || 
          (strcmp(name, "<clinit>") == 0));
    } else {
    char *p;
    if (type == LegalClass && name[0] == SIGNATURE_ARRAY) {
        p = skip_over_field_signature(name, FALSE);
    } else {
        p = skip_over_fieldname(name, type == LegalClass);
    }
    result = (p != 0 && p[0] == '\0');
    }
    if (!result) {
    char *thing =    (type == LegalField) ? "Field" 
                   : (type == LegalMethod) ? "Method" : "Class";
             
    CCerror(cb, "Illegal %s name \"%s\"", thing, name);
    return FALSE;
    } else {
        return TRUE;
    
    }
}

/*=========================================================================
 * FUNCTION:      is_legal_field_signature
 * OVERVIEW:      Returns true if the entire given string within the given 
 *                ClassClass structure consists of a legal field signature.
 * INTERFACE:
 *   parameters:  pointer to the ClassClass structure. 
 *                char*: field name 
 *                char*: field signature 
 *                
 *   returns:     boolean type
 *=======================================================================*/
static bool_t 
is_legal_field_signature(ClassClass *cb, char *fieldname, char *signature) 
{
    char *p = skip_over_field_signature(signature, FALSE);
    if (p != 0 && p[0] == '\0') {
    return TRUE;
    } else {
    CCerror(cb, "Field \"%s\" has illegal signature \"%s\"", 
           fieldname, signature);
    return FALSE;
    }
}

/*=========================================================================
 * FUNCTION:      is_legal_method_signature
 * OVERVIEW:      Returns true if the entire given string within the given 
 *                ClassClass structure consists of a legal method signature.
 * INTERFACE:
 *   parameters:  pointer to the ClassClass structure. 
 *                char*: method name 
 *                char*: method signature  
 *                
 *   returns:     boolean type
 *=======================================================================*/
static bool_t 
is_legal_method_signature(ClassClass *cb, char *methodname, char *signature)
{
    char *p = signature;
    char *next_p;
    /* The first character must be a '(' */
    if (*p++ == SIGNATURE_FUNC) {
    /* Skip over however many legal field signatures there are */
    while ((next_p = skip_over_field_signature(p, FALSE)) != 0) 
        p = next_p;
    /* The first non-signature thing better be a ')' */
    if (*p++ == SIGNATURE_ENDFUNC) {
        if (methodname[0] == '<') {
        /* All internal methods must return void */
        if ((p[0] == SIGNATURE_VOID) && (p[1] == '\0'))
            return TRUE;
        } else {
        /* Now, we better just have a return value. */
        next_p =  skip_over_field_signature(p, TRUE);
        if (next_p && next_p[0] == '\0')
            return TRUE;
        }
    }
    }
    CCerror(cb, "Method \"%s\" has illegal signature \"%s\"", 
       methodname, signature);
    return FALSE;
}
    

/*=========================================================================
 * Automatic code generation tables 
 *=======================================================================*/
  /* The following tables and code generated using: */
  /* java GenerateCharacter -verbose -c -identifiers -spec UnicodeData-2.1.2.txt -template check_class.c.template -o check_class.c 8 4 4 */
  /* The X table has 256 entries for a total of 256 bytes. */

  static unsigned char X[256] = {
      0,   1,   2,   3,   4,   5,   6,   7,  /* 0x0000 */
      7,   8,   9,  10,  11,  12,  13,  14,  /* 0x0800 */
     15,  16,   7,   7,   7,   7,   7,   7,  /* 0x1000 */
      7,   7,   7,   7,   7,   7,  17,  18,  /* 0x1800 */
     19,  20,   7,   7,   7,   7,   7,   7,  /* 0x2000 */
      7,   7,   7,   7,   7,   7,   7,   7,  /* 0x2800 */
     21,  22,   7,   7,   7,   7,   7,   7,  /* 0x3000 */
      7,   7,   7,   7,   7,   7,   7,   7,  /* 0x3800 */
      7,   7,   7,   7,   7,   7,   7,   7,  /* 0x4000 */
      7,   7,   7,   7,   7,   7,  23,  23,  /* 0x4800 */
     23,  23,  23,  23,  23,  23,  23,  23,  /* 0x5000 */
     23,  23,  23,  23,  23,  23,  23,  23,  /* 0x5800 */
     23,  23,  23,  23,  23,  23,  23,  23,  /* 0x6000 */
     23,  23,  23,  23,  23,  23,  23,  23,  /* 0x6800 */
     23,  23,  23,  23,  23,  23,  23,  23,  /* 0x7000 */
     23,  23,  23,  23,  23,  23,  23,  23,  /* 0x7800 */
     23,  23,  23,  23,  23,  23,  23,  23,  /* 0x8000 */
     23,  23,  23,  23,  23,  23,  23,  23,  /* 0x8800 */
     23,  23,  23,  23,  23,  23,  23,  23,  /* 0x9000 */
     23,  23,  23,  23,  23,  23,  23,  24,  /* 0x9800 */
      7,   7,   7,   7,   7,   7,   7,   7,  /* 0xA000 */
      7,   7,   7,   7,  23,  23,  23,  23,  /* 0xA800 */
     23,  23,  23,  23,  23,  23,  23,  23,  /* 0xB000 */
     23,  23,  23,  23,  23,  23,  23,  23,  /* 0xB800 */
     23,  23,  23,  23,  23,  23,  23,  23,  /* 0xC000 */
     23,  23,  23,  23,  23,  23,  23,  23,  /* 0xC800 */
     23,  23,  23,  23,  23,  23,  23,  25,  /* 0xD000 */
      7,   7,   7,   7,   7,   7,   7,   7,  /* 0xD800 */
      7,   7,   7,   7,   7,   7,   7,   7,  /* 0xE000 */
      7,   7,   7,   7,   7,   7,   7,   7,  /* 0xE800 */
      7,   7,   7,   7,   7,   7,   7,   7,  /* 0xF000 */
      7,  23,  26,  27,  23,  28,  29,  30   /* 0xF800 */
  };

  /* The Y table has 496 entries for a total of 496 bytes. */

  static unsigned char Y[496] = {
      0,   0,   1,   2,   3,   4,   3,   5,  /*   0 */
      0,   0,   6,   7,   8,   9,   8,   9,  /*   0 */
      8,   8,   8,   8,   8,   8,   8,   8,  /*   1 */
      8,   8,   8,   8,   8,   8,   8,  10,  /*   1 */
      8,  11,   0,   0,   0,   8,   8,   8,  /*   2 */
      8,   8,  12,  13,  14,  14,  15,   0,  /*   2 */
     16,  16,  16,  16,  17,   0,  18,  19,  /*   3 */
     20,   8,  21,   8,  22,  23,  24,  25,  /*   3 */
     26,   8,   8,   8,   8,  26,   8,   8,  /*   4 */
     27,   8,   8,   8,  28,   8,  29,  30,  /*   4 */
      0,   0,   0,   3,   8,  31,   3,   8,  /*   5 */
     11,  32,  33,  34,  35,   8,   5,  36,  /*   5 */
      0,   0,   3,   5,  37,  38,   2,  39,  /*   6 */
      8,   8,   8,  40,  22,  41,  42,   2,  /*   6 */
      0,   0,   0,   0,   0,   0,   0,   0,  /*   7 */
      0,   0,   0,   0,   0,   0,   0,   0,  /*   7 */
     43,   8,   8,  44,  45,  46,  47,   0,  /*   8 */
     48,  49,  50,  51,  52,  53,  47,  25,  /*   8 */
     54,  49,  50,  55,  56,  57,  58,  59,  /*   9 */
     60,  21,  50,  61,  62,   0,  63,   0,  /*   9 */
     48,  49,  50,  64,  65,  66,  67,   0,  /*  10 */
     68,  69,  70,  71,  72,  73,  74,   0,  /*  10 */
     75,  24,  50,  76,  77,  78,  67,   0,  /*  11 */
     79,  24,  50,  76,  77,  80,  67,   0,  /*  11 */
     79,  24,  50,  81,  82,  73,  67,   0,  /*  12 */
      0,   0,   0,   0,   0,   0,   0,   0,  /*  12 */
      3,   8,  22,  83,  84,   2,   0,   0,  /*  13 */
     85,  86,  87,  88,  89,  90,   0,   0,  /*  13 */
      0,  91,   2,  92,  93,   8,  94,  32,  /*  14 */
     95,  96,  45,  97,   0,   0,   0,   0,  /*  14 */
      0,   0,   0,   0,   0,   0,   0,   0,  /*  15 */
      0,   0,   8,   8,  98,   8,   8,  99,  /*  15 */
      8,   8,   8,   8,   8, 100,   8,   8,  /*  16 */
      8,   8, 101,   8,   8,   8,   8,  94,  /*  16 */
      8,   8,   8,   8,   8,   8,   8,   8,  /*  17 */
      8, 102,   8,   8,   8,   8,   8,  94,  /*  17 */
      8, 103,   8,   8, 103, 104,   8, 105,  /*  18 */
      8,   8,   8, 106, 107, 108, 109, 107,  /*  18 */
      0,   0,   0, 110, 111,   0,   0, 110,  /*  19 */
      0,   0, 109,   0,   0, 112, 113,   0,  /*  19 */
    114, 115, 116, 117,   0,   0,   8,   8,  /*  20 */
     36,   0,   0,   0,   0,   0,   0,   0,  /*  20 */
    118,   0, 119, 120,   3,   8,   8,   8,  /*  21 */
      8, 121,   3,   8,   8,   8,   8, 122,  /*  21 */
    123,   8, 109,   3,   8,   8,   8,   8,  /*  22 */
     22,   0,   0,   0,   0,   0,   0,   0,  /*  22 */
      8,   8,   8,   8,   8,   8,   8,   8,  /*  23 */
      8,   8,   8,   8,   8,   8,   8,   8,  /*  23 */
      8,   8,   8,   8,   8,   8,   8,   8,  /*  24 */
      8,   8,  98,   0,   0,   0,   0,   0,  /*  24 */
      8,   8,   8,   8,   8,   8,   8,   8,  /*  25 */
      8,   8,  25,   0,   0,   0,   0,   0,  /*  25 */
      8,   8, 105,   0,   0,   0,   0,   0,  /*  26 */
      0,   0,   0,   0,   0,   0,   0,   0,  /*  26 */
     99, 124,  50, 125, 126,   8,   8,   8,  /*  27 */
      8,   8,   8,  14,   0, 127,   8,   8,  /*  27 */
      8,   8,   8, 105,   0,   8,   8,   8,  /*  28 */
      8, 128,   8,   8,  11,   0,   0, 102,  /*  28 */
      0,   0, 129, 130, 131,   0, 132, 133,  /*  29 */
      8,   8,   8,   8,   8,   8,   8, 109,  /*  29 */
      1,   2,   3,   4,   3,   5, 134,   8,  /*  30 */
      8,   8,   8,  22, 135, 136, 137,   0   /*  30 */
  };

  /* The A table has 2208 entries for a total of 552 bytes. */

  static unsigned long A[138] = {
    0x00000000,  /*   0 */
    0x00000300,  /*   1 */
    0x00055555,  /*   2 */
    0xFFFFFFFC,  /*   3 */
    0xC03FFFFF,  /*   4 */
    0x003FFFFF,  /*   5 */
    0x00300FF0,  /*   6 */
    0x00300C00,  /*   7 */
    0xFFFFFFFF,  /*   8 */
    0xFFFF3FFF,  /*   9 */
    0xFFF00FFF,  /*  10 */
    0x0000FFFF,  /*  11 */
    0x0003FFFF,  /*  12 */
    0xFFC3FFFF,  /*  13 */
    0x0000000F,  /*  14 */
    0x000003FF,  /*  15 */
    0x55555555,  /*  16 */
    0x00000555,  /*  17 */
    0x00000005,  /*  18 */
    0x00300000,  /*  19 */
    0xF33F3000,  /*  20 */
    0xFFFFFFCF,  /*  21 */
    0x3FFFFFFF,  /*  22 */
    0x33303FFF,  /*  23 */
    0xFFFFFFF3,  /*  24 */
    0x000000FF,  /*  25 */
    0xF3FFFFFC,  /*  26 */
    0x0000154F,  /*  27 */
    0x03C3C3FF,  /*  28 */
    0xF0FFFFFF,  /*  29 */
    0x000F0FFF,  /*  30 */
    0x000C3FFF,  /*  31 */
    0x55555554,  /*  32 */
    0x55555545,  /*  33 */
    0x45455555,  /*  34 */
    0x00000114,  /*  35 */
    0x0000003F,  /*  36 */
    0x557FFFFF,  /*  37 */
    0x00000015,  /*  38 */
    0xFFFFFFFD,  /*  39 */
    0x3FF0FFFF,  /*  40 */
    0x41555CFF,  /*  41 */
    0x05517D55,  /*  42 */
    0xFFFFFC54,  /*  43 */
    0x5D0FFFFF,  /*  44 */
    0x05555555,  /*  45 */
    0xFFFF0154,  /*  46 */
    0x5555505F,  /*  47 */
    0xC3FFFC54,  /*  48 */
    0xFFFFFFC3,  /*  49 */
    0xFFF3FFFF,  /*  50 */
    0x510FF033,  /*  51 */
    0x05414155,  /*  52 */
    0xCF004000,  /*  53 */
    0xC03FFC10,  /*  54 */
    0x510F3CF3,  /*  55 */
    0x05414015,  /*  56 */
    0x33FC0000,  /*  57 */
    0x55555000,  /*  58 */
    0x000003F5,  /*  59 */
    0xCCFFFC54,  /*  60 */
    0x5D0FFCF3,  /*  61 */
    0x05454555,  /*  62 */
    0x55555003,  /*  63 */
    0x5D0FF0F3,  /*  64 */
    0x05414055,  /*  65 */
    0xCF005000,  /*  66 */
    0x5555500F,  /*  67 */
    0xF03FFC50,  /*  68 */
    0xF33C0FF3,  /*  69 */
    0xF03F03C0,  /*  70 */
    0x500FCFFF,  /*  71 */
    0x05515015,  /*  72 */
    0x00004000,  /*  73 */
    0x55554000,  /*  74 */
    0xF3FFFC54,  /*  75 */
    0x500FFCFF,  /*  76 */
    0x05515155,  /*  77 */
    0x00001400,  /*  78 */
    0xF3FFFC50,  /*  79 */
    0x30001400,  /*  80 */
    0x500FFFFF,  /*  81 */
    0x05515055,  /*  82 */
    0xC01555F7,  /*  83 */
    0x15557FFF,  /*  84 */
    0x0C33C33C,  /*  85 */
    0xFFFCFF00,  /*  86 */
    0x3CF0CCFC,  /*  87 */
    0x0D4555F7,  /*  88 */
    0x055533FF,  /*  89 */
    0x0F055555,  /*  90 */
    0x00050000,  /*  91 */
    0x50044400,  /*  92 */
    0xFFFCFFFF,  /*  93 */
    0x000FFFFF,  /*  94 */
    0x00555155,  /*  95 */
    0x55544555,  /*  96 */
    0x00045554,  /*  97 */
    0x00000FFF,  /*  98 */
    0x00003FFF,  /*  99 */
    0xC00FFFFF,  /* 100 */
    0xFFFF003F,  /* 101 */
    0x00FFFFFF,  /* 102 */
    0x0FFF0FFF,  /* 103 */
    0xCCCCFFFF,  /* 104 */
    0x0FFFFFFF,  /* 105 */
    0x33FFF3FF,  /* 106 */
    0x03FFF3F0,  /* 107 */
    0x00FFF0FF,  /* 108 */
    0x03FFFFFF,  /* 109 */
    0xC0000000,  /* 110 */
    0x00000003,  /* 111 */
    0x01555555,  /* 112 */
    0x00000004,  /* 113 */
    0xFFF0C030,  /* 114 */
    0x0FFF0CFF,  /* 115 */
    0xFFF33300,  /* 116 */
    0x0003FFCF,  /* 117 */
    0x0000CC00,  /* 118 */
    0x555FFFFC,  /* 119 */
    0x00000FFC,  /* 120 */
    0x3FD403FF,  /* 121 */
    0x3F3FFFFF,  /* 122 */
    0xFFFFFC00,  /* 123 */
    0xD000FFC0,  /* 124 */
    0x33FF3FFF,  /* 125 */
    0xFFFFF3CF,  /* 126 */
    0xFFFFFFC0,  /* 127 */
    0xFFFFFFF0,  /* 128 */
    0x00000055,  /* 129 */
    0x000003C0,  /* 130 */
    0xFC000000,  /* 131 */
    0x000C0000,  /* 132 */
    0xFFFFF33F,  /* 133 */
    0xFFFFF000,  /* 134 */
    0xFFF0FFF0,  /* 135 */
    0x03F0FFF0,  /* 136 */
    0x00003C0F   /* 137 */
  };

  /* In all, the character property tables require 1304 bytes. */

/*
 * This code mirrors Character.isJavaIdentifierStart.  It determines whether
 * the specified character is a legal start of a Java identifier as per JLS.
 *
 * The parameter ch is the character to be tested; return 1 if the 
 * character is a letter, 0 otherwise.
 */
#define isJavaIdentifierStart(ch) (((A[Y[(X[ch>>8]<<4)|((ch>>4)&0xF)]]>>((ch&0xF)<<1))&3) & 0x2)

/*
 * This code mirrors Character.isJavaIdentifierPart.  It determines whether
 * the specified character is a legal part of a Java identifier as per JLS.
 * 
 * The parameter ch is the character to be tested; return 1 if the
 * character is a digit, 0 otherwise.
 */  
#define isJavaIdentifierPart(ch) (((A[Y[(X[ch>>8]<<4)|((ch>>4)&0xF)]]>>((ch&0xF)<<1))&3) & 0x1)

/*=========================================================================
 * FUNCTION:      skip_over_fieldname 
 * OVERVIEW:      Skips over the longest part of the string that could be 
 *                taken as a fieldname given a pointer to a string.
 *                Allows '/' if slash_okay parameter is true.
 *                Returns a pointer to just past the fieldname. 
 *                Returns NULL if no fieldname is found, or in the case of
 *                slash_okay being TRUE, we may see consecutive slashes 
 *                (meaning that we were looking for a qualified path, but 
 *                found something that was badly-formed.)
 * INTERFACE:
 *   parameters:  char*: name
 *                boolean: slash_okay
 *                
 *   returns:     char* field name or NULL
 *=======================================================================*/
static char *
skip_over_fieldname(char *name, bool_t slash_okay)
{
    bool_t first;
    char *p;
    unicode last_ch = 0;
    for (p = name, first = TRUE; ; first = FALSE) {
    char *old_p = p;
    unicode ch = next_utf2unicode(&p);
    if (isJavaIdentifierStart(ch) || (!first && isJavaIdentifierPart(ch)) 
          || (slash_okay && ch == '/' && !first)
          || ch == '_' || ch == '$') {
        if (ch == '/' && last_ch == '/') {
        return 0;    /* Don't permit consecutive slashes */
        } else {
        last_ch = ch;
        }
    } else {
        return first ? 0 : old_p;
    }
    }
}

/*=========================================================================
 * FUNCTION:      skip_over_field_signature 
 * OVERVIEW:      Skips over the longest part of the string that could be 
 *                taken as a field signature given a pointer to a string.
 *                Allows "void" if void_okay parameter is true.
 *                Returns a pointer to just past the signature. 
 *                Returns NULL if no legal signature is found. 
 * INTERFACE:
 *   parameters:  char*: name
 *                boolean: void_okay
 *                
 *   returns:     char* field signature or NULL
 *=======================================================================*/
static char *
skip_over_field_signature(char *name, bool_t void_okay)
{
    for (;;) {
    switch (name[0]) {
            case SIGNATURE_VOID:
        if (!void_okay) return 0;
        /* FALL THROUGH */
            case SIGNATURE_BOOLEAN:
            case SIGNATURE_BYTE:
            case SIGNATURE_CHAR:
            case SIGNATURE_SHORT:
            case SIGNATURE_INT:
            case SIGNATURE_LONG:
        return name + 1;

            case SIGNATURE_FLOAT:
            case SIGNATURE_DOUBLE:
                return no_floating_point ? NULL : name + 1;

        case SIGNATURE_CLASS: {
        /* Skip over the classname, if one is there. */
        char *p = skip_over_fieldname(name + 1, TRUE);
        /* The next character better be a semicolon. */
        if (p && p[0] == ';') {
            return p + 1;
                }
        return NULL;
        }
        
        case SIGNATURE_ARRAY: 
        /* The rest of what's there better be a legal signature.  */
        name++;
        void_okay = FALSE;
        break;

        default:
        return NULL;
    }
    }
}

/*=========================================================================
 * FUNCTION:      CCerror
 * OVERVIEW:      Handles formating errors found during class file 
 *                verification. 
 * INTERFACE:
 *   parameters:  pointer to the ClassClass structure. 
 *                char *: format
 *                
 *   returns:     nothing 
 *=======================================================================*/
static void 
CCerror (ClassClass *cb, char *format, ...)
{
    if (verbose) { 
    va_list args;
    jio_fprintf(stderr, "VERIFIER CLASS ERROR %s:\n", cbName(cb));
    va_start(args, format);
        jio_vfprintf(stderr, format, args);        
    va_end(args);
    jio_fprintf(stderr, "\n");
    }
}

/*=========================================================================
 * FUNCTION:      IsLegalClassname
 * OVERVIEW:      Determines if the specified name is a legal UTF name for
 *                a class name.
 *                Note that this routine is intended for external use and 
 *                expects the internal form of qualified classes 
 *                (i.e. the dots should have been replaced by slashes.)
 * INTERFACE:
 *   parameters:  char*: name
 *                boolean: allowArrayClass 
 *                
 *   returns:     boolean type 
 *=======================================================================*/
bool_t IsLegalClassname(char *name, bool_t allowArrayClass) 
{ 
    char *p;
    if (name[0] == SIGNATURE_ARRAY) {
    if (!allowArrayClass) {
        return FALSE;
    } else { 
        /* Everything that's left better be a field signature */
        p = skip_over_field_signature(name, FALSE);
    }
    } else {
    /* skip over the fieldname.  Slashes are okay */
    p = skip_over_fieldname(name, TRUE);
    }
    return (p != 0 && p[0] == '\0');
}
