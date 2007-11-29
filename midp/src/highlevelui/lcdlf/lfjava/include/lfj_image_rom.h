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
 
#ifndef _LFJ_IMAGE_ROM_H_
#define _LFJ_IMAGE_ROM_H_


/**
 * @defgroup highui_lfj Java Look and Feel Specific External Interface
 * @ingroup highui_lcdlf
 */

/**
 * @file
 * @ingroup highui_lfj
 *
 * @brief  This file contains functions for loading ROMized skin images
 */

/**
 * Loads a ROMized skin image from ROM, if present.
 *
 * @param imageId    The image ID
 * @param **bufPtr   Pointer where a buffer will be allocated and data stored
 * @return           -1 if failed, else length of buffer
 */
int lfj_load_image_from_rom(int imageId, unsigned char **bufPtr);

/**
 * Loads a ROMized skin description from ROM, if present.
 *
 * @return NULL if failed, otherwise a pointer to the skin description data
 */
const unsigned char* lfj_get_skin_description();

/**
 * Retrieves the size of the skin description data.
 *
 * @return -1 if failed, otherwise a size of the skin description data
 */
int lfj_get_skin_description_size();

#endif /* _LFJ_PORT_IMAGE_ROM_H_ */


 

