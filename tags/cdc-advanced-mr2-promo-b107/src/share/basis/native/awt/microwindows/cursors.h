/*
 * @(#)cursors.h	1.12 06/10/10
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
 *
 */

#include "mwtypes.h"


#define CURSOR_WIDTH 16
#define CURSOR_HEIGHT 16

extern MWIMAGEBITS mwawt_defaultCursor[16];
extern MWIMAGEBITS mwawt_defaultCursorMask[16];
extern MWIMAGEBITS mwawt_crosshair[16];
extern MWIMAGEBITS mwawt_crosshairMask[16];
extern MWIMAGEBITS mwawt_eResize[16];
extern MWIMAGEBITS mwawt_eResizeMask[16];
extern MWIMAGEBITS mwawt_handcursor[16];
extern MWIMAGEBITS mwawt_handcursorMask[16];
extern MWIMAGEBITS mwawt_nResize[16];
extern MWIMAGEBITS mwawt_nResizeMask[16];
extern MWIMAGEBITS mwawt_neResize[16];
extern MWIMAGEBITS mwawt_neResizeMask[16];
extern MWIMAGEBITS mwawt_nwResize[16];
extern MWIMAGEBITS mwawt_nwResizeMask[16];
extern MWIMAGEBITS mwawt_sResize[16];
extern MWIMAGEBITS mwawt_sResizeMask[16];
extern MWIMAGEBITS mwawt_seResize[16];
extern MWIMAGEBITS mwawt_seResizeMask[16];
extern MWIMAGEBITS mwawt_swResize[16];
extern MWIMAGEBITS mwawt_swResizeMask[16];
extern MWIMAGEBITS mwawt_textcursor[16];
extern MWIMAGEBITS mwawt_textcursorMask[16];
extern MWIMAGEBITS mwawt_wResize[16];
extern MWIMAGEBITS mwawt_wResizeMask[16];
extern MWIMAGEBITS mwawt_waitcursor[16];
extern MWIMAGEBITS mwawt_waitcursorMask[16];
extern MWIMAGEBITS mwawt_movecursor[16];
extern MWIMAGEBITS mwawt_movecursorMask[16];

