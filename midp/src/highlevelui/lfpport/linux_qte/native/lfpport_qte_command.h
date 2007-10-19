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
 * QT menu widget for abstract command.
 */

#ifndef _LFPPORT_QTE_COMMANDS_H_
#define _LFPPORT_QTE_COMMANDS_H_

#include <qmenubar.h>
#include <qaccel.h>
#include "lfpport_qte_patched.h"

#include <lfpport_command.h>

/** Label to be used for the header of the Action menu of abstract commands.
 * Commands of type <tt>ITEM</tt>, <tt>SCREEN</tt>, and <tt>EXIT</tt> appear
 * on the action menu. */
#define ACTION_MENUITEM_TEXT	"&Action"

/** Label to be used for the header of the Go menu of abstract commands.
 * Commands of type <tt>BACK</tt>, <tt>OK</tt>, <tt>CANCEL</tt>, and
 * <tt>STOP</tt> appear on the go menu. */
#define GO_MENUITEM_TEXT	"&Go"

/** Label to be used for the header of the Help menu of abstract
 * commands. Abstract commands of type <tt>HELP</tt> appear on this menu. */
#define HELP_MENUITEM_TEXT     "&Help"


#ifdef QT_KEYPAD_MODE
#define MOVE_FOCUS  Qt::Key_Context1
#else
#define MOVE_FOCUS  Qt::Key_F1
#endif

/**
 * Extend QPopupMenu to support text truncation 
 */
class QPopupMenuExt : public PatchedQPopupMenu {
public:
    /**
     * Constructor.
     *
     * @param parent parent widget
     */
    QPopupMenuExt(QWidget *parent) : PatchedQPopupMenu(parent) { }

    /**
     * Insert item, truncating the text if it cannot fit on the screen.
     * @param text0 menu item text
     * @param id the menu item identifier
     * @return actual id
     */
    int insertItemTrunc( const QString & text0, int id);
};


/**
 * Menubar that shows all abstract commands in sub-menus:
 * Action/Go/Help.
 */
class CommandManager : public QMenuBar {
    Q_OBJECT

    /** Pointer to the the action menu */
    QPopupMenuExt *actionMenu;
    /** Pointer to the the go menu */
    QPopupMenuExt *goMenu;
    /** Pointer to the the help menu */
    QPopupMenuExt *helpMenu;

public:
    /**
     * Construct a menubar.
     *
     * @param parent parent widget
     */
    CommandManager(QWidget *parent);
 
    /**
     * Adds the given commands to the action, go, or help menus (each command
     * is assigned to the appropriate menu based on the command type).
     *
     * @param cmds array of commands to be added to the menus.
     * @param numOfCmds size of the <tt>cmds</tt> array.
     *
     * @return an indication of success or the reason for failure     
     */
    MidpError setCommands(MidpCommand* cmds, int numOfCmds);

    /**
     * Dismiss any open menu on this menu bar.
     */
    void dismissMenu();

public slots:
    /**
     * Notifies the Java platform of the given command, which the user
     * selected. 
     *
     * @param commandId identifier of the command chosen by the user.
     */
    void commandActivated(int commandId);

    /**
     * Move focus for menu label. Set focus to the first item in menu bar
     * if menu is still not in focus otherwise move focus out  
     */
    void moveFocus();
};


#endif
