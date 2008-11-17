/*
 * @(#)jcov_setup.h	1.13 06/10/10
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

#ifndef _JCOV_SETUP_H
#define _JCOV_SETUP_H

#include "jcov_types.h"
#include "jcov_hash.h"
#include "jcov_util.h"

#define JCOV_DATA_B 'B'
#define JCOV_DATA_M 'M'
#define JCOV_DATA_A 'A'
#define JCOV_DATA_C 'C'
#define JCOV_SKIP_CLASS '-'

#define CALLER_MATCH 0
#define CALLER_MISMATCH 1
#define LOADING_EARLY_CLASS 2

extern char jcov_data_type;
extern char *jcov_file;
extern jcov_filter_t *class_filter;
extern jcov_filter_t *caller_filter;
extern int verbose_mode;
extern Bool include_abstracts;
extern Bool overwrite_jcov_file;
extern Bool load_early_classes;
extern const char *COV_TABLE_ATTR_NAME;
extern const char *CRT_TABLE_ATTR_NAME;
extern const char *SRC_FILE_ATTR_NAME;
extern const char *ABS_SRC_PATH_ATTR_NAME;
extern const char *TIMESTAMP_ATTR_NAME;
extern const char *COMPILATION_ID_ATTR_NAME;
extern const char *LINE_NUMBER_TABLE_ATTR_NAME;
extern const char *CODE_ATTR_NAME;
extern const char *DUMMY_SRC_PREF;
extern const char *DUMMY_SRC_SUFF;
extern const char *JVM_ENABLE_INSTR_START;

void jcov_init(char *jcov_options);

#endif /* _JCOV_SETUP_H */
