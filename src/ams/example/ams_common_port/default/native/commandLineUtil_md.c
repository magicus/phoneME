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
#include <sys/stat.h>
#include <ctype.h>

#include <midpStorage.h>
#include <commandLineUtil.h>

#define APPDB_DIR  "appdb"
#define CONFIG_DIR "lib"


static char appDirBuffer[MAX_FILENAME_LENGTH+1];
static char confDirBuffer[MAX_FILENAME_LENGTH+1];

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
 * <p>
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
char* getMidpHome(char *cmd, char *dirBuffer) {
    int   i;
    char* filesep = NULL;
    char* lastsep;
    char* midp_home;
    struct stat statbuf;
    int j = 1;

    /*
     * If MIDP_HOME is set, just use it. Does not check if MIDP_HOME is
     * pointing to a directory contain APPDB_DIR.
     */
    midp_home = getenv("MIDP_HOME");
    if (midp_home != NULL) {
        return midp_home;
    }

    filesep = getCharFileSeparator();

    dirBuffer[MAX_FILENAME_LENGTH] = 0;

    /* in some builds cmd is NULL */
    if (cmd != NULL) {
        strncpy(dirBuffer, cmd, MAX_FILENAME_LENGTH);
    } else {
        /* use the current directory */
        dirBuffer[0] = '.';
        dirBuffer[1] = 0;
    }

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

        strcat(dirBuffer, APPDB_DIR);

        i = 0;

        /* try to search for APPDB_DIR 3 times only (see above) */
        while (i < 3) {
            memset(&statbuf, 0, sizeof(statbuf));

            /* found it and it is a directory */
            if ((stat(dirBuffer, &statbuf) == 0) &&
                (statbuf.st_mode & S_IFDIR)) {
                break;
            }

            /* strip off APPDB_DIR to add 1 more level of ".." */
            *(strrchr(dirBuffer, (int) *filesep)) = '\0';
            strcat(dirBuffer, filesep);
            strcat(dirBuffer, "..");
            strcat(dirBuffer, filesep);
            strcat(dirBuffer, APPDB_DIR);

            i++;
        }

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

    /* strip off APPDB_DIR from the path, keep the last seperator */
    *(strrchr(dirBuffer, (int) *filesep)+1) = '\0';

    return dirBuffer;
}

char* getApplicationDir(char *cmd) {

    getMidpHome(cmd,appDirBuffer);
    strcat(appDirBuffer,APPDB_DIR);

    return appDirBuffer;
}

char* getConfigurationDir(char *cmd) {

    getMidpHome(cmd,confDirBuffer);
    strcat(confDirBuffer,CONFIG_DIR);

    return confDirBuffer;
}
