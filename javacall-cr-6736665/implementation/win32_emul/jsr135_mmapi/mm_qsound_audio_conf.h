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

#ifndef __MMAUDIO_CONF_H
#define __MMAUDIO_CONF_H

#define ENV_CHANNELS        2                             /* channels     */
#define ENV_RATE            44100                         /* samples/sec  */
#define ENV_BITS            16                            /* bits/channel */
#define ENV_SAMPLE_BYTES    (ENV_CHANNELS * (ENV_BITS/8)) /* bytes/sample */

#define ENV_BLOCK_TIME      20                                     /* ms/block    */
#define ENV_BLOCK_SAMPLES   (ENV_BLOCK_TIME * ENV_RATE / 1000 )    /* smpls/block */
#define ENV_BLOCK_BYTES     (ENV_BLOCK_SAMPLES * ENV_SAMPLE_BYTES) /* bytes/block */

#define MM_DEVICE           "/dev/dsp"

#endif /* __MMAUDIO_CONF_H */
