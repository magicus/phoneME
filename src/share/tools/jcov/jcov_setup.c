/*
 * @(#)jcov_setup.c	1.17 06/10/10
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

#include "javavm/include/porting/ansi/stdio.h"
#include "javavm/include/porting/ansi/string.h"

#include "jvmpi.h"
#include "jcov.h"
#include "jcov_types.h"
#include "jcov_setup.h"
#include "jcov_error.h"
#include "jcov_htables.h"
#include "jcov_hash.h"
#include "jcov_events.h"
#include "jcov_util.h"
#include "jcov_file.h"

#define OPT_DELIM     ','
#define OPT_VAL_DELIM '='

char jcov_data_type;
char *jcov_file = "java.jcov";
jcov_filter_t *class_filter;
jcov_filter_t *caller_filter;
int verbose_mode;
Bool include_abstracts;
Bool overwrite_jcov_file;
Bool load_early_classes;

static FILE *opt_file;
static char *cur_opt;

const char *COV_TABLE_ATTR_NAME         = "CoverageTable";
const char *CRT_TABLE_ATTR_NAME         = "CharacterRangeTable";
const char *SRC_FILE_ATTR_NAME          = "SourceFile";
const char *ABS_SRC_PATH_ATTR_NAME      = "AbsoluteSourcePath";
const char *TIMESTAMP_ATTR_NAME         = "TimeStamp";
const char *COMPILATION_ID_ATTR_NAME    = "CompilationID";
const char *LINE_NUMBER_TABLE_ATTR_NAME = "LineNumberTable";
const char *CODE_ATTR_NAME              = "Code";

const char *OPT_HELP[]         = { "help",             "h"  };
const char *OPT_INCLUDE[]      = { "include",          "i"  };
const char *OPT_EXCLUDE[]      = { "exclude",          "e"  };
const char *OPT_TYPE[]         = { "type",             "t"  };
const char *OPT_FILE[]         = { "file",             "f"  };
const char *OPT_CALLER_INCL[]  = { "caller_include",   "ci" };
const char *OPT_CALLER_EXCL[]  = { "caller_exclude",   "ce" };
const char *OPT_ABSTR_METH[]   = { "abstract_methods", "am" };
const char *OPT_VERBOSITY[]    = { "verbosity",        "v"  };
const char *OPT_OPTIONS_FILE[] = { "options_file",     "of" };
const char *OPT_OVERWRITE[]    = { "overwrite",        "o"  };
const char *OPT_EC[]           = { "early_classes",    "ec" };
const char *OPT_ON[]           = { "on",               "y"  };

const char *JVM_ENABLE_INSTR_START    = "-XX:+EnableJVMPIInstructionStartEvent";
const char *JVM_DISABLE_FAST_EMPTY    = "-XX:-UseFastEmptyMethods";
const char *JVM_DISABLE_FAST_ACCESSOR = "-XX:-UseFastAccessorMethods";

static void jcov_usage(void)
{
    jcov_close(&opt_file);
    printf("Jcov usage:\n");
    printf("\t-Xrunjcov[:help]|[<option>=<value>, ...]\n\n");
    printf("Where option is one of the following (short option names are given in '()'):\n");
    printf("\t%s(%s)=<class name prefix>\n", OPT_INCLUDE[0], OPT_INCLUDE[1]);
    printf("\t\tonly profile classes whose names start with the specified prefix\n");
    printf("\t%s(%s)=<class name prefix>\n", OPT_EXCLUDE[0], OPT_EXCLUDE[1]);
    printf("\t\tdon't profile classes whose names start with the specified prefix\n");
    printf("\t%s(%s)=['B'|'M']\n", OPT_TYPE[0], OPT_TYPE[1]);
    printf("\t\tspecifies save file data format, default - B\n");
    printf("\t%s(%s)=<filename>\n", OPT_FILE[0], OPT_FILE[1]);
    printf("\t\tspecifies savefile name, default - 'java.jcov'\n");
    printf("\t%s(%s)=<class name prefix>\n", OPT_CALLER_INCL[0], OPT_CALLER_INCL[1]);
    printf("\t\tgather coverage data only for methods invoked from the specified\n");
    printf("\t\tclass or classes beloning to the specified package;\n");
    printf("\t\tthe option can be used multiple times;\n");
    printf("\t\tclass or package names should use '.' as a package separator\n");
    printf("\t%s(%s)=<class name prefix>\n", OPT_CALLER_EXCL[0], OPT_CALLER_EXCL[1]);
    printf("\t\tgather coverage data only for methods invoked *not* from the specified\n");
    printf("\t\tclass or classes *not* beloning to the specified package;\n");
    printf("\t\tthe option can be used multiple times;\n");
    printf("\t\tclass or package names should use '.' as a package separator\n");
    printf("\t%s(%s)=<on|off>\n", OPT_ABSTR_METH[0], OPT_ABSTR_METH[1]);
    printf("\t\tstore/do not store jcov data for abstract methods; default is 'off'\n");
    printf("\t%s(%s)=<on|off>\n", OPT_OVERWRITE[0], OPT_OVERWRITE[1]);
    printf("\t\toverwrite jcov data file; default is 'off' (jcov data is merged)\n");
    printf("\t%s(%s)=<filename>\n", OPT_OPTIONS_FILE[0], OPT_OPTIONS_FILE[1]);
    printf("\t\tspecifies where to read options from; (caller_)include/(caller_)exclude\n");
    printf("\t\toptions values read from the file are added to those, specified in the\n");
    printf("\t\tcommand line\n");
    printf("\t%s(%s)=<on|off>\n", OPT_EC[0], OPT_EC[1]);
    printf("\t\ttry to workaround JVMPI bug which prevents the agent from gathering\n");
    printf("\t\tcoverage info for some \"early\" system classes loaded before the agent;\n");
    printf("\t\tdefault is 'off' (don't workaround)\n");
    printf("\nExamples (JDK 1.3.1, 1.4):\n\n");
    printf("1) java -Xrunjcov:%s=java,%s=java.lang,", OPT_INCLUDE[0], OPT_EXCLUDE[0]);
    printf("%s=java.io,%s=M,%s=sys.jcov <main class>\n", OPT_EXCLUDE[0], OPT_TYPE[0], OPT_FILE[0]);
    printf("2) java %s -Xrunjcov <main class>\n", JVM_ENABLE_INSTR_START);
    printf("\nNote that in JDK 1.4 some additional JVM options should be used to get\n");
    printf("coverage data for empty methods and accessor methods:\n");
    printf("%s %s\n\n", JVM_DISABLE_FAST_EMPTY, JVM_DISABLE_FAST_ACCESSOR);

    CALL(ProfilerExit)((jint)0);
}
             
#define ERROR_AND_EXIT(msg) \
    jcov_close(&opt_file); \
    jcov_error_stop(msg)

static int read_line(char *buf, FILE *f) {
    int ch;
    char *tmp = buf;
    
    do {
        ch = getc(f);
    } while (ch != EOF && (ch == '\n' || ch == '\r'));
    
    if (ch == EOF) {
        return 0;
    }
    while (ch != '\n' && ch != '\r' && ch != EOF) {
        if (tmp - buf >= MAX_PATH_LEN - 1) {
            ERROR_AND_EXIT("Too long option encountered");
        }
        *(tmp++) = (char)ch;
        ch = getc(f);
    }
    *(tmp++) = '\0';
    return 1;
}

static char *extract_option(char *opt_name, char *opt_val, char *src) {
    char *tmp;

    tmp = strchr(src, OPT_VAL_DELIM);
    if (tmp == NULL) {
        printf("Invalid option : %s\n", src);
        jcov_usage();
    }
    strncpy(opt_name, src, tmp - src);
    opt_name[tmp - src] = '\0';
    src = tmp + 1;
    tmp = strchr(src, OPT_DELIM);
    if (tmp == NULL) {
        strcpy(opt_val, src);
        return src + strlen(src);
    } else {
        strncpy(opt_val, src, tmp - src);
        opt_val[tmp - src] = '\0';
        return tmp + 1;
    }
}

static int read_option_cmdline(char *opt_name, char *opt_val) {
    if (strlen(cur_opt) == 0)
        return 0;
    cur_opt = extract_option(opt_name, opt_val, cur_opt);
    return 1;
}

static int read_option_disk(char *opt_name, char *opt_val) {
    char buf[MAX_PATH_LEN*2];
    if (!read_line(buf, opt_file))
        return 0;
    extract_option(opt_name, opt_val, buf);
    return 1;
}

#define ADD_FILTER(filter, op) \
    if (filter == NULL) \
        filter = jcov_filter_new(); \
    while ((tmp = (char*)strchr(opt_val, '.'))) { \
        *tmp = '/'; \
    } \
    filter_add_##op(filter, opt_val)

static int opt_cmp(char *opt_name, const char **opt) {
    if (!strcmp(opt_name, opt[0]))
        return 0;
    return strcmp(opt_name, opt[1]);
}

static void parse_options(char *options) {
    char *tmp;
    int (*read_option)(char *, char *);
    char opt_name[MAX_PATH_LEN];
    char opt_val[MAX_PATH_LEN];
    int i;

    if (options == NULL)
        return;

    if ((opt_cmp(options, OPT_HELP)) == 0) {
        jcov_usage();
    }
    
    cur_opt = options;
    read_option = read_option_cmdline;
    
    while (1) {
        if (read_option(opt_name, opt_val) == 0) {
            jcov_close(&opt_file);
            if (read_option == read_option_disk)
                read_option = read_option_cmdline;
            else
                break;
        } else if (opt_cmp(opt_name, OPT_OPTIONS_FILE) == 0) {
            if ((opt_file = fopen(opt_val, "rb")) == NULL) {
                sprintf(opt_name,"cannot open file : %s\n", opt_val);
                jcov_error_stop(opt_name);
            }
            read_option = read_option_disk;
        } else if (opt_cmp(opt_name, OPT_FILE) == 0) {
            jcov_file = (char*)jcov_calloc(strlen(opt_val) + 1);
            strcpy(jcov_file, opt_val);
        } else if (opt_cmp(opt_name, OPT_TYPE) == 0) {
            jcov_data_type = opt_val[0];
            if (jcov_data_type == 'M' || jcov_data_type == 'm')
                jcov_data_type = JCOV_DATA_M;
            else if (jcov_data_type == 'B' || jcov_data_type == 'b')
                jcov_data_type = JCOV_DATA_B;
            else {
                printf("Invalid data type : %c\n", jcov_data_type);
                jcov_usage();
            }
        } else if (opt_cmp(opt_name, OPT_ABSTR_METH) == 0) {
            if (opt_cmp(opt_val, OPT_ON) == 0)
                include_abstracts = 1;
        } else if (opt_cmp(opt_name, OPT_OVERWRITE) == 0) {
            if (opt_cmp(opt_val, OPT_ON) == 0)
                overwrite_jcov_file = 1;
        } else if (opt_cmp(opt_name, OPT_EC) == 0) {
            if (opt_cmp(opt_val, OPT_ON) == 0)
                load_early_classes = 1;
        } else if (opt_cmp(opt_name, OPT_VERBOSITY) == 0) {
            if (strlen(opt_val) > 1) {
                jcov_usage();
            }
            i = opt_val[0] - '0';
            if (i < 0 || i > 9) {
                jcov_usage();
            }
            verbose_mode = i;
        } else if (opt_cmp(opt_name, OPT_INCLUDE) == 0) {
            ADD_FILTER(class_filter, incl);
        } else if (opt_cmp(opt_name, OPT_EXCLUDE) == 0) {
            ADD_FILTER(class_filter, excl);
        } else if (opt_cmp(opt_name, OPT_CALLER_INCL) == 0) {
            ADD_FILTER(caller_filter, incl);
        } else if (opt_cmp(opt_name, OPT_CALLER_EXCL) == 0) {
            ADD_FILTER(caller_filter, excl);
        } else {
            printf("Unrecognized option : %s\n", opt_name);
            jcov_usage();
        }
    }
}

void jcov_init(char *jcov_options) {
    jcov_data_type = JCOV_DATA_B;
    class_filter = NULL;
    caller_filter = NULL;
    verbose_mode = 0;
    include_abstracts = 0;
    overwrite_jcov_file = 0;
    load_early_classes = 0;
    opt_file = NULL;

    parse_options(jcov_options);
    
    filter_finalize(class_filter);
    filter_finalize(caller_filter);

    class_key_table = jcov_hash_new(997,  hash_class_key, size_class,  cmp_class_key);
    class_id_table  = jcov_hash_new(997,  hash_class_id,  size_class,  cmp_class_id);
    method_table    = jcov_hash_new(2999, hash_method,    size_method, cmp_method);

    if (load_early_classes) {
        class_filt_table = jcov_hash_new(997,  hash_classID, size_classID, cmp_classID);
    }

    jcov_util_init();
    jcov_htables_init();
    jcov_events_init();
    jcov_file_init();
}
