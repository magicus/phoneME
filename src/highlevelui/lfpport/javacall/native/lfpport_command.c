/*
 *
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
 */

/**
 * @file
 * @ingroup highui_lfpport
 *
 * @brief Menu and soft-button porting functions and data structures.
 */
#include <lfpport_component.h>
#include <lfp_command.h>
#include "lfpport_gtk.h"
#include "javacall_keypress.h"

#ifdef __cplusplus
extern "C" {
#endif

extern GtkWidget *main_window;


/**
 * Show the menu upon Java's request.
 *
 * @param cmPtr pointer to menu native peer
 * @return status of this call
 */
MidpError cmdmanager_show(MidpFrame *cmPtr) {
    /* Suppress unused-parameter warning */
    (void)cmPtr;
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

/**
 * Hide and delete resource function pointer.
 * This function should notify its Items to hide as well.
 *
 * @param cmPtr pointer to menu native peer
 * @param onExit  true if this is called during VM exit.
 * 		  All native resource must be deleted in this case.
 * @return status of this call
 */
MidpError
cmdmanager_hide_and_delete(MidpFrame* cmPtr, jboolean onExit) {
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}



/**
 * Create native resources (like menu and/or soft buttons) to show
 * a set of commands.
 *
 * The resources should not be visible after the creation.
 * When this function returns successfully, *cmPtr should be populated.
 *
 * @param cmPtr pointer to the command manager's MidpFrame structure.
 * @return an indication of success or the reason for failure
 */
MidpError cmdmanager_create(MidpFrame* cmPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    cmPtr->widgetPtr = main_window;

    cmPtr->show = cmdmanager_show;
    cmPtr->hideAndDelete = cmdmanager_hide_and_delete;
    cmPtr->handleEvent = NULL;

    LIMO_TRACE("<<<%s\n", __FUNCTION__);

    if (cmPtr->widgetPtr == NULL) {
        return KNI_ENOMEM;
    }
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

static int priorities[] = {
    8, /* COMMAND_TYPE_NONE */
    8, /* COMMAND_TYPE_SCREEN */
    0, /* COMMAND_TYPE_BACK */
    2, /* COMMAND_TYPE_CANCEL */
    8, /* COMMAND_TYPE_OK */
    8, /* COMMAND_TYPE_HELP */
    3, /* COMMAND_TYPE_STOP */
    1, /* COMMAND_TYPE_EXIT */
    8  /* COMMAND_TYPE_ITEM */
};

gint softkey_cb(GtkWidget *widget, GtkSoftkeyPosition softkey_position, gpointer data) {
    LIMO_TRACE(">>>%s id\n", __FUNCTION__);
    LIMO_TRACE(">>>%s id is %d\n", __FUNCTION__, (int)data);
    javanotify_widget_menu_selection((int)data);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return 0;
}

//void menuitem_activate(void){
void menuitem_activate(GtkWidget *widget, gpointer data){
    LIMO_TRACE(">>>%s id is %d\n", __FUNCTION__, (int)data);
    javanotify_widget_menu_selection((int)data);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
}

/**
 * Update the contents of the given command manager.
 *
 * @param cmPtr pointer to the MidpFrame structure.
 * @param cmds the array of commands the menu should contain
 * @param numCmds size of the array
 *
 * @return an indication of success or the reason for failure
 */
MidpError cmdmanager_set_commands(MidpFrame* cmPtr,
				  MidpCommand* cmds, int numCmds){
    GtkMainWindow* gtkMainWindow;
    GtkForm *form;
    GtkWidget *gtkSoftKeyBar;
    GtkWidget *menu;
    GtkWidget *menuitem;
    int menuIsNeededFlag = 0;
    int priority, place;
    gchar label_buf[MAX_TEXT_LENGTH];
    int i;
    int label_len;

    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    /* IMPL_NOTE:  size of critical section can be reduced
     * by removing string manipulation methods to improve
     * performance */

    pthread_mutex_lock(&mutex);
    gtkMainWindow = (GtkMainWindow* )cmPtr->widgetPtr;
    form = gtk_main_window_get_current_form(main_window);
    if (form == NULL) {
        return KNI_OK;
    }
    gtkSoftKeyBar = form->softkeybar;

    if (numCmds <= 0) {
        LIMO_TRACE("<<<%s no commands! returning...\n", __FUNCTION__);
        pthread_mutex_unlock(&mutex);
        return KNI_OK;
    }

    gtk_widget_show (gtkSoftKeyBar);

    //find (if any) command to populate the left softbutton
    place = -1;
    priority = 8;
    for (i = 0; i < numCmds; i++) {
        if (priority > priorities[cmds[i].type]) {
            priority = priorities[cmds[i].type];
            place = i;
        }
    }
    //set left softbutton
    if (place != -1) {
        pcsl_string_convert_to_utf8(&cmds[place].shortLabel_str,
                                    label_buf, MAX_TEXT_LENGTH, &label_len);
        gtk_softkey_bar_set_softkey(gtkSoftKeyBar, LEFT_SOFTKEY, label_buf,
             NULL, SOFTKEY_CALLBACK, softkey_cb, cmds[place].id);
    }

    //determine whether the right softbutton is a pop-up menu or a command
    if (numCmds == 2 && place == -1 ||
        numCmds > 2) {
        menuIsNeededFlag = 1;
    }
    //create menu - if needed
    if (menuIsNeededFlag == 1) {
        menu = gtk_menu_new();
        gtk_widget_show(menu);
        for (i = 0; i < numCmds; i++) {
            if (i != place) {
                pcsl_string_convert_to_utf8(&cmds[i].shortLabel_str,
                                            label_buf, MAX_TEXT_LENGTH, &label_len);
                menuitem = gtk_menu_item_new_with_label(label_buf);
                g_signal_connect(G_OBJECT (menuitem),"activate",
                           G_CALLBACK(menuitem_activate), cmds[i].id);
//                 g_signal_connect(G_OBJECT (menuitem),"activate",
//                            G_CALLBACK(menuitem_activate), NULL);
                gtk_widget_show(menuitem);
                gtk_menu_shell_append(GTK_MENU_SHELL (menu), menuitem);
            }
        }
        gtk_softkey_bar_set_softkey(gtkSoftKeyBar, RIGHT_SOFTKEY, "Menu",
                   NULL, SOFTKEY_MENU, menu, NULL);
    }
    else {  //set right softbutton
        /* find the command for left softkey */
        for (i = 0; i < numCmds; i++) {
            if (i != place) {
                pcsl_string_convert_to_utf8(&cmds[i].shortLabel_str,
                                            label_buf, MAX_TEXT_LENGTH, &label_len);
                gtk_softkey_bar_set_softkey(gtkSoftKeyBar, RIGHT_SOFTKEY, label_buf,
                     NULL, SOFTKEY_CALLBACK, softkey_cb, cmds[i].id);
            }
        }
    }

    pthread_mutex_unlock(&mutex);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
