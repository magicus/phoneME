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

/**
 * @file
 *
 * This file contains common the <b>internal</b> abstracted command and
 * menu related functions.
 */


#include <stdlib.h>

#include <midpMalloc.h>
#include <lfp_command.h>

/**
 * Get a C structure representing the given <tt>Command</tt> class.
 *
 * This requires the VM romization enabled.
 */
#define getMidpCommandPtr(handle) \
    (unhand(struct Java_javax_microedition_lcdui_Command, (handle)))
#include <midpUtilKni.h>

static const char SORT_ALL_TABLE[10] = COMMAND_SORT_ALL_TABLE;

static const char SORT_NEGATIVE_TABLE[10] = COMMAND_SORT_NEGATIVE_TABLE;

static const char SORT_POSITIVE_TABLE[10] = COMMAND_SORT_POSITIVE_TABLE;

static int
compare(const MidpCommand  *a, const  MidpCommand *b, const char table[]) {
    if (a->type == b->type) {
        return a->priority - b->priority;
    } else {
        int aPos =
            ((a->type < 1) || (a->type > 8)) ? 127 : table[a->type];
        int bPos =
            ((b->type < 1) || (b->type > 8)) ? 127 : table[b->type];

        return aPos - bPos;
    }
}

static int
compareForAll(const void *first, const void *second) {
    return compare((MidpCommand *)first, (MidpCommand *)second, SORT_ALL_TABLE);
}

/**
 * Select a command to map to a user negative or positive action.
 *
 * This is NOT a platform dependent function and do NOT need to be ported.
 * The order in which the command is selected is defined in 
 * table parameter. Override it to control the sorting order.
 *
 * @param cmds command array to search
 * @param numOfCmds total number of commands.
 * @return index of the selected command in the passed in command array.
 *	   -1 if no matching command is found.
 */
static int
mapCommand(MidpCommand* cmds, int numOfCmds, const char table[]) {
    int i, candidate;
    
    if (numOfCmds <= 0) {
	return -1;
    }

    candidate = 0; // Initial candidate index
    
    // Search for most likely candidate for a negative action
    for (i = 1; i < numOfCmds; i++) {
	if (compare(cmds+i, cmds+candidate, table) < 0) {
	    candidate = i;
	}
    }

    // Check if we have found any negative/positive command
    if (table[cmds[candidate].type] == table[COMMAND_TYPE_NONE]) {
	// Even the most likely candidate is not with the desired type
	return -1;
    } else {
	return candidate;
    }
}

/**
 * Sort the given commands based upon the definition of 
 * COMMAND_SORT_ALL_TABLE.
 *
 * This is NOT a platform dependent function and do NOT need to be ported.
 * Override the definition of COMMAND_SORT_ALL_TABLE to control sorting order.
 *
 * @param  ItemCmdArray The Item Command Java array
 * @param  numItemCmds  The number of Item Commands
 * @param  DispCmdArray The Displayable Command Java array
 * @param  numDispCmds  The number of the Displayable Commands 
 *
 * @returns A sorted command array. Caller is responsible to 
 * 	    free the array after use. NULL if out of memory.
 */
MidpCommand* MidpCommandSortAll(jobject ItemCmdArray, int numItemCmds,
				jobject DispCmdArray, int numDispCmds) {
    int nc = numItemCmds + numDispCmds;
    /* Need to allocate an extra command for qsort. */
    MidpCommand *c = (MidpCommand*)midpMalloc((nc+1)*sizeof(MidpCommand));

    jobjectArray  itemCmds = (jobjectArray)ItemCmdArray;
    jobjectArray  cmds = (jobjectArray)DispCmdArray;

    int      j;

    if (c == NULL) {
        return NULL;
    }

    KNI_StartHandles(2);
    KNI_DeclareHandle(i);
    KNI_DeclareHandle(str);

    /*
     * We need to copy the string data (not just keep a
     * pointer to it) because if the garbage collector is allowed
     * to move the contents of the heap, the pointers will become
     * invalid.
     */
    for (j = 0; j < nc; ++j) {
        /* First fill c array with Item commands from ItemCmdArray;
         * then fill it with Displayable commands from DispCmdArray;
         * Later all these commands will be sorted together.
         */
        if (j < numItemCmds) {
            KNI_GetObjectArrayElement(itemCmds, j, i);
        } else {
            KNI_GetObjectArrayElement(cmds, j - numItemCmds, i);
        }

        str = hand((getMidpCommandPtr(i)->shortLabel));
        if(PCSL_STRING_OK
                != midp_jstring_to_pcsl_string(str, &c[j].shortLabel_str)) {
            break;
        }

        str = hand(getMidpCommandPtr(i)->longLabel);
        if(PCSL_STRING_OK
                != midp_jstring_to_pcsl_string(str, &c[j].longLabel_str)) {
            pcsl_string_free(&c[j].shortLabel_str);
            break;
        }
        c[j].priority  = (int)(getMidpCommandPtr(i)->priority);
        c[j].type  = (int)(getMidpCommandPtr(i)->commandType);
        c[j].id  = (int)(getMidpCommandPtr(i)->id);

    } /* end for (j=0; j<nc; ++j); */

    if (j < nc) {
        /* Whoops! We need to undo all previous allocs */
        for (j--; j >= 0; j--) {
            pcsl_string_free(&c[j].longLabel_str);
            pcsl_string_free(&c[j].shortLabel_str);
        }
        midpFree(c);
        c = NULL;
    } else if (nc > 1) {
	qsort(c, nc, sizeof(MidpCommand), compareForAll);
    }

    KNI_EndHandles();
    return c;
}

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
 *	   -1 if no negative command is found.
 */
int MidpCommandMapNegative(MidpCommand* cmds, int numOfCmds) {
    return mapCommand(cmds, numOfCmds, SORT_NEGATIVE_TABLE);
}

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
 *	   -1 if no positive command is found.
 */
int MidpCommandMapPositive(MidpCommand* cmds, int numOfCmds) {
    return mapCommand(cmds, numOfCmds, SORT_POSITIVE_TABLE);
}
