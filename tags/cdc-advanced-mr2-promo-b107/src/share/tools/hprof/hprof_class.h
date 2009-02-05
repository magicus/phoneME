/*
 * @(#)hprof_class.h	1.13 06/10/10
 *
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
 *
 */

#ifndef _HPROF_CLASS_H
#define _HPROF_CLASS_H

#include "hprof_hash.h"

/* Methods */
typedef struct hprof_method_t {
    jmethodID method_id;                      /* id from JVM,  -1 on class unload */
    struct hprof_class_t *class;              /* class to which this method belongs */
    struct hprof_name_t *method_name;         /* name of method */
    struct hprof_name_t *method_signature;    /* signature of method */
} hprof_method_t;

/* Fields */
typedef struct {
    struct hprof_name_t *name;                /* name of the field */
    jint type;                                /* type */
} hprof_field_t;

/* Classes */
typedef struct hprof_class_t {
    struct hprof_objmap_t *class_id;   /* ptr to jvm<--->hprof object map, 
					  -1 on unload */
    struct hprof_class_t *super;       /* super class */
    unsigned int serial_num;           /* unique serial number */
    struct hprof_name_t *name;         /* class name */
    struct hprof_name_t *src_name;     /* id of src file name */
    int num_interfaces;                /* number of interfaces */
    int num_statics;                   /* number of static fields */
    hprof_field_t *statics;            /* statics */
    int num_instances;                 /* number of instances */
    hprof_field_t *instances;          /* instances */
    int num_methods;                   /* number of methods */
    hprof_method_t **methods;          /* methods */
} hprof_class_t;

extern hprof_class_t *java_lang_object_class;

void hprof_class_table_init(void);
hprof_class_t * hprof_lookup_class_objmap(struct hprof_objmap_t *objmap);
hprof_class_t * hprof_lookup_class(jobjectID class_id);
void hprof_class_load_event(JNIEnv *env_id, 
                            const char *name,
			    char* src_name,
			    int num_interfaces,
			    int num_statics,
			    JVMPI_Field *statics,
			    int num_instances,
			    JVMPI_Field *instances,
			    int num_methods,
			    JVMPI_Method *methods,
			    jobjectID class_id,
			    int requested);
void hprof_class_unload_event(JNIEnv *env_id, jobjectID class_id);
void hprof_superclass_link(jobjectID class_id, jobjectID super_id);
#ifdef HASH_STATS
void hprof_print_class_hash_stats(FILE *fp);
#endif /* HASH_STATS */
#ifdef WATCH_ALLOCS
void hprof_free_class_table(void);
#endif /* WATCH_ALLOCS */

#endif /* _HPROF_CLASS_H */
