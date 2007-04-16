/*
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "nams.h"
#include "javautil_string.h"
#include "javautil_jad_parser.h"
#include "javacall_memory.h"
 
javacall_result nams_get_classname_from_jad(const javacall_utf16* jadPath, int jadPathLen, 
                                                        char* pClassName, int mIndex)
{
    long jadSize;
    char* jadBuffer;
    int propsNum=0;
    javacall_result res;
    int i;
    char* jadLine;
    int jadLineSize;
    int index;
    char* jadPropName;
    char* jadPropValue;
    char* midletName;
    
    if (mIndex < 0 && mIndex > 256)
    {
        return JAVACALL_FAIL;
    }
    jadSize = javautil_read_jad_file(jadPath, jadPathLen, &jadBuffer);
    if (jadSize <= 0 || jadBuffer == NULL)
    {
        return JAVACALL_FAIL;
    }

    res = javautil_get_number_of_properties(jadBuffer, &propsNum);
    if ((res != JAVACALL_OK) || (propsNum <= 0)) 
    {
        return JAVACALL_OUT_OF_MEMORY;
    }

    for (i=0; i<propsNum; i++) 
    {
        // read a line from the jad file.
        res = javautil_read_jad_line(&jadBuffer, &jadLine, &jadLineSize);
        if (res == JAVACALL_END_OF_FILE) 
        {
            break;
        } 
        else if ((res != JAVACALL_OK) || (jadLine == NULL) || (jadLineSize == 0)) 
        {
            return JAVACALL_FAIL;
        }
        
        // find the index of ':'
        res = javautil_string_index_of(jadLine, COLON, &index);
        if ((res != JAVACALL_OK) || (index <= 0)) 
        {
            javacall_free(jadLine);
            continue;
        }
        
        // get sub string of jad property name
        res = javautil_string_substring(jadLine, 0, index, &jadPropName);
        if (res == JAVACALL_OUT_OF_MEMORY) 
        {
            javacall_free(jadLine);
            return res;
        }
        if ((res != JAVACALL_OK) || (jadPropName == NULL)) 
        {
            javacall_free(jadLine);
            continue;
        }
        res = javautil_string_trim(jadPropName);
        if (res != JAVACALL_OK) 
        {
            javacall_free(jadLine);
            javacall_free(jadPropName);
            continue;
        }
        midletName = string_cat_num("MIDlet-", mIndex);
        if (!javautil_string_equals(midletName, jadPropName))
        {
            javacall_free(midletName);
            continue;
        }
        javacall_free(midletName);
        
        // found the midlet, get out the class name
        // find the index of the second ','
        res = javautil_string_index_of_ex(jadLine, ',', 2, &index);
        if ((res != JAVACALL_OK) || (index <= 0)) 
        {
            javacall_free(jadLine);
            continue;
        }

        // skip white space between jad property name and value
        while (*(jadLine+index+1) == SPACE) 
        {
            index++;
        }

        // get sub string of jad property value
        res = javautil_string_substring(jadLine, index+1, jadLineSize, &jadPropValue);
        if (res == JAVACALL_OUT_OF_MEMORY) 
        {
            javacall_free(jadLine);
            javacall_free(jadPropName);
            return res;
        }
        if ((res != JAVACALL_OK) || (jadPropValue == NULL)) 
        {
            javacall_free(jadLine);
            javacall_free(jadPropName);
            continue;
        }
        
        // jad property name an value are available,  we can free the jad file line
        javacall_free(jadLine);
        
        res = javautil_string_trim(jadPropValue);
        if (res != JAVACALL_OK) 
        {
            javacall_free(jadPropName);
            javacall_free(jadPropValue);
            continue;
        }
        
        strcpy(pClassName, jadPropValue);
        javacall_free(jadPropName);
        javacall_free(jadPropValue);

        break;
        
    } // end for loop

    return res;
}

javacall_result nams_get_jarname_from_jad(const char* jadPath, char** jarName)
{
    /* TO DO: read real jar name from jad */
    int len;
    len = strlen(jadPath);

    *jarName = (char *)javacall_malloc(len+1);
    strcpy(*jarName, jadPath);
    (*jarName)[len-1] = 'r';

    return JAVACALL_OK;
}

char* string_cat_num(char* text, int mIndex)
{
    char pIndex[10];
    char* destBuffer;

    _itoa(mIndex, pIndex, 10);
    destBuffer = (char *)javacall_malloc(strlen(text) + strlen(pIndex) + 1);
    strcpy(destBuffer, text);
    strcat(destBuffer, pIndex);
    return destBuffer;
}

javacall_result javautil_string_index_of_ex(char* str, char c, int times, /* OUT */ int* index) 
{
    int i=0;
    int appTimes=0;
    int len = strlen(str);

    *index = -1;

    if (len <= 0) {
        return JAVACALL_FAIL;
    }

    while ((i < len) && appTimes < times) {
        if (*(str+i) == c)
        {
            appTimes++;
        }
        i++;
    }

    if (i == len) {
        return JAVACALL_FAIL;
    }

    *index = i;
    return JAVACALL_OK;
}

javacall_result nams_get_midlets_from_jad(const javacall_utf16* jadPath, int jadPathLen, 
                                                   contentList* pMidlets, int* count)
{
    long jadSize;
    char* jadBuffer;
    int propsNum=0;
    javacall_result res;
    int i;
    char* jadLine;
    int jadLineSize;
    int index;
    char* jadPropName;
    int subIndex;
    char* subPropName;
    int x=0;

    jadSize = javautil_read_jad_file(jadPath, jadPathLen, &jadBuffer);
    if (jadSize <= 0 || jadBuffer == NULL)
    {
        return JAVACALL_FAIL;
    }
    res = javautil_get_number_of_properties(jadBuffer, &propsNum);
    if ((res != JAVACALL_OK) || (propsNum <= 0)) 
    {
        return JAVACALL_OUT_OF_MEMORY;
    }
    for (i=0; i<propsNum; i++) 
    {
        // read a line from the jad file.
        res = javautil_read_jad_line(&jadBuffer, &jadLine, &jadLineSize);
        if (res == JAVACALL_END_OF_FILE) 
        {
            break;
        } 
        else if ((res != JAVACALL_OK) || (jadLine == NULL) || (jadLineSize == 0)) 
        {
            return JAVACALL_FAIL;
        }
        
        // find the index of ':'
        res = javautil_string_index_of(jadLine, ':', &index);
        if ((res != JAVACALL_OK) || (index <= 0)) 
        {
            javacall_free(jadLine);
            continue;
        }
        
        // get sub string of jad property name
        res = javautil_string_substring(jadLine, 0, index, &jadPropName);
        if (res == JAVACALL_OUT_OF_MEMORY) 
        {
            javacall_free(jadLine);
            return res;
        }
        if ((res != JAVACALL_OK) || (jadPropName == NULL)) 
        {
            javacall_free(jadLine);
            continue;
        }
        res = javautil_string_trim(jadPropName);
        if (res != JAVACALL_OK) 
        {
            javacall_free(jadLine);
            javacall_free(jadPropName);
            continue;
        }

        // get sub string "MIDlet"
        res = javautil_string_index_of(jadPropName, '-', &subIndex);
        if ((res != JAVACALL_OK) || (subIndex <= 0)) 
        {
            javacall_free(jadLine);
            continue;
        }
        res = javautil_string_substring(jadPropName, 0, subIndex, &subPropName);
        if (res == JAVACALL_OUT_OF_MEMORY) 
        {
            javacall_free(jadPropName);
            javacall_free(jadLine);
            return res;
        }
        if ((res != JAVACALL_OK) || (subPropName == NULL)) 
        {
            javacall_free(jadPropName);
            javacall_free(jadLine);
            continue;
        }
        if (!javautil_string_equals(subPropName, "MIDlet"))
        {
            javacall_free(jadPropName);
            javacall_free(jadLine);
            continue;
        }

        // get sub string number
        res = javautil_string_substring(jadPropName, subIndex+1, index, &subPropName);
        if (res == JAVACALL_OUT_OF_MEMORY) 
        {
            javacall_free(jadPropName);
            javacall_free(jadLine);
            return res;
        }
        if ((res != JAVACALL_OK) || (subPropName == NULL)) 
        {
            javacall_free(jadPropName);
            javacall_free(jadLine);
            continue;
        }
        if (atoi(subPropName) == 0)
        {
            javacall_free(jadPropName);
            javacall_free(jadLine);
            continue;
        }

        nams_content_list_add(pMidlets, jadLine);
        x ++;

        javacall_free(subPropName);
        javacall_free(jadPropName);

    } // end for loop

    *count = x;
    return res;
}

javacall_result nams_get_classname_from_textline(char* textLine, char** className)
{
    javacall_result res;
    int index;
    char* jadPropValue;

    res = javautil_string_index_of_ex(textLine, ',', 2, &index);
    if ((res != JAVACALL_OK) || (index <= 0)) 
    {
        return res;
    }

    // skip white space between jad property name and value
    while (*(textLine+index+1) == SPACE) 
    {
        index++;
    }

    // get sub string of jad property value
    res = javautil_string_substring(textLine, index+1, strlen(textLine), &jadPropValue);
    if (res != JAVACALL_OK) 
    {
        javacall_free(jadPropValue);
        return res;
    }

    *className = (char *)javacall_malloc(strlen(jadPropValue)+1);
    strcpy(*className, jadPropValue);
    javacall_free(jadPropValue);

    return res;
}


 