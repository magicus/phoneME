/*
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

#ifndef _LCD_H_
#define _LCD_H_

/*
 * Translates screen coordinates into displayable coordinate system.
 */
void getScreenCoordinates(short screenX, short screenY, short* x, short* y);

/* Rotates display according to code.
 * If code is 0 no screen transformations made;
 * If code is 1 then screen orientation is reversed.
 * if code is 2 then screen is turned upside-down.
 * If code is 3 then both screen orientation is reversed
 * and screen is turned upside-down.
 */
void RotateDisplay(short code);


#endif /* _LCD_H_ */
