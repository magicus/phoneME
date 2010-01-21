/*
 * @(#)named_sys_monitor.h	1.11 06/10/27
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

#ifndef _INCLUDED_NAMED_SYS_MONITOR_H
#define _INCLUDED_NAMED_SYS_MONITOR_H

/* NOTE: This module defines the CVMNamedSysMonitor which is currently solely
         used by the JVMTI and JVMPI APIs.  CVMNamedSysMonitor inherits from
         CVMProfiledMonitor (which is defined in sync.h), and adds a name
         field.
*/

#if (defined(CVM_JVMTI) || defined(CVM_JVMPI))

#include "javavm/include/indirectmem.h"
#include "javavm/include/sync.h"

/*=============================================== class CVMNamedSysMonitor ==*/

/* Purpose: "raw" named object-less monitor for the use of JVMTI & JVMPI. */
/* NOTE: Make sure to use CVMnamedSysMonitorComputeSize() to compute the
         amount of memory in bytes to be allocated for this monitor.  This is
         necessary because the name of the monitor is stored immediately after
         the monitor part.  This allows the needed memory to be allocated in
         one single request and simplifies the handling of "Out of Memory"
         errors by JVMTI & JVMPI.
*/
/* NOTE: The CVMNamedSysMonitor implementation describes the following
         abstraction:

    class CVMNamedSysMonitor extends CVMProfiledMonitor {
        private static CVMUint32 computeSize(const char *name);
        public const char *getName();

        public static CVMNamedSysMonitor *init(const char *name);
        public void destroy();
        public void enter(CVMExecEnv *current_ee);
        public void exit(CVMExecEnv *current_ee);

        public CVMBool notify(CVMExecEnv *current_ee);
        public CVMBool notifyAll(CVMExecEnv *current_ee);

        public CVMWaitStatus wait(CVMExecEnv *current_ee, CVMInt64 millis);
        public CVMExecEnv *getOwner();
        public CVMUint32 getCount();

        public CVMBool tryEnter(CVMExecEnv *current_ee);
        public void enterUnsafe(CVMExecEnv *current_ee);
        public void enterSafe(CVMExecEnv *current_ee);
    };
*/

struct CVMNamedSysMonitor {
    CVMProfiledMonitor _super;
    char name[1];
};

/* Purpose: Gets the name of this monitor. */
/* inline const char *CVMnamedSysMonitorGetName(CVMNamedSysMonitor *self); */
#define CVMnamedSysMonitorGetName(self) \
    ((char *)(self)->name)

/* Purpose: Gets the type of this monitor. */
/* inline const char *CVMnamedSysMonitorGettype(CVMNamedSysMonitor *self); */
#define CVMnamedSysMonitorGetType(self) \
    ((self)->_super.type)

/* Purpose: Constructor. */
extern CVMNamedSysMonitor *CVMnamedSysMonitorInit(const char *name);

/* Purpose: Destructor. */
void CVMnamedSysMonitorDestroy(CVMNamedSysMonitor *self);

/* Purpose: Enters the monitor. */
/* inline void CVMnamedSysMonitorEnter(CVMNamedSysMonitor *self,
            CVMExecEnv *current_ee); */
#define CVMnamedSysMonitorEnter(self, current_ee) \
    CVMprofiledMonitorEnter((&(self)->_super), (current_ee), CVM_FALSE)

/* Purpose: Exits the monitor. */
/* inline void CVMnamedSysMonitorExit(CVMNamedSysMonitor *self,
            CVMExecEnv *current_ee); */
#define CVMnamedSysMonitorExit(self, current_ee) \
    CVMprofiledMonitorExit((&(self)->_super), (current_ee))

/* Purpose: Notifies the next thread waiting on this monitor. */
/* Returns: If the current_ee passed in is not the owner of the monitor,
            CVM_FALSE will be returned.  Else, returns CVM_TRUE. */
/* inline CVMBool CVMnamedSysMonitorNotify(CVMNamedSysMonitor *self,
            CVMExecEnv *current_ee); */
#define CVMnamedSysMonitorNotify(self, current_ee) \
    CVMprofiledMonitorNotify((&(self)->_super), (current_ee))

/* Purpose: Notifies all threads waiting on this monitor. */
/* Returns: If the current_ee passed in is not the owner of the monitor,
            CVM_FALSE will be returned.  Else, returns CVM_TRUE. */
/* inline CVMBool CVMnamedSysMonitorNotifyAll(CVMNamedSysMonitor *self,
            CVMExecEnv *current_ee); */
#define CVMnamedSysMonitorNotifyAll(self, current_ee) \
    CVMprofiledMonitorNotifyAll((&(self)->_super), (current_ee))

/* Purpose: Wait for the specified time period (in milliseconds).  If the
            period is 0, then wait forever. */
/* Returns: CVM_WAIT_NOT_OWNER if current_ee is not the owner of the monitor,
            CVM_WAIT_OK if the thread was woken up by a timeout, and
            CVM_WAIT_INTERRUPTED is the thread was woken up by a notify. */
/* NOTE:    Thread must be in a GC safe state before calling this. */
/* inline CVMWaitStatus CVMnamedSysMonitorWait(CVMNamedSysMonitor *self,
            CVMExecEnv *current_ee, CVMInt64 millis); */
#define CVMnamedSysMonitorWait(self, current_ee, millis) \
    CVMprofiledMonitorWait((&(self)->_super), (current_ee), (millis))

/* Purpose: Gets the current owner ee of this monitor. */
/* inline CVMExecEnv *CVMnamedSysMonitorGetOwner(CVMNamedSysMonitor *self); */
#define CVMnamedSysMonitorGetOwner(self) \
    CVMprofiledMonitorGetOwner(&(self)->_super)

/* Purpose: Gets the current lock count of this monitor. */
/* inline CVMUint32 CVMnamedSysMonitorGetCount(CVMNamedSysMonitor *self); */
#define CVMnamedSysMonitorGetCount(self) \
    CVMprofiledMonitorGetCount(&(self)->_super)

/* Purpose: Tries to enter the monitor. */
/* inline CVMBool CVMnamedSysMonitorTryEnter(CVMNamedSysMonitor *self,
            CVMExecEnv *current_ee); */
#define CVMnamedSysMonitorTryEnter(self, current_ee) \
    CVMprofiledMonitorTryEnter((&(self)->_super), (current_ee))

/* Purpose: Enters monitor while thread is in a GC unsafe state. */
/* inline void CVMnamedSysMonitorEnterUnsafe(CVMNamedSysMonitor *self,
            CVMExecEnv *current_ee); */
#define CVMnamedSysMonitorEnterUnsafe(self, current_ee) {           \
	CVMprofiledMonitorEnterUnsafe((&(self)->_super), (current_ee), CVM_FALSE); \
}

/* Purpose: Enters monitor while thread is in a GC safe state. */
/* inline void CVMnamedSysMonitorEnterSafe(CVMNamedSysMonitor *self,
            CVMExecEnv *current_ee); */
#define CVMnamedSysMonitorEnterSafe(self, current_ee) {           \
    CVMprofiledMonitorEnterSafe((&(self)->_super), (current_ee)); \
}

/* Purpose: Casts a CVMProfiledMonitor into a CVMNamedSysMonitor. */
/* NOTE: The specified CVMProfiledMonitor must have already been determined to
         be an instance of CVMNamedSysMonitor.  This is possible because
         CVMNamedSysMonitor is actually a subclass of CVMProfiledMonitor. */
#define CVMcastProfiledMonitor2NamedSysMonitor(self) \
    ((CVMNamedSysMonitor *)(((CVMUint8 *)(self)) -   \
                            CVMoffsetof(CVMNamedSysMonitor, _super)))

/* Purpose: Casts a CVMNamedSysMonitor into a CVMProfiledMonitor. */
/* NOTE: This is possible because CVMNamedSysMonitor is a subclass of
         CVMProfiledMonitor. */
#define CVMcastNamedSysMonitor2ProfiledMonitor(self) \
    ((CVMProfiledMonitor *)(&(self)->_super))

#endif /* (defined(CVM_JVMTI) || defined(CVM_JVMPI)) */

#endif /* _INCLUDED_NAMED_SYS_MONITOR_H */
