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

#ifndef _FBAPP_DEVICE_TYPE_H
#define _FBAPP_DEVICE_TYPE_H


/**
 * @defgroup highui_fbapp Linux/Framebuffer Application Internal Interface
 * @ingroup highui
 */

/**
 * @file
 * @ingroup highui_fbapp
 *
 * @brief Linux/framebuffer application device type interface
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LINUX_FB_VERSATILE_INTEGRATOR,   /* ARM Versatile or Integrator dev board*/
    LINUX_FB_ZAURUS,                 /* Sharp Zaurus */
    LINUX_FB_INTEL_MAINSTONE,        /* Intel MainStone dev board */
    LINUX_FB_OMAP730                 /* TI-Perseus2 OMAP730 */
} LinuxFbDeviceType ;

#ifdef __cplusplus
}
#endif

#endif /* _FBAPP_DEVICE_TYPE_H */
