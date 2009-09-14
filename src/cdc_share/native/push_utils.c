/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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
#include "jsrop_push_utils.h"

/**
 * Wildcard comparing the pattern and the string.
 * (This procedure is a copy from midp/src/push/push_server/reference/native/push_server.c)
 * @param pattern The pattern that can contain '*' and '?'
 * @param str The string for comparing
 * @return <tt>1</tt> if the comparison is successful, <tt>0</tt> if it fails
 */
int wildComp(const char *pattern, const char *str){
    /* Current compare position of the pattern */
    const char *p1 = NULL;
    /* Current compare position of the string */
    const char *p2 = NULL;
    char tmp, tmp1;
    /* Last position of '*' in pattern */
    const char *posStar = NULL;
    /* Last position of string when '*' was found in the pattern */
    const char *posCmp = NULL;
    int num_quest, i;

    if ((pattern == NULL) || (str == NULL)) return 0;

    /* Filter is exactly "*", then any string is allowed. */
    if (strcmp(pattern, "*") == 0) return 1;

    /*
     * Otherwise walk through the pattern string looking for character
     * matches and wildcard matches.
     * The pattern pointer is incremented in the main loop and the
     * string pointer is incremented as characters and wildcards
     * are matched.
     */
    for (p1=pattern, p2=str; *p1 && *p2; ){
        switch (tmp = *p1){
        case '*' :
            /*
             * For an asterisk, consume all the characters up to
             * a matching next character.
             */
            num_quest = 0;
            posStar = p1; /* Save asterisk position in pattern */
            posCmp = p2;
            posCmp++;    /* Pointer to next position in string */
            do{
                tmp = *++p1;
                if (tmp == '?'){
                    num_quest++; /* number of question symbols */
                }
            } while ((tmp == '*') || (tmp == '?'));
            for (i = 0; i < num_quest; i++){
                if (*p2++ == 0){ /* EOL before questions number was exhausted */
                    return 0;
                }
            }
            if (tmp == 0){ /* end of pattern */
                return 1;
            }
            /* tmp is a next non-wildcard symbol */
            /* search it in the str */
            while (((tmp1 = *p2) != 0) && (tmp1 != tmp)){
                p2++;
            }
            if (tmp1 == 0){ /* no match symbols in str */
                return 0;
            }
            /* symbol found - goto next symbols */
            break;

        case '?' :
            /*
             * Skip a single symbol of str.
             * p1 and p2 points to non-EOL symbol
             */
            p1++;
            p2++;
            break;

        default :
            /*
             * Any other symbol - compare.
             * p1 and p2 points to non-EOL symbol
             */
            if (tmp != *p2){ /* symbol is not match */
                if (posStar == NULL){ /* No previous stars */
                    return 0;
                } else{ /* Return to the previous star position */
                    if (posCmp < p2){
                        p2 = posCmp++;
                    }
                    p1 = posStar;
                }
            } else{ /* match symbol */
                p1++;
                p2++;
            }
            break;
        } /* end if switch */

        if ((*p1 == 0) && (*p2 != 0)){
            if (posStar == NULL){ /* end of pattern */
                return 0;
            } else{
                if (posCmp < p2){
                    p2 = posCmp++;
                }
                p1 = posStar;
            }
        }
    } /* end of loop */

    if (*p2 != 0){ /* symbols remainder in str - not match */
        return 0;
    }

    if (*p1 != 0){ /* symbols remainder in pattern */
        while (*p1++ == '*'){ /* Skip multiple wildcard symbols. */
        }
        if (*--p1 != 0){ /* symbols remainder in pattern - not match */
            return 0;
        }
    }

    return 1;
}

