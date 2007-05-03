/*
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

/*=========================================================================
 * SYSTEM:    Verifier
 * SUBSYSTEM: main program
 * FILE:      main.c
 * OVERVIEW:  Runs the Java class verifier.
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#include <sys_api.h>
#include <path_md.h>
#include <path.h>
#include <oobj.h>
#include <jar.h>
#include <locale_md.h>

/*=========================================================================
 * Globals and extern declarations
 *=======================================================================*/

char str_buffer[STRINGBUFFERSIZE];   /*  shared string buffer */

int processedfile = 0;
int errorCode = 0;               /* Error code returned by program */

bool_t no_native_methods = FALSE;
bool_t no_floating_point = FALSE;
bool_t no_finalizers = FALSE;
char tmp_dir[32];                /* temporary directory */
extern char *output_dir;         /* output directory */
bool_t tmpDirExists = FALSE;

extern void VerifyFile(register char *fn);
extern bool_t ProcessJARfile(char *buf, int len);
extern bool_t isJARfile(char *fn, int length);

static void usage(char *progname);

/*=========================================================================
 * FUNCTION:      recurse_dir()
 * TYPE:          Handles recursive directories
 * OVERVIEW:      Internal function called by ProcessInputs().
 *
 *  This function reads a directory, searching for either another directory,
 *  JAR file or an individual class name that is to be verified.
 *
 * INTERFACE:
 *   parameters:  dirname   name of the directory entry.
 *                pkgname   name of the package
 *   returns:     nothing
 *=======================================================================*/
static void recurse_dir(char *dirname, char *pkgname)
{
    struct dirent *ent;
        char buf[MAXPACKAGENAME];
        char pkgbuf[MAXPACKAGENAME];
    DIR *dir = opendir(dirname);

    if (dir == NULL) {
        fprintf(stderr, "Can't open dir %s\n", dirname);
        exit(1);
    }
    for (ent = readdir(dir); ent; ent = readdir(dir)) {
        struct stat stat_buf;
        char *name = ent->d_name;
        int len;

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        strcpy(pkgbuf, pkgname);
        if (pkgname[0] != 0) {
            strcat(pkgbuf, "/");
        }

                if (JAR_DEBUG && verbose)
                    jio_fprintf(stderr,
            "recurse_dir: Reading filename [%s] from directory [%s]\n",
                        name, dirname);

        strcat(pkgbuf, name);

        strcpy(buf, dirname);
        strcat(buf, name);

        stat(buf, &stat_buf);
        len = strlen(buf);

        if (stat_buf.st_mode & S_IFDIR) {
            /* handle the recursive directory found */
            strcat(buf, "/");
            if (JAR_DEBUG && verbose)
                jio_fprintf(stderr,
                   "recurse_dir: Recursive directory found, calling recurse_dir ([%s] [%s])\n",
                       buf, pkgbuf);
            recurse_dir(buf, pkgbuf);
            continue;
        } else if (isJARfile (buf, len)) {

            /*
                     * this directory contains a JAR file which contains
                     * the classes to be verified
                     */

                    if (JAR_DEBUG && verbose)
                    jio_fprintf(stderr,
                     "recurse_dir: Found JAR file [%s] in dir!\n", buf);

                if (!ProcessJARfile(buf, len)) {

                fprintf(stderr, "Not a valid JAR file [%s]\n", buf);
                exit(1);
                }
           }

            /* we just have a class file that needs verification */

        len = strlen(pkgbuf);
        if (len > 6 && strcmp(pkgbuf + len - 6, ".class") == 0) {

                    pkgbuf[len - 6] = 0;

                    if (JAR_DEBUG && verbose)
                    jio_fprintf(stderr,
                "recurse_dir: Verifying Class [%s] \n", pkgbuf);
            VerifyFile(pkgbuf);
        }
    }

    closedir(dir);
}

/*=========================================================================
 * FUNCTION:      ProcessInputs()
 * TYPE:          Processes inputs
 * OVERVIEW:      Internal function called by main().
 *
 *  This function processes input entries for specifying classes that are to
 *  be verified. The inputs may be in 3 forms: a directory, or nested
 *  directories, one or more JAR files, or one or more individual class files.
 *  If a directory entry is specified, it may also contain one or more JAR
 *  files, or one or more individual class files.
 *
 * INTERFACE:
 *   parameters:  argname:  directory, JAR file or an individual class file.
 *   returns:     nothing
 *=======================================================================*/

static void ProcessInputs(char *argname)
{
    char buf[MAXPACKAGENAME];
    struct stat stat_buf;
    int res = stat(argname, &stat_buf);
    int len;


    strcpy(buf, argname);
    len = strlen(argname);


    if ((res == 0) && (stat_buf.st_mode & S_IFDIR)) {
        /* Append dir separator if it does not yet exist. */
        if (buf[len - 1] != LOCAL_DIR_SEPARATOR &&
        buf[len - 1] != DIR_SEPARATOR) {
        buf[len] = DIR_SEPARATOR;
        buf[len + 1] = 0;
        }
        pushDirectoryOntoClassPath(buf);
        recurse_dir(buf, "");
        popClassPath();
    } else if ((res == 0) && (isJARfile (buf, len))) {
        /* the classes to be verified are in a JAR file */
        if (!ProcessJARfile(buf, len)) {
        fprintf(stderr, "Not a valid JAR file [%s]\n", buf);
        exit(1);
        }
    } else {
        char *p;
        /* Convert all periods in the argname to slashes */
        for (p = buf; ((p = strchr(p, '.')) != 0); *p++ = '/');
            if (JAR_DEBUG && verbose)
            jio_fprintf(stderr,
            "ProcessInputs: Verifying file [%s]\n", buf );
        VerifyFile(buf);
    }
}

/*=========================================================================
 * FUNCTION:      main()
 * TYPE:          Runs the verifier.
 * OVERVIEW:      Main function.
 *
 *  This is the main function that invokes the Java class verifier.
 *
 * INTERFACE:
 *   parameters:  argc: arg count.
 *                argv: arg value(s)
 *   returns:     nothing
 *=======================================================================*/
int main(argc, argv)
    register char **argv;
{
    char *progname;
    char *argv0 = argv[0];

    SET_DEFAULT_LOCALE;

    if ((progname = strrchr(argv[0], LOCAL_DIR_SEPARATOR)) != 0) {
    progname++;
    } else {
    progname = argv[0];
    }

    /* initialize tmp_dir */
    memset(tmp_dir, 0, 32);

    /* initialize the random number generator */
    srand(time(NULL));

    /* generate a random temp directory name */
    sprintf(tmp_dir, "%s%d%c", "tmp", rand(), '\0');

    while (--argc > 0)
    if ((++argv)[0][0] == '-') {
            if (strcmp(argv[0], "-verbose") == 0) {
                extern bool_t verbose;
                verbose = TRUE;
#ifdef DEBUG_VERIFIER
        } else if (strcmp(argv[0], "-verify-verbose") == 0) {
        extern int verify_verbose;
        verify_verbose++;
#endif
            } else if (strcmp(argv[0], "-cldc1.0") == 0) {
                no_native_methods = TRUE;
                no_floating_point = TRUE;
                no_finalizers = TRUE;
            } else if (strcmp(argv[0], "-nofinalize") == 0) {
                no_finalizers = TRUE;
            } else if (strcmp(argv[0], "-nonative") == 0) {
                no_native_methods = TRUE;
            } else if (strcmp(argv[0], "-nofp") == 0) {
                no_floating_point = TRUE;
            } else if (strcmp(argv[0], "-Xns") == 0) {
                extern bool_t stack_map_on;
                stack_map_on = FALSE;
            } else if (strcmp(argv[0], "-Xni") == 0) {
                extern bool_t inline_jsr_on;
                inline_jsr_on = FALSE;
            } else if (strcmp(argv[0], "-d") == 0) {
                output_dir = strdup(argv[1]);
        argc--; argv++;
        } else if (strcmp(argv[0], "-classpath") == 0) {
        if (argc > 1) {
            char *buf = (char *)malloc(strlen(argv[1]) + 32);
            sprintf(buf, "CLASSPATH=%s", argv[1]);
            putenv(buf);
            argc--; argv++;
        } else {
            fprintf(stderr,
             "-classpath requires class path specification\n");
            usage(progname);
            exit(1);
        }
        } else {
        fprintf(stderr, "%s: Illegal option %s\n\n", progname, argv[0]);
        usage(progname);
        exit(1);
        }
    } else if (argv[0][0] == '@') {
            char *new_argv[MAXOPTIONS];
            char *buffer;
        int new_argc = 1;
            char *token;
            bool_t done = FALSE;
            struct stat sbuf;
        FILE *fp;
            int buflen;
            int filelength;
            int i;

        new_argv[0] = argv0;

        fp = fopen(&argv[0][1], "rt");
        if (fp == NULL) {
        fprintf(stderr, "Can't open %s\n", &argv[0][1]);
        exit(1);
        }

            /* get the size of the file */
            stat(&argv[0][1], &sbuf);
            filelength = sbuf.st_size;

            /* allocate the buffer */
            if ((buffer = (char *) malloc(filelength+1)) == NULL) {
                fprintf(stderr, "Out of memory\n");
                exit(1);
            }

            if (fgets(buffer, filelength+1, fp) != NULL) {
                buflen = strlen(buffer);

                if (DEBUG_READFROMFILE) {
                    fprintf(stderr, "Buffer = [%s]\n", buffer);
                }
                /* get the first parameter */
                token = strtok(buffer, " \0\n\r\t\"");

                /* Search for parameters until none can be found */
                while (token != NULL && !done) {

                    if ((strcmp(token, "\n") == 0) ||
                        (strcmp(token, "\r") == 0) ||
                        (strcmp(token, "\t") == 0)) {
                        /* <NL>, <CR> or <TAB> */
                        done = TRUE;
                    } else {
                        /* On Win32, it's possible to have a <CR> <LF>
                         * appended at the end of the token, which needs
                         * to be extracted before restoring the arg
                         */
                        char *p;
                        /* Convert all <CR> <LF> to NULL */
                        for (p = token; *p != '\0'; p++) {
                            if (*p == '\n' || *p == '\r')
                                *p = '\0';
                        }

                        /* save the parameter */
                        new_argv[new_argc++] = token;

                        if ((strcmp(token, "-classpath") == 0) ||
                            (strcmp(token, "-d")         == 0)) {
                            /* search for the beginning of a string literal */
                            token = strtok(NULL, "\"");

                        } else { /* get the next token */
                            token = strtok(NULL," \0\n\r\t\"");
                        }
                    }
                }

                if (DEBUG_READFROMFILE) {
                    /* print the arguments after restoring all of them */
                    for (i=0; i < new_argc; i++)
                        fprintf(stderr, "new_argv[%d] = [%s]\n", i, new_argv[i]);
                }
            }

        fclose(fp);
        main(new_argc, new_argv);

            if (buffer != NULL) {
                free(buffer);
            }
        argc--; argv++;
    } else {
        processedfile++;
            if (strlen(argv[0]) > MAXPACKAGENAME) {
        fprintf(stderr,
             "Package name for class exceeds max allowed size [1024]\n");
        usage(progname);
        exit(1);
            }

            /* indicate location of the verified classes only for verbose mode */
            if (verbose) {
                if (!output_dir) {
                    fprintf(stderr, "[Output directory for verified classes: output]\n");
                } else {
                    fprintf(stderr, "[Output directory for verified classes: %s]\n",
                        output_dir);
                }
            }
        ProcessInputs(argv[0]);
    }
    if (processedfile == 0) {
    usage(progname);
    exit(1);
    }
    return errorCode;  /* normal errorCode=0 indicates successful completion */
}

static void usage(char *progname) {
    fprintf(stderr, "Usage: %s [options] classnames|dirnames ...\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "where options include:\n");
    fprintf(stderr, "   -classpath     <directories separated by '%c'>\n", (char)PATH_SEPARATOR);
    fprintf(stderr, "                  Directories in which to look for classes\n");
    fprintf(stderr, "   -d <directory> Directory in which output is written (default is ./output/)\n");

    fprintf(stderr, "   -cldc1.0       Checks for existence of language features prohibited \n");
    fprintf(stderr, "                  by CLDC 1.0 (native methods, floating point and finalizers)\n");

    fprintf(stderr, "   -nofinalize    No finalizers allowed\n");
    fprintf(stderr, "   -nonative      No native methods allowed\n");
    fprintf(stderr, "   -nofp          No floating point operations allowed\n");
    fprintf(stderr, "   @<filename>    Read command line arguments from a text file\n");
    fprintf(stderr, "                  Command line arguments must all be on a single line\n");
    fprintf(stderr, "                  Directory names must be enclosed in double quotes (\")\n");

    fprintf(stderr, "\n");
}


#ifndef O_BINARY
#define O_BINARY 0
#endif

int
OpenCode(char *fn, char *sfn, char *dir, struct stat * st)
{
    long codefd;
    if (fn == 0 || (codefd = open(fn, O_BINARY, 0644)) < 0
        || fstat(codefd, st) < 0)
    return -2;
    return codefd;
}
