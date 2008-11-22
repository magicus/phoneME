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

#include <midp_constants_data.h>
#include <midp_properties_port.h>
#include <midp_logging.h>

#if ENABLE_MULTIPLE_ISOLATES
/**
 * Reads AMS_MEMORY_RESERVED_MVM property and returns the amount of Java
 * heap memory reserved for AMS isolate. Whether the property is not found,
 * the same name hardcoded constant is used instead.
 *
 * @return total heap size in bytes available for AMS isolate,
 *    or -1 if unlimitted
 */
int getAmsHeapReserved() {
    int reserved;
    reserved = getInternalPropertyInt("AMS_MEMORY_RESERVED_MVM");
    if (0 == reserved) {
        REPORT_ERROR(LC_AMS, "AMS_MEMORY_RESERVED_MVM property not set");
        reserved = AMS_MEMORY_RESERVED_MVM;
    }
    reserved = reserved * 1024;
    return reserved;
}

/**
 * Reads AMS_MEMORY_LIMIT_MVM property and returns the maximal Java heap size
 * avilable for AMS isolate. Whether the property is not found, the same name
 * hardcoded constant is used instead.
 *
 * @return total heap size available for AMS isolate
 */
int getAmsHeapLimit() {
    int limit;
    limit = getInternalPropertyInt("AMS_MEMORY_LIMIT_MVM");
    if (0 == limit) {
        REPORT_ERROR(LC_AMS, "AMS_MEMORY_LIMIT_MVM property not set");
        limit = AMS_MEMORY_LIMIT_MVM;
    }
    if (limit <= 0) {
        limit = 0x7FFFFFFF;  /* MAX_INT */
    } else {
        limit = limit * 1024;
    }
    return limit;
}
#endif

/**
 * Reads JAVA_HEAP_SIZE property and returns it as required heap size.
 * If JAVA_HEAP_SIZE has not been found, then reads MAX_ISOLATES property,
 * calculates and returns size of the required heap. If the MAX_ISOLATES
 * has not been found, default heap size is returned.
 *
 * @return <tt>heap size</tt>
 */
 int getHeapRequirement() {
    int max_isolates;
    int midp_heap_requirement;

    midp_heap_requirement = getInternalPropertyInt("JAVA_HEAP_SIZE");
    if (midp_heap_requirement > 0) {
        return midp_heap_requirement;
    }

#if ENABLE_MULTIPLE_ISOLATES
    max_isolates = getInternalPropertyInt("MAX_ISOLATES");
    if (max_isolates <= 0) {
        max_isolates = MAX_ISOLATES;
    }
#else
    max_isolates = 1;
#endif

    /*
     * Calculate heap size.
     *
     * IMPL_NOTE: bellow ENABLE_NATIVE_APP_MANAGER value is checked instead of
     * moving this part into a separate library
     * (for ex., ams/example/ams_parameters)
     * because currently amount of memory needed for AMS isolate is the only
     * property that has different values for JAMS and NAMS. If new such values
     * are added, a new library should be introduced.
     */
#if ENABLE_NATIVE_APP_MANAGER
    /*
     * Actually, when using NAMS, AMS isolate requires less then 1024K memory,
     * so the value bellow can be tuned for each particular project.
     */
    midp_heap_requirement = max_isolates * 1024 * 1024;
#else
    /*
     * In JAMS AMS isolate requires a little bit more memory because
     * it holds skin images that are shared among all isolates.
     */
    midp_heap_requirement = 1280 * 1024 + (max_isolates - 1) * 1024 * 1024;
#endif

    return midp_heap_requirement;
}
