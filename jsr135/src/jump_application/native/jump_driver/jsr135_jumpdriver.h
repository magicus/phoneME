/*
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

#ifndef _JSR135_JUMPDRIVER_H
#define _JSR135_JUMPDRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef JSR135_KNI_LAYER

#define jsr135_open_tunnel        Djsr135_open_tunnel
#define jsr135_close_tunnel       Djsr135_close_tunnel
#define jsr135_get_pcmctl         Djsr135_get_pcmctl
#define jsr135_mixer_start        Djsr135_mixer_start
#define jsr135_mixer_stop         Djsr135_mixer_stop
    

#endif

enum {
    ID_jsr135_open_tunnel = 0x100,
    ID_jsr135_close_tunnel,
    ID_jsr135_get_pcmctl,
    ID_jsr135_mixer_start,
    ID_jsr135_mixer_stop
};

#ifdef __cplusplus
}
#endif

#endif /* #ifdef _JSR135_JUMPDRIVER_H */
