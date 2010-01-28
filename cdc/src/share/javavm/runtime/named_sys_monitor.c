/*
 * @(#)named_sys_monitor.c	1.12 06/10/25
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

#include "javavm/include/defs.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/named_sys_monitor.h"
#include "javavm/include/clib.h"

#if (defined(CVM_JVMTI) || defined(CVM_JVMPI))

/*=============================================== class CVMNamedSysMonitor ==*/

/* Purpose: Computes the size of an instance of the monitor. */
/* inline static CVMUint32 CVMnamedSysMonitorComputeSize(const char *name); */
#define CVMnamedSysMonitorComputeSize(name) \
    (CVMoffsetof(CVMNamedSysMonitor, name) + strlen(name) + 1)

/* Purpose: Constructor. */
CVMNamedSysMonitor *CVMnamedSysMonitorInit(const char *name)
{
    CVMNamedSysMonitor *monitor;
#ifdef CVM_JVMPI
    CVMExecEnv *ee = CVMgetEE();
    CVMassert(CVMD_isgcSafe(ee));
#endif
    monitor = malloc (CVMnamedSysMonitorComputeSize(name));
    if (monitor == NULL) {
	return NULL;
    }
    if (!CVMprofiledMonitorInit(&monitor->_super, NULL, 0,
				CVM_LOCKTYPE_NAMED_SYSMONITOR)) {
	return NULL;
    }
    strcpy (monitor->name, name);
#ifdef CVM_JVMPI
    {
        CVMProfiledMonitor **firstPtr = &CVMglobals.rawMonitorList;
        CVMProfiledMonitor *mon;
        mon = CVMcastNamedSysMonitor2ProfiledMonitor(monitor);
        CVMsysMutexLock(ee, &CVMglobals.jvmpiSyncLock);
        mon->next = *firstPtr;
        mon->previousPtr = firstPtr;
        if (*firstPtr != NULL) {
            (*firstPtr)->previousPtr = &mon->next;
        }
        *firstPtr = mon;
        CVMsysMutexUnlock(ee, &CVMglobals.jvmpiSyncLock);
    }
#endif
    return monitor;
}

/* Purpose: Destructor. */
void CVMnamedSysMonitorDestroy(CVMNamedSysMonitor *self)
{
    CVMProfiledMonitor *mon = CVMcastNamedSysMonitor2ProfiledMonitor(self);
#ifdef CVM_JVMPI
    CVMExecEnv *ee = CVMgetEE();

    CVMassert(CVMD_isgcSafe(ee));

    CVMsysMutexLock(ee, &CVMglobals.jvmpiSyncLock);
    *mon->previousPtr = mon->next;
    if (mon->next != NULL) {
        mon->next->previousPtr = mon->previousPtr;
    }
    CVMsysMutexUnlock(ee, &CVMglobals.jvmpiSyncLock);
#endif
    CVMprofiledMonitorDestroy(mon);
    free (self);
}

#endif

