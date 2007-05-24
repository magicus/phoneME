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
 * 
 * This source file is specific for Qt-based configurations.
 */

/**
 * @file
 * Command manager for Qt.
 */

#include "lfpport_qte_command.h"
#include <moc_lfpport_qte_command.cpp>
#include "lfpport_qte_util.h"
#include "lfpport_qte_mscreen.h"
#include "lfpport_qte_mainwindow.h"
#include <qmessagebox.h>
#include <qlayout.h>
#include <midpUtilKni.h>

int QPopupMenuExt::insertItemTrunc( const QString & text0, int id) {
    QString text(text0);
    truncateQString(text,
       font(),
       qteapp_get_application()->mainWidget()->width()-16);/*magic number*/
    return insertItem(text,id);
}

/**
 * Constructor of CommandManager.
 *
 * @param parent parent widget
 */
CommandManager::CommandManager(QWidget *parent)
    : QMenuBar(parent)
{
    actionMenu = new QPopupMenuExt(parent);
    goMenu = new QPopupMenuExt(parent);
    helpMenu = new QPopupMenuExt(parent);

    // Listen on menu item selection
    connect(actionMenu, SIGNAL(activated(int)), SLOT(commandActivated(int)));
    connect(goMenu, SIGNAL(activated(int)), SLOT(commandActivated(int)));
    connect(helpMenu, SIGNAL(activated(int)), SLOT(commandActivated(int)));
    
#ifndef QT_NO_ACCEL
    QAccel* focusAccel = new QAccel(this, "focus-change accelerator");
    focusAccel->connectItem(focusAccel->insertItem(MOVE_FOCUS, 0), 
        this, SLOT(moveFocus()));
#endif

}

/**
 * Notify Java peer that a Menu item is selected.
 *
 * @param commandId Id of the selected abstract command
 */
void CommandManager::commandActivated(int commandId) {
    // Fire a Java abstract command event

    MidpCommandSelected(commandId);
}

/**
 * Move focus for menu label. Set focus to the first item in menu bar
 * if menu is still not in focus otherwise move focus out  
 */
void CommandManager::moveFocus() {
    activateItemAt(hasFocus() ? -1 : 0);
}

/**
 * Add an item to the menu. It will be inserted into the 
 * proper popup menu basing on its type.
 *
 * @param cmds new abstract commands
 * @param numOfCmds number of new abstract commands
 * @return status of this call
 */
MidpError CommandManager::setCommands(MidpCommand* cmds, int numOfCmds) {
    MidpError err = KNI_OK;
    int i = count()-1;

    // Remove any popup menu
    for (; i >= 0; i--) {
	    removeItemAt(0);
    }

    // Clear existing popup menu content
    actionMenu->clear();
    goMenu->clear();
    helpMenu->clear();

    // reset state of menu bar when item is active
    actItem = -1;

    // Insert each command to each popup menu basing on its type
    for (i = 0; i < numOfCmds; i++) {
        QString text;
        pcsl_string* short_label = &cmds[i].shortLabel_str;
        GET_PCSL_STRING_DATA_AND_LENGTH(short_label) {
            if(PCSL_STRING_PARAMETER_ERROR(short_label)) {
                REPORT_ERROR(LC_HIGHUI, "out-of-memory error"
                             " in CommandManager::setCommands");
            } else {
                text.setUnicodeCodes(short_label_data,
                                     short_label_len);
            }
        } RELEASE_PCSL_STRING_DATA_AND_LENGTH

        if (cmds[i].type == COMMAND_TYPE_HELP) {
            helpMenu->insertItemTrunc(text, cmds[i].id);

        } else if (cmds[i].type == COMMAND_TYPE_BACK
            || cmds[i].type == COMMAND_TYPE_OK
            || cmds[i].type == COMMAND_TYPE_CANCEL
            || cmds[i].type == COMMAND_TYPE_STOP) {
            goMenu->insertItemTrunc(text, cmds[i].id);
        } else {
            actionMenu->insertItemTrunc(text, cmds[i].id);
        }
    }

    // Only add non-empty popup menus
    if (goMenu->count() > 0) {
	    insertItem(GO_MENUITEM_TEXT, goMenu, -1, 0);
    }
    if (actionMenu->count() > 0) {
	    insertItem(ACTION_MENUITEM_TEXT, actionMenu, -1, 0);
    }
    if (helpMenu->count() > 0) {
        insertItem(HELP_MENUITEM_TEXT, helpMenu, -1, 0);
    }
    return err;
}

/**
 * Dismiss any popup menu that is opening.
 */
void CommandManager::dismissMenu() {
    // Hide any popup menu
    actionMenu->hide();
    goMenu->hide();
    helpMenu->hide();
}

/**
 * Show the menu upon Java's request.
 * Not supported in Qt port since user triggers menu popup through Qt.
 * Do nothing here.
 *
 * @param cmPtr pointer to menu native peer
 * @return status of this call
 */
extern "C" MidpError
cmdmanager_show(MidpFrame *cmPtr) {
    /* Suppress unused-parameter warning */
    (void)cmPtr;

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
extern "C" MidpError
cmdmanager_hide_and_delete(MidpFrame* cmPtr, jboolean onExit) {
    CommandManager *cmdMgr = (CommandManager *)cmPtr->widgetPtr;

    if (onExit) {
	// Delete Command Manager and its children popup menus
	delete cmdMgr;
    } else {
	// Hide any popup menu
	cmdMgr->dismissMenu();
    }

    return KNI_OK;
}

/**
 * Create the menubar.
 * @param cmPtr pointer to pre-allocated MidpFrame structure whose fields
 *		should be set properly upon return.
 * @return status of this call
 */
extern "C"
MidpError cmdmanager_create(MidpFrame* cmPtr)
{
    cmPtr->widgetPtr 		= 
      PlatformMIDPMainWindow::getMainWindow()->getMenu();
    cmPtr->show 		= cmdmanager_show;
    cmPtr->hideAndDelete	= cmdmanager_hide_and_delete;
    cmPtr->handleEvent		= NULL; // Qt uses SIGNAL/SLOT instead

    if (cmPtr->widgetPtr == NULL) {
	return KNI_ENOMEM;
    } else {
	return KNI_OK;
    }
}

/**
 * Update content of the menu.
 *
 * @param cmPtr pointer to menu native peer
 * @param cmds array of commands the menu should contain
 * @param numCmds size of the array
 * @return status of this call
 */
MidpError cmdmanager_set_commands(MidpFrame* cmPtr,
				  MidpCommand* cmds, int numCmds)
{
    return ((CommandManager *)cmPtr->widgetPtr)->setCommands(cmds, numCmds);
}
