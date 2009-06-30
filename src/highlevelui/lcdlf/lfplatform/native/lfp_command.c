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

#include <lfp_registry.h>
#include "lfp_intern_registry.h"

#include <lfpport_command.h>
#include <lfpport_alert.h>
#include <lfpport_choicegroup.h>

#include <midpMalloc.h>
#include <midpEvents.h>
#include <midpEventUtil.h>
#include <midpError.h>

/**
 * @file
 * Cross platform Abstract Commands related functions.
 */

/**
 * The singleton abstract command manager resource.
 */
static MidpFrame commandManager;

/**
 * Free an abstract command list.
 *
 * @param commands pointer to the command list
 * @param length size of the list
 */
static void
freeCommandList(MidpCommand *commands, int length) {
    int i;
    
    if (commands != NULL) {
        /* Free all of the command strings */
        for (i = 0; i < length; ++i) {
            pcsl_string_free(&commands[i].shortLabel_str);
            pcsl_string_free(&commands[i].longLabel_str);
	}
        midpFree(commands);
    }
}

/**
 * Called upon VM startup to allocate menu resource.
 */
void
initMenus() {
    cmdmanager_create(&commandManager);
}

/**
 * Called upon VM exit to release menu resource.
 */
void
finalizeMenus() {
    commandManager.hideAndDelete(&commandManager, KNI_TRUE);
}

/**
 * Re-populate contents of menu and soft buttons.
 * Upon return, the passed-in command list will be freed already.
 *
 * @param commands abstract command list that has been sorted by priority
 * @param length size of the list
 */
static void
MidpCommandSetAll(MidpCommand *commands, int length) {

    if (MidpCurrentScreen != NULL
	&& MidpCurrentScreen->component.type >= MIDP_NULL_ALERT_TYPE
	&& MidpCurrentScreen->component.type <= MIDP_CONFIRMATION_ALERT_TYPE) {
	/* Delegate to alert to handle */
	lfpport_alert_set_commands(MidpCurrentScreen, commands, length);
    } else {
	/* Delegate to ported command manager */
	cmdmanager_set_commands(&commandManager, commands, length);
    }

    freeCommandList(commands, length);
}

/**
 * Set the current set of active Abstract Commands.
 * <p>
 * Java declaration:
 * <pre>
 *     updateCommands([Ljavax/microedition/lcdui/Command;I
 *                    [Ljavax/microedition/lcdui/Command;I)V
 * </pre>
 * Java parameters:
 * <pre>
 *     itemCommands    The list of Item Commands that 
 *                     should be active
 *     numItemCommands The number of commands in the list 
 *                     of Item Commands
 *     commands        The list of Commands that should be active
 *     numCommands     The number of commands in the list of 
 *                     Commands
 *     returns:        void
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_NativeMenu_updateCommands() {
    int numItemCmds = KNI_GetParameterAsInt(2);
    int numCmds     = KNI_GetParameterAsInt(4);

    KNI_StartHandles(2);

    KNI_DeclareHandle(itemCmds);
    KNI_DeclareHandle(cmds);

    KNI_GetParameterAsObject(1, itemCmds);
    KNI_GetParameterAsObject(3, cmds);

    if (numItemCmds == 0 && numCmds == 0) {
        MidpCommandSetAll(NULL, 0);
    } else {
        MidpCommand *menuList = MidpCommandSortAll(itemCmds, numItemCmds,
						   cmds, numCmds);
        if (menuList != NULL ) {
            MidpCommandSetAll(menuList, numItemCmds + numCmds);
        } else {
            MidpCommandSetAll(NULL, 0);
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Calls to platform specific function to draw the command menu on the screen.
 * <p>
 * Java declaration:
 * <pre>
 *     showMenu()V
 * </pre>
 * Java parameters:
 * <pre>
 *   parameters:  none
 *   returns:     void
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_NativeMenu_showMenu() {
    commandManager.show(&commandManager);
    KNI_ReturnVoid();
}

/**
 * Call to platform specific function to dismiss the current menu or
 * popup in the case of setCurrent() being called while the Display
 * is suspended by a system screen.
 * <p>
 * Java declaration:
 * <pre>
 *     dismissMenuAndPopup()V
 * </pre>
 * Java parameters:
 * <pre>
 *   parameters:  none
 *   returns:     void
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_NativeMenu_dismissMenuAndPopup() {
    // Static commandManager is shared across Displays,
    // following call should only dismiss but not delete
    // the platform widget. It should only be deleted in
    // finalizeMenus() function
    commandManager.hideAndDelete(&commandManager, KNI_FALSE);
    lfpport_choicegroup_dismiss_popup();
    KNI_ReturnVoid();
}

/**
 * Java notification function for command activated either through menu or
 * soft button.
 *
 * This is NOT a platform dependent function and does NOT need to be ported.
 *
 * @param cmdId identifier of the selected command
 *
 * @return an indication of success or the reason for failure
 */
MidpError
MidpCommandSelected(int cmdId) {
    MidpEvent event;

    MIDP_EVENT_INITIALIZE(event);

    event.type = MIDP_COMMAND_EVENT;
    event.COMMAND = cmdId;

    midpStoreEventAndSignalForeground(event);

    return ALL_OK;  
}
