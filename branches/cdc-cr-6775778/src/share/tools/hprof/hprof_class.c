/*
 * @(#)hprof_class.c	1.19 06/10/10
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

#include "jvmpi.h"
#include "hprof.h"
#include "javavm/include/porting/ansi/string.h"

static unsigned int class_serial_number = 1;
static hprof_hash_t hprof_class_table;
hprof_class_t *java_lang_object_class = NULL;

static unsigned int hash_class(void *_class)
{
    hprof_class_t *class = _class;
    return ((unsigned int)class->class_id >> HASH_OBJ_SHIFT) %
	hprof_class_table.size;
}

static unsigned int size_class(void *_class)
{
    return sizeof(hprof_class_t);
}

static int compare_class(void *_class1, void *_class2)
{
    hprof_class_t *class1 = _class1;
    hprof_class_t *class2 = _class2;

    if (class1->class_id == class2->class_id) {
        return 0;
    } else {
        return 1;
    }
}

void hprof_class_table_init(void)
{
    hprof_hash_init(ALLOC_HASH_CLASS, &hprof_class_table, 256,
		    hash_class, size_class, compare_class);
}

hprof_class_t * hprof_lookup_class_objmap(hprof_objmap_t *objmap)
{
    hprof_class_t class_tmp;

    class_tmp.class_id = objmap;
    return hprof_hash_lookup(&hprof_class_table, &class_tmp);
}

hprof_class_t * hprof_lookup_class(jobjectID class_id)
{
    hprof_objmap_t *objmap = hprof_fetch_object_info(class_id);

    if (objmap == NULL) {
        return NULL;
    }
    return hprof_lookup_class_objmap(objmap);
}

static unsigned char sigToTy(char sig)
{
    switch(sig) {
    case 'L':
    case '[':
        return JVMPI_CLASS;
    case 'Z':
        return JVMPI_BOOLEAN;
    case 'B':
        return JVMPI_BYTE;
    case 'C':
        return JVMPI_CHAR;
    case 'S':
        return JVMPI_SHORT;
    case 'I':
        return JVMPI_INT;
    case 'J':
        return JVMPI_LONG;
    case 'F':
        return JVMPI_FLOAT;
    case 'D':
        return JVMPI_DOUBLE;
    default:
        return 0;
    }
}

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
			    int requested)
{
    hprof_class_t *result;
    hprof_class_t class_tmp;
    hprof_field_t *hstatics;
    hprof_field_t *hinstances;
    hprof_objmap_t *objmap;
    int i;
    
    CALL(RawMonitorEnter)(data_access_lock); 
   
    objmap = hprof_fetch_object_info(class_id);

    if (objmap == NULL) {
        fprintf(stderr, "HPROF ERROR: unable to map JVMPI class ID to hprof "
		"class ID in class_load \n");
	goto classload_done;
    }
   
    class_tmp.class_id = objmap;
    if (hprof_hash_lookup(&hprof_class_table, &class_tmp) != NULL) {
        fprintf(stderr, "HPROF ERROR: class ID already in use\n");
	goto classload_done;
    }
    result = hprof_hash_put(&hprof_class_table, &class_tmp);
    
    result->super = NULL;
    result->num_methods = num_methods;
    result->num_interfaces = num_interfaces;
    result->num_statics = num_statics;
    result->num_instances = num_instances;
    result->name = hprof_intern_name(name);
    result->src_name = hprof_intern_name(src_name);
    result->serial_num = class_serial_number++;
    if (num_methods)
      result->methods = HPROF_CALLOC(ALLOC_TYPE_ARRAY,
	  sizeof(hprof_method_t *) * num_methods);
    else
	result->methods = NULL;

    /* intern methods in the class */
    for (i = 0; i < num_methods; i++) {
	result->methods[i] = hprof_intern_method(&methods[i], result);
    }
    if (num_statics) {
	hstatics =
	    HPROF_CALLOC(ALLOC_TYPE_FIELD, num_statics * sizeof(hprof_field_t));
	for (i = 0; i < num_statics; i++) {
	    hstatics[i].name = hprof_intern_name(statics[i].field_name);
	    hstatics[i].type = sigToTy(statics[i].field_signature[0]);
        }
    } else {
	hstatics = NULL;
    }

    result->statics = hstatics;

    if (num_instances) {
	hinstances = HPROF_CALLOC(ALLOC_TYPE_FIELD,
	    num_instances * sizeof(hprof_field_t));
	for (i = 0; i < num_instances; i++) {
	    hinstances[i].name = hprof_intern_name(instances[i].field_name);
	    hinstances[i].type = sigToTy(instances[i].field_signature[0]);
	}
    } else {
	hinstances = NULL;
    }
    result->instances = hinstances;

    if (java_lang_object_class == NULL && strcmp(name, "java/lang/Object") == 0) {
        java_lang_object_class = result;
    }
    
    if (output_format == 'b') {
        int trace_num;
	if (requested) {
	    trace_num = 0;
	} else {
	    hprof_trace_t *htrace = hprof_get_trace(env_id, max_trace_depth);
	    if (htrace == NULL) {
	        fprintf(stderr, "HPROF ERROR : got a NULL trace in class_load\n");
		goto classload_done;
	    }
	    trace_num = htrace->serial_num;
	}
	hprof_write_header(HPROF_LOAD_CLASS, 2 * sizeof(void *) + 8);
	hprof_write_u4(result->serial_num);
	hprof_write_id(objmap);
	hprof_write_u4(trace_num);
	hprof_write_id(result->name);
    }
 classload_done:
    CALL(RawMonitorExit)(data_access_lock);
}

void hprof_class_unload_event(JNIEnv *env_id, 
			      jobjectID class_id)
{
  hprof_class_t *class;
  int i;
  CALL(RawMonitorEnter)(data_access_lock); 
  class = hprof_lookup_class(class_id);
  if (class == NULL) {
      fprintf(stderr, "HPROF ERROR : unknown class ID in class_unload\n");
  } else {
      if (output_format == 'b') {
	  hprof_write_header(HPROF_UNLOAD_CLASS, 4);
	  hprof_write_u4(class->serial_num);
      }
      /* resolve the methodIDs in the thread local tables, as they 
       * may have become invalid now.
       */
      hprof_bill_all_thread_local_tables();
      /* invalidate this entry */
      class->class_id = (hprof_objmap_t *)-1; 
      /* invalidate all the method entries defined in this class */
      for (i = 0; i < class->num_methods; i++) {
	  class->methods[i]->method_id = (jmethodID)-1; 
      }
  }
  CALL(RawMonitorExit)(data_access_lock);
}

void hprof_superclass_link(jobjectID class_id,
			   jobjectID super_id)
{
  hprof_class_t *class;
  hprof_class_t *super;

  CALL(RawMonitorEnter)(data_access_lock); 
  class = hprof_lookup_class(class_id);
  if (class == NULL) {
      fprintf(stderr, "HPROF ERROR: unknown class ID in superclass_link\n");
  } else {
      if (super_id == NULL) {
	  super = NULL;
      } else {
	  super = hprof_lookup_class(super_id);
	  if (super == NULL) {
	      fprintf(stderr, 
		      "HPROF ERROR: unknown superclass ID in superclass_link\n");
	  }
      }
      class->super = super;
  }
  CALL(RawMonitorExit)(data_access_lock);
}

#ifdef HASH_STATS
void hprof_print_class_hash_stats(FILE *fp) {
    hprof_print_tbl_hash_stats(fp, &hprof_class_table);
}
#endif /* HASH_STATS */


#ifdef WATCH_ALLOCS
static void * hprof_free_class_table_helper(void *_class, void *_arg)
{
    hprof_class_t *class = _class;
    if (class->statics != NULL)   hprof_free(class->statics);
    if (class->instances != NULL) hprof_free(class->instances);
    if (class->methods != NULL)   hprof_free(class->methods);
    return _arg;
}


void hprof_free_class_table(void)
{
    hprof_hash_iterate(&hprof_class_table, hprof_free_class_table_helper, NULL);
    hprof_hash_removeall(&hprof_class_table);
    hprof_hash_free(&hprof_class_table);
}
#endif /* WATCH_ALLOCS */
