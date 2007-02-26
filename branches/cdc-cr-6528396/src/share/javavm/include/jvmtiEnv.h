/*
 * @(#)jvmtiEnv.h	1.5 06/10/27
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


#ifndef _JAVA_JVMTIENV_H_
#define _JAVA_JVMTIENV_H_

enum {
    JVMTI_INTERNAL_CAPABILITY_COUNT = 39
};

typedef enum {
  EXT_EVENT_CLASS_UNLOAD = JVMTI_MIN_EVENT_TYPE_VAL-1,
  EXT_MIN_EVENT_TYPE_VAL = EXT_EVENT_CLASS_UNLOAD,
  EXT_MAX_EVENT_TYPE_VAL = EXT_EVENT_CLASS_UNLOAD,
  TOTAL_MIN_EVENT_TYPE_VAL = EXT_MIN_EVENT_TYPE_VAL,
  TOTAL_MAX_EVENT_TYPE_VAL = JVMTI_MAX_EVENT_TYPE_VAL
} jvmtiExtEvent;

/*
 * The complete range of events is EXT_MIN_EVENT_TYPE_VAL to 
 * JVMTI_MAX_EVENT_TYPE_VAL (inclusive and contiguous).
 */

typedef struct {
  jvmtiExtensionEvent ClassUnload;
} jvmtiExtEventCallbacks;

typedef struct {
  jlong tag;                  /* opaque tag */
  jobject      ref;           /* could be strong or weak */
  struct TagNode *next;       /* next RefNode* in bucket chain */
} JvmtiTagMap;

enum {
  JVMTI_MAGIC = 0x71EE,
  BAD_MAGIC   = 0xDEAD
};

typedef struct {
  jlong _enabled_bits;
} JvmtiEventEnabled;

typedef struct {

  /* user set global event enablement indexed by jvmtiEvent   */
  JvmtiEventEnabled _event_user_enabled;

  /*
   * this flag indicates the presence (true) or absence (false) of event callbacks
   *   it is indexed by jvmtiEvent  
   */
  JvmtiEventEnabled _event_callback_enabled;

  /*
   * indexed by jvmtiEvent true if enabled globally or on any thread.
   * True only if there is a callback for it.
   */
  JvmtiEventEnabled _event_enabled;

} JvmtiEnvEventEnable;

typedef struct _JvmtiEnv {

  jvmtiEnv _jvmti_external;
  const void *_env_local_storage;     /* per env agent allocated data. */
  jvmtiEventCallbacks _event_callbacks;
  jvmtiExtEventCallbacks _ext_event_callbacks;
  JvmtiTagMap* _tag_map;
  jboolean _is_valid;
  jboolean _thread_events_enabled;

  jint _magic;
  jint _index;

  JvmtiEnvEventEnable _env_event_enable;

  jvmtiCapabilities _current_capabilities;
  jvmtiCapabilities _prohibited_capabilities;

  jboolean  _class_file_load_hook_ever_enabled;

  char** _native_method_prefixes;
  int    _native_method_prefix_count;
        
}JvmtiEnv;

jint CVMdestroyJvmti(JvmtiEnv *env);

#endif /* !_JAVA_JVMTIENV_H_ */
