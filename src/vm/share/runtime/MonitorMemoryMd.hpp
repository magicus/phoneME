/*
 *   
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

#if ENABLE_MEMORY_MONITOR
/*
 * MonitorMemoryMd.hpp:
 */

typedef unsigned long u_long;

class MonitorMemoryMd {
public:
    static void startup(void);
    static void shutdown(void);
    static void startFlushThread(void);
    static void stopFlushThread(void);
    static void lock(void);
    static void unlock(void);
private:
    static volatile int memmonitor_flushed;
    static u_long htonl_m(u_long);
    friend class MonitorMemory;

}
 
#ifdef __cplusplus
extern "C" {
#endif

#include <kvmcompat.h>
#define BOOL_DEFINED
#include <lime.h>
#include <limesocket.h>

#ifdef __cplusplus
}
#endif

#endif
