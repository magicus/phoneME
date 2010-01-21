/*
 * @(#)java_md.c  1.17 06/10/10
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

#include <windows.h>
#include <stdio.h>
#include <errno.h>

#include "javavm\include\winntUtil.h"
#ifndef CVM_DLL
#define _JNI_IMPLEMENTATION_
#include "jni.h" /* for JNI_CreateJavaVM */
#endif

#include "portlibs/ansi_c/java.h"

#include <tchar.h>
#include <stdlib.h>

#ifdef ENABLE_SPLASH_SCREEN
#include "splash.h"
#define SHOW_SPLASH() showSplash()
#define HIDE_SPLASH() hideSplash()
#else
#define SHOW_SPLASH()
#define HIDE_SPLASH()
#endif

#define GET_NEXT0(p1, p2) \
    ((p1 == NULL) ? p2 : ((p2 == NULL) ? p1 : (min(p1, p2))))

static char* getNext(char* p)
{
    char *p1, *p2;
    p1 = GET_NEXT0(strchr(p, ' '), strchr(p, '\t'));
    p2 = GET_NEXT0(strchr(p, '\n'), strchr(p, '\r'));
    return GET_NEXT0(p1, p2);
}

char** convertArgs(int argc, const char** argv_a)
{
    char **argv;
    int argi;
    argv = malloc(argc * sizeof(char*));
    for (argi=0; argi<argc; ++argi) {
        argv[argi] = createU8fromA(argv_a[argi]);
    }
    return argv;
}

void freeArgs(int argc, char** argv)
{
    int i;
    for (i=0; i<argc; ++i) {
        free(argv[i]);
    }
    free(argv);
}

static int
ansiJavaMain0(int argc, const char** argv, JNI_CreateJavaVM_func* f)
{
    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            const char* filename = argv[i+1];
            long        fileSize;
            char*       argsBuf;
            FILE*       fd;
            
            fd = fopen(filename, "rb");
            if (fd == NULL) {
#if 0
                perror("fopen");
#endif
                fprintf(stderr, "Could not open \"%s\"", filename);
                return errno;
            }

            if (fseek(fd, 0, SEEK_END)) {
#if 0
                perror("fseek");
#endif
                fprintf(stderr, "Could not stat \"%s\"", filename);
                return errno;
            }
            fileSize = ftell(fd);
            fseek(fd, 0, SEEK_SET);
            
            argsBuf = malloc(fileSize+1);
            if (argsBuf == NULL) {
                fprintf(stderr, "Could not allocate \"-f\" args buffer\n");
                return errno;
            }

            if (fread(argsBuf, sizeof(char), fileSize, fd) != fileSize) {
                fprintf(stderr, "Could not read entire \"-f\" file");
                return 1;
            }
            argsBuf[fileSize] = '\000';

            {
                const char** newArgv;
                int    newArgc;
                int j;
                char* p = argsBuf;
                int numFileArgs = 0;
                
                /* count the number of arguments from the file */
                while (p != NULL && *p != '\000') {
                    switch(*p) {
                        case ' ':
                        case '\t':
                        case '\n':
                        case '\r':
                            p++; /* skip */
                            break;
                        default: {
                            numFileArgs++;
                            if (*p == '"') {
                                p = strchr(++p, '"');
                            } else {
                                p = getNext(p);
                            }
                            if (p != NULL) {
                                p++;
                            }
                            break;
                        }
                    }
                }
                newArgc = argc - 2 + numFileArgs;
                newArgv = (const char**)malloc(newArgc*sizeof(char*));
                
                if (newArgv == NULL) {
                    fprintf(stderr,
                            "Could not allocate \"-f\" newArgc buffer\n");
                    return errno;
                }

                /* get the arguments from command line */
                for (j = 0; j < i; j++) {
                    newArgv[j] = argv[j];
                }

                /* skip the -f argument from command line */
                for (j = i + 2; j < argc; j++) {
                    newArgv[j - 2 + numFileArgs] = argv[j];
                }

                /* now get the arguments from the file */
                p = argsBuf;
                while (p != NULL && *p != '\000') {
                    switch (*p) {
                    case ' ':
                    case '\t':
                    case '\n':
                    case '\r':
                        p++; /* skip */
                        break;
                    default:
                        if (*p == '"') {
                            newArgv[i++] = ++p;
                            p = strchr(p, '"');
                        } else {
                            newArgv[i++] = p;
                            p = getNext(p);
                        }
                        if (p != NULL) {
                            *p = '\0';
                            p++;
                        }
                        break;
                    }
                }
                assert(i == newArgc);
#if 0
                for (j = 0; j < newArgc; j++) {
                    fprintf(stderr,"%d: %s\n", j, newArgv[j]);
                }
#endif

                {
                    char** newArgv_u;
                    int result;
                    newArgv_u = convertArgs(newArgc, newArgv);
                    result = ansiJavaMain(newArgc, newArgv_u, f);
                    free(argsBuf);
                    freeArgs(newArgc, newArgv_u);
                    return result;
                }
            }
        } /* if "-f" */
    } /* for */
    {
        char** argv_u;
        int result;
        argv_u = convertArgs(argc, argv);
        result = ansiJavaMain(argc, argv_u, f);
        freeArgs(argc, argv_u);
        return result;
    }
}

#ifdef CVM_DLL
static HMODULE
loadCVM()
{
    TCHAR path[256];
    HMODULE h;
    TCHAR *p0, *p1;

    GetModuleFileName(0, path, sizeof path);

    p0 = _tcsrchr(path, _T('\\'));
    if (p0 != NULL) {
        _tcscpy(&p0[1], TEXT("cvmi.dll"));
        h = LoadLibrary(path);
    } else {
        h = NULL;
    }

 done:
    if (h == NULL) {
        fprintf(stderr, "LoadLibrary failed for cvmi.dll (0x%x)\n",
                GetLastError());
    }
    return h;
}
#endif /* CVM_DLL */

#include <excpt.h>

static DWORD
getpc(struct _EXCEPTION_POINTERS* ep, DWORD* addr)
{
    *addr = (DWORD)ep->ExceptionRecord->ExceptionAddress;
#ifdef ARM
    return ep->ContextRecord->Pc;
#endif
#ifdef _X86_
    return ep->ContextRecord->Eip;
#endif
}


#include <EXCPT.H>

#ifndef WINCE

#define parse_argv(argc, argv) argv
#define free_parsed_argv(argc, argv)

#else

static char** parse_argv(int* argc, char** argv) {
    
    int argv_i, new_argv_i, new_argv_len, new_argc = *argc;
    char** new_argv; /* new argument array to pass back to the cvm */
    
    new_argv  = (char**) malloc(sizeof(char*) * (*argc));
    
    /* get the first argument, which is the executable */
    new_argv[0] = _strdup(argv[0]);
    
    for (argv_i = 0, new_argv_i = 1;
         argv_i < (*argc - 1);
         argv_i++, new_argv_i++) {
        int argv_len, argv_pos, new_argv_pos, consecutive_slashes=0;
        boolean quote_delimiter=FALSE, quote_literal=FALSE;
        
        argv_len = strlen(argv[argv_i+1]);
        new_argv[new_argv_i] = (char*)malloc(sizeof(char) * (argv_len + 1));
        new_argv[new_argv_i][0] = '\0';

        for (argv_pos = 0, new_argv_pos = 0; argv_pos < argv_len; argv_pos++) {
            char** temp_new_argv;
            int slash_count, count;
            
            /* a '\\' is found */
            if (argv[argv_i+1][argv_pos] == '\\') {
                consecutive_slashes++;
            } else if ((argv[argv_i+1][argv_pos] == ' ') ||
                       (argv[argv_i+1][argv_pos] == '\t')) {
                /* if it gets here, the wince didn't split argv */
                if (quote_delimiter) {
                    new_argv[new_argv_i][new_argv_pos] =
                        argv[argv_i+1][argv_pos];
                    new_argv_pos++;
                    consecutive_slashes = 0;
                } else {
                    /* check for consecutive delimiters */
                    if (argv[argv_i+1][argv_pos+1] == ' ' ||        \
                        argv[argv_i+1][argv_pos+1] == '\t') {
                        continue;
                    }
                    new_argv_len = strlen(&(argv[argv_i+1][argv_pos+1]));
                    if (new_argv_len <= 0) {
                        continue;
                    }
                    new_argv_i++;
                    /* we need to malloc more memory because wince didn't
                       calculate argc correctly. So, we malloc the
                       temp_new_argv array, copy the old one to the new one,
                       and set new_argv to it. */
                    new_argc++;
                    temp_new_argv = (char**)malloc(sizeof(char*) * new_argc);
                    for (count = 0; count < new_argv_i; count++) {
                        temp_new_argv[count] = _strdup(new_argv[count]);
                        free(new_argv[count]);
                    }

                    free(new_argv);
                    new_argv = temp_new_argv;

                    /* malloc the new argument */
                    new_argv[new_argv_i] =
                        (char*)malloc(sizeof(char) * new_argv_len);
                    new_argv_pos = 0;
                    new_argv[new_argv_i][new_argv_pos] = '\0';
                    quote_literal = quote_delimiter = FALSE;
                }
            } else if (argv[argv_i+1][argv_pos] == '\"') {
                if (consecutive_slashes) {
                    if (consecutive_slashes % 2 == 0) {
                        /* even number, quote delimiter */
                        for (slash_count = 0;
                             slash_count < (consecutive_slashes / 2);
                             slash_count++, new_argv_pos++) {
                            new_argv[new_argv_i][new_argv_pos] = '\\';
                        }
                        quote_delimiter=TRUE;
                    } else {
                        /* odd number, literal quote */
                        for (slash_count = 0;
                             slash_count < (consecutive_slashes / 2);
                             slash_count++, new_argv_pos++) {
                            new_argv[new_argv_i][new_argv_pos] = '\\';
                        }
                        new_argv[new_argv_i][new_argv_pos] = '\"';
                        new_argv_pos++;
                        quote_literal=TRUE;
                    }
                } else if (quote_delimiter) {
                    quote_delimiter = FALSE;
                } else if (!quote_delimiter) {
                    quote_delimiter = TRUE;
                }
                consecutive_slashes = 0;

            } else {
                /* slashes that don't end with a double quote */
                for (slash_count = 0; slash_count < consecutive_slashes; \
                     slash_count++, new_argv_pos++) {
                    new_argv[new_argv_i][new_argv_pos] = '\\';
                }
                new_argv[new_argv_i][new_argv_pos] = argv[argv_i+1][argv_pos];
                new_argv_pos++;
                new_argv[new_argv_i][new_argv_pos] = '\0';
                consecutive_slashes = 0;
            }
        } /* for loop */
    } /* for loop */

    *argc = new_argc;
    return new_argv;
}

static void free_parsed_argv(int argc, char** argv) {
    int i;

    for (i = 0; i < argc; i++) {
        free (argv[i]);
    }

    free(argv);
}
#endif

/*
 * Macro to handle differences between wince and win32 in the handling
 * of GetProcAddress parameters. It would be useful to move this to a
 * header file so it can be used elsewhere.
 */
#ifdef WINCE
#define GET_PROC_ADDRESS(h, f) GetProcAddress(h, TEXT(f))
#else
#define GET_PROC_ADDRESS(h, f) GetProcAddress(h, f)
#endif

int main(int argc, char* argv[])
{
    int retCode;
    JNI_CreateJavaVM_func* JNI_CreateJavaVMFunc;

    SHOW_SPLASH();

#ifdef CVM_DLL
    {
        HMODULE h = loadCVM();
        JNI_CreateJavaVMFunc = (JNI_CreateJavaVM_func*)
            GET_PROC_ADDRESS(h, "JNI_CreateJavaVM");
    }
#else
    JNI_CreateJavaVMFunc = JNI_CreateJavaVM;
#endif

    if (JNI_CreateJavaVMFunc == NULL) {
        retCode = GetLastError();
        fprintf(stderr, "GetProcAddress(\"JNI_CreateJavaVM\") failed (0x%x)\n",
                retCode);
    } else {
        char** parsed_argv = parse_argv(&argc, argv);
#ifndef CVM_DEBUG
        retCode = ansiJavaMain0(argc, parsed_argv, JNI_CreateJavaVMFunc);
#else
        {
            DWORD pc = 0, addr = 0;
            __try {
                retCode =
                    ansiJavaMain0(argc, parsed_argv, JNI_CreateJavaVMFunc);
            } __except (pc = getpc(_exception_info(), &addr),
                        EXCEPTION_EXECUTE_HANDLER) {
                retCode = _exception_code();
                fprintf(stderr,
                        "exception %x in main thread at pc %x addr %x\n",
                    retCode, pc, addr);
            }
        }
#endif
        free_parsed_argv(argc, parsed_argv);
    }
    HIDE_SPLASH();
    return retCode;
}

#if 0
/*
  This is old code for the main entry point that we no longer use.
*/

extern char** __argv;
extern int  __argc;

static char* cdcName = "cvm.exe";

int WINAPI
_tWinMain(HINSTANCE inst, HINSTANCE previnst, TCHAR* cmdline, int cmdshow) {
    int i = 0;
    char* sz;
    HMODULE h;
    JNI_CreateJavaVM_func* JNI_CreateJavaVMFunc;
    TCHAR path[256];
    TCHAR *p0, *p1;
    h = loadCVM();
    JNI_CreateJavaVMFunc = (JNI_CreateJavaVM_func*)
        GET_PROC_ADDRESS(h, "JNI_CreateJavaVM");

    printf("WinMain\n");

    if (cmdline != NULL) {
        int dwLen = _tcslen(cmdline);
        sz = (char*)malloc(dwLen + 1);
        wcstombs(sz, cmdline, dwLen);

        /* count argc */
        {
            char* p = sz;
            char* as = 0;
            int len = 0;
            
            /* count argc */
            while (*p != 0) {
                /* skip white space */
                while ((*p != 0) && (*p == ' ' || *p == '\t'))  p++;
                if (*p == 0)
                    break;
                
                as = p;
                /* search white space to determine args */
                while ((*p != 0) && (*p != ' ' && *p != '\t'))  p++;
                
                __argc++;
            }
        }

        /* alloate argument */
        __argc++;
        __argv = (char**)malloc(sizeof(char*) * __argc);
        __argv[0] = cdcName;
        {
            char* p = sz;
            char* as = 0;
            int len = 0;
            int argc = 1;
            
            /* count argc */
            while (*p != 0) {
                /* skip white space */
                while ((*p != 0) && (*p == ' ' || *p == '\t'))  p++;
                if (*p == 0)
                    break;
                
                as = p;
                /* search white space to determine args */
                while ((*p != 0) && (*p != ' ' && *p != '\t'))  p++;
                
                len =  p - as;
                __argv[argc] = (char*)malloc(sizeof(char) * (len + 1));
                memcpy(__argv[argc], as, len);
                __argv[argc][len] = 0;
                argc++;
            }
        }
        free(sz);
    }
    return ansiJavaMain(__argc, __argv, JNI_CreateJavaVMFunc);
}
#endif
