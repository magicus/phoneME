/*
 * @(#)jvmtiCapabilities.c	1.4 06/10/27
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.  
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

#include "javavm/include/porting/ansi/stdarg.h"
#include "javavm/include/defs.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/globalroots.h"
#include "javavm/include/localroots.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/basictypes.h"
#include "javavm/include/signature.h"
#include "javavm/include/globals.h"
#include "javavm/include/bag.h"
#include "javavm/include/porting/time.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/named_sys_monitor.h"
#include "generated/javavm/include/opcodes.h"
#include "generated/offsets/java_lang_Thread.h"
#include "generated/jni/java_lang_reflect_Modifier.h"
#include "javavm/export/jvm.h"
#include "javavm/export/jni.h"
#include "javavm/export/jvmti.h"
#include "javavm/include/jvmtiEnv.h"


static jint CAPA_SIZE = (JVMTI_INTERNAL_CAPABILITY_COUNT + 7) / 8; 

/* capabilities which are always potentially available */
jvmtiCapabilities always_capabilities;

/* capabilities which are potentially available during OnLoad */
jvmtiCapabilities onload_capabilities;

/* capabilities which are always potentially available */
/* but to only one environment */
jvmtiCapabilities always_solo_capabilities;

/* capabilities which are potentially available during OnLoad */
/* but to only one environment */
jvmtiCapabilities onload_solo_capabilities;

/* remaining capabilities which are always potentially available */
/* but to only one environment */
jvmtiCapabilities always_solo_remaining_capabilities;

/* remaining capabilities which are potentially available during OnLoad */
/* but to only one environment */
jvmtiCapabilities onload_solo_remaining_capabilities;

/* all capabilities ever acquired */
jvmtiCapabilities acquired_capabilities;


/*
 * if the capability sets are initialized in the onload phase then
 * it happens before class data sharing (CDS) is initialized. If it
 * turns out that CDS gets disabled then we must adjust the always
 * capabilities. To ensure a consistent view of the capabililties
 * anything we add here should already be in the onload set.
 */
void recompute_always_capabilities() {
}


/* corresponding init functions */
jvmtiCapabilities init_always_capabilities() { 
  jvmtiCapabilities jc;
  
  memset(&jc, 0, sizeof(jc));
  /*  jc.can_get_bytecodes = 1; */
  jc.can_signal_thread = 1;
  jc.can_get_source_file_name = 1;
  jc.can_get_line_numbers = 1;
  /*  jc.can_get_synthetic_attribute = 1; */
  /*  jc.can_get_monitor_info = 1; */
  /*  jc.can_get_constant_pool = 1; */
  jc.can_generate_monitor_events = 1;
  jc.can_generate_garbage_collection_events = 1;
  jc.can_generate_compiled_method_load_events = 1;
  jc.can_generate_native_method_bind_events = 1;
  jc.can_generate_vm_object_alloc_events = 1;
  jc.can_redefine_classes = 1;
  if (CVMtimeIsThreadCpuTimeSupported()) {
      jc.can_get_thread_cpu_time = 1;
      jc.can_get_current_thread_cpu_time = 1;
  }
  /*
  jc.can_retransform_classes = 1;
  jc.can_set_native_method_prefix = 1;
  */
  return jc;
} 

jvmtiCapabilities init_onload_capabilities() {
  jvmtiCapabilities jc;
  
  memset(&jc, 0, sizeof(jc));
#ifdef CVM_JIT
  /* can't keep track of locks so can't pop frames */
  jc.can_pop_frame = 0;
  jc.can_force_early_return = 0;
#else
  jc.can_pop_frame = 1;
  jc.can_force_early_return = 1;
#endif
  /*  jc.can_get_source_debug_extension = 1; */
  jc.can_maintain_original_method_order = 1;
  /*  jc.can_redefine_any_class = 1; */
  /*  jc.can_retransform_any_class = 1; */
  jc.can_generate_all_class_hook_events = 1;
  jc.can_generate_exception_events = 1;
  jc.can_get_owned_monitor_info = 1;
  /*  jc.can_get_owned_monitor_stack_depth_info = 1; */
  jc.can_get_current_contended_monitor = 1;
  jc.can_tag_objects = 1;
  /*  jc.can_get_monitor_info = 1; */
  jc.can_generate_object_free_events = 1;
  if (CVMglobals.jvmtiDebuggingFlag) {
      /* Debugging session, turn on some capabilities */
      jc.can_generate_single_step_events = 1;
      jc.can_generate_method_entry_events = 1;
      jc.can_generate_method_exit_events = 1;
      jc.can_generate_frame_pop_events = 1;
      jc.can_access_local_variables = 1;
      jc.can_pop_frame = 1;
      jc.can_force_early_return = 1;
  }
  return jc;
}


jvmtiCapabilities init_always_solo_capabilities() {
  jvmtiCapabilities jc;
  
  memset(&jc, 0, sizeof(jc));
  jc.can_suspend = 1;
  return jc;
}


jvmtiCapabilities init_onload_solo_capabilities() {
  jvmtiCapabilities jc;
  
  memset(&jc, 0, sizeof(jc));
  if (CVMglobals.jvmtiDebuggingFlag) {
      /* Debugging session, turn on some capabilities */
      jc.can_generate_field_modification_events = 1;
      jc.can_generate_field_access_events = 1;
      jc.can_generate_breakpoint_events = 1;
  }
  return jc;
}

void CVMjvmtiInitializeCapabilities() {
  always_capabilities = init_always_capabilities();
  onload_capabilities = init_onload_capabilities();
  always_solo_capabilities = init_always_solo_capabilities();
  onload_solo_capabilities = init_onload_solo_capabilities();
  always_solo_remaining_capabilities = init_always_solo_capabilities();
  onload_solo_remaining_capabilities = init_onload_solo_capabilities();
  memset(&acquired_capabilities, 0, sizeof(acquired_capabilities));
}


jvmtiCapabilities *either(const jvmtiCapabilities *a,
                          const jvmtiCapabilities *b,
                          jvmtiCapabilities *result) {
  int i;
  char *ap = (char *)a;
  char *bp = (char *)b;
  char *resultp = (char *)result;

  for (i = 0; i < CAPA_SIZE; ++i) {
    *resultp++ = *ap++ | *bp++;
  }

  return result;
}


jvmtiCapabilities *both(const jvmtiCapabilities *a,
                        const jvmtiCapabilities *b, 
                        jvmtiCapabilities *result) {
  int i;
  char *ap = (char *)a;
  char *bp = (char *)b;
  char *resultp = (char *)result;

  for (i = 0; i < CAPA_SIZE; ++i) {
    *resultp++ = *ap++ & *bp++;
  }

  return result;
}


jvmtiCapabilities *exclude(const jvmtiCapabilities *a,
                           const jvmtiCapabilities *b, 
                           jvmtiCapabilities *result) {
  int i;
  char *ap = (char *)a;
  char *bp = (char *)b;
  char *resultp = (char *)result;

  for (i = 0; i < CAPA_SIZE; ++i) {
    *resultp++ = *ap++ & ~*bp++;
  }

  return result;
}


jboolean has_some(const jvmtiCapabilities *a) {
  int i;
  char *ap = (char *)a;

  for (i = 0; i < CAPA_SIZE; ++i) {
    if (*ap++ != 0) {
      return CVM_TRUE;
    }
  }

  return CVM_FALSE;
}


void CVMjvmtiCopyCapabilities(const jvmtiCapabilities *from, jvmtiCapabilities *to) {
  int i;
  char *ap = (char *)from;
  char *resultp = (char *)to;

  for (i = 0; i < CAPA_SIZE; ++i) {
    *resultp++ = *ap++;
  }
}


void CVMjvmtiGetPotentialCapabilities(const jvmtiCapabilities *current, 
                                const jvmtiCapabilities *prohibited, 
                                jvmtiCapabilities *result) {
  /* exclude prohibited capabilities, must be before adding current */
  exclude(&always_capabilities, prohibited, result);

  /* must include current since it may possess solo capabilities and now prohibited */
  either(result, current, result);

  /* add other remaining */
  either(result, &always_solo_remaining_capabilities, result);

  /* if this is during OnLoad more capabilities are available */
  /*  if (JvmtiEnv::get_phase() == JVMTI_PHASE_ONLOAD) { */
    either(result, &onload_capabilities, result);
    either(result, &onload_solo_remaining_capabilities, result);
    /*  } */

}

void update() {
  jvmtiCapabilities avail;

  jboolean interp_events;

  /* all capabilities */
  either(&always_capabilities, &always_solo_capabilities, &avail);

  interp_events = 
    avail.can_generate_field_access_events ||
    avail.can_generate_field_modification_events ||
    avail.can_generate_single_step_events ||
    avail.can_generate_frame_pop_events ||
    avail.can_generate_method_entry_events ||
    avail.can_generate_method_exit_events;

  /*
   * If can_redefine_classes is enabled in the onload phase then we know that the
   * dependency information recorded by the compiler is complete.
   */
  /*
  if ((avail.can_redefine_classes || avail.can_retransform_classes) && 
      JvmtiEnv::get_phase() == JVMTI_PHASE_ONLOAD) {
    JvmtiExport::set_all_dependencies_are_recorded(true);
  }
  */
  CVMjvmtiSetCanGetSourceDebugExtension(avail.can_get_source_debug_extension);
  CVMjvmtiSetCanExamineOrDeoptAnywhere(
    avail.can_generate_breakpoint_events ||
    interp_events || 
    avail.can_redefine_classes ||
    avail.can_retransform_classes ||
    avail.can_access_local_variables ||
    avail.can_get_owned_monitor_info ||
    avail.can_get_current_contended_monitor ||
    avail.can_get_monitor_info ||
    avail.can_get_owned_monitor_stack_depth_info);
  CVMjvmtiSetCanMaintainOriginalMethodOrder(avail.can_maintain_original_method_order);
  CVMjvmtiSetCanPostInterpreterEvents(interp_events);
  CVMjvmtiSetCanHotswapOrPostBreakpoint(
    avail.can_generate_breakpoint_events ||
    avail.can_redefine_classes ||
    avail.can_retransform_classes);
  CVMjvmtiSetCanModifyAnyClass(
    avail.can_generate_breakpoint_events ||
    avail.can_retransform_classes || /* NOTE: remove when there is support for redefine with class sharing */
    avail.can_retransform_any_class   ||
    avail.can_redefine_classes ||  /* NOTE: remove when there is support for redefine with class sharing */
    avail.can_redefine_any_class ||
    avail.can_generate_all_class_hook_events);
  CVMjvmtiSetCanWalkAnySpace(
                         avail.can_tag_objects);  /* NOTE: remove when IterateOverReachableObjects supports class sharing */
  CVMjvmtiSetCanAccessLocalVariables(
    avail.can_access_local_variables  ||
    avail.can_redefine_classes ||
    avail.can_retransform_classes);
  CVMjvmtiSetCanPostExceptions(
    avail.can_generate_exception_events ||
    avail.can_generate_frame_pop_events ||
    avail.can_generate_method_exit_events);
  CVMjvmtiSetCanPostBreakpoint(avail.can_generate_breakpoint_events);
  CVMjvmtiSetCanPostFieldAccess(avail.can_generate_field_access_events);
  CVMjvmtiSetCanPostFieldModification(avail.can_generate_field_modification_events);
  CVMjvmtiSetCanPostMethodEntry(avail.can_generate_method_entry_events);
  CVMjvmtiSetCanPostMethodExit(avail.can_generate_method_exit_events ||
                                        avail.can_generate_frame_pop_events);
  CVMjvmtiSetCanPopFrame(avail.can_pop_frame);
  CVMjvmtiSetCanForceEarlyReturn(avail.can_force_early_return);
  CVMjvmtiSetShouldCleanUpHeapObjects(avail.can_generate_breakpoint_events);
}

jvmtiError CVMjvmtiAddCapabilities(const jvmtiCapabilities *current,
                            const jvmtiCapabilities *prohibited, 
                            const jvmtiCapabilities *desired, 
                            jvmtiCapabilities *result) {
  /* check that the capabilities being added are potential capabilities */
  jvmtiCapabilities temp;
  CVMjvmtiGetPotentialCapabilities(current, prohibited, &temp);
  if (has_some(exclude(desired, &temp, &temp))) {
    return JVMTI_ERROR_NOT_AVAILABLE;
  }

  /* add to the set of ever acquired capabilities */
  either(&acquired_capabilities, desired, &acquired_capabilities);

  /* onload capabilities that got added are now permanent - so, also remove from onload */
  both(&onload_capabilities, desired, &temp);
  either(&always_capabilities, &temp, &always_capabilities);
  exclude(&onload_capabilities, &temp, &onload_capabilities);

  /* same for solo capabilities (transferred capabilities in the remaining sets handled as part of standard grab - below) */
  both(&onload_solo_capabilities, desired, &temp);
  either(&always_solo_capabilities, &temp, &always_solo_capabilities);
  exclude(&onload_solo_capabilities, &temp, &onload_solo_capabilities);

  /* remove solo capabilities that are now taken */
  exclude(&always_solo_remaining_capabilities, desired, &always_solo_remaining_capabilities);
  exclude(&onload_solo_remaining_capabilities, desired, &onload_solo_remaining_capabilities);

  /* return the result */
  either(current, desired, result);

  update();

  return JVMTI_ERROR_NONE;
}


void CVMjvmtiRelinquishCapabilities(const jvmtiCapabilities *current,
                             const jvmtiCapabilities *unwanted, 
                             jvmtiCapabilities *result) {
  jvmtiCapabilities to_trash;
  jvmtiCapabilities temp;

  /* can't give up what you don't have */
  both(current, unwanted, &to_trash);

  /* restore solo capabilities but only those that belong */
  either(&always_solo_remaining_capabilities, both(&always_solo_capabilities, &to_trash, &temp), 
         &always_solo_remaining_capabilities);
  either(&onload_solo_remaining_capabilities, both(&onload_solo_capabilities, &to_trash, &temp),
         &onload_solo_remaining_capabilities);

  update();

  /* return the result */
  exclude(current, unwanted, result);
}



#ifdef DEBUG

void print_cr(char *s) {
  CVMdebugPrintf(("%s\n", s));
}

void  print_capabilities(const jvmtiCapabilities* cap) {
  print_cr("----- capabilities -----");
  if (cap->can_tag_objects)				    
    print_cr("can_tag_objects");
  if (cap->can_generate_field_modification_events)	    
    print_cr("can_generate_field_modification_events");
  if (cap->can_generate_field_access_events)		    
    print_cr("can_generate_field_access_events");
  if (cap->can_get_bytecodes)				    
    print_cr("can_get_bytecodes");
  if (cap->can_get_synthetic_attribute)			    
    print_cr("can_get_synthetic_attribute");
  if (cap->can_get_owned_monitor_info)			    
    print_cr("can_get_owned_monitor_info");
  if (cap->can_get_current_contended_monitor)		    
    print_cr("can_get_current_contended_monitor");
  if (cap->can_get_monitor_info)			    
    print_cr("can_get_monitor_info");
  if (cap->can_get_constant_pool)	    
    print_cr("can_get_constant_pool");
  if (cap->can_pop_frame)				    
    print_cr("can_pop_frame");
  if (cap->can_force_early_return)				    
    print_cr("can_force_early_return");
  if (cap->can_redefine_classes)			    
    print_cr("can_redefine_classes");
  if (cap->can_retransform_classes)			    
    print_cr("can_retransform_classes");
  if (cap->can_signal_thread)				    
    print_cr("can_signal_thread");
  if (cap->can_get_source_file_name)			    
    print_cr("can_get_source_file_name");
  if (cap->can_get_line_numbers)			    
    print_cr("can_get_line_numbers");
  if (cap->can_get_source_debug_extension)		    
    print_cr("can_get_source_debug_extension");
  if (cap->can_access_local_variables)			    
    print_cr("can_access_local_variables");
  if (cap->can_maintain_original_method_order)		    
    print_cr("can_maintain_original_method_order");
  if (cap->can_generate_single_step_events)		    
    print_cr("can_generate_single_step_events");
  if (cap->can_generate_exception_events)		    
    print_cr("can_generate_exception_events");
  if (cap->can_generate_frame_pop_events)		    
    print_cr("can_generate_frame_pop_events");
  if (cap->can_generate_breakpoint_events)		    
    print_cr("can_generate_breakpoint_events");
  if (cap->can_suspend)					    
    print_cr("can_suspend");
  if (cap->can_redefine_any_class )			    
    print_cr("can_redefine_any_class");
  if (cap->can_retransform_any_class )			    
    print_cr("can_retransform_any_class");
  if (cap->can_get_current_thread_cpu_time)		    
    print_cr("can_get_current_thread_cpu_time");
  if (cap->can_get_thread_cpu_time)			    
    print_cr("can_get_thread_cpu_time");
  if (cap->can_generate_method_entry_events)		    
    print_cr("can_generate_method_entry_events");
  if (cap->can_generate_method_exit_events)		    
    print_cr("can_generate_method_exit_events");
  if (cap->can_generate_all_class_hook_events)		    
    print_cr("can_generate_all_class_hook_events");
  if (cap->can_generate_compiled_method_load_events)	    
    print_cr("can_generate_compiled_method_load_events");
  if (cap->can_generate_monitor_events)			    
    print_cr("can_generate_monitor_events");
  if (cap->can_generate_vm_object_alloc_events)		    
    print_cr("can_generate_vm_object_alloc_events");
  if (cap->can_generate_native_method_bind_events)	    
    print_cr("can_generate_native_method_bind_events");
  if (cap->can_generate_garbage_collection_events)	    
    print_cr("can_generate_garbage_collection_events");
  if (cap->can_generate_object_free_events)		    
    print_cr("can_generate_object_free_events");
}

#endif


     
