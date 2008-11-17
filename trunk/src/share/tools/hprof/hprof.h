/*
 * @(#)hprof.h	1.13 06/10/10
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

#ifndef _HPROF_H_
#define _HPROF_H_

#include "hprof_md.h"
#include "jni.h"
#include "jvmpi.h"
#include "jlong.h"

#include "hprof_global.h"
#include "hprof_setup.h"
#include "hprof_trace.h"
#include "hprof_thread.h"
#include "hprof_heapdump.h"
#include "hprof_site.h"
#include "hprof_io.h"
#include "hprof_listener.h"
#include "hprof_cpu.h"
#include "hprof_monitor.h"
#include "hprof_class.h"
#include "hprof_method.h"
#include "hprof_jni.h"
#include "hprof_object.h"
#include "hprof_name.h"
#include "hprof_sys.h"
#include "hprof_gc.h"

/*
 * -Xhprof binary format: (result either written to a file or sent over
 * the network).
 * 
 * WARNING: This format is still under development, and is subject to
 * change without notice.
 *
 *  header    "JAVA PROFILE 1.0.1" (0-terminated)
 *  u4        size of identifiers. Identifiers are used to represent
 *            UTF8 strings, objects, stack traces, etc. They usually
 *            have the same size as host pointers. For example, on
 *            Solaris and Win32, the size is 4.
 * u4         high word 
 * u4         low word    number of milliseconds since 0:00 GMT, 1/1/70
 * [record]*  a sequence of records.
 */

/*
 * Record format:
 *
 * u1         a TAG denoting the type of the record
 * u4         number of *microseconds* since the time stamp in the
 *            header. (wraps around in a little more than an hour)
 * u4         number of bytes *remaining* in the record. Note that
 *            this number excludes the tag and the length field itself.
 * [u1]*      BODY of the record (a sequence of bytes)
 */

/*
 * The following TAGs are supported:
 *
 * TAG           BODY       notes
 *----------------------------------------------------------
 * HPROF_UTF8               a UTF8-encoded name  
 *
 *               id         name ID
 *               [u1]*      UTF8 characters (no trailing zero)
 *
 * HPROF_LOAD_CLASS         a newly loaded class
 *
 *                u4        class serial number (> 0)
 *                id        class object ID
 *                u4        stack trace serial number
 *                id        class name ID
 *
 * HPROF_UNLOAD_CLASS       an unloading class
 *
 *                u4        class serial_number
 *
 * HPROF_FRAME              a Java stack frame
 *
 *                id        stack frame ID
 *                id        method name ID
 *                id        method signature ID
 *                id        source file name ID
 *                u4        class serial number
 *                i4        line number. >0: normal
 *                                       -1: unknown
 *                                       -2: compiled method
 *                                       -3: native method
 *
 * HPROF_TRACE              a Java stack trace
 *
 *               u4         stack trace serial number
 *               u4         thread serial number
 *               u4         number of frames
 *               [id]*      stack frame IDs
 *
 *
 * HPROF_ALLOC_SITES        a set of heap allocation sites, obtained after GC
 *
 *               u2         flags 0x0001: incremental vs. complete
 *                                0x0002: sorted by allocation vs. live
 *                                0x0004: whether to force a GC
 *               u4         cutoff ratio
 *               u4         total live bytes
 *               u4         total live instances
 *               u8         total bytes allocated
 *               u8         total instances allocated
 *               u4         number of sites that follow
 *               [u1        is_array: 0:  normal object
 *                                    2:  object array
 *                                    4:  boolean array
 *                                    5:  char array
 *                                    6:  float array
 *                                    7:  double array
 *                                    8:  byte array
 *                                    9:  short array
 *                                    10: int array
 *                                    11: long array
 *                u4        class serial number (may be zero during startup)
 *                u4        stack trace serial number
 *                u4        number of bytes alive
 *                u4        number of instances alive
 *                u4        number of bytes allocated
 *                u4]*      number of instance allocated
 *
 * HPROF_START_THREAD       a newly started thread.
 *
 *               u4         thread serial number (> 0)
 *               id         thread object ID
 *               u4         stack trace serial number
 *               id         thread name ID
 *               id         thread group name ID
 *               id         thread group parent name ID
 *
 * HPROF_END_THREAD         a terminating thread. 
 *
 *               u4         thread serial number
 *
 * HPROF_HEAP_SUMMARY       heap summary
 *
 *               u4         total live bytes
 *               u4         total live instances
 *               u8         total bytes allocated
 *               u8         total instances allocated
 *
 * HPROF_HEAP_DUMP          denote a heap dump
 *
 *               [heap dump sub-records]*
 *
 *                          There are four kinds of heap dump sub-records:
 *
 *               u1         sub-record type
 *
 *               HPROF_GC_ROOT_UNKNOWN         unknown root
 *
 *                          id         object ID
 *
 *               HPROF_GC_ROOT_THREAD_OBJ      thread object
 *
 *                          id         thread object ID  (may be 0 for a
 *                                     thread newly attached through JNI)
 *                          u4         thread sequence number
 *                          u4         stack trace sequence number
 *
 *               HPROF_GC_ROOT_JNI_GLOBAL      JNI global ref root
 *
 *                          id         object ID
 *                          id         JNI global ref ID
 *
 *               HPROF_GC_ROOT_JNI_LOCAL       JNI local ref
 *
 *                          id         object ID
 *                          u4         thread serial number
 *                          u4         frame # in stack trace (-1 for empty)
 *
 *               HPROF_GC_ROOT_JAVA_FRAME      Java stack frame
 *
 *                          id         object ID
 *                          u4         thread serial number
 *                          u4         frame # in stack trace (-1 for empty)
 *
 *               HPROF_GC_ROOT_NATIVE_STACK    Native stack
 *
 *                          id         object ID
 *                          u4         thread serial number
 *
 *               HPROF_GC_ROOT_STICKY_CLASS    System class
 *
 *                          id         object ID
 *
 *               HPROF_GC_ROOT_THREAD_BLOCK    Reference from thread block
 *
 *                          id         object ID
 *                          u4         thread serial number
 *
 *               HPROF_GC_ROOT_MONITOR_USED    Busy monitor
 *
 *                          id         object ID
 *
 *               HPROF_GC_CLASS_DUMP           dump of a class object
 *
 *                          id         class object ID
 *                          u4         stack trace serial number
 *                          id         super class object ID
 *                          id         class loader object ID
 *                          id         signers object ID
 *                          id         protection domain object ID
 *                          id         reserved
 *                          id         reserved
 *
 *                          u4         instance size (in bytes)
 *
 *                          u2         size of constant pool
 *                          [u2,       constant pool index,
 *                           ty,       type 
 *                                     2:  object
 *                                     4:  boolean
 *                                     5:  char
 *                                     6:  float
 *                                     7:  double
 *                                     8:  byte
 *                                     9:  short
 *                                     10: int
 *                                     11: long
 *                           vl]*      and value
 *
 *                          u2         number of static fields
 *                          [id,       static field name,
 *                           ty,       type,
 *                           vl]*      and value
 *
 *                          u2         number of inst. fields (not inc. super)
 *                          [id,       instance field name,
 *                           ty]*      type
 *
 *               HPROF_GC_INSTANCE_DUMP        dump of a normal object
 *
 *                          id         object ID
 *                          u4         stack trace serial number
 *                          id         class object ID
 *                          u4         number of bytes that follow
 *                          [vl]*      instance field values (class, followed
 *                                     by super, super's super ...)
 *
 *               HPROF_GC_OBJ_ARRAY_DUMP       dump of an object array
 *
 *                          id         array object ID
 *                          u4         stack trace serial number
 *                          u4         number of elements
 *                          id         element class ID
 *                          [id]*      elements
 *
 *               HPROF_GC_PRIM_ARRAY_DUMP      dump of a primitive array
 *
 *                          id         array object ID
 *                          u4         stack trace serial number
 *                          u4         number of elements
 *                          u1         element type
 *                                     4:  boolean array
 *                                     5:  char array
 *                                     6:  float array
 *                                     7:  double array
 *                                     8:  byte array
 *                                     9:  short array
 *                                     10: int array
 *                                     11: long array
 *                          [u1]*      elements
 *
 * HPROF_CPU_SAMPLES        a set of sample traces of running threads
 *
 *                u4        total number of samples
 *                u4        # of traces
 *               [u4        # of samples
 *                u4]*      stack trace serial number
 *
 * HPROF_CONTROL_SETTINGS   the settings of on/off switches
 *
 *                u4        0x00000001: alloc traces on/off
 *                          0x00000002: cpu sampling on/off
 *                u2        stack trace depth
 *
 */

#define HPROF_UTF8                    0x01
#define HPROF_LOAD_CLASS              0x02
#define HPROF_UNLOAD_CLASS            0x03
#define HPROF_FRAME                   0x04
#define HPROF_TRACE                   0x05
#define HPROF_ALLOC_SITES             0x06
#define HPROF_HEAP_SUMMARY            0x07
#define HPROF_START_THREAD            0x0a
#define HPROF_END_THREAD              0x0b
#define HPROF_HEAP_DUMP               0x0c
#define HPROF_CPU_SAMPLES             0x0d
#define HPROF_CONTROL_SETTINGS        0x0e
#define HPROF_LOCKSTATS_WAIT_TIME     0x10
#define HPROF_LOCKSTATS_HOLD_TIME     0x11

/* 
 * Heap dump constants
 */
#define HPROF_GC_ROOT_UNKNOWN       0xff
#define HPROF_GC_ROOT_JNI_GLOBAL    0x01
#define HPROF_GC_ROOT_JNI_LOCAL     0x02
#define HPROF_GC_ROOT_JAVA_FRAME    0x03
#define HPROF_GC_ROOT_NATIVE_STACK  0x04
#define HPROF_GC_ROOT_STICKY_CLASS  0x05
#define HPROF_GC_ROOT_THREAD_BLOCK  0x06
#define HPROF_GC_ROOT_MONITOR_USED  0x07
#define HPROF_GC_ROOT_THREAD_OBJ    0x08

#define HPROF_GC_CLASS_DUMP         0x20
#define HPROF_GC_INSTANCE_DUMP      0x21 
#define HPROF_GC_OBJ_ARRAY_DUMP     0x22
#define HPROF_GC_PRIM_ARRAY_DUMP    0x23

/* When the VM is collected via a socket to the profiling client, the
 * client may send the VM a set of commands. The commands have the 
 * following format:
 *
 * u1         a TAG denoting the type of the record
 * u4         a serial number
 * u4         number of bytes *remaining* in the record. Note that
 *            this number excludes the tag and the length field itself.
 * [u1]*      BODY of the record (a sequence of bytes)
 */

/* The following commands are presently supported:
 *
 * TAG           BODY       notes
 * ----------------------------------------------------------
 * HPROF_CMD_GC             force a GC.
 *
 * HPROF_CMD_DUMP_HEAP      obtain a heap dump
 *
 * HPROF_CMD_ALLOC_SITES    obtain allocation sites
 *
 *               u2         flags 0x0001: incremental vs. complete
 *                                0x0002: sorted by allocation vs. live
 *                                0x0004: whether to force a GC
 *               u4         cutoff ratio (0.0 ~ 1.0)
 *
 * HPROF_CMD_HEAP_SUMMARY   obtain heap summary
 *
 * HPROF_CMD_DUMP_TRACES    obtain all newly created traces
 *
 * HPROF_CMD_CPU_SAMPLES    obtain a HPROF_CPU_SAMPLES record
 *
 *               u2         ignored for now
 *               u4         cutoff ratio (0.0 ~ 1.0)
 *
 * HPROF_CMD_CONTROL        changing settings
 *
 *               u2         0x0001: alloc traces on
 *                          0x0002: alloc traces off
 *
 *                          0x0003: CPU sampling on
 *
 *                                  id:   thread object id (NULL for all)
 *
 *                          0x0004: CPU sampling off
 *
 *                                  id:   thread object id (NULL for all)
 *
 *                          0x0005: CPU sampling clear
 *
 *                          0x0006: clear alloc sites info
 *
 *                          0x0007: set max stack depth in CPU samples
 *                                  and alloc traces
 *
 *                                  u2:   new depth
 */

#define HPROF_CMD_GC           0x01
#define HPROF_CMD_DUMP_HEAP    0x02
#define HPROF_CMD_ALLOC_SITES  0x03
#define HPROF_CMD_HEAP_SUMMARY 0x04
#define HPROF_CMD_EXIT         0x05
#define HPROF_CMD_DUMP_TRACES  0x06
#define HPROF_CMD_CPU_SAMPLES  0x07
#define HPROF_CMD_CONTROL      0x08


/* flags used in output of allocation sites */
#define HPROF_SITE_DUMP_INCREMENTAL 0x01
#define HPROF_SITE_SORT_BY_ALLOC    0x02
#define HPROF_SITE_FORCE_GC         0x04

#endif /*HPROF_H_*/
