/*
 * @(#)jamUtil.cpp	1.13 06/10/10 10:08:36
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

#include "jam.hpp"
#include <ctype.h>

char *
GetCharsOfLong64(int64 msec) {
#ifdef __SYMBIAN32__
    return "0";
#else
    static char buff[200];

    char * p = buff + sizeof(buff) - 1;
    *--p = 0;
    while (msec > 0) {
      *--p = (char)(msec % 10) + '0';
      msec /= 10;
    }
    return p;
#endif
}

int64
ParseInt64(char* str, int len) {
  int64 res = 0;
  int64 pwr = 1;
  char* p = str+len;
  do {
    if (!isdigit(*p)) continue;
    res += pwr*(*p-'0');
    pwr *= 10;
  } while (p-- != str);
  return res;
}

char *
PlafCurrentTimeMillisStr() {
    return GetCharsOfLong64(PlafCurrentTimeMillis());
}

static void
parseVmArgs(char * str) {
    char *p, * q;
    int n=0, len;

    // p points to the start of an arg
    // q points to just past the end of an arg
    for (p=q=str; *q; q++) {
        if (*q == ' ') {
            len = q - p;
            if (len > 1) {
                char *arg = (char*)jam_malloc(len + 1);
                strnzcpy(arg, p, len);
                g_vmargs[n++] = arg;
            }
            p = q+1;
        }
    }

    len = q - p;
    if (len > 1) {
        char *arg = (char*)jam_malloc(len + 1);
        strnzcpy(arg, p, len);
        g_vmargs[n++] = arg;
    }

    g_vmargs[n++] = NULL;
}

static void
usage() {
    printf(PlafGetUsage());
    exit(1);
}

void
ParseCmdArgs(int argc, char **argv) {
    int i;

    for (i=1; i<argc; ) {
        if (strcmp(argv[i], "-repeat") == 0) {
            g_repeat = 1;
            i++;
        }
        else if (strcmp(argv[i], "-url") == 0 && (i+1)<argc) {
            g_url = argv[i+1];
            i += 2;
        }
        else if (strcmp(argv[i], "-vm") == 0 && (i+1)<argc) {
            g_vm = argv[i+1];
            i += 2;
        }
        else if (strcmp(argv[i], "-tempfile") == 0 && (i+1)<argc) {
            g_tempfile = argv[i+1];
            i += 2;
        }
        else if ((strcmp(argv[i], "-bootclasspath") == 0 ||
                  strcmp(argv[i], "-bp") == 0) && (i+1)<argc) {
            g_bootclasspath = argv[i+1];
            i += 2;
        }
        else if ((strcmp(argv[i], "-vmargs") == 0 ||
                  strcmp(argv[i], "-J") == 0) && (i+1)<argc) {
            parseVmArgs(argv[i+1]);
            i += 2;
        } 
	else if ((strcmp(argv[i], "-timeout") == 0 ||
                  strcmp(argv[i], "-T") == 0) && (i+1)<argc) {
	  g_timeout = atoi(argv[i+1]);
	  i += 2;
        } else if ((strcmp(argv[i], "-midp") == 0 ||
		    strcmp(argv[i], "-M") == 0)) {
	  g_midpMode = 1;
	  i += 1;
        }  else if ((strcmp(argv[i], "-type") == 0 ||
		      strcmp(argv[i], "-Y") == 0) && (i+1)<argc) {	 
	  for (int j=0; j<VM_TYPE_UNKNOWN; j++) {
	    if (!strcmp(g_opt_info[j].typeStr, argv[i+1])) {
	      g_vm_type = j;
	      if ((g_vm_type == VM_TYPE_MIDP) || 
		  (g_vm_type == VM_TYPE_MIDP2)) {
		g_midpMode = 1;
	      }

	    }
	  }
	  if (g_vm_type < 0) { 
	    printf("Unknown JVM type: \"%s\", supported types are: \n  ", argv[i+1]);
	    for (int j=0; j<VM_TYPE_UNKNOWN; j++)  
	      printf("%s ", g_opt_info[j].typeStr);
	    printf("\n\n");
	    exit(1);
	  }
	  i += 2;
	} else if ((strcmp(argv[i], "-standalone") == 0 ||
		    strcmp(argv[i], "-S") == 0)) {
	  g_standaloneMode = 1;
	  i += 1;
	} else {	  
	  printf("unknown parameter: %s\n", argv[i++]);
	}
    }

    if (g_url == NULL || g_vm == NULL) {
        usage();
    }
    if (g_vm_type<0) {
      //printf("defaulting VM type to UNKNOWN (JDK-like options)\n");
      g_vm_type = VM_TYPE_UNKNOWN;
    }
}

#ifdef _WIN32_

typedef unsigned __int64 ulong64;

#define FT2INT64(ft) \
    ((ulong64)(ft).dwHighDateTime << 32 | (ulong64)(ft).dwLowDateTime)

int64
PlafCurrentTimeMillis() {
    static ulong64 offset = 0;    

    if (offset == 0) {
      SYSTEMTIME st0, st1;
      FILETIME   ft0, ft1;
      ulong64 t;

      memset(&st0, 0, sizeof(st0));
      st0.wYear  = 1970;
      st0.wMonth = 1;
      st0.wDay   = 1;
      SystemTimeToFileTime(&st0, &ft0);
      GetSystemTime(&st1);
      t = GetTickCount();
      SystemTimeToFileTime(&st1, &ft1);
      
      offset = (FT2INT64(ft1) - FT2INT64(ft0))/10000 - t;  
    }

    return GetTickCount() + offset;
}

#endif /* WIN32 */

