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

#ifndef _LFPPORT_COMMAND_H_
#define _LFPPORT_COMMAND_H_

/**
 * @file
 * @ingroup highui_lfpport
 *
 * @brief Menu and soft-button porting functions and data structures.
 */

#include <lfpport_component.h>
#include <lfp_command.h>

#ifdef __cplusplus
extern "C" {
#endif

/**********************
  Menu functions
**********************/


/**
 * Called upon VM startup to allocate menu resource.
 */
void initMenus();

/**
 * Called upon VM exit to release menu resource.
 */
void finalizeMenus();

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
MidpError cmdmanager_create(MidpFrame* cmPtr);

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
				  MidpCommand* cmds, int numCmds);

/************************************
 Callback functions for porting layer
 ************************************/

/**
 * Notifies the Java platform layer when the user activates a command 
 * on a menu or a soft button.  
 *
 * This is NOT a platform dependent function and do NOT need to be ported.
 *
 * @param cmdId identifier of the selected command
 *
 * @return an indication of success or the reason for failure
 */
MidpError MidpCommandSelected(int cmdId);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LFPPORT_COMMAND_H_ */
