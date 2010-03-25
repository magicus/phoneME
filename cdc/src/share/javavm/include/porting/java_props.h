/*
 * @(#)java_props.h	1.18 06/10/10
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

#ifndef _JAVA_PROPS_H
#define _JAVA_PROPS_H

typedef struct {
    const char *os_name;
    const char *os_version;
    const char *os_arch;

    const char *tmp_dir;
    const char *font_dir;
    const char *user_dir;

    const char *file_separator;
    const char *path_separator;
    const char *line_separator;

    const char *user_name;
    const char *user_home;

    const char *language;
    const char *encoding;
    const char *region;
    const char *timezone;

    const char *printerJob;
    const char *graphics_env;
    const char *awt_toolkit;

    const char *unicode_encoding;      /* The default endianness of unicode
					  i.e. UnicodeBig or UnicodeLittle  */

    const char *cpu_isalist;           /* list of supported instruction sets */

    const char *cpu_endian;            /* endianness of platform */
    const char *commports;	       /* comma-delimited list of available
					  comm (serial) ports */

#ifdef JAVASE    
    const char *util_prefs_PreferencesFactory; /* system's Preferences Factory */

    const char *data_model;           /* 32 or 64 bit data model */
 
    const char *patch_level;          /* patches/service packs installed */

    const char *country;
    const char *variant;
    const char *sun_jnu_encoding;
#endif

} java_props_t;

/*
 * Get platform dependent system properties
 */
CVMBool CVMgetJavaProperties(java_props_t *props);
void CVMreleaseJavaProperties(java_props_t *props);

#endif /* _JAVA_PROPS_H */
