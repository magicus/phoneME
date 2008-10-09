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
 * Test utility functions that produce events containing information passed to
 * the Native AMS (NAMS) listeners. Posts these events to the designated event
 * queue.  These functions are useful for writing Java programs that test the
 * NAMS API. They are not used under normal system operation.
 */

/**
 * Posts a NAMS background test event to the event queue for isolateId,
 * filling in the reason code.
 */
extern void nams_send_bg_test_event(int isolateId, jint reason);

/**
 * Posts a NAMS foreground test event to the event queue for isolateId,
 * filling in the appId and the reason code.
 */
extern void nams_send_fg_test_event(int isolateId, jint appId, jint reason);

/**
 * Posts a NAMS state change test event to the event queue for isolateId,
 * filling in the appId, the new state, and the reason code.
 */
extern void nams_send_state_test_event(int isolateId,
                                       jint appId,
                                       jint state,
                                       jint reason);
