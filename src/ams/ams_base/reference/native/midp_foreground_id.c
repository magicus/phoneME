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

#include <midp_foreground_id.h>

/** Isolate ID of current foreground MIDlet. */
int gForegroundIsolateId = 0;

#if ENABLE_MULTIPLE_DISPLAYS

/** Foreground display IDs of current foreground MIDlet. */
int *gForegroundDisplayIds;
int maxDisplays = 2; /* Should be equal to number of the hardware displays */


int isForegroundDisplay(int displayId) {
  int ret = 0;
  int i;
  for (i = 0; i < maxDisplays; i++) {
    if (gForegroundDisplayIds[i] == displayId) {
      ret = 1;
      break;
    }
  }
  return ret;
}
#else
/** Foreground display ID of current foreground MIDlet. */
int gForegroundDisplayId = 0;
#endif /* ENABLE_MULTIPLE_DISPLAYS */
