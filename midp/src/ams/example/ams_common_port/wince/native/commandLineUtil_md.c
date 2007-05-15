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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <midpMalloc.h>
#include <midpStorage.h>
#include <midpString.h>

/* for getCharFileSeparator()   */
/* #include <commandLineUtil.h> */

static char dirBuffer[MAX_FILENAME_LENGTH+1];

/**
 * Generates a correct MIDP home directory based on several rules. If
 * the <tt>MIDP_HOME</tt> environment variable is set, its value is used
 * unmodified. Otherwise, this function will search for the <tt>appdb</tt>
 * directory in the following order:
 * <ul>
 * <li>current directory (if the MIDP executable is in the <tt>PATH</tt>
 *     environment variable and the current directory is the right place)
 * <li>the parent directory of the midp executable
 * <li>the grandparent directory of the midp executable
 * </ul>
4 * <p>
 * If <tt>cmd</tt> does not contain a directory (i.e. just the text
 * <tt>midp</tt>), the search starts from the current directory. Otherwise,
 * the search starts from the directory specified in <tt>cmd</tt> (i.e.
 * start in the directory <tt>bin</tt> if <tt>cmd</tt> is <tt>bin/midp</tt>).
 * <p>
 * <b>NOTE:</b> This is only applicable for development platforms.
 *
 * @param cmd A 'C' string containing the command used to start MIDP.
 * @return A 'C' string the found MIDP home directory, otherwise
 *         <tt>NULL</tt>, this will be a static buffer, so that it safe
 *       to call this function before midpInitialize, don't free it
 */
char* midpFixMidpHome(char *cmd) {
#if 1
    FILE *fp;

    if ((fp = fopen("\\Storage Card\\appdb\\_main.ks", "r")) != NULL) {
        /* This is usually the case when running on emulator, where
         * the MIDP_OUT_DIR is mapped as a shared directory.
         */
        fclose(fp);
        return "\\Storage Card";
    } else if ((fp = fopen("\\jwc1.1.3\\appdb\\_main.ks", "r")) != NULL) {
        /* Running on WM 5.0, where there's no \\Storage anymore */
        fclose(fp);
        return "\\jwc1.1.3";
    } else if ((fp = fopen("\\My Documents\\java\\appdb\\_main.ks", "r")) != NULL) {
        fclose(fp);
        return "\\My Documents\\java";
    } else {
        /* This is usually the case when running on actual device. */
        return "\\Storage\\jwc1.1.3";
    }
#else
    int   i;
    char* filesep = NULL;
    char* lastsep;
    char* midp_home = NULL;
    int j = 1;

    filesep = getCharFileSeparator();

    dirBuffer[sizeof (dirBuffer) - 1] = 0;
    strncpy(dirBuffer, cmd, sizeof (dirBuffer) - 1);

    while (j < 2) {

        /* Look for the last slash in the pathanme. */
        lastsep = strrchr(dirBuffer, (int) *filesep);
        if (lastsep != 0) {
            *(lastsep + 1) = '\0';
        } else {
            /* no file separator */
            strcpy(dirBuffer, ".");
            strcat(dirBuffer, filesep);
        }

        strcat(dirBuffer, "appdb");

        i = 0;

#if 0
        /* try to search for "appdb" 3 times only (see above) */
        while (i < 3) {
            memset(&statbuf, 0, sizeof(statbuf));

            /* found it and it is a directory */
            if ((stat(dirBuffer, &statbuf) == 0) &&
                (statbuf.st_mode & S_IFDIR)) {
                break;
            }

            /* strip off "lib" to add 1 more level of ".." */
            *(strrchr(dirBuffer, (int) *filesep)) = '\0';
            strcat(dirBuffer, filesep);
            strcat(dirBuffer, "..");
            strcat(dirBuffer, filesep);
            strcat(dirBuffer, "appdb");

            i++;
        }
#endif

        if (i < 3) {
            break;
        }

        j++;
    }

    if (j == 2) {
        fprintf(stderr, "Warning: cannot find appdb subdirectory.\n"
                "Please specify MIDP_HOME environment variable such "
                "that $MIDP_HOME%clib contains the proper configuration "
                "files.\n", *filesep);
        return NULL;
    }

    /* strip off "appdb" from the path */
    *(strrchr(dirBuffer, (int) *filesep)) = '\0';

    return dirBuffer;
#endif
}
