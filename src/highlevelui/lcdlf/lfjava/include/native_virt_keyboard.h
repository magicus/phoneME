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
 
#ifndef _NATIVE_VIRT_KEYBOARD_H_
#define _NATIVE_VIRT_KEYBOARD_H_

#include <javacall_defs.h>
#include <kni.h>

/**
 * Displays the native keyboard and returns the entered string.
 * @param prepopulate prepopulate string
 * @param mode  mode value: 0  - qwerty alpha mode,
 *                          1  - qwerty number mode,
 * @param maxSize  the maximal string length
 * @param string_length ptr to the length of entered string
 * @return string which entered from the keyboard
 */
javacall_utf16_string get_str_from_virt_kbd(javacall_utf16_string prepopulate, int mode, int maxSize, int* string_length, CVMExecEnv* _ee);

#endif
