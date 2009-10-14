/*
 *
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
 */

#include <kni.h>
#include <string.h>
#include <midpMalloc.h>
#include <midpAMS.h>
#include <midpStorage.h>
#include <midpResourceLimit.h>
#include <midp_properties_port.h>
#include <midpInit.h>
#include <suitestore_common.h>
#if !ENABLE_CDC
#include <pcsl_network.h>
#include <suspend_resume.h>
#endif
#if MEASURE_STARTUP
#include <stdio.h>
#include <pcsl_print.h>
#endif
#if ENABLE_MULTIPLE_ISOLATES
#include <midp_links.h>
#endif

#if ENABLE_ICON_CACHE
#include <suitestore_icon_cache.h>
#endif

/**
 * @file
 *
 * Used to initialize the midp runtime.
 */

static int initLevel = NO_INIT;

static char* midpAppDir = NULL;

static char* midpConfig = NULL;

static void (*vmFinalizer)(void) = NULL;

/**
 * Sets the application directory for MIDP if needed.
 * So the suites and other MIDP persistent
 * state can be found. Only had an effect when called before 
 * any other method except midpInitialize is called.
 * If not called directory specified by midpSetConfigDir will 
 * be used.
 * 
 * @param dir application directory of MIDP
 */
void midpSetAppDir(const char* dir) {

    midpAppDir = (char *)dir;
}

/**
 * Sets the config directory for MIDP if needed.
 * In this directory static data as images and 
 * configuration files are located. If not called
 * directory specified by midpSetAppDir will be used.
 * Only had an effect when called before any
 * other method except midpInitialize is called.
 *
 * @param dir config directory of MIDP
 */
void midpSetConfigDir(const char* dir) {
    midpConfig = (char *)dir;
}

/**
 * The public MIDP initialization function. If not running for the MIDP
 * home directory, midpSetAppDir, midpSetConfigDir should the first calls 
 * after this with the directory of the MIDP system.
 */
int midpInitialize() {

#if MEASURE_STARTUP
    extern jlong Java_java_lang_System_currentTimeMillis();
    char msg[128];
    sprintf(msg, "System Startup Time: Begin at %lld\n",
            Java_java_lang_System_currentTimeMillis());
    pcsl_print(msg);
#endif

    /*
     * Initialization is performed in steps so that we do use any
     * extra resources such as the VM for the operation being performed.
     *
     * The common functionality needed initially is memory, storage and
     * configuration information . So we will start with the LIST_LEVEL.
     *
     * Public function that need a higher level of initialization will do
     * so as their first step, to make the caller's code less complex.
     */
    return midpInit(LIST_LEVEL);
}

/**
 * Internal function to initialize MIDP to only the level required by
 * the current functionality being used. This can be called multiple
 * times with the same level or lower. See midpInitCallback for more.
 *
 * @param level level of initialization required
 *
 * @return zero for success, non-zero if out of memory
 */
int midpInit(int level) {
    return midpInitCallback(level, NULL, NULL);
}

/**
 * Internal function for retrieving current MIDP initialization level.
 *
 * @return maximum initialization level achieved
 */
int getMidpInitLevel() {
    return initLevel;
}

/**
 * Internal function to initialize MIDP to only the level required by
 * the current functionality being used. This can be called multiple
 * times with the same level or lower.
 * <p>
 * On device with more conventional operating systems like Linux,
 * listing, removing and running MIDlets may be done by different executables
 * and the executables may be linked statically. In this case
 * we do not want the list- and remove-MIDlet executables to have to link
 * the VM code, just the run executable. If the initialize and finalize
 * functions were referenced directly, the list- and remove- executables would
 * need to link them; by calling the VM functions indirectly we avoid this.
 *
 * @param level level of initialization required
 * @param init pointer to a VM init function that returns 0 for success
 * @param final pointer to a VM finalize function
 *
 * @return zero for success, non-zero if out of memory
 */
int midpInitCallback(int level, int (*init)(void), void (*final)(void)) {
    const char* spaceProp;
    long totalSpace;
    int result = -1; /* error status by default */
    int main_memory_chunk_size;

    if (initLevel >= level) {
        return 0;
    }

    do {
        if ((midpAppDir == NULL) && (midpConfig == NULL)) {
            /*
             * The caller has to set midpAppDir of midpConfig before
             * calling midpInitialize().
             */
            break;
        }

        /* duplicate values if not set */
        if (midpConfig == NULL) {
        	midpConfig = midpAppDir;
        } else if (midpAppDir == NULL) {
        	midpAppDir = midpConfig;
        } 

        if (level >= MEM_LEVEL && initLevel < MEM_LEVEL) {
	    /* Get java heap memory size */
	    main_memory_chunk_size = getInternalPropertyInt("MAIN_MEMORY_CHUNK_SIZE");
	    if (main_memory_chunk_size == 0) {
		main_memory_chunk_size = -1;
	    }
            /* Do initialization for the next level: MEM_LEVEL */
            if (midpInitializeMemory(main_memory_chunk_size) != 0) {
                break;
            }
            initLevel = MEM_LEVEL;
        }

        if (level >= LIST_LEVEL && initLevel < LIST_LEVEL) {
            /* Do initialization for the next level: LIST_LEVEL */
            MIDPError status;
            int err;

            /* 
             * initializing suspend/resume system first, other systems may then
             * register their resources there. 
             */
#if !ENABLE_CDC
            sr_initSystem();
#endif
            err = storageInitialize(midpConfig, midpAppDir);
            if (err == 0) {
                status = midp_suite_storage_init();
            } else {
                status = OUT_OF_MEMORY;
            }

            if (status != ALL_OK) {
                break;
            }

            if (initializeConfig() != 0) {
                break;
            }

            /*
             * IMPL_NOTE: revisit for theme loading clean up
             * look in javax.microedition.lcdui.Theme for default theme.
             */
            /*
            if (initializeThemeConfig() != 0) {
                break;
            }
            */

            /*
             * in the event that the system.jam_space property is set,
             * use it as the maximum space use limit for MIDP MIDlet
             * suites and their record stores.
             */
            spaceProp = getInternalProperty("system.jam_space");
            if (spaceProp != NULL) {
                totalSpace = atoi(spaceProp);
                storageSetTotalSpace(totalSpace);
            }

            initLevel = LIST_LEVEL;
        }

        if (level >= VM_LEVEL && initLevel < VM_LEVEL) {
            /* Do initialization for the next level: VM_LEVEL */
            if (init != NULL) {
                if (init() != 0) {
                    break;
                }
            }

            vmFinalizer = final;
            initLevel   = VM_LEVEL;
        }

        result = 0; /* OK status */
    } while (0);

    if (result != 0) {
        midpFinalize();
    }

    return result;
}

/**
 * Cleans up MIDP resources. This should be last MIDP function called or
 * midpInitialize should be called again if another MIDP function
 * is needed such as running MIDP in a loop.
 */
void midpFinalize() {
    if (initLevel == NO_INIT) {
        return;
    }

    if (initLevel >= VM_LEVEL) {
        if (vmFinalizer != NULL) {
            vmFinalizer();
            vmFinalizer = NULL;
        }
    }

#if ENABLE_ICON_CACHE
    midp_free_suites_icons();
#endif    
    midp_suite_storage_cleanup();

    /** Now it makes no sense to process suspend/resume requests. */
#if !ENABLE_CDC
    sr_finalizeSystem();
#endif
    if (initLevel > MEM_LEVEL) {
        /* Cleanup native code resources on exit */
        midpFinalizeResourceLimit();
        finalizeConfig();

        /*
         * IMPL_NOTE: revisit for theme loading clean up
         * look in javax.microedition.lcdui.Theme for default theme.
         */
        /* finalizeThemeConfig(); */

        storageFinalize();
    }

#if ENABLE_MULTIPLE_ISOLATES
    midp_links_shutdown();
#endif    
#if !ENABLE_CDC
    pcsl_network_finalize_start(NULL);
#endif
    midpAppDir = NULL;
    midpFinalizeMemory();

    initLevel = NO_INIT;
}
