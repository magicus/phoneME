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

#ifndef _LFP_COMMAND_H_
#define _LFP_COMMAND_H_

#include <kni.h>
#include <ROMStructs.h>
#include <commonKNIMacros.h>
#include <midpString.h>


/**
 * @defgroup highui_lfp Platform Widget Specific External Interface
 * @ingroup highui_lcdlf
 */

/**
 * @file
 * @ingroup highui_lfp
 *
 * @brief  This file contains functions for manipulating commands and soft buttons.
 */

#ifndef COMMAND_SORT_ALL_TABLE
/**
 * Weight table for sorting all types of abstract commands.
 * Used to initialize a static constant signed 8-bit char array
 * with size of 10, as:
 * <p>
 * 	const static int8 SORT_ALL_TABLE = COMMAND_SORT_ALL_TABLE;
 * <p>
 * Commands will be first sorted by the type weight in ascending order.
 * For those with same type weight, they are then sorted by priority
 * in ascending order.
 *
 * This definition is platform specific. Each port can override this
 * default value by defining this key and its value in global configuration.
 */
#define COMMAND_SORT_ALL_TABLE { 		\
	    127, /* COMMAND_TYPE_NONE */ 	\
	    2,   /* COMMAND_TYPE_SCREEN */ 	\
	    5,   /* COMMAND_TYPE_BACK */ 	\
	    7,   /* COMMAND_TYPE_CANCEL */	\
	    3,   /* COMMAND_TYPE_OK */		\
	    4,   /* COMMAND_TYPE_HELP */	\
	    8,   /* COMMAND_TYPE_STOP */	\
	    6,   /* COMMAND_TYPE_EXIT */	\
	    1,   /* COMMAND_TYPE_ITEM */	\
	}
#endif

#ifndef COMMAND_SORT_NEGATIVE_TABLE
/**
 * Weight table for sorting all negative types of abstract commands.
 * Used to initialize a static constant signed 8-bit char array
 * with size of 10, as:
 * <p>
 * 	const static int8 SORT_NEGATIVE_TABLE = COMMAND_SORT_NEGATIVE_TABLE;
 * <p>
 * Commands will be first sorted by the type weight in ascending order.
 * For those with same type weight, they are then sorted by priority
 * in ascending order.
 *
 * Types that have the same weight as COMMAND_TYPE_NONE will never be
 * selected as a negative command.
 *
 * This definition is platform specific. Each port can override this
 * default value by defining this key and its value in global configuration.
 */
#define COMMAND_SORT_NEGATIVE_TABLE { 		\
	    127, /* COMMAND_TYPE_NONE */ 	\
	    127, /* COMMAND_TYPE_SCREEN */ 	\
	    2,   /* COMMAND_TYPE_BACK */ 	\
	    1,   /* COMMAND_TYPE_CANCEL */	\
	    127, /* COMMAND_TYPE_OK */		\
	    127, /* COMMAND_TYPE_HELP */	\
	    3,   /* COMMAND_TYPE_STOP */	\
	    4,   /* COMMAND_TYPE_EXIT */	\
	    127, /* COMMAND_TYPE_ITEM */	\
	}
#endif

#ifndef COMMAND_SORT_POSITIVE_TABLE
/**
 * Weight table for sorting all positive types of abstract commands.
 * Used to initialize a static constant signed 8-bit char array
 * with size of 10, as:
 * <p>
 * 	const static int8 SORT_POSITIVE_TABLE = COMMAND_SORT_POSITIVE_TABLE;
 * <p>
 * Commands will be first sorted by the type weight in ascending order.
 * For those with same type weight, they are then sorted by priority
 * in ascending order.
 *
 * Types that have the same weight as COMMAND_TYPE_NONE will never be
 * selected as a positive command.
 *
 * This definition is platform specific. Each port can override this
 * default value by defining this key and its value in global configuration.
 */
#define COMMAND_SORT_POSITIVE_TABLE { 		\
	    127, /* COMMAND_TYPE_NONE */ 	\
	    127, /* COMMAND_TYPE_SCREEN */ 	\
	    127, /* COMMAND_TYPE_BACK */ 	\
	    127, /* COMMAND_TYPE_CANCEL */	\
	    1,   /* COMMAND_TYPE_OK */		\
	    127, /* COMMAND_TYPE_HELP */	\
	    127, /* COMMAND_TYPE_STOP */	\
	    127, /* COMMAND_TYPE_EXIT */	\
	    127, /* COMMAND_TYPE_ITEM */	\
	}
#endif

/******************************************
 Common definitions - Should not be changed
 *****************************************/

/**
 * Abstract command type: invalid type.
 */
#define COMMAND_TYPE_NONE   0

/**
 * Abstract command type as defined in MIDP Spec: Command.SCREEN.
 */
#define COMMAND_TYPE_SCREEN 1

/**
 * Abstract command type as defined in MIDP Spec: Command.BACK.
 */
#define COMMAND_TYPE_BACK   2

/**
 * Abstract command type as defined in MIDP Spec: Command.CANCEL.
 */
#define COMMAND_TYPE_CANCEL 3

/**
 * Abstract command type as defined in MIDP Spec: Command.OK.
 */
#define COMMAND_TYPE_OK     4

/**
 * Abstract command type as defined in MIDP Spec: Command.HELP.
 */
#define COMMAND_TYPE_HELP   5

/**
 * Abstract command type as defined in MIDP Spec: Command.STOP.
 */
#define COMMAND_TYPE_STOP   6

/**
 * Abstract command type as defined in MIDP Spec: Command.EXIT.
 */
#define COMMAND_TYPE_EXIT   7

/**
 * Abstract command type as defined in MIDP Spec: Command.ITEM.
 */
#define COMMAND_TYPE_ITEM   8


/**
 * Abstract command data structure.
 * It contains information copied from a Java Command object.
 */
typedef struct {
  /**
   * Priority of this command.
   */
  int priority;
  /**
   * Short description of this command.
   */
  pcsl_string shortLabel_str;
  /**
   * Long description of this command.
   */
  pcsl_string longLabel_str;
  /**
   * Abstract command type.
   * Must be one of the type defined in this file.
   */
  unsigned int type:8;
  /**
   * Internal ID of this command.
   * When an abstract command is shown, the Display Java object assigns it
   * a ID. When a command is selected by user, the platform specific code
   * needs to notify the Display of which command is selected by passing
   * this ID to MidpCommandSelect() function.
   */
  unsigned int id:24;
} MidpCommand;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sort the given commands based upon the definition of 
 * COMMAND_SORT_ALL_TABLE.
 *
 * This is NOT a platform dependent function and do NOT need to be ported.
 * Override the definition of COMMAND_SORT_ALL_TABLE to control sorting order.
 *
 * @param  ItemCmdArray The Item Commands
 * @param  numItemCmds  The number of Item Commands
 * @param  DispCmdArray The Displayable Commands
 * @param  numDispCmds  The number of the Displayable Commands 
 *
 * @returns A sorted command array. Caller is responsible to 
 * 	    free the array after use. NULL if out of memory.
 */
MidpCommand* MidpCommandSortAll(jobject ItemCmdArray, int numItemCmds,
				jobject DispCmdArray, int numDispCmds);

/**
 * Select a command to map to a user negative action, like 
 * pressing left soft button, Cancel, No, or close window icon.
 *
 * This is NOT a platform dependent function and do NOT need to be ported.
 * The order in which the command is selected is defined in 
 * COMMAND_SORT_NEGATIVE_TABLE. Override it to control the sorting order.
 *
 * @param cmds command array to search
 * @param numOfCmds total number of commands.
 * @return index of the selected command in the passed in command array.
 */
int MidpCommandMapNegative(MidpCommand* cmds, int numOfCmds);

/**
 * Select a command to map to a user positive action, like 
 * pressing right soft button, OK, Yes, or SELECT button.
 *
 * This is NOT a platform dependent function and do NOT need to be ported.
 * The order in which the command is selected is defined in 
 * COMMAND_SORT_POSITIVE_TABLE. Override it to control the sorting order.
 *
 * @param cmds command array to search
 * @param numOfCmds total number of commands.
 * @return index of the selected command in the passed in command array.
 */
int MidpCommandMapPositive(MidpCommand* cmds, int numOfCmds);

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* _LFP_COMMAND_H_ */
