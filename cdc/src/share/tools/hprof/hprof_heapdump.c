/*
 * @(#)hprof_heapdump.c	1.17 06/10/10
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

#include "javavm/include/porting/ansi/string.h"

#include "jvmpi.h"
#include "hprof.h"

void hprof_get_heap_dump(void) 
{
    if (CALL(RequestEvent)(JVMPI_EVENT_HEAP_DUMP, NULL) != JVMPI_SUCCESS) {
        fprintf(stderr, "HPROF ERROR: heap dump from JVM did not succeed\n");
    }
}

#ifdef HPROF_HEAPDUMP_DEBUG
static void
hprof_print_tag(int tag){
    char *t;
    switch(tag){
    case HPROF_GC_ROOT_MONITOR_USED:
	t = "busy monitor"; break;
    case HPROF_GC_ROOT_STICKY_CLASS:
	t = "sticky class"; break;
    case HPROF_GC_ROOT_UNKNOWN:
	t = "unknown root"; break;
    case HPROF_GC_ROOT_JNI_GLOBAL:
	t = "jni global"; break;
    case HPROF_GC_ROOT_THREAD_BLOCK:
	t = "thread block"; break;
    case HPROF_GC_ROOT_NATIVE_STACK:
	t = "native stack"; break;
    case HPROF_GC_ROOT_JAVA_FRAME:
	t = "java frame"; break;
    case HPROF_GC_ROOT_JNI_LOCAL:
	t = "jni local"; break;
    case HPROF_GC_CLASS_DUMP:
	t = "class object"; break;
    case HPROF_GC_INSTANCE_DUMP:
	t = "instance"; break;
    case HPROF_GC_OBJ_ARRAY_DUMP:
	t = "array of object"; break;
    case HPROF_GC_PRIM_ARRAY_DUMP:
	t = "array of scalar"; break;
    default:
	t = "GARBAGE"; break;
    }
    fprintf(stderr,"Heap dump object %s\n", t);
}
#endif /* HPROF_HEAPDUMP_DEBUG */

/* this function is used to get the size of the heap dump
   record output by hprof if the output format is binary */
static int hprof_get_dump_size(char *begin_roots, 
			       char *end_objects)
{
    int size = 0;
    hprof_dump_seek(begin_roots);
    while (hprof_dump_cur() < end_objects) {
        unsigned char tag;
	int in;
	int out;
	int sizeof_u1 = sizeof(unsigned char);
	int sizeof_u2 = sizeof(unsigned short);
	int sizeof_u4 = sizeof(unsigned int);
	int sizeof_id = sizeof(void *);
	tag = hprof_dump_read_u1();
	size += sizeof_u1;
#ifdef HPROF_HEAPDUMP_DEBUG
	hprof_print_tag(tag);
#endif /* HPROF_HEAPDUMP_DEBUG */
	switch (tag) {
	case HPROF_GC_ROOT_MONITOR_USED:
	case HPROF_GC_ROOT_STICKY_CLASS:
	case HPROF_GC_ROOT_UNKNOWN: {
	    in = sizeof_id;
	    out = sizeof_id;
	    size += out;
	    hprof_dump_seek(hprof_dump_cur() + in);
	    break;
	}
	case HPROF_GC_ROOT_JNI_GLOBAL: {
	    in = 2 * sizeof_id;
	    out = 2 * sizeof_id;
	    size += out;
	    hprof_dump_seek(hprof_dump_cur() + in);
	    break;
	}
	case HPROF_GC_ROOT_THREAD_BLOCK:
	case HPROF_GC_ROOT_NATIVE_STACK: {
	    in = 2 * sizeof_id;
	    out = sizeof_id + sizeof_u4;
	    size += out;
	    hprof_dump_seek(hprof_dump_cur() + in);
	    break;
	}
	case HPROF_GC_ROOT_JAVA_FRAME:
	case HPROF_GC_ROOT_JNI_LOCAL: {
	    in = 2 * sizeof_id + sizeof_u4;
	    out = sizeof_id + 2 * sizeof_u4;
	    size += out;
	    hprof_dump_seek(hprof_dump_cur() + in);
	    break;
	}
	case HPROF_GC_CLASS_DUMP: {
	    int n;
	    int i;

	    jobjectID class_id = hprof_dump_read_id();
	    hprof_class_t *class = hprof_lookup_class(class_id);
	    if (class == NULL) {
	        fprintf(stderr, "HPROF ERROR: unknown class_id in get_dump_size %p\n",
			class_id);
	        return -1;
	    }

	    in = 6 * sizeof_id + sizeof_u4;
	    out = 7 * sizeof_id + 2 * sizeof_u4;
	    size += out;
	    hprof_dump_seek(hprof_dump_cur() + in);

	    in = class->num_interfaces * sizeof_id;
	    hprof_dump_seek(hprof_dump_cur() + in);

	    n = hprof_dump_read_u2();
	    size += sizeof_u2;
	    in = n*(sizeof_u2 + sizeof_u1 + sizeof_id);
	    out = in;
	    size += out;
	    hprof_dump_seek(hprof_dump_cur() + in);

	    n = class->num_statics;
	    size += sizeof_u2;
	    for (i = 0; i < n; i++) {
	        jint ty = class->statics[i].type;
		size += sizeof_id + sizeof_u1;		  
		if (ty == JVMPI_LONG || ty == JVMPI_DOUBLE) {
		    in = sizeof(jvalue);
		    out = in;
		    size += out;
		    hprof_dump_seek(hprof_dump_cur() + in);
		} else if (ty == JVMPI_CLASS) {
		    in = sizeof_id;
		    out = sizeof_id;
		    size += out;
		    hprof_dump_seek(hprof_dump_cur() + in);
		} else if (ty == JVMPI_INT || ty == JVMPI_FLOAT) {
		    in = sizeof_u4;
		    out = sizeof_u4;
		    size += out;
		    hprof_dump_seek(hprof_dump_cur() + in);
		} else if (ty == JVMPI_CHAR || ty == JVMPI_SHORT) {
		    in = sizeof_u2;
		    out = sizeof_u2;
		    size += out;
		    hprof_dump_seek(hprof_dump_cur() + in);
		} else if (ty == JVMPI_BYTE || ty == JVMPI_BOOLEAN) {
		    in = sizeof_u1;
		    out = sizeof_u1;
		    size += out;
		    hprof_dump_seek(hprof_dump_cur() + in);
		}
	    }
	
	    size += sizeof_u2 + 
	        class->num_instances*(sizeof_id + sizeof_u1);
	    break;
	}
	case HPROF_GC_INSTANCE_DUMP: {
	    unsigned int valbytes;

	    in = 2 * sizeof_id;
	    out = 2 * sizeof_id + sizeof_u4;
	    size += out;
	    hprof_dump_seek(hprof_dump_cur() + in);

	    valbytes = hprof_dump_read_u4();
	    size += sizeof_u4;

	    size += valbytes;
	    hprof_dump_seek(hprof_dump_cur() + valbytes);
	    break;
	}
	case HPROF_GC_OBJ_ARRAY_DUMP: {
	    unsigned int num_elements;
	        
	    in = sizeof_id;
	    out = sizeof_id + sizeof_u4;
	    size += out;
	    hprof_dump_seek(hprof_dump_cur() + in);
		
	    num_elements = hprof_dump_read_u4();
	    size += sizeof_u4;

	    in = sizeof_id + num_elements * sizeof_id;
	    out = in;
	    size += out;
	    hprof_dump_seek(hprof_dump_cur() + in);
	    break;
	}
	case HPROF_GC_PRIM_ARRAY_DUMP: {
	    unsigned int num_elements;
	    unsigned char ty;
		
	    in = sizeof_id;
	    out = sizeof_id + sizeof_u4;
	    size += out;
	    hprof_dump_seek(hprof_dump_cur() + in);
		
	    num_elements = hprof_dump_read_u4();
	    size += sizeof_u4;
	    ty = hprof_dump_read_u1();
	    size += sizeof_u1;
		
	    switch (ty) {
	    case JVMPI_BYTE:
	    case JVMPI_BOOLEAN:
	        in = num_elements * sizeof_u1;
		out = in;
		size += out;
		hprof_dump_seek(hprof_dump_cur() + in);
		break;
	    case JVMPI_SHORT:
	    case JVMPI_CHAR:
	        in = num_elements * sizeof_u2;
		out = in;
		size += out;
		hprof_dump_seek(hprof_dump_cur() + in);
		break;
	    case JVMPI_FLOAT:
	    case JVMPI_INT:
	        in = num_elements * sizeof_u4;
		out = in;
		size += out;
		hprof_dump_seek(hprof_dump_cur() + in);
		break;
	    case JVMPI_DOUBLE:
	    case JVMPI_LONG:
	        in = num_elements * 2 * sizeof_u4;
		out = in;
		size += out;
		hprof_dump_seek(hprof_dump_cur() + in);
		break;
	    }
	    break;
	  }
	}
    }
    return size;
}

/* this function processes the heap dump buffer and outputs the 
   second part of the heap dump record */
static void hprof_process_dump_buffer(char *begin_roots,
				      char *end_objects)
{
    int i;
    
    hprof_dump_seek(begin_roots);
    while (hprof_dump_cur() < end_objects) {
        unsigned char tag;
	tag = hprof_dump_read_u1();
	if (output_format == 'b') {
	    hprof_write_u1(tag);
	}
	switch (tag) {
	case HPROF_GC_ROOT_UNKNOWN: {
	    void *obj_id = hprof_dump_read_id();
	    hprof_objmap_t *objmap = hprof_fetch_object_info(obj_id);
	    if (output_format == 'b') {
	        hprof_write_id(objmap);
	    } else {
	        hprof_printf("ROOT %x (kind=<unknown>)\n", objmap);
	    }
	    break;
	}
	case HPROF_GC_ROOT_JNI_GLOBAL: {
	    void *obj_id;
	    void *ref_id;
	    hprof_globalref_t *gref;
	    unsigned int trace_serial_num;

	    hprof_dump_read_id(); /* skip object id, we can get it from gref */
	    ref_id = hprof_dump_read_id();
	    gref = hprof_globalref_find(ref_id);
	    
	    if (gref == NULL) {
	        obj_id = NULL;
		trace_serial_num = 0;
	    } else {
	        obj_id = gref->obj_id; /* already objmapped */
		trace_serial_num = gref->trace_serial_num;
	    }
	        
	    if (output_format == 'b') {
	        hprof_write_id(obj_id);
		hprof_write_id(ref_id);
	    } else {
	        hprof_printf("ROOT %x (kind=<JNI global ref>, "
			     "id=%x, trace=%u)\n",
			     obj_id, ref_id, trace_serial_num);
	    }
	    break;
	}
	case HPROF_GC_ROOT_NATIVE_STACK: {
	    void *h = hprof_dump_read_id();
	    hprof_objmap_t *objmap = hprof_fetch_object_info(h);
	    void *env = hprof_dump_read_id();
	    hprof_thread_t *thread = hprof_lookup_thread(env);
	    unsigned int thread_serial_num;
	    void *tid;
	    
	    if (thread == NULL) {
	        thread_serial_num = 0;
		tid = NULL;
	    } else {
	        thread_serial_num = thread->serial_num;
		tid = thread->thread_id; /* already objmapped */
	    }
	    
	    if (output_format == 'b') {
	        hprof_write_id(objmap);
		hprof_write_u4(thread_serial_num);
	    } else {
	        hprof_printf("ROOT %x (kind=<native stack>, thread=%u)\n",
			     objmap, thread_serial_num, tid);
	    }
	    break;
	}
	case HPROF_GC_ROOT_STICKY_CLASS: {
	    void *class_id = hprof_dump_read_id();
	    hprof_objmap_t *objmap = hprof_fetch_object_info(class_id);
	   
	    if (output_format == 'b') {
	        hprof_write_id(objmap);
	    } else {
		hprof_class_t *class = hprof_lookup_class_objmap(objmap);
                const char *class_name = (class == NULL) ? "<Unknown>" : class->name->name;
		
		hprof_printf("ROOT %x (kind=<system class>, name=%s)\n",
			     objmap, class_name);
	    }
	    break;
	}
	case HPROF_GC_ROOT_THREAD_BLOCK: {
	    void *h = hprof_dump_read_id();
	    hprof_objmap_t *objmap = hprof_fetch_object_info(h);
	    void *env = hprof_dump_read_id();
	    hprof_thread_t *thread = hprof_lookup_thread(env);
	    unsigned int thread_serial_num = 
	        (thread == NULL) ? 0 : thread->serial_num;
	    if (output_format == 'b') {
	        hprof_write_id(objmap);
		hprof_write_u4(thread_serial_num);
	    } else {
	        hprof_printf("ROOT %x (kind=<thread block>, thread=%u)\n",
			     objmap, thread_serial_num);
	    }
	    break;
	}
	case HPROF_GC_ROOT_MONITOR_USED: {
	    void *h = hprof_dump_read_id();
	    hprof_objmap_t *objmap = hprof_fetch_object_info(h);
	    if (output_format == 'b') {
	        hprof_write_id(objmap);
	    } else {
	        hprof_printf("ROOT %x (kind=<busy monitor>)\n", objmap);
	    }
	    break;
	}
	case HPROF_GC_ROOT_JNI_LOCAL: {
	    void *h = hprof_dump_read_id();
	    hprof_objmap_t *objmap = hprof_fetch_object_info(h);
	    void *env = hprof_dump_read_id();
	    int depth = hprof_dump_read_u4();
	    hprof_thread_t *thread = hprof_lookup_thread(env);
	    unsigned int thread_serial_num = 
	        (thread == NULL) ? 0 : thread->serial_num;
	    if (output_format == 'b') {
	        hprof_write_id(objmap);
		hprof_write_u4(thread_serial_num);
		hprof_write_u4(depth);
	    } else {
	        hprof_printf("ROOT %x (kind=<JNI local ref>, "
			     "thread=%u, frame=%d)\n",
			     objmap, thread_serial_num, depth);
	    }
	    break;
	}
	case HPROF_GC_ROOT_JAVA_FRAME: {
	    void *h = hprof_dump_read_id();
	    hprof_objmap_t *objmap = hprof_fetch_object_info(h);
	    void *env = hprof_dump_read_id();
	    int depth = hprof_dump_read_u4();
	    hprof_thread_t *thread = hprof_lookup_thread(env);
	    unsigned int thread_serial_num = 
	        (thread == NULL) ? 0 : thread->serial_num;
	    if (output_format == 'b') {
	        hprof_write_id(objmap);
		hprof_write_u4(thread_serial_num);
		hprof_write_u4(depth);
	    } else {
	        hprof_printf("ROOT %x (kind=<Java stack>, "
			     "thread=%u, frame=%d)\n",
			     objmap, thread_serial_num, depth);
	    }
	    break;
	}
	case HPROF_GC_CLASS_DUMP: {
	    void *class_id;
	    hprof_objmap_t *class_map;
	    hprof_objmap_t *super_map;
	    hprof_objmap_t *loader_map;
	    hprof_objmap_t *signers_map;
	    hprof_objmap_t *domain_map;
	    unsigned int trace_serial_num;
	    hprof_class_t *class;
            const char *class_name;
	    void *resv1;
	    void *resv2;
	    unsigned int size;
	    unsigned short n;
	    int i;
	    
	    class_id = hprof_dump_read_id();
	    class_map = hprof_fetch_object_info(class_id);
	    class = hprof_lookup_class_objmap(class_map);
	    if (class == NULL) {
	        fprintf(stderr, "HPROF ERROR: unknown class_id in heap dump %p %p\n",
			class_id, class_map);
		return;
	    }

	    trace_serial_num = class_map->site->trace_serial_num;
	    class_name = class->name->name;
	    super_map = hprof_fetch_object_info(hprof_dump_read_id());
	    loader_map = hprof_fetch_object_info(hprof_dump_read_id());
	    signers_map = hprof_fetch_object_info(hprof_dump_read_id());
	    domain_map = hprof_fetch_object_info(hprof_dump_read_id());
	    resv1 = hprof_dump_read_id();  /* reserved */
	    resv2 = hprof_dump_read_id();  /* reserved */
	    size = hprof_dump_read_u4();
		
	    if (output_format == 'b') {
	        hprof_write_id(class_map);
		hprof_write_u4(trace_serial_num);
		hprof_write_id(super_map);
		hprof_write_id(loader_map);
		hprof_write_id(signers_map);
		hprof_write_id(domain_map);
		hprof_write_id(resv1);
		hprof_write_id(resv2);
		hprof_write_u4(size);
	    } else {
	        hprof_printf("CLS %x (name=%s, trace=%u)\n",
			     class_map, class_name, trace_serial_num);
		if (super_map) {
		    hprof_printf("\tsuper\t\t%x\n", super_map);
		}
		if (loader_map) {
		    hprof_printf("\tloader\t\t%x\n", loader_map);
		}
		if (signers_map) {
		    hprof_printf("\tsigners\t\t%x\n", signers_map);
		}
		if (domain_map) {
		    hprof_printf("\tdomain\t\t%x\n", domain_map);
		}
	    }

	    /* skip interfaces */
	    for (i = 0; i < class->num_interfaces; i++) {
	        hprof_dump_read_id();
	    }

	    n = hprof_dump_read_u2();
	    if (output_format == 'b') {
	        hprof_write_u2(n);
	    }
	    for (i = 0; i < n; i++) {
	        unsigned short index;
		unsigned char ty;
		void *p;

		index = hprof_dump_read_u2();
		ty = hprof_dump_read_u1();
		p = hprof_fetch_object_info(hprof_dump_read_id());
		if (output_format == 'b') {
		    hprof_write_u2(index);
		    hprof_write_u1(ty);
		    hprof_write_id(p);
		} else {
		    if (ty == JVMPI_CLASS) {
		        hprof_printf("\tconstant[%u]\t%x\n", index, p);
		    }
		}
	    }
	    n = class->num_statics;
	    if (output_format == 'b') {
	        hprof_write_u2(n);
	    }
	    for (i = 0; i < n; i++) {
	        jint ty = class->statics[i].type;
		hprof_name_t *name =  class->statics[i].name;
		if (output_format == 'b') {
		    hprof_write_id(name);
		    hprof_write_u1((unsigned char)ty);
		}
		if (ty == JVMPI_LONG || ty == JVMPI_DOUBLE) {
		    jvalue t;
		    hprof_dump_read(&t, sizeof(jvalue));
		    if (output_format == 'b') {
		        hprof_write_raw(&t, sizeof(jvalue));
		    }
		} else if (ty == JVMPI_CLASS) {
		    void *p = hprof_fetch_object_info(hprof_dump_read_id());
		    if (output_format == 'b') {
		        hprof_write_id(p);
		    } else {
		        if (p) {
			    hprof_printf("\tstatic %s\t%x\n", name->name, p);
			}
		    }
		} else if (ty == JVMPI_INT || ty == JVMPI_FLOAT) {
		    unsigned int u4 = hprof_dump_read_u4();
		    if (output_format == 'b') {
		        hprof_write_u4(u4);
		    }
		} else if (ty == JVMPI_CHAR || ty == JVMPI_SHORT) {
		    unsigned short u2 = hprof_dump_read_u2();
		    if (output_format == 'b') {
		        hprof_write_u2(u2);
		    }
		} else if (ty == JVMPI_BYTE || ty == JVMPI_BOOLEAN) {
		    unsigned char u1 = hprof_dump_read_u1();
		    if (output_format == 'b') {
		        hprof_write_u1(u1);
		    }
		}
	    }
		
	    n = class->num_instances;
	    if (output_format == 'b') {
	        hprof_write_u2(n);
	    }
	    for (i = 0; i < n; i++) {
	        jint ty = class->instances[i].type;
		hprof_name_t *name = class->instances[i].name;
		if (output_format == 'b') {
		    hprof_write_id(name);
		    hprof_write_u1((unsigned char)ty);
		}
	    }
	    break;
	}
	case HPROF_GC_INSTANCE_DUMP: {
	    void *obj_id = hprof_dump_read_id();
	    hprof_objmap_t *objmap = hprof_fetch_object_info(obj_id);
	    void *class_id = hprof_dump_read_id();
	    hprof_objmap_t *classmap = hprof_fetch_object_info(class_id);
	    unsigned int valbytes = hprof_dump_read_u4();
	    hprof_class_t *class;
            const char *class_name;
	    unsigned int trace_serial_num = 0;
	    int size = 0;
	    char *saved_offset;
	    
	    if (objmap != NULL) {
	        trace_serial_num = objmap->site->trace_serial_num;
		size = objmap->size;
	    }

	    class = hprof_lookup_class_objmap(classmap);
	    class_name = (class == NULL) ? "<Unknown>" : class->name->name;
		   
	    if (output_format == 'b') {
	        hprof_write_id(objmap);
		hprof_write_u4(trace_serial_num);
		hprof_write_id(classmap);
		hprof_write_u4(valbytes);
	    } else {
	        hprof_printf("OBJ %x (sz=%u, trace=%u, class=%s@%x)\n",
			     objmap, size, trace_serial_num, class_name, classmap);
	    }
	    saved_offset = hprof_dump_cur();

	    while (class != NULL) {
	        int i;
		int n_inst = class->num_instances;
		unsigned int val_u4;
		unsigned short val_u2;
		unsigned char val_u1;
		jvalue val_j;
		void *val_id;
		jint ty;

		for (i = 0; i < n_inst; i++) {
		    ty = class->instances[i].type;
		    
		    if (ty == JVMPI_LONG || ty == JVMPI_DOUBLE) {
		        hprof_dump_read(&val_j, sizeof(jvalue));
			
			if (output_format == 'b') {
			  hprof_write_raw(&val_j, sizeof(jvalue));
			}
		    } else if (ty == JVMPI_CLASS) {
		        val_id = hprof_fetch_object_info(hprof_dump_read_id());
			
			if (output_format == 'b') {
			    hprof_write_id(val_id);
			} else if (val_id) {
                            const char *field_name =  class->instances[i].name->name;
			    char *sep = strlen(field_name) < 8 ? "\t" : "";
			    hprof_printf("\t%s\t%s%x\n", field_name, sep, val_id);
			}
		    } else if (ty == JVMPI_INT || ty == JVMPI_FLOAT) {
		        val_u4 = hprof_dump_read_u4();
			
			if (output_format == 'b') {
			    hprof_write_u4(val_u4);
			}
		    } else if (ty == JVMPI_CHAR || ty == JVMPI_SHORT) {
		        val_u2 = hprof_dump_read_u2();

			if (output_format == 'b') {
			    hprof_write_u2(val_u2);
			}
		    } else if (ty == JVMPI_BYTE || ty == JVMPI_BOOLEAN) {
		        val_u1 = hprof_dump_read_u1();

			if (output_format == 'b') {
			    hprof_write_u1(val_u1);
			}
		    }
		}

		if (class->super == NULL && class != java_lang_object_class) {
		    CALL(RequestEvent)(JVMPI_EVENT_OBJECT_DUMP, 
				       class->class_id->obj_id);
		}

		class = class->super;
	    }
	    /* If the class is null for any reason then we must make sure
	     * that the buffer pointer is updated to point to the next
	     * record.  It shouldn't be null so we exit if so
	     */
	    if (hprof_dump_cur() != saved_offset + valbytes) {
	        fprintf(stderr,
			"HPROF ERROR: class info missing in heap dump.\n");
		return;
	    }
	    break;
	}
	case HPROF_GC_OBJ_ARRAY_DUMP: {
	    void *obj_id = hprof_dump_read_id();
	    hprof_objmap_t *objmap = hprof_fetch_object_info(obj_id);
	    unsigned int num_elements = hprof_dump_read_u4();
	    void *class_id = hprof_dump_read_id();
	    hprof_objmap_t *classmap = hprof_fetch_object_info(class_id);
	    hprof_class_t *class;
	    unsigned int trace_serial_num = 0;
	    int size = 0;
            const char *class_name;

	    if (objmap != NULL) {
	        trace_serial_num = objmap->site->trace_serial_num;
		size = objmap->size;
	    }
	    class = hprof_lookup_class_objmap(classmap);
	    class_name = (class == NULL) ? "<Unknown>" : class->name->name;

	    if (output_format == 'b') {
	        hprof_write_id(objmap);
		hprof_write_u4(trace_serial_num);
		hprof_write_u4(num_elements);
		hprof_write_id(classmap);
	    } else {
	        hprof_printf("ARR %x (sz=%u, trace=%u, nelems=%u, elem type=%s@%x)\n",
			     objmap, size, trace_serial_num, num_elements, 
			     class_name, classmap);
	    }
	    for (i = 0; i < num_elements; i++) {  
	        void *p = hprof_fetch_object_info(hprof_dump_read_id());
		if (output_format == 'b') {
		    hprof_write_id(p);
		} else {
		    if (p) {
		        hprof_printf("\t[%u]\t\t%x\n", i, p);
		    }
		}
	    }
	    break;
	}
	case HPROF_GC_PRIM_ARRAY_DUMP: {
	    void *obj_id = hprof_dump_read_id();
	    hprof_objmap_t *objmap = hprof_fetch_object_info(obj_id);
	    unsigned int num_elements = hprof_dump_read_u4();
	    unsigned char ty = hprof_dump_read_u1();
	    unsigned int trace_serial_num = 0;
	    int size = 0;
		
	    if (objmap != NULL) {
	        trace_serial_num = objmap->site->trace_serial_num;
		size = objmap->size;
	    }
	    if (output_format == 'b') {
	        hprof_write_id(objmap);
		hprof_write_u4(trace_serial_num);
		hprof_write_u4(num_elements);
		hprof_write_u1(ty);
	    } else {
	        hprof_printf("ARR %x (sz=%u, trace=%u, nelems=%u, elem type=",
			     objmap, size, trace_serial_num, num_elements);
	    }

	    switch (ty) {
	    case JVMPI_BOOLEAN:
	        if (output_format != 'b') {
		    hprof_printf("boolean)\n");
		}
		for (i = 0; i < num_elements; i++) {  
		    unsigned char u1 = hprof_dump_read_u1();
		    if (output_format == 'b') {
		        hprof_write_u1(u1);
		    }
		}
		break;
	    case JVMPI_BYTE:
	        if (output_format != 'b') {
		    hprof_printf("byte)\n");
		}
		for (i = 0; i < num_elements; i++) {  
		    unsigned char u1 = hprof_dump_read_u1();
		    if (output_format == 'b') {
		        hprof_write_u1(u1);
		    }
		}
		break;
	    case JVMPI_CHAR:
	        if (output_format != 'b') {
		    hprof_printf("char)\n");
		}
		for (i = 0; i < num_elements; i++) {  
		    unsigned short u2 = hprof_dump_read_u2();
		    if (output_format == 'b') {
		        hprof_write_u2(u2);
		    }
		}
		break;
	    case JVMPI_SHORT:
	        if (output_format != 'b') {
		    hprof_printf("short)\n");
		}
		for (i = 0; i < num_elements; i++) {  
		    unsigned short u2 = hprof_dump_read_u2();
		    if (output_format == 'b') {
		        hprof_write_u2(u2);
		    }
		}
		break;
	    case JVMPI_INT:
	        if (output_format != 'b') {
		    hprof_printf("int)\n");
		}
		for (i = 0; i < num_elements; i++) {  
		    unsigned int u4 = hprof_dump_read_u4();
		    if (output_format == 'b') {
		        hprof_write_u4(u4);
		    }
		}
		break;
	    case JVMPI_LONG:
	        if (output_format != 'b') {
		    hprof_printf("long)\n");
		}
		for (i = 0; i < num_elements; i++) {  
		    unsigned int u41 = hprof_dump_read_u4();
		    unsigned int u42 = hprof_dump_read_u4();
		    if (output_format == 'b') {
		        hprof_write_u4(u41);
			hprof_write_u4(u42);
		    }
		}
		break;
	    case JVMPI_FLOAT:
	        if (output_format != 'b') {
		    hprof_printf("float)\n");
		}
		for (i = 0; i < num_elements; i++) {  
		    unsigned int u4 = hprof_dump_read_u4();
		    if (output_format == 'b') {
		        hprof_write_u4(u4);
		    }
		}
		break;
	    case JVMPI_DOUBLE:
	        if (output_format != 'b') {
		    hprof_printf("long)\n");
		}
		for (i = 0; i < num_elements; i++) {  
		    unsigned int u41 = hprof_dump_read_u4();
		    unsigned int u42 = hprof_dump_read_u4();
		    if (output_format == 'b') {
		        hprof_write_u4(u41);
			hprof_write_u4(u42);
		    }
		}
		break;
	    }
	    break;
	}
	}
    }
} 

void hprof_heap_dump_event(char *begin_roots,
			   char *end_objects,
			   int num_traces,
			   JVMPI_CallTrace *traces)
{
    int i;
    int dump_size;
    hprof_trace_t **htraces =
        HPROF_CALLOC(ALLOC_TYPE_ARRAY, sizeof(hprof_trace_t *)*num_traces);
    
    CALL(RawMonitorEnter)(data_access_lock);
    
    /* intern the jvmpi traces */
    for (i = 0; i < num_traces; i++) {
        JVMPI_CallTrace *jtrace = &(traces[i]);
        htraces[i] = hprof_intern_jvmpi_trace(jtrace->frames, 
					      jtrace->num_frames,
					      jtrace->env_id);
	if (htraces[i] == NULL) {
	    fprintf(stderr, "HPROF ERROR: got a NULL trace in heap_dump\n");
	    goto heap_dump_done;
	}
    }
    
    /* First write all trace we might refer to. */
    hprof_output_unmarked_traces();
    
    /* get dump size, we need the dump size only for the binary
     * format, but we do it always to check for any errors */
    dump_size = hprof_get_dump_size(begin_roots, end_objects);
    if (dump_size < 0) {
        fprintf(stderr, "HPROF ERROR: heap dump size < 0\n");
	goto heap_dump_done;
    }

    /* print header */
    if (output_format == 'b') {
        int threads_size = num_traces * (1 + sizeof(void *) + 8);
	/* Take into account the size for HPROF_GC_ROOT_THREAD_OBJ,
	 *  in addition to other roots and objects.
	 */
	hprof_write_header(HPROF_HEAP_DUMP, dump_size + threads_size);
    } else {
        time_t t = time(0);
	hprof_printf("HEAP DUMP BEGIN (%u objects, %u bytes) %s",
		     total_live_instances,
		     total_live_bytes,
		     ctime(&t));
    }
        
    /* Process the root threads first */
    for (i = 0; i < num_traces; i++) {
        hprof_thread_t *thread = hprof_lookup_thread(traces[i].env_id);
	hprof_objmap_t *thread_id;
	unsigned int thread_serial_num;

	if (thread == NULL) {
	    thread_id = NULL;
	    thread_serial_num = 0;
	} else {
	    thread_id = thread->thread_id;
	    thread_serial_num = thread->serial_num;
	}
	
	if (output_format == 'b') {
	     hprof_write_u1(HPROF_GC_ROOT_THREAD_OBJ);
	     hprof_write_id(thread_id);
	     hprof_write_u4(thread_serial_num);
	     hprof_write_u4(htraces[i]->serial_num);
	} else {
	    hprof_printf("ROOT %x (kind=<thread>, id=%u, trace=%u)\n",
			 thread_id,
			 thread_serial_num,
			 htraces[i]->serial_num);
	}
    }
    hprof_free(htraces);

    /* process the heap dump buffer */
    hprof_process_dump_buffer(begin_roots, end_objects);
    
    if (output_format != 'b') {
        hprof_printf("HEAP DUMP END\n");
    }

 heap_dump_done:
      CALL(RawMonitorExit)(data_access_lock);
}

void hprof_object_dump_event(char *data)
{
    char *ptr_bak = hprof_dump_cur();
    int kind;
    hprof_dump_seek(data);
    kind = hprof_dump_read_u1();
    if (kind == JVMPI_GC_CLASS_DUMP) {
        jobjectID self = hprof_dump_read_id();
        jobjectID super = hprof_dump_read_id();
	hprof_superclass_link(self, super);
    }
    hprof_dump_seek(ptr_bak);
}
