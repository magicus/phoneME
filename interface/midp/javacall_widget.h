/*
 * $LastChangedDate: 2006-04-14 12:27:05 +0800 (星期五, 14 四月 2006) $  
 *
 * Copyright (c) 2005 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */ 
#ifndef __JAVACALL_WIDGET_H_
#define __JAVACALL_WIDGET_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file javacall_widget.h
 * @ingroup lcd
 * @brief Javacall interfaces for lcd
 */

#include "javacall_defs.h"
/**
 * @defgroup Widget Widget API
 * @{
 */


/**
 * Set Java Windows title text
 *
 * @return 
 */
void /*OPTIONAL*/ javacall_widget_set_title(javacall_utf16 * title, int titleLen);

/**
 * @enum javacall_widget_softbutton
 * @define the name of softbutton
 */
typedef enum {
    JAVACALL_WIDGET_LEFT_SOFTBUTTON = 0,
    JAVACALL_WIDGET_RIGHT_SOFTBUTTON,
} javacall_widget_softbutton;


void javacall_widget_set_softbutton(javacall_widget_softbutton button, javacall_utf16 * label, int labelLen);


typedef struct {
	javacall_utf16 * text;
	int textLen;
} javacall_unicode_string;


void javacall_widget_set_popupmenu(javacall_unicode_string * menuItems, 
        int* menuDisabled, int* subMenuID, int menuItemsCount);

void javanotify_widget_menu_selection(int cmd);

void javacall_widget_HideNativeEditor(void);
void javacall_widget_EnableNativeEditor(int x, int y, int w, int h, javacall_bool multiline,int constraints,int maxSize);
void javacall_widget_DisableNativeEditor(void);
void javacall_widget_SetNativeEditorContent(javacall_utf16* str, int cursorIndex, javacall_bool bUpdateIfNativeShown);
int javacall_widget_GetNativeEditorContent(javacall_utf16* buffer, int maxBufSize);
int javacall_widget_GetNativeEditorIndex(void);

/**
 * Notfy native peer widget state changed, such as key pressed in editor control.
 * Now only java TextField/TextBox has native peer.
 */
void javanotify_peerchanged_event(void);

/** @} */
    
#ifdef __cplusplus
}
#endif

#endif 
