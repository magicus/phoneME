/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#include <Windows.h>
#include <string.h>
#include "nams.h" 
#include "javacall_nams.h"
#include "javacall_memory.h"
#include "javacall_logging.h"
 
MidletNode* MidletList[MAX_MIDLET_NUM];
int current_midlet_count;

// Pending midlet info for recording by javacall_ams_midlet_stateChanged()
static MidletNode pendingMidletInfo;
static javacall_bool pending;
 
void nams_init_midlet_list()
{
    int i;
    for (i = 1; i < MAX_MIDLET_NUM; i ++)
    {
        MidletList[i] = NULL;
    }
    current_midlet_count = 0;
}

void nams_clean_midlet_list()
{
    int i;
    for (i = 1; i < MAX_MIDLET_NUM; i ++)
    {
        javacall_free(MidletList[i]);
    }
    current_midlet_count = 0;
}

void nams_set_midlet_static_info(int appID, MidletNode* pInfo)
{
    int i;
    char key[]="MIDlet-Jar-RSA-SHA1";
    javacall_utf16* uKey;
    javacall_utf16 uValue[MAX_VALUE_NAME_LEN]; // not used

    pending = TRUE;
    memset(&pendingMidletInfo, 0, sizeof(MidletNode));
    memcpy(&pendingMidletInfo, pInfo, sizeof(MidletNode));

    pendingMidletInfo.requestForeground = FALSE;
    for (i = 0; i < JAVACALL_AMS_PERMISSION_LAST; i++)
    {
        pendingMidletInfo.permissions.permission[i] 
                          = JAVACALL_AMS_PERMISSION_VAL_BLANKET;
    }

    nams_string_to_utf16(key, strlen(key), &uKey, strlen(key));

    if (javacall_ams_getSuiteProperty(appID, uKey, uValue, MAX_VALUE_NAME_LEN) 
        == JAVACALL_OK)
    {
        pendingMidletInfo.domain = JAVACALL_AMS_DOMAIN_TRUSTED;
    }
    else
    {
        pendingMidletInfo.domain = JAVACALL_AMS_DOMAIN_UNTRUSTED;
    }

    javacall_free(uKey);

}

javacall_result nams_allocate_appid(javacall_suite_id* pAppID)
{
    int i;

    if (current_midlet_count > MAX_MIDLET_NUM - 1)
    {
        return JAVACALL_FAIL; // No free slot
    }
    for (i = 1; i < MAX_MIDLET_NUM; i ++)
    {
        if (MidletList[i] == NULL) // Found a free slot
        {
            break;
        }
    }
    *pAppID = i;

    return JAVACALL_OK;
}

javacall_result nams_add_midlet(javacall_suite_id appID)
{
    if (appID <= 0 || appID >= MAX_MIDLET_NUM)
    {
        return JAVACALL_FAIL;
    }

    MidletList[appID] = (MidletNode *)javacall_malloc(sizeof(MidletNode));

    memcpy(MidletList[appID], &pendingMidletInfo, sizeof(MidletNode));
    current_midlet_count ++;
    pending = FALSE;
    
    return JAVACALL_OK;
}

javacall_result nams_remove_midlet(javacall_suite_id appID)
{
    if (appID <= 0 || appID >= MAX_MIDLET_NUM)
    {
        return JAVACALL_FAIL;
    }
    javacall_free(MidletList[appID]);
    MidletList[appID] = NULL;

    current_midlet_count --;

    return JAVACALL_OK;
}

int nams_get_current_midlet_count()
{
    return current_midlet_count;
}

javacall_result nams_find_midlet_by_state(javacall_midlet_state state, int* index)
{
    int i;
    for (i = 1; i < MAX_MIDLET_NUM; i ++)
    {
        if (MidletList[i] != NULL && MidletList[i]->state == state)
        {
            break;
        }
    }
    if (i >= MAX_MIDLET_NUM)
    {
        return JAVACALL_FAIL;
    }
    *index = i;
    return JAVACALL_OK;
}

javacall_result nams_get_midlet_state(int index, javacall_midlet_state *state)
{
    if (index >= MAX_MIDLET_NUM || index < 1)
    {
        return JAVACALL_FAIL;
    }
    if (MidletList[index] == NULL)
    {
        return JAVACALL_FAIL;
    }
    *state = MidletList[index]->state;
    return JAVACALL_OK;
}

javacall_result nams_set_midlet_state(javacall_midlet_state state, int index, javacall_change_reason reason)
{
    if (index >= MAX_MIDLET_NUM || index < 1)
    {
        return JAVACALL_FAIL;
    }
    if (MidletList[index] == NULL)
    {
        return JAVACALL_FAIL;
    }
    MidletList[index]->state = state;
    MidletList[index]->lastChangeReason = reason;

    return JAVACALL_OK;
}

javacall_result nams_get_midlet_jadpath(int index, char** outPath)
{
    if (pending)
    {
        *outPath = pendingMidletInfo.jadPath;
    }
    else
    {
        if (index >= MAX_MIDLET_NUM || index < 1)
        {
            return JAVACALL_FAIL;
        }
        if (MidletList[index] == NULL)
        {
            return JAVACALL_FAIL;
        }
        *outPath = MidletList[index]->jadPath;
    }
    return JAVACALL_OK;
}

javacall_result nams_get_midlet_permissions(int index, javacall_ams_permission_set* pSet)
{
    if (pending)
    {
        *pSet = pendingMidletInfo.permissions;
    }
    else
    {
        if (index >= MAX_MIDLET_NUM || index < 1)
        {
            return JAVACALL_FAIL;
        }
        if (MidletList[index] == NULL)
        {
            return JAVACALL_FAIL;
        }
        *pSet = MidletList[index]->permissions;
    }

    return JAVACALL_OK;
}

javacall_result nams_set_midlet_permission(int index, javacall_ams_permission permission, javacall_ams_permission_val value)
{
    if (index >= MAX_MIDLET_NUM || index < 1)
    {
        return JAVACALL_FAIL;
    }
    if (MidletList[index] == NULL)
    {
        return JAVACALL_FAIL;
    }
    if (permission < 0 || permission > JAVACALL_AMS_PERMISSION_LAST)
    {
        return JAVACALL_FAIL;
    }

    MidletList[index]->permissions.permission[permission] = value;

    return JAVACALL_OK;
}

javacall_result nams_get_midlet_domain(int index, javacall_ams_domain* domain)
{
    if (pending)
    {
        *domain = pendingMidletInfo.domain;
    }
    else
    {
        if (index >= MAX_MIDLET_NUM || index < 1)
        {
            return JAVACALL_FAIL;
        }
        if (MidletList[index] == NULL)
        {
            return JAVACALL_FAIL;
        }
        *domain = MidletList[index]->domain;
    }

    return JAVACALL_OK;
}

javacall_result nams_get_midlet_classname(int index, contentList* className)
{
    if (index >= MAX_MIDLET_NUM || index < 1)
    {
        return JAVACALL_FAIL;
    }
    if (MidletList[index] == NULL)
    {
        return JAVACALL_FAIL;
    }
    nams_content_list_add(className, MidletList[index]->className);

    return JAVACALL_OK;
}

char* nams_trans_state(javacall_midlet_state state)
{
    switch (state)
    {
    case JAVACALL_MIDLET_STATE_PAUSED:
        return "paused";
    case JAVACALL_MIDLET_STATE_DESTROYED:
        return "destoryed";
    case JAVACALL_MIDLET_STATE_ERROR:
        return "error";
    case JAVACALL_MIDLET_STATE_FOREGROUND:
        return "foreground";
    case JAVACALL_MIDLET_STATE_BACKGROUND:
        return "background";
    default:
        return "error";
    }
}

void nams_set_midlet_request_foreground(int index)
{
    if (index >= MAX_MIDLET_NUM || index < 1 || MidletList[index] == NULL)
    {
        return;
    }
    MidletList[index]->requestForeground = TRUE;

}

void nams_clear_midlet_request_foreground(int index)
{
    if (index >= MAX_MIDLET_NUM || index < 1 || MidletList[index] == NULL)
    {
        return;
    }
    MidletList[index]->requestForeground = FALSE;

}

javacall_result nams_if_midlet_exist(int index)
{
    if (index >= MAX_MIDLET_NUM || index < 1)
    {
        return JAVACALL_FAIL;
    }
    if (MidletList[index] == NULL)
    {
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

javacall_bool nams_get_midlet_requestforeground(int index)
{
    if (index >= MAX_MIDLET_NUM || index < 1)
    {
        return JAVACALL_FALSE;
    }
    if (MidletList[index] == NULL)
    {
        return JAVACALL_FALSE;
    }

    return MidletList[index]->requestForeground;
}

