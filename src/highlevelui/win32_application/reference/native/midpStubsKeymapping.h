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

#ifndef _MIDP_STUBS_KEY_MAPPING_H_
#define _MIDP_STUBS_KEY_MAPPING_H_

#include <keymap_input.h>

/**
 * @file
 *
 * This is the native key mapping for Windows.
 */

typedef struct _Rectangle {
    int x;
    int y;
    int width;
    int height;
} XRectangle;

typedef struct {
    KeymapKeyCode button;
    XRectangle bounds;
    char *name;
} WKey;

/**
 * Do not alter the sequence of this
 * without modifying the one in .cpp 
 */
const static WKey Keys[] = {
#ifdef NO_POWER_BUTTON
    /*
     * Add -DNO_POWER_BUTTON to the Makefile if you want to disable
     * the power button during user testing.
     */
    {KEYMAP_KEY_POWER,    {-10, -10,  1,  1},   "POWER"},
#else
    {KEYMAP_KEY_POWER,    {266, 21, 30, 30},    "POWER"},
#endif
    {KEYMAP_KEY_SOFT1,    {80, 420, 40, 28},    "SOFT1"},
    {KEYMAP_KEY_SOFT2,    {240, 420, 40, 28},   "SOFT2"},
    
    {KEYMAP_KEY_UP,       {163, 417, 38, 18},   "UP"},
    {KEYMAP_KEY_DOWN,     {163, 452, 38, 18},   "DOWN"},
    {KEYMAP_KEY_LEFT,     {125, 425, 24, 33},   "LEFT"},
    {KEYMAP_KEY_RIGHT,    {214, 425, 24, 33},   "RIGHT"},
    {KEYMAP_KEY_SELECT,   {157, 436, 50, 15},   "SELECT"},
    
    {KEYMAP_KEY_SEND,     {53, 457, 61, 31},    "SEND"},
    {KEYMAP_KEY_END,      {246, 457, 61, 31},   "END"},
    {KEYMAP_KEY_CLEAR,    {138, 472, 85, 40},   "CLEAR"},
 
    {KEYMAP_KEY_1,        {56, 494, 73, 37},    "1"},
    {KEYMAP_KEY_2,        {140, 514, 84, 37},   "2"},
    {KEYMAP_KEY_3,        {231, 494, 73, 37},   "3"},
    {KEYMAP_KEY_4,        {56, 533, 73, 37},    "4"},
    {KEYMAP_KEY_5,        {140, 553, 84, 37},   "5"},
    {KEYMAP_KEY_6,        {231, 533, 73, 37},   "6"},
    {KEYMAP_KEY_7,        {56, 572, 73, 37},    "7"},
    {KEYMAP_KEY_8,        {140, 592, 84, 37},   "8"},
    {KEYMAP_KEY_9,        {231, 572, 73, 37},   "9"},

    {KEYMAP_KEY_ASTERISK, {56, 611, 73, 37},    "*"},
    {KEYMAP_KEY_0,        {140, 631, 84, 37},   "0"},
    {KEYMAP_KEY_POUND,    {231, 611, 73, 37},   "#"},

};

#endif /* _MIDP_STUBS_KEY_MAPPING_H_ */
