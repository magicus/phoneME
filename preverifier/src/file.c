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
 * SUBSYSTEM: Operations on class files.
 * FILE:      file.c
 * OVERVIEW:  Routines for outputing the internal class file.
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#ifdef UNIX
#include <unistd.h>
#endif

#include <oobj.h>
#include <jar.h>
#include <tree.h>
#include <sys_api.h>
#include <convert_md.h>

#ifdef WIN32
#include <direct.h>
#endif

/* initialize opnames[256]; */
#include <opcodes.init>

/*=========================================================================
 * Globals and extern declarations
 *=======================================================================*/

extern char *progname;
extern bool_t JARfile;
extern bool_t tmpDirExists;

extern struct StrIDhash *nameTypeHash;

char *current_class_name = NULL;

bool_t stack_map_on = TRUE;

static char *PrintableClassname(char *classname);

extern char tmp_dir[32];

char *output_dir = "output";
unsigned char *class_buf;
int class_buf_size = 0;
int class_index;
int last_not_utf8_index;

#define INIT_CLASS_BUF_SIZE 256

#define Str2IDFree_Local(localHashRoot) Str2IDFree(localHashRoot)

void ensure_capacity(int sz)
{
    if (class_index + sz >= class_buf_size) {
        while (class_index + sz >= class_buf_size) {
            class_buf_size *= 2;
        }
    }
    class_buf = (unsigned char *)realloc(class_buf, class_buf_size);
}

void write_u1(unsigned long u1)
{
    ensure_capacity(1);
    class_buf[class_index++] = (unsigned char)u1;
}

void write_u2(unsigned long u2)
{
    ensure_capacity(2);
    class_buf[class_index++] = (unsigned char)(u2 >> 8);
    class_buf[class_index++] = (unsigned char)u2;
}

void write_u4(unsigned long u4)
{
    ensure_capacity(4);
    class_buf[class_index++] = (unsigned char)(u4 >> 24);
    class_buf[class_index++] = (unsigned char)(u4 >> 16);
    class_buf[class_index++] = (unsigned char)(u4 >> 8);
    class_buf[class_index++] = (unsigned char)u4;
}

int new_utf8_index(ClassClass *cb, int index)
{
    if (index > last_not_utf8_index) {
        return index + unhand(cb)->n_new_class_entries;
    } else {
        return index;
    }
}

unsigned long lookup_utf8(ClassClass *cb, char *utf8)
{
    int i;
    cp_item_type *constant_pool = cbConstantPool(cb);
    int cpEntries, newEntries;
    unsigned char *type_table =
        constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].type;
    cpEntries = cbConstantPoolCount(cb);
    newEntries = unhand(cb)->n_new_utf8_entries;
    for (i = 0; i < cpEntries; i++) {
        if (type_table[i] == (CONSTANT_Utf8 | CONSTANT_POOL_ENTRY_RESOLVED) &&
            strcmp(utf8, constant_pool[i].cp) == 0) {
            return new_utf8_index(cb, i);
        }
    }
    for (i = 0; i < newEntries; i++) {
        if (strcmp(unhand(cb)->new_utf8_entries[i], utf8) == 0) {
            return i + cpEntries + unhand(cb)->n_new_class_entries;
        }
    }
    unhand(cb)->new_utf8_entries[newEntries] = utf8;
    unhand(cb)->n_new_utf8_entries = newEntries + 1;
    return newEntries + cpEntries + unhand(cb)->n_new_class_entries;
}

unsigned long lookup_class(ClassClass *cb, char *name)
{
    int i;
    cp_item_type *constant_pool = cbConstantPool(cb);
    unsigned char *type_table =
        constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].type;
    for (i = 0; i < cbConstantPoolCount(cb); i++) {
        if (type_table[i] == (CONSTANT_Class | CONSTANT_POOL_ENTRY_RESOLVED) &&
            strcmp(name, cbName(constant_pool[i].clazz)) == 0) {
            return i;
        }
        if (type_table[i] == CONSTANT_Class &&
            strcmp(name, constant_pool[constant_pool[i].i].cp) == 0) {
            return i;
        }
    }
    panic("class expected to be in constant pool: %s", name);
    return 0;
}

int get_last_not_utf8_index(ClassClass *cb)
{
    int i;
    int result;
    cp_item_type *constant_pool = cbConstantPool(cb);
    unsigned char *type_table =
        constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].type;
    for (i = 0, result = -1; i < cbConstantPoolCount(cb); i++) {
        if (type_table[i] != (CONSTANT_Utf8 | CONSTANT_POOL_ENTRY_RESOLVED)) {
            result = i;
        }
    }
    return result;
}

void write_constant_pool(ClassClass *cb)
{
    int i, j;
    cp_item_type *constant_pool = cbConstantPool(cb);
    unsigned char *type_table =
        constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].type;
    unsigned int sealedValue;

    last_not_utf8_index = get_last_not_utf8_index(cb);
    if (stack_map_on && unhand(cb)->has_stack_maps) {
        /* The maximum number of new utf8 entries we'll need is one
         * for "StackMap", plus one for each of the new class entries.
         * However, we bump the number to 2 in case we need to add another
         * entry for the SourceFile attribute.
         */
        unhand(cb)->n_new_utf8_entries = 0;
        unhand(cb)->new_utf8_entries =
            (char **)malloc((unhand(cb)->n_new_class_entries + 2) *
                            sizeof(char *));
        /* Put sure we have all the Utf8 entries that we need, so that
         * we'll know the final size of the constant pool.
         */
        lookup_utf8(cb, "StackMap");
        for (i = 0; i < unhand(cb)->n_new_class_entries; i++) {
            lookup_utf8(cb, unhand(cb)->new_class_entries[i]);
        }
    } else {
        unhand(cb)->n_new_utf8_entries = 0;
        unhand(cb)->new_utf8_entries = NULL;
        unhand(cb)->n_new_class_entries = 0;
        if (unhand(cb)->new_class_entries != NULL) {
            free(unhand(cb)->new_class_entries);
            unhand(cb)->new_class_entries = NULL;
        }
    }
    /* At this, we should not add have to create any more utf8 entries.
     * We  find the value of unhand(cb)->n_new_utf8_entries, and complain
     * later if this value has changed.
     */
    sealedValue = unhand(cb)->n_new_utf8_entries;

    write_u2(cbConstantPoolCount(cb) +
             unhand(cb)->n_new_class_entries +
             unhand(cb)->n_new_utf8_entries);

    for (i = 1; i < cbConstantPoolCount(cb); i++) {
        write_u1(type_table[i] & CONSTANT_POOL_ENTRY_TYPEMASK);
        switch(type_table[i]) {
        case CONSTANT_Utf8 | CONSTANT_POOL_ENTRY_RESOLVED:
            write_u2(strlen(constant_pool[i].cp));
            for (j = 0; j < (int)strlen(constant_pool[i].cp); j++) {
                write_u1(constant_pool[i].cp[j]);
            }
            break;

        case CONSTANT_Class:
        case CONSTANT_String:
            write_u2(new_utf8_index(cb, constant_pool[i].i));
            break;

        case CONSTANT_Class | CONSTANT_POOL_ENTRY_RESOLVED:
            write_u2(lookup_utf8(cb, cbName(constant_pool[i].clazz)));
            break;

        case CONSTANT_Fieldref:
        case CONSTANT_Methodref:
        case CONSTANT_InterfaceMethodref:
            write_u2(constant_pool[i].i >> 16);
            write_u2(constant_pool[i].i & 0xFFFF);
            break;

        case CONSTANT_NameAndType:
            write_u2(new_utf8_index(cb, constant_pool[i].i >> 16));
            write_u2(new_utf8_index(cb, constant_pool[i].i & 0xFFFF));
            break;

        case CONSTANT_Integer | CONSTANT_POOL_ENTRY_RESOLVED:
        case CONSTANT_Float | CONSTANT_POOL_ENTRY_RESOLVED:
            write_u4(constant_pool[i].i);
            break;

        case CONSTANT_Long | CONSTANT_POOL_ENTRY_RESOLVED:
        case CONSTANT_Double | CONSTANT_POOL_ENTRY_RESOLVED:
            write_u4(constant_pool[i].i);
            write_u4(constant_pool[i + 1].i);
            i++;
            break;

        default:
            panic("bad constant pool entry type: %d", type_table[i]);
        }
        if (i == last_not_utf8_index) {
            /* Write extra CONSTANT_Class entries created by the stackmaps */
            for (j = 0; j < unhand(cb)->n_new_class_entries; j++) {
                char *classname = unhand(cb)->new_class_entries[j];
                write_u1(CONSTANT_Class);
                write_u2(lookup_utf8(cb, classname));
            }
        }
    }
    /* Write extra CONSTANT_Utf8 entries created by the stackmaps */
    for (i = 0; i < unhand(cb)->n_new_utf8_entries; i++) {
        char *string = unhand(cb)->new_utf8_entries[i];
        int length = strlen(string);
        write_u1(CONSTANT_Utf8);
        write_u2(length);
        for (j = 0; j < length; j++) {
            write_u1(string[j]);
        }
    }
    if (sealedValue != (unsigned int) unhand(cb)->n_new_utf8_entries) {
        panic("New utf8 entries have been added???");
    }

}

void write_interfaces(ClassClass *cb)
{
    int i;
    write_u2(cbImplementsCount(cb));
    for (i = 0; i < cbImplementsCount(cb); i++) {
        write_u2(cbImplements(cb)[i]);
    }
}

void write_fields(ClassClass *cb)
{
    int i;
    write_u2(cbFieldsCount(cb));
    for (i = 0; i < cbFieldsCount(cb); i++) {
        struct fieldblock *fb = &cbFields(cb)[i];
        write_u2(fb->access & ACC_WRITTEN_FLAGS);
        write_u2(lookup_utf8(cb, fb->name));
        write_u2(lookup_utf8(cb, fb->signature));
        /* Number of attributes we're about to output.
         * Each item in the following sum is a boolean that returns 1 or 0.
         */
        write_u2(  ((fb->access & ACC_VALKNOWN) != 0)
                 + (fb->synthetic != 0)
                 + (fb->deprecated != 0));
        if (fb->access & ACC_VALKNOWN) {
            write_u2(lookup_utf8(cb, "ConstantValue")); /* Attribute name */
            write_u4(2);                                /* Length */
            write_u2(fb->u.offset);
        }
        if (fb->deprecated) {
            write_u2(lookup_utf8(cb, "Deprecated"));     /* Attribute name  */
            write_u4(0);                                 /* Length */
        }
        if (fb->synthetic) {
            write_u2(lookup_utf8(cb, "Synthetic"));      /* Attribute name  */
            write_u4(0);                                 /* Length */
        }
    }
}

void write_stack_map(int nentries, struct map_entry *entries)
{
    int i;
    for (i = 0; i < nentries; i++) {
        struct map_entry *entry = entries + i;
        write_u1(entry->type);
        if (entry->type == CF_ITEM_NewObject) {
            write_u2(entry->info);
        }
        if (entry->type == CF_ITEM_Object) {
            /* This is ugly
             * Non-negative entries refer to classes
             *       in the original constant pool.
             * Negative entries, ~(entry->info) is the entry's index
             *       in the list of additional constant pool entries.
             */
            if (entry->info >= 0) {
                write_u2(entry->info);
            } else {
                write_u2(last_not_utf8_index + 1 + ~entry->info);
            }
        }
    }
}

int stack_map_size(int nentries, struct map_entry *entries)
{
    int i;
    int result = 0;
    for (i = 0; i < nentries; i++) {
        struct map_entry *entry = entries + i;
        result += 1;
        if (entry->type == CF_ITEM_Object || entry->type == CF_ITEM_NewObject) {
            result += 2;
        }
    }
    return result;
}

void write_methods(ClassClass *cb)
{
    int i;
    write_u2(cbMethodsCount(cb));
    for (i = 0; i < cbMethodsCount(cb); i++) {
        struct methodblock *mb = &cbMethods(cb)[i];
        write_u2(mb->fb.access & ACC_WRITTEN_FLAGS);
        write_u2(lookup_utf8(cb, mb->fb.name));
        write_u2(lookup_utf8(cb, mb->fb.signature));

        /* Attribute count
         * Each item in the following sum is a boolean that returns 1 or 0.
         */
        write_u2(  (mb->code_length > 0)
                 + (mb->nexceptions > 0)
                 + (mb->fb.synthetic != 0)
                 + (mb->fb.deprecated != 0));

        if (mb->code_length > 0) {
            int j;
            int stack_map_attr_length = 0;   /* Stack Map attribute length */
            int line_no_attr_length = 0;     /* line_number_table attr Length */
            int localvar_attr_length = 0;    /* localVar_table attr Length */
            int code_attrs = 0;        /* Attributes count for Code attribute */

            if (stack_map_on && mb->n_stack_maps > 0) {
                stack_map_attr_length = 8;
                code_attrs++;                /* increment code attributes */
                for (j = 0; j < mb->n_stack_maps; j++) {
                    stack_map_attr_length += 6 +
                        stack_map_size(mb->stack_maps[j].nlocals,
                                       mb->stack_maps[j].locals) +
                        stack_map_size(mb->stack_maps[j].nstacks,
                                       mb->stack_maps[j].stacks);
                }
            }

            /* attribute_name_index for "Code" attribute */
            write_u2(lookup_utf8(cb, "Code"));
            if (mb->line_number_table_length > 0) {
                /* calculate the size of the line_number_table attribute */
                /* Attribute length for line_number_table
                 * sizeof line_number_table(4) * no. of table entries + 8
                 * 8 bytes = 2 bytes for attr_name_index +
                 *           4 bytes for attr_length +
                 *           2 bytes for line_number_table_length
                 */
                code_attrs++;      /* increment code attributes */
                line_no_attr_length = 4 * mb->line_number_table_length + 8;
            }
            if (mb->localvar_table_length > 0) {
                /* calculate the size of the localvar_table attribute */
                /* Attribute length for localvar_table
                 * sizeof localvar_table (10) * no. of table entries + 8
                 * 8 bytes = 2 bytes for attr_name_index +
                 *           4 bytes for attr_length +
                 *           2 bytes for line_number_table_length
                 */
                code_attrs++;      /* increment code attributes */
                localvar_attr_length = 10 * mb->localvar_table_length + 8;
            }

            /* Attribute Length */
            write_u4(12 + mb->code_length
                     + mb->exception_table_length * 8
                     + stack_map_attr_length
                     + line_no_attr_length
                     + localvar_attr_length);
            write_u2(mb->maxstack);
            write_u2(mb->nlocals);
            write_u4(mb->code_length);
            for (j = 0; j < (int)mb->code_length; j++) {
                write_u1(mb->code[j]);
            }
            write_u2(mb->exception_table_length);
            for (j = 0; j < (int)mb->exception_table_length; j++) {
                write_u2(mb->exception_table[j].start_pc);
                write_u2(mb->exception_table[j].end_pc);
                write_u2(mb->exception_table[j].handler_pc);
                write_u2(mb->exception_table[j].catchType);
            }

            /* Attributes count for Code attribute */
            write_u2(code_attrs);

            /* check if we have a valid line_number_table entries and
             * if so, write out the pc and line_number entries.
             */

            if (mb->line_number_table_length > 0) {
                /* line_number_table attribute exists */
                /* attribute_name_index for "LineNumberTable" */
                write_u2(lookup_utf8(cb, "LineNumberTable"));
                /* Attribute length for line_number_table
                 * (exclude initial 6 bytes = 2 bytes for attr_name_index +
                 *                            4 bytes for attr_length)
                 */
                write_u4(line_no_attr_length - 6);
                /* Length of line_number_table */
                write_u2(mb->line_number_table_length);
                /* write out the line_number_table entries */
                for (j=0; j< (int) mb->line_number_table_length; j++) {
                    if (mb->line_number_table != NULL) {
                        write_u2(mb->line_number_table[j].pc);
                        write_u2(mb->line_number_table[j].line_number);
                    }
                }
            }

            /* check if we have a valid localvar_table entries and
             * if so, write out its entries
             */

            if (mb->localvar_table_length > 0) {
                /* localvar_table attribute exists */
                /* attribute_name_index for "LocalVariableTable" */
                write_u2(lookup_utf8(cb, "LocalVariableTable"));

                /* Attribute length for localvar_table
                 * (exclude initial 6 bytes = 2 bytes for attr_name_index +
                 *                            4 bytes for attr_length)
                 */
                write_u4(localvar_attr_length - 6);

                /* Length of localvar_table */
                write_u2(mb->localvar_table_length);

                /* write out the localvar_table entries */
                for (j=0; j< (int) mb->localvar_table_length; j++) {
                    if (mb->localvar_table != NULL) {
                        write_u2(mb->localvar_table[j].pc0);
                        write_u2(mb->localvar_table[j].length);
                        write_u2(lookup_utf8(cb,mb->localvar_table[j].name));
                        write_u2(lookup_utf8(cb,mb->localvar_table[j].signature));
                        write_u2(mb->localvar_table[j].slot);
                    }
                }
            }

            if (stack_map_on && mb->n_stack_maps > 0) {
                write_u2(lookup_utf8(cb, "StackMap"));
                write_u4(stack_map_attr_length - 6);
                write_u2(mb->n_stack_maps);
                for (j = 0; j < mb->n_stack_maps; j++) {
                    write_u2(mb->stack_maps[j].offset);
                    write_u2(mb->stack_maps[j].nlocals);
                    write_stack_map(mb->stack_maps[j].nlocals,
                                    mb->stack_maps[j].locals);
                    write_u2(mb->stack_maps[j].nstacks);
                    write_stack_map(mb->stack_maps[j].nstacks,
                                    mb->stack_maps[j].stacks);
                }
            }
        }
        if (mb->nexceptions > 0) {
            int j;
            write_u2(lookup_utf8(cb, "Exceptions"));
            write_u4(2 + mb->nexceptions * 2);
            write_u2(mb->nexceptions);
            for (j = 0; j < mb->nexceptions; j++) {
                write_u2(mb->exceptions[j]);
            }
        }
        if (mb->fb.deprecated) {
            write_u2(lookup_utf8(cb, "Deprecated"));
            write_u4(0);                                 /* Length */
        }
        if (mb->fb.synthetic) {
            write_u2(lookup_utf8(cb, "Synthetic"));
            write_u4(0);                                 /* Length */
        }
    }
}

void ensure_dir_exists(char *dir)
{
    struct stat stat_buf;
    char *parent;
    char *q;
    if (dir[0] == 0) {
        return;
    }
    parent = strdup(dir);
    q = strrchr(parent, '/');
    if (q) {
        *q = 0;
        ensure_dir_exists(parent);
    }
    if (stat(dir, &stat_buf) < 0) {
        if (JAR_DEBUG && verbose) {
            jio_fprintf(stderr, "Creating output directory [%s]\n", dir);
        }
#ifdef WIN32
        mkdir(dir);
#endif
#ifdef UNIX
        mkdir(dir, 0755);
#endif
    }
    free(parent);
}

void ensure_dir_writable(char *dir)
{
    struct stat stat_buf;

    stat(dir, &stat_buf);
    if ((stat_buf.st_mode & S_IFMT) == S_IFDIR) { /* is dir ? */
#ifdef WIN32
        if (access(dir, 06) < 0) {
#endif
#ifdef UNIX
        if (access(dir, R_OK | W_OK) < 0) {
#endif
            panic("%s is write protected\n", dir);
        }
    } else {
        panic("%s is not a directory\n", dir);
    }
}

void
WriteClass(ClassClass *cb)
{
    int fd;
    char fname[1024];
    char buff[BUFSIZ];
    char *nativeName = &buff[0];

    class_buf = (unsigned char*)malloc(INIT_CLASS_BUF_SIZE);
    if (class_buf == NULL) {
        panic("out of memory");
    }
    class_buf_size = INIT_CLASS_BUF_SIZE;

    class_index = 0;

    write_u4(0xcafebabe);

    write_u2(cbMinorVersion(cb));
    write_u2(cbMajorVersion(cb));

    write_constant_pool(cb);

    write_u2(cbAccess(cb) & ACC_WRITTEN_FLAGS);

    write_u2(lookup_class(cb, cbName(cb)));
    write_u2(cbSuperclass(cb)
             ? lookup_class(cb,  cbName(cbSuperclass(cb))) : 0);
    write_interfaces(cb);
    write_fields(cb);
    write_methods(cb);

    /* Output number of attributes
     * Each item in the following sum is a boolean that returns 1 or 0.
     */
    write_u2(   (cbSourceName(cb) != NULL)
              + (cbAbsoluteSourceName(cb) != NULL)
              + (unhand(cb)->hasTimeStamp)
              + (unhand(cb)->deprecated)
              + (unhand(cb)->synthetic)
              + (cbInnerClasses(cb) != NULL)
            );
    if (cbSourceName(cb) != NULL) {
        /* write the source file attribute used for debugging purposes */
        write_u2(lookup_utf8(cb, "SourceFile"));     /* SourceFile attribute */
        write_u4(2);                                 /* Length */
                                 /* CP entry containing the source name */
        write_u2(lookup_utf8(cb, cbSourceName(cb)));
    }
    if (cbAbsoluteSourceName(cb) != NULL) {
        /* write the source file attribute used for debugging purposes */
        write_u2(lookup_utf8(cb, "AbsoluteSourcePath"));
        write_u4(2);
        write_u2(lookup_utf8(cb, cbAbsoluteSourceName(cb)));
    }
    if (unhand(cb)->hasTimeStamp) {
        write_u2(lookup_utf8(cb, "TimeStamp"));
        write_u4(8);
        write_u4(cbTimestamp(cb).high);
        write_u4(cbTimestamp(cb).low);
    }
    if (unhand(cb)->deprecated) {
        write_u2(lookup_utf8(cb, "Deprecated"));     /* attribute name */
        write_u4(0);                                 /* Length */
    }
    if (unhand(cb)->synthetic) {
        write_u2(lookup_utf8(cb, "Synthetic"));     /* attribute name */
        write_u4(0);                                /* Length */
    }

    if (cbInnerClasses(cb) != NULL) {
        int count = cbInnerClassesCount(cb);
        struct innerClasses *thisInnerClass= cbInnerClasses(cb);
        struct innerClasses *lastInnerClass = thisInnerClass + count;
        write_u2(lookup_utf8(cb, "InnerClasses"));  /* Attribute name */
        write_u4(8 * count + 2);                    /* Length */
        write_u2(count);
        for ( ; thisInnerClass < lastInnerClass; thisInnerClass++) {
            char *innerName = thisInnerClass->inner_name;
            write_u2(thisInnerClass->inner_class);
            write_u2(thisInnerClass->outer_class);
            write_u2( (innerName != 0) ? lookup_utf8(cb, innerName) : 0);
            write_u2(thisInnerClass->access);
        }
    }

    /* Conversion for Japanese filenames */
    utf2native(cbName(cb), nativeName, BUFSIZ);

    if (JARfile) {
        /* classes need to be put in a JAR file */
        sprintf(fname, "%s/%s.class", tmp_dir, nativeName);
    } else {
        sprintf(fname, "%s/%s.class", output_dir, nativeName);
    }

    {
        char *dir = strdup(fname);
        char *q;

        q = strrchr(dir, '/');
        if (q) {
            *q = 0;
            ensure_dir_exists(dir);
            ensure_dir_writable(dir);
        }
        free(dir);
    }

#ifdef UNIX
    fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC , 0644);
#endif
#ifdef WIN32
    fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
#endif

    if (fd < 0) {
        panic("failed to open %s", fname);
    }

    tmpDirExists = TRUE;   /* tmpDir exists with verified classes */
    write(fd, class_buf, class_index);
    close(fd);
    free(class_buf);
    class_buf_size = 0;
}

void
VerifyFile(char *fn)
{
    /* If this is the first class, we'll run into problems if loading the
     * class forces Object to be loaded, when then forces this class to
     * be loaded.  To prohibit such problems, we force Object to be loaded
     */
    FindClass(0, "java/lang/Object", TRUE);

    {
        ClassClass *cb = FindClass(0, fn, TRUE);
        char *class_name = PrintableClassname(fn);

        if (cb == NULL) {
            errorCode = 1;  /* set error status to indicate error */
            jio_fprintf(stderr, "Error loading class %s\n", class_name);
        } else {
            if (no_native_methods) {
                /* Check for native methods in classes */
                struct methodblock *mb;
                int size;
                mb = cbMethods(cb);
                for (size=0; size < (int) cbMethodsCount(cb); size++, mb++) {
                    if (mb->fb.access & ACC_NATIVE) {
                        current_class_name = fn;
                        panic("native methods should not appear");
                    }
                }
            }
            WriteClass(cb);
            Str2IDFree_Local(&nameTypeHash);
        }
    }
}

char *PrintableClassname(char *class_name)
{
    char *p;
    static char class_copy[257];
    strncpy(class_copy, class_name, 256);

    /* Convert all slashes in the classname to periods */
    for (p = class_copy; ((p = strchr(p, '/')) != 0); *p++ = '.');
    return class_copy;
}

void printCurrentClassName(void)
{
    if (current_class_name) {
        fprintf(stderr, "Error preverifying class %s\n    ",
                PrintableClassname(current_class_name));
    }
}
