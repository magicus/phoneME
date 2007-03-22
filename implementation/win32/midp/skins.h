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

#ifndef __SKINS_H
#define __SKINS_H

#include "res/resource.h"

#define SKINS_MENU_SUPPORTED

#define NUMBEROF(x) (sizeof(x)/sizeof(x[0]))

/* key definitons */
typedef struct _Rectangle {
    int x;
    int y;
    int width;
    int height;
} XRectangle;

typedef struct {
    javacall_key button;
    XRectangle bounds;
    char *name;
} WKey;

typedef struct {
   /*
    * Display bounds
    */
    XRectangle displayRect;
    /*
     * Key mapping
     */
    const WKey* Keys;
    /*
     * Number of keys
     */
    int keyCnt;

    /*
     * Emulator skin bitmap resource ID
     */
    int resourceID;
    /*
     * Emulator skin bitmap
     */
    HBITMAP hBitmap;

} ESkin;

#define KEY_POWER  (JAVACALL_KEY_GAME_RIGHT - 100)
#define KEY_END    (JAVACALL_KEY_GAME_RIGHT - 101)
#define KEY_SEND   (JAVACALL_KEY_GAME_RIGHT - 102)

const static WKey VKeys[] = {
#ifdef NO_POWER_BUTTON
{KEY_POWER,    {-10, -10,  1,  1}, "POWER"},
#else
{KEY_POWER,    {160, 59, 24, 24}, "POWER"},
#endif

//#define USE_SWAP_SOFTBUTTON
#ifndef USE_SWAP_SOFTBUTTON // !USE_SWAP_SOFTBUTTON 
{JAVACALL_KEY_SOFT1,    {78, 420, 40, 35}, "SOFT1"},//
{JAVACALL_KEY_SOFT2,    {241, 424, 40, 35}, "SOFT2"},//
#else // USE_SWAP_SOFTBUTTON 
{JAVACALL_KEY_SOFT2,    {78, 420, 40, 35}, "SOFT2"},//
{JAVACALL_KEY_SOFT1,    {241, 424, 40, 35}, "SOFT1"},//
#endif

{JAVACALL_KEY_UP,       {169, 421, 24, 9}, "UP"},//
{JAVACALL_KEY_DOWN,     {169, 454, 24, 9}, "DOWN"},//
{JAVACALL_KEY_LEFT,     {132, 431, 9, 24}, "LEFT"},//
{JAVACALL_KEY_RIGHT,    {218, 431, 9, 24}, "RIGHT"},//
{JAVACALL_KEY_SELECT,   {162, 434, 39, 15}, "SELECT"},//

{JAVACALL_KEY_SEND,     {60, 454, 51, 31}, "SEND"},//
{KEY_END,               {253, 454, 51, 31}, "END"},//
{JAVACALL_KEY_CLEAR,    {150, 478, 60, 28}, "CLEAR"},//

{JAVACALL_KEY_1,        {64, 500, 60, 29}, "1"},//
{JAVACALL_KEY_2,        {146, 519, 70, 26}, "2"},//
{JAVACALL_KEY_3,        {237, 500, 60, 29}, "3"},//
{JAVACALL_KEY_4,        {66, 534, 60, 29}, "4"},//
{JAVACALL_KEY_5,        {146, 554, 70, 26}, "5"},//
{JAVACALL_KEY_6,        {233, 537, 60, 29}, "6"},//
{JAVACALL_KEY_7,        {68, 569, 60, 29}, "7"},//
{JAVACALL_KEY_8,        {146, 591, 70, 26}, "8"},//
{JAVACALL_KEY_9,        {234, 575, 60, 29}, "9"},//
{JAVACALL_KEY_ASTERISK, {73, 610, 60, 29}, "*"},//
{JAVACALL_KEY_0,        {146, 628, 70, 26}, "0"},//
{JAVACALL_KEY_POUND,    {228, 612, 60, 29}, "#"},//

};

static ESkin VSkin = {
    {56, 94, 240, 320}, //displayRect
    VKeys,
    NUMBEROF(VKeys),
    IDB_BITMAP_PHONE_V, //resource ID
    NULL //hBitmap
};

static ESkin HSkin = {
    {56, 94, 320, 240}, //displayRect
    NULL, 0,
    IDB_BITMAP_PHONE_H, //resource ID
    NULL //hBitmap
};


#endif /* __SKINS_H */
