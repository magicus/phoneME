/*
 * @(#)java_props_md.c	1.27 06/10/10
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

#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <ioLib.h>
#include <kernelLib.h>

#include "jni_util.h"

#include "javavm/include/porting/java_props.h"
#include "generated/javavm/include/build_defs.h"

static const char *valueOrDefault(const char *v, const char *def)
{
    return v != NULL ? v : def;
}

/* This function gets called very early, before VM_CALLS are setup.
 * Do not use any of the VM_CALLS entries!!!
 */
CVMBool
CVMgetJavaProperties(java_props_t *sprops)
{
    const char *v; /* tmp var */

    if (sprops->user_dir) {
        return CVM_TRUE;
    }

    /* tmp dir */
    sprops->tmp_dir = valueOrDefault(getenv("TMP"), "/tmp");

    /* Printing properties */
    sprops->printerJob = NULL;

    /* Java 2D properties */
    sprops->graphics_env = NULL;
    sprops->awt_toolkit = NULL;

    v = getenv("JAVA_FONTS");
    /*
    sprops->font_dir = v ? v :
    "/usr/openwin/lib/X11/fonts/Type1:/usr/openwin/lib/X11/fonts/TrueType";
    */

    /* If JAVA_FONTS is not set, the system-dependent fontpath will
     * be evaluated later
     */
    sprops->font_dir = v ? v : "";

    /* os properties */
    {
	sprops->os_name = "VxWorks";
	sprops->os_version = kernelVersion();

#undef CPU_STRING
#define CPU_STRING(x) #x
	sprops->os_arch = CPU_STRING(TARGET_CPU_FAMILY);
    }

    sprops->language = "en";
    sprops->encoding = "8859_1";
    
#if _BYTE_ORDER == _LITTLE_ENDIAN
    sprops->unicode_encoding = "UnicodeLittle";
#else
    sprops->unicode_encoding = "UnicodeBig";
#endif

    /* user properties */
    {
	sprops->user_name = valueOrDefault(getenv("USER"), "nobody");
	sprops->user_home = valueOrDefault(getenv("HOME"), "/");
    }

    /* User TIMEZONE */
    sprops->timezone = getenv("JAVA_TIMEZONE");

    /* Current directory */
    {
        char buf[MAX_FILENAME_LENGTH];
        getcwd(buf, sizeof(buf));
	sprops->user_dir = strdup(buf);
	if (sprops->user_dir == NULL ) {
	    goto out_of_memory;
	}
    }

    sprops->file_separator = "/";
    sprops->path_separator = ";";
    sprops->line_separator = "\n";

    return CVM_TRUE;

 out_of_memory:
    CVMreleaseJavaProperties(sprops);
    return CVM_FALSE;
}

/*
 * Free up memory allocated by CVMgetJavaProperties().
 */
void CVMreleaseJavaProperties(java_props_t *sprops)
{
    if (sprops->user_dir != NULL) {
	free((char*)sprops->user_dir);
    }
}
