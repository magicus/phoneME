/*
 * 	
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
    KeyType button;
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
    {KEY_POWER,    {-10, -10,  1,  1}, "POWER"},
#else
    {KEY_POWER,    {266, 21, 30, 30}, "POWER"},
#endif
    {KEY_SOFT1,    {80, 420, 40, 28}, "SOFT1"},
    {KEY_SOFT2,    {240, 420, 40, 28}, "SOFT2"},
    
    {KEY_UP,       {163, 417, 38, 18}, "UP"},
    {KEY_DOWN,     {163, 452, 38, 18}, "DOWN"},
    {KEY_LEFT,     {125, 425, 24, 33}, "LEFT"},
    {KEY_RIGHT,    {214, 425, 24, 33}, "RIGHT"},
    {KEY_SELECT,   {157, 436, 50, 15}, "SELECT"},
    
    {KEY_SEND,     {53, 457, 61, 31}, "SEND"},
    {KEY_END,      {246, 457, 61, 31}, "END"},
    {KEY_CLEAR,    {138, 472, 85, 40}, "CLEAR"},

    {KEY_1,        {56, 494, 73, 37}, "1"},
    {KEY_2,        {140, 514, 84, 37}, "2"},
    {KEY_3,        {231, 494, 73, 37}, "3"},
    {KEY_4,        {56, 533, 73, 37}, "4"},
    {KEY_5,        {140, 553, 84, 37}, "5"},
    {KEY_6,        {231, 533, 73, 37}, "6"},
    {KEY_7,        {56, 572, 73, 37}, "7"},
    {KEY_8,        {140, 592, 84, 37}, "8"},
    {KEY_9,        {231, 572, 73, 37}, "9"},

    {KEY_ASTERISK, {56, 611, 73, 37}, "*"},
    {KEY_0,        {140, 631, 84, 37}, "0"},
    {KEY_POUND,    {231, 611, 73, 37}, "#"},

};

#endif /* _MIDP_STUBS_KEY_MAPPING_H_ */
