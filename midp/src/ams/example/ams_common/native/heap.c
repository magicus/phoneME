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
#include <midpServices.h>
#include <jvm.h>

#if ENABLE_MULTIPLE_ISOLATES

/**
 * Reads named property with heap size parameter and returns it in bytes.
 * When the property is not found provided default value is used instead.
 *
 * @param propertyName name of heap size property
 * @param defaultValue default value for the case of missing property
 * @return size value in bytes
 */
static int getHeapSizeProperty(const char *propertyName, int defaultValue) {

    int value;
    value = getInternalPropertyInt(propertyName);
    if (0 == value) {
        REPORT_WARN1(LC_AMS, "%s property not set", propertyName);
        value = defaultValue;
    }
    /* Check overflow */
    value = (value < 0 || value > 0x200000) ?
        0x7FFFFFFF /* MAX_INT */ :
        value * 1024;
    return value;
}

/**
 * Reads AMS_MEMORY_RESERVED_MVM property and returns the amount of Java
 * heap memory reserved for AMS isolate. Whether the property is not found,
 * the same name hardcoded constant is used instead.
 *
 * @return total heap size in bytes available for AMS isolate,
 *   0 means no memory is reserved for AMS isolate
 *
 */
int getAmsHeapReserved() {
    return getHeapSizeProperty(
        "AMS_MEMORY_RESERVED_MVM", AMS_MEMORY_RESERVED_MVM);
}

/**
 * Reads AMS_MEMORY_LIMIT_MVM property and returns the maximal Java heap size
 * available for AMS isolate. Whether the property is not found, the same name
 * hardcoded constant is used instead.
 *
 * @return total heap size available for AMS isolate
 */
int getAmsHeapLimit() {
    return getHeapSizeProperty(
        "AMS_MEMORY_LIMIT_MVM", AMS_MEMORY_LIMIT_MVM);
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
    int maxIsolates;
    int midpHeapRequirement;

    midpHeapRequirement = getInternalPropertyInt("JAVA_HEAP_SIZE");
    if (midpHeapRequirement > 0) {
        return midpHeapRequirement;
    }

    maxIsolates = getMaxIsolates();

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
    midpHeapRequirement = maxIsolates * 1024 * 1024;
#else
    /*
     * In JAMS AMS isolate requires a little bit more memory because
     * it holds skin images that are shared among all isolates.
     */
    midpHeapRequirement = 1280 * 1024 + (maxIsolates - 1) * 1024 * 1024;
#endif

    return midpHeapRequirement;
}

/**
 * Reads properties with Java heap parameters
 * and passes them to VM.
 */
void setHeapParameters() {
    int midpHeapRequirement;

    /* Sets Java heap size for VM */
    midpHeapRequirement = getHeapRequirement();
    JVM_SetConfig(JVM_CONFIG_HEAP_CAPACITY, midpHeapRequirement);

#if ENABLE_MULTIPLE_ISOLATES
{
    /* Sets Java heap parameters for AMS */
    int amsHeapReserved = getAmsHeapReserved();
    int amsHeapLimit = getAmsHeapLimit();
    JVM_SetConfig(JVM_CONFIG_FIRST_ISOLATE_RESERVED_MEMORY, amsHeapReserved);
    JVM_SetConfig(JVM_CONFIG_FIRST_ISOLATE_TOTAL_MEMORY, amsHeapLimit);
}
#endif
}
