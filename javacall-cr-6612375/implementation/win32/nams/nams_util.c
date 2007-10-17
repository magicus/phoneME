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

#include <windows.h>
#include <string.h>
#include "nams.h"
#include "javacall_memory.h"

void nams_content_list_init(contentList* pList)
{
    pList->itemCount = 0;
    pList->curr = pList->head = pList->tail = NULL;
}

javacall_result nams_content_list_add(contentList* pList, char* text)
{
    contentNode* pNode;

    if (pList == NULL)
    {
        return JAVACALL_FAIL;
    }

    pNode = (contentNode *)javacall_malloc(sizeof(contentNode));
    pNode->content = (char *)javacall_malloc(strlen(text)+1);
    strcpy(pNode->content, text);

    if (pList->head == NULL)
    {
        pList->head = pNode;
        pList->curr = pList->head;
    }
    if (pList->tail != NULL)
    {
        pList->tail->next = pNode;
    }
    pList->tail = pNode;
    pList->tail->next = NULL;

    pList->itemCount++;

    return JAVACALL_OK;
}

char* nams_content_list_get_next_line(contentList* pList)
{
    contentNode* p;

    if (pList->curr == NULL)
    {
       return NULL;
    }
    p = pList->curr;
    pList->curr = pList->curr->next;

    return p->content;
}

void nams_content_list_release(contentList* pList)
{
    contentNode* p;
    contentNode* q;

    if (pList == NULL || pList->head == NULL)
    {
        return;
    }

    p = pList->head, 
    q = p->next;

    while (p != NULL)
    {
        javacall_free(p->content);
        javacall_free(p);
        p = q;
        if (p != NULL)
        {
            q = p->next;
        }
    }
    nams_content_list_init(pList);

}

javacall_result nams_string_to_utf16(char* srcStr, int srcSize, javacall_utf16** destStr, int destSize)
{
    int len;
    javacall_utf16* uStr;

    uStr = (javacall_utf16*)javacall_malloc((destSize+1)*sizeof(javacall_utf16));

    len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, srcStr, srcSize, uStr, destSize);
    if (len == 0)
    {
        return JAVACALL_FAIL;
    }
    uStr[len] = 0;

    *destStr = uStr;
    return JAVACALL_OK;
}

