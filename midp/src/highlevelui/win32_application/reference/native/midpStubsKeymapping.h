/*
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
    {KEY_POWER,    {160, 59, 24, 24}, "POWER"},
#endif
    {KEY_SOFT1,    {32, 347, 36, 27}, "SOFT1"},
    {KEY_SOFT2,    {174, 347, 36, 27}, "SOFT2"},
    
    {KEY_UP,       {105, 343, 30, 25}, "UP"},
    {KEY_DOWN,     {105, 399, 30, 25}, "DOWN"},
    {KEY_LEFT,     {72, 370, 25, 25}, "LEFT"},
    {KEY_RIGHT,    {148, 370, 25, 25}, "RIGHT"},
    {KEY_SELECT,   {110, 371, 24, 24}, "SELECT"},
    
    {KEY_SEND,     {27, 387, 31, 36}, "SEND"},
    {KEY_END,      {187, 387, 31, 36}, "END"},
    {KEY_CLEAR,    {93, 427, 56, 29}, "CLEAR"},

    {KEY_1,        {36, 462, 39, 31}, "1"},
    {KEY_2,        {101, 466, 44, 28}, "2"},
    {KEY_3,        {170, 462, 39, 31}, "3"},
    {KEY_4,        {40, 499, 39, 31}, "4"},
    {KEY_5,        {101, 505, 44, 28}, "5"},
    {KEY_6,        {166, 500, 39, 31}, "6"},
    {KEY_7,        {46, 539, 39, 31}, "7"},
    {KEY_8,        {101, 543, 44, 28}, "8"},
    {KEY_9,        {158, 539, 39, 31}, "9"},
    {KEY_ASTERISK, {50, 582, 41, 23}, "*"},
    {KEY_0,        {101, 580, 44, 28}, "0"},
    {KEY_POUND,    {153, 582, 41, 23}, "#"},

};

#endif /* _MIDP_STUBS_KEY_MAPPING_H_ */
