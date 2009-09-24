/*
 * @(#)canonicalize_md.c	1.9 06/10/10
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

/*
 * Pathname canonicalization for Unix file systems
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

/* Note: The comments in this file use the terminology
         defined in the java.io.File class */


/* Check the given name sequence to see if it can be further collapsed.
   Return zero if not, otherwise return the number of names in the sequence. */

static int
collapsible(char *names)
{
    char *p = names;
    int dots = 0, n = 0;

    while (*p) {
	if ((p[0] == '.') && ((p[1] == '\0')
			      || (p[1] == '/')
			      || ((p[1] == '.') && ((p[2] == '\0')
						    || (p[2] == '/'))))) {
	    dots = 1;
	}
	n++;
	while (*p) {
	    if (*p == '/') {
		p++;
		break;
	    }
	    p++;
	}
    }
    return (dots ? n : 0);
}


/* Split the names in the given name sequence,
   replacing slashes with nulls and filling in the given index array */

static void
splitNames(char *names, char **ix)
{
    char *p = names;
    int i = 0;

    while (*p) {
	ix[i++] = p++;
	while (*p) {
	    if (*p == '/') {
		*p++ = '\0';
		break;
	    }
	    p++;
	}
    }
}


/* Join the names in the given name sequence, ignoring names whose index
   entries have been cleared and replacing nulls with slashes as needed */

static void
joinNames(char *names, int nc, char **ix)
{
    int i;
    char *p;

    for (i = 0, p = names; i < nc; i++) {
	if (!ix[i]) continue;
	if (i > 0) {
	    p[-1] = '/';
	}
	if (p == ix[i]) {
	    p += strlen(p) + 1;
	} else {
	    char *q = ix[i];
	    while ((*p++ = *q++) != 0);
	}
    }
    *p = '\0';
}


/* Collapse "." and ".." names in the given path wherever possible.
   A "." name may always be eliminated; a ".." name may be eliminated if it
   follows a name that is neither "." nor "..".  This is a syntactic operation
   that performs no filesystem queries, so it should only be used to cleanup
   after invoking the realpath() procedure. */

static int
collapse(char *path)
{
    char *names = (path[0] == '/') ? path + 1 : path; /* Preserve first '/' */
    int nc;
    char **ix;
    int i, j;

    nc = collapsible(names);
    if (nc < 2) return 0;		/* Nothing to do */
    ix = (char **)malloc(nc * sizeof(char *));
    if (ix == NULL) {
	errno = ENOMEM;
	return -1;
    }
    splitNames(names, ix);

    for (i = 0; i < nc; i++) {
	int dots = 0;

	/* Find next occurrence of "." or ".." */
	do {
	    char *p = ix[i];
	    if (p[0] == '.') {
		if (p[1] == '\0') {
		    dots = 1;
		    break;
		}
		if ((p[1] == '.') && (p[2] == '\0')) {
		    dots = 2;
		    break;
		}
	    }
	    i++;
	} while (i < nc);
	if (i >= nc) break;

	/* At this point i is the index of either a "." or a "..", so take the
	   appropriate action and then continue the outer loop */
	if (dots == 1) {
	    /* Remove this instance of "." */
	    ix[i] = 0;
	}
	else {
	    /* If there is a preceding name, remove both that name and this
	       instance of ".."; otherwise, leave the ".." as is */
	    for (j = i - 1; j >= 0; j--) {
		if (ix[j]) break;
	    }
	    if (j < 0) continue;
	    ix[j] = 0;
	    ix[i] = 0;
	}
	/* i will be incremented at the top of the loop */
    }

    joinNames(names, nc, ix);
    free(ix);
    return 0;
}


/* Convert a pathname to canonical form.  The input path is assumed to contain
   no duplicate slashes.  On Solaris we can use realpath() to do most of the
   work, though once that's done we still must collapse any remaining "." and
   ".." names by hand. */

int
canonicalize(char *original, char *resolved, int len)
{
    if (len < PATH_MAX) {
	errno = EINVAL;
	return -1;
    }

    if (strlen(original) > PATH_MAX) {
	errno = ENAMETOOLONG;
	return -1;
    }

    /* Nothing resolved, so just return the original path */
    strcpy(resolved, original);
    return collapse(resolved);
}
