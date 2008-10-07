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

#ifndef _NAMS_H_
#define _NAMS_H_

#define MAX_MIDLET_NUM              20
#define MAX_MIDLET_NAME_LENGTH      128
#define MAX_MIDLET_SHUTDOWN_TIMEOUT 1000
#define MAX_PROPERTY_NAME_LEN       128
#define MAX_VALUE_NAME_LEN          128
#define MAX_SUITE                   128

#include "javacall_defs.h"
#include "javacall_ams_platform.h"
#include "javacall_ams_app_manager.h"
#include "javacall_ams_installer.h"
#include "javacall_ams_suitestore.h"

/*************************************************************************************************
*  NAMS util related 
*************************************************************************************************/
typedef struct ContentNode {
    char* content;
    struct ContentNode* next;

} contentNode;

typedef struct ContentList {
    int itemCount;
    struct ContentNode* curr;
    struct ContentNode* head;
    struct ContentNode* tail;

} contentList;


void nams_content_list_init(contentList* pList);
javacall_result nams_content_list_add(contentList* pList, char* text);
char* nams_content_list_get_next_line(contentList* pList);
void nams_content_list_release(contentList* pList);
javacall_result nams_string_to_utf16(char* srcStr, int srcSize, javacall_utf16** destStr, int destSize);

/*************************************************************************************************
*  Midlet list related 
*************************************************************************************************/
typedef struct midletNode {
    javacall_suite_id           suiteID;
    char                        jadPath[MAX_MIDLET_NAME_LENGTH];
    char                        className[MAX_MIDLET_NAME_LENGTH];
    javacall_lifecycle_state    state;
    javacall_change_reason      lastChangeReason;
    javacall_ams_permission_val permissions[JAVACALL_AMS_NUMBER_OF_PERMISSIONS];
    javacall_ams_domain         domain;
    javacall_bool               requestForeground;
} MidletNode;

void nams_init_midlet_list();
void nams_clean_midlet_list();
javacall_result nams_add_midlet(javacall_suite_id appID);
javacall_result nams_allocate_appid(javacall_suite_id* pAppID);
javacall_result nams_remove_midlet(javacall_suite_id appID);
javacall_result nams_find_midlet_by_state(javacall_lifecycle_state state, int* index);
javacall_result nams_get_midlet_state(int index, javacall_lifecycle_state *state);
javacall_result nams_set_midlet_state(javacall_lifecycle_state state, int index, javacall_change_reason reason);
javacall_result nams_get_midlet_jadpath(int index, char** outPath);
javacall_result nams_get_midlet_permissions(int index, javacall_ams_permission_val* pSet);
javacall_result nams_set_midlet_permission(int index, javacall_ams_permission permission, javacall_ams_permission_val value);
javacall_result nams_get_midlet_domain(int index, javacall_ams_domain* domain);
int nams_get_current_midlet_count();
javacall_result nams_get_midlet_classname(int index, contentList* className);
char* nams_trans_state(javacall_lifecycle_state state);
char* nams_trans_ui_state(javacall_lifecycle_state state);
void nams_set_midlet_request_foreground(int index);
javacall_result nams_if_midlet_exist(int index);
void nams_set_midlet_static_info(int appID, MidletNode* pInfo);
javacall_bool nams_get_midlet_requestforeground(int index);

/*************************************************************************************************
*  NAMS data base related 
*************************************************************************************************/
javacall_result nams_db_init();
javacall_result nams_db_allocate_suiteid(javacall_suite_id* suiteID);
javacall_result nams_db_get_suiteid(char* textLine, javacall_suite_id* suiteID);
javacall_result nams_db_get_suitepath(char* textLine, char* outText);
javacall_result nams_db_open(int flag, javacall_handle* handle);
javacall_result nams_db_close(javacall_handle handle);
javacall_result nams_db_get_app_list(contentList* appList, int* entryCount);
javacall_result nams_db_read_file(char** destBuffer, long* size);
javacall_result nams_db_read_line(char** dbBuf, char** textLine, int* lineSize);
javacall_result nams_db_remove_suite_home(int removeID);
javacall_result nams_db_get_suite_home(javacall_suite_id suiteID, 
                                                  javacall_utf16* outPath, int* pathLen);
javacall_result nams_db_get_root(javacall_utf16* outPath, int* pathLen);
javacall_result nams_db_remove_line(char** dbBuf, int lineNum, long* outSize, int* outID);
javacall_result nams_db_install_app(char* textLine, javacall_utf16_string jarName);
javacall_result nams_db_remove_app(int itemIndex);

/*************************************************************************************************
*  NAMS JAD parser related 
*************************************************************************************************/
javacall_result nams_get_classname_from_jad(const javacall_utf16* jadPath, int jadPathLen, char* pClassName, int mIndex);
javacall_result nams_get_jarname_from_jad(const char* jadPath, char** jarName);
char* string_cat_num(char* propName, int mIndex);
javacall_result javautil_string_index_of_ex(char* str, char c, int times, /* OUT */ int* index);
javacall_result nams_get_midlets_from_jad(const javacall_utf16* jadPath, int jadPathLen, 
                                                    contentList* pMidlets, int* count);
javacall_result nams_get_classname_from_textline(char* textLine, char** className);

#endif /* _NAMS_H_ */
